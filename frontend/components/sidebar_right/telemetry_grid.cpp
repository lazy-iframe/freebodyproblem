/**************************************************************************
 * Copyright (C) 2026  Azhar Tanweer
 * Contact: azhar.tanweer404@gmail.com
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 **************************************************************************/

// Right sidebar: the configurable 3x3 telemetry grid and its field picker.

#define _USE_MATH_DEFINES
#include <cmath>
#include "frontend/elements/sidebar_right.hpp"
#include "frontend/components/sidebar_right/sidebar_internal.hpp"
#include "frontend/elements/center_view.hpp"
#include "frontend/source/vehicle_ui_state.hpp"
#include "frontend/source/layout.hpp"
#include "frontend/source/theme.hpp"
#include "frontend/components/others/ui_kit.hpp"
#include "imgui.h"
#include "frontend/components/sidebar_left/mavlink_field_catalog_generated.hpp"
#include "frontend/source/settings.hpp"
#include "lib/text.hpp"
#include <vector>
#include <cstdio>
#include <cstring>
#include <limits>
#include <cfloat>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <deque>
#include <cctype>
#include <ctime>
#include <string>

namespace right_sidebar {

// ── Data grid state ───────────────────────────────────────────────────────────

struct GridCellConfig {
    uint32_t msg_id         = UINT32_MAX;  // UINT32_MAX → unconfigured
    char     field_name[64] = {};
    char     title[48]      = {};          // short label shown at bottom of cell
};

static GridCellConfig s_cells[9];
bool           s_cells_loaded = false;  // have we pulled from AppSettings yet?

// Copy persistent settings → runtime cells (called once on first draw).
void cells_load(const AppSettings& settings)
{
    for (int i = 0; i < 9; ++i) {
        const GridCellPersist& src = settings.grid_cells[i];
        s_cells[i].msg_id = src.msg_id;
        strncpy(s_cells[i].field_name, src.field_name.c_str(),
                sizeof(s_cells[i].field_name) - 1);
        s_cells[i].field_name[sizeof(s_cells[i].field_name) - 1] = '\0';
        strncpy(s_cells[i].title, src.title.c_str(),
                sizeof(s_cells[i].title) - 1);
        s_cells[i].title[sizeof(s_cells[i].title) - 1] = '\0';
    }
}

// Copy runtime cells → persistent settings and flush to disk.
static void cells_save(AppSettings& settings)
{
    for (int i = 0; i < 9; ++i) {
        settings.grid_cells[i].msg_id     = s_cells[i].msg_id;
        settings.grid_cells[i].field_name = s_cells[i].field_name;
        settings.grid_cells[i].title      = s_cells[i].title;
    }
    settings_save(settings);
}

// Message IDs already asked for with SET_MESSAGE_INTERVAL, so the panel does
// not re-spam a vehicle every frame while data is absent.
//
// Per vehicle, and it has to be: the record is of what *this* vehicle was
// asked, and a shared set would tell the panel it had already requested a
// stream from an aircraft it has never spoken to — leaving the cell blank for
// good. See source/vehicle_ui_state.hpp.
static VehicleUiState<std::unordered_set<uint32_t>> s_requested_msgs_state;
static std::unordered_set<uint32_t>& requested_msgs() { return *s_requested_msgs_state; }

// Picker state ─────────────────────────────────────────────────────────────────
static int  s_picker_cell        = -1;
static bool s_picker_needs_open  = false;
static bool s_picker_scroll_msgs = false;  // scroll msg list to selection once
static bool s_picker_scroll_flds = false;  // scroll field list to selection once

static char     s_picker_filter[128]     = {};
static uint32_t s_picker_sel_msg         = UINT32_MAX;
static char     s_picker_sel_field[64]   = {};
static char     s_picker_sel_label[48]   = {};

// ── Picker helpers ────────────────────────────────────────────────────────────

// Returns unique message IDs whose fields match the filter.
// When filter is empty, returns all messages.
// When filter is set, searches field labels (not message names).
static std::vector<uint32_t> picker_unique_msgs(const char* filter)
{

    std::vector<uint32_t> result;
    uint32_t prev_id = UINT32_MAX;

    for (size_t i = 0; i < kMavlinkFieldCatalogCount; ++i) {
        const MavlinkFieldDesc& fd = kMavlinkFieldCatalog[i];

        if (fd.msg_id == prev_id) {
            // We already added this message — still need to check if this entry
            // is the reason it should appear (for filter matching we just continue,
            // the message is already in the list once any field matched).
            continue;
        }

        bool match;
        if (!filter[0]) {
            match = true;
        } else {
            // Scan all fields of this message for a label match
            match = false;
            for (size_t j = i; j < kMavlinkFieldCatalogCount; ++j) {
                if (kMavlinkFieldCatalog[j].msg_id != fd.msg_id) break;
                if (text_icontains(kMavlinkFieldCatalog[j].field_name, filter) ||
                    text_icontains(kMavlinkFieldCatalog[j].label,      filter)) {
                    match = true;
                    break;
                }
            }
        }

        if (match) {
            result.push_back(fd.msg_id);
            prev_id = fd.msg_id;
        }
    }
    return result;
}

// Fields for a given msg_id, optionally filtered by label.
static std::vector<const MavlinkFieldDesc*> picker_fields_for_msg(uint32_t msg_id,
                                                                    const char* filter)
{

    std::vector<const MavlinkFieldDesc*> result;
    for (size_t i = 0; i < kMavlinkFieldCatalogCount; ++i) {
        const MavlinkFieldDesc& fd = kMavlinkFieldCatalog[i];
        if (fd.msg_id != msg_id) continue;
        if (filter[0]) {
            if (!text_icontains(fd.field_name, filter) && !text_icontains(fd.label, filter))
                continue;
        }
        result.push_back(&fd);
    }
    return result;
}

// ── Draw picker popup ─────────────────────────────────────────────────────────

static void draw_picker_popup(const VehicleState* vs, MavlinkSender* sender,
                               AppSettings* settings)
{
    ui_push_dialog_style();
    if (!ImGui::BeginPopupModal("##field_picker", nullptr,
                                ImGuiWindowFlags_AlwaysAutoResize |
                                ImGuiWindowFlags_NoTitleBar)) {
        ui_pop_dialog_style();
        return;
    }

    const float panel_h = 300.0f;
    const float left_w  = 190.0f;
    const float right_w = 230.0f;
    const float body_w  = left_w + right_w + 6.0f;

    char title_buf[48];
    snprintf(title_buf, sizeof(title_buf), "ASSIGN TELEMETRY \xe2\x80\x94 TILE %d",
             s_picker_cell + 1);
    ui_dialog_title(title_buf, body_w);

    // Filter — searches field names / labels.
    // Auto-focus on the first frame the popup appears so typing works immediately.
    if (ImGui::IsWindowAppearing())
        ImGui::SetKeyboardFocusHere();
    ImGui::SetNextItemWidth(body_w);
    bool filter_changed = ImGui::InputTextWithHint("##filter", "SEARCH FIELDS\xe2\x80\xa6",
                                                   s_picker_filter,
                                                   sizeof(s_picker_filter));
    ImGui::Spacing();

    // If the filter changed and the currently selected message no longer has
    // matching fields, clear the message selection so the user sees a fresh list.
    if (filter_changed && s_picker_sel_msg != UINT32_MAX) {
        const auto fields = picker_fields_for_msg(s_picker_sel_msg, s_picker_filter);
        if (fields.empty()) {
            s_picker_sel_msg      = UINT32_MAX;
            s_picker_sel_field[0] = '\0';
            s_picker_sel_label[0] = '\0';
        }
    }

    // ── Left: message list ────────────────────────────────────────────────────
    ImGui::PushStyleColor(ImGuiCol_ChildBg, bg_child_darker());
    ImGui::BeginChild("##msg_list", { left_w, panel_h }, true);

    const std::vector<uint32_t> msg_ids = picker_unique_msgs(s_picker_filter);

    for (uint32_t mid : msg_ids) {
        const char* msg_name = nullptr;
        for (size_t i = 0; i < kMavlinkFieldCatalogCount; ++i) {
            if (kMavlinkFieldCatalog[i].msg_id == mid) {
                msg_name = kMavlinkFieldCatalog[i].msg_name;
                break;
            }
        }
        if (!msg_name) continue;

        const bool has_data = vs && vs->last_messages.count(mid) > 0;
        const bool selected = (mid == s_picker_sel_msg);

        // Scroll to the pre-selected item only once (on open / selection change).
        if (selected && s_picker_scroll_msgs) {
            ImGui::SetScrollHereY(0.5f);
            s_picker_scroll_msgs = false;
        }

        if (!has_data)
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.33f, 0.0f, 1.0f));

        if (ImGui::Selectable(msg_name, selected)) {
            if (s_picker_sel_msg != mid) {
                s_picker_sel_msg      = mid;
                s_picker_sel_field[0] = '\0';
                s_picker_sel_label[0] = '\0';
                s_picker_scroll_flds  = true;  // scroll field list to new selection
            }
        }

        if (!has_data) {
            ImGui::PopStyleColor();
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
                ImGui::BeginTooltip();
                ImGui::TextDisabled("Not receiving — will be requested on OK");
                ImGui::EndTooltip();
            }
        }
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::SameLine(0, 6);

    // ── Right: field list ─────────────────────────────────────────────────────
    ImGui::PushStyleColor(ImGuiCol_ChildBg, bg_child_darker());
    ImGui::BeginChild("##field_list", { right_w, panel_h }, true);

    if (s_picker_sel_msg == UINT32_MAX) {
        ImGui::TextDisabled("Select a message.");
    } else {
        const auto fields = picker_fields_for_msg(s_picker_sel_msg, s_picker_filter);
        if (fields.empty()) {
            ImGui::TextDisabled("No matches.");
        } else {
            for (const MavlinkFieldDesc* fd : fields) {
                const bool selected = (strcmp(fd->field_name, s_picker_sel_field) == 0);

                // Scroll to the pre-selected field only once.
                if (selected && s_picker_scroll_flds) {
                    ImGui::SetScrollHereY(0.5f);
                    s_picker_scroll_flds = false;
                }

                if (ImGui::Selectable(fd->label, selected)) {
                    strncpy(s_picker_sel_field, fd->field_name,
                            sizeof(s_picker_sel_field) - 1);
                    s_picker_sel_field[sizeof(s_picker_sel_field) - 1] = '\0';
                    strncpy(s_picker_sel_label, fd->field_name,
                            sizeof(s_picker_sel_label) - 1);
                    s_picker_sel_label[sizeof(s_picker_sel_label) - 1] = '\0';
                }
            }
        }
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Separator, col_separator());
    ImGui::Separator();
    ImGui::PopStyleColor();
    ImGui::Spacing();

    const bool can_ok = (s_picker_sel_msg != UINT32_MAX &&
                         s_picker_sel_field[0] != '\0');

    // Centred [ASSIGN][CLEAR][CANCEL] row
    constexpr float BW = 110.0f, BGAP = 8.0f;
    ui_dialog_row(BW * 3.0f + BGAP * 2.0f);

    if (!can_ok) ImGui::PushStyleVar(ImGuiStyleVar_Alpha,
                                      ImGui::GetStyle().Alpha * 0.4f);

    const bool hit_assign = ui_solid_button("ASSIGN", { BW, UI_DIALOG_BH },
                                            btn_write_base(), btn_write_hov());

    if (hit_assign && can_ok) {
        GridCellConfig& cell = s_cells[s_picker_cell];
        cell.msg_id = s_picker_sel_msg;
        strncpy(cell.field_name, s_picker_sel_field, sizeof(cell.field_name) - 1);
        cell.field_name[sizeof(cell.field_name) - 1] = '\0';
        strncpy(cell.title, s_picker_sel_label, sizeof(cell.title) - 1);
        cell.title[sizeof(cell.title) - 1] = '\0';

        // Request the message from the FC if it's not already streaming.
        if (vs && vs->last_messages.count(s_picker_sel_msg) == 0 &&
            sender && vs->has_heartbeat &&
            requested_msgs().find(s_picker_sel_msg) == requested_msgs().end())
        {
            sender->request_message_interval_at_least(vs->sysid, vs->compid,
                                                       s_picker_sel_msg,
                                                       200000 /* 5 Hz */);
            requested_msgs().insert(s_picker_sel_msg);
        }

        if (settings) cells_save(*settings);
        ImGui::CloseCurrentPopup();
    }

    if (!can_ok) ImGui::PopStyleVar();

    ImGui::SameLine(0, BGAP);
    const bool hit_clear = ui_grid_button("CLEAR", { BW, UI_DIALOG_BH });
    if (hit_clear) {
        s_cells[s_picker_cell].msg_id         = UINT32_MAX;
        s_cells[s_picker_cell].field_name[0]  = '\0';
        s_cells[s_picker_cell].title[0]       = '\0';
        if (settings) cells_save(*settings);
        ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine(0, BGAP);
    const bool hit_cancel = ui_grid_button("CANCEL", { BW, UI_DIALOG_BH });
    if (hit_cancel || ImGui::IsKeyPressed(ImGuiKey_Escape, false))
        ImGui::CloseCurrentPopup();

    ImGui::EndPopup();
    ui_pop_dialog_style();
}

// ── Draw 9-cell data grid ─────────────────────────────────────────────────────

void draw_data_grid(const VehicleState* vs, MavlinkSender* sender,
                     AppSettings* settings)
{
    const float avail_w  = ImGui::GetContentRegionAvail().x;
    const float cell_w   = avail_w / 3.0f;
    const float cell_h   = GRID_CELL_H;
    const float pad      = 1.0f;

    ImDrawList*  dl     = ImGui::GetWindowDrawList();
    const ImVec2 origin = ImGui::GetCursorScreenPos();

    // "--" when data absent: muted link colour
    const ImU32 col_no_data = ui_col(g_theme.col_no_link_muted);
    // Live value text: standard data colour
    const ImU32 col_val     = ui_col_value();

    for (int idx = 0; idx < 9; ++idx) {
        const int row = idx / 3;
        const int col = idx % 3;

        const ImVec2 p0 = { origin.x + col * cell_w + pad,
                            origin.y + row * cell_h + pad };
        const ImVec2 p1 = { p0.x + cell_w - pad * 2.0f,
                            p0.y + cell_h - pad * 2.0f };

        ImGui::SetCursorScreenPos(p0);
        char btn_id[16]; snprintf(btn_id, sizeof(btn_id), "##gc%d", idx);
        ImGui::InvisibleButton(btn_id, { cell_w - pad * 2.0f, cell_h - pad * 2.0f });
        const bool hovered = ImGui::IsItemHovered();
        const bool clicked = ImGui::IsItemClicked();

        const GridCellConfig& cell = s_cells[idx];

        if (cell.msg_id == UINT32_MAX) {
            // Empty slot — well with a dim "+ ASSIGN" hint
            dl->AddRectFilled(p0, p1, hovered ? ui_col_strip() : ui_col_well());
            ui_frame(dl, p0, p1, hovered ? ui_col_accent() : ui_col(g_theme.separator, 0.5f));
            const ImU32 col_hint = ui_col(g_theme.col_no_link_muted);
            const float tw = ui_tracked_width(g_font_micro, UI_SZ_MICRO, "+ ASSIGN");
            ui_tracked_text(dl, g_font_micro, UI_SZ_MICRO,
                            { p0.x + (p1.x - p0.x - tw) * 0.5f,
                              p0.y + (p1.y - p0.y - UI_SZ_MICRO) * 0.5f },
                            hovered ? ui_col_accent() : col_hint, "+ ASSIGN");
        } else {
            // Read current value
            double  value    = std::numeric_limits<double>::quiet_NaN();
            bool    has_data = false;
            if (vs) {
                auto it = vs->last_messages.find(cell.msg_id);
                if (it != vs->last_messages.end())
                    has_data = mavlink_field_read(cell.msg_id, cell.field_name,
                                                  it->second, &value);
            }

            // ── Value + label, drawn as a tactical readout tile ───────────
            char val_buf[32];
            if (has_data && !std::isnan(value) && !std::isinf(value))
                snprintf(val_buf, sizeof(val_buf), "%.4g", value);
            else
                snprintf(val_buf, sizeof(val_buf), "--");

            // Field labels are lowercase in the catalog — uppercase them so the
            // tile matches the rest of the chrome.
            const std::string lbl = text_upper(cell.title);

            ui_readout_tile(dl, p0, p1, lbl.c_str(), val_buf,
                            has_data ? col_val : col_no_data, hovered);

            // If this message is configured but not yet received, request it
            // once — at 5 Hz unless something already asked for it faster: the
            // connect burst's requests go out in the same moment the tiles
            // first see no data, and a flat 5 Hz here would overwrite them.
            if (!has_data && sender && vs && vs->has_heartbeat &&
                requested_msgs().find(cell.msg_id) == requested_msgs().end())
            {
                sender->request_message_interval_at_least(vs->sysid, vs->compid,
                                                           cell.msg_id, 200000 /* 5 Hz */);
                requested_msgs().insert(cell.msg_id);
            }
        }

        if (clicked) {
            s_picker_cell        = idx;
            s_picker_needs_open  = true;
            s_picker_scroll_msgs = true;   // auto-scroll to pre-selection once on open
            s_picker_scroll_flds = true;
            s_picker_filter[0]   = '\0';

            // Pre-select current cell
            if (cell.msg_id != UINT32_MAX) {
                s_picker_sel_msg = cell.msg_id;
                strncpy(s_picker_sel_field, cell.field_name,
                        sizeof(s_picker_sel_field) - 1);
                s_picker_sel_field[sizeof(s_picker_sel_field) - 1] = '\0';
                strncpy(s_picker_sel_label, cell.title,
                        sizeof(s_picker_sel_label) - 1);
                s_picker_sel_label[sizeof(s_picker_sel_label) - 1] = '\0';
            } else {
                s_picker_sel_msg      = UINT32_MAX;
                s_picker_sel_field[0] = '\0';
                s_picker_sel_label[0] = '\0';
            }
        }
    }

    // Advance cursor below grid
    ImGui::SetCursorScreenPos({ origin.x, origin.y + 3.0f * cell_h });
    ImGui::Dummy({ avail_w, 0.0f });

    if (s_picker_needs_open) {
        ImGui::OpenPopup("##field_picker");
        s_picker_needs_open = false;
    }
    draw_picker_popup(vs, sender, settings);
}

} // namespace right_sidebar

using namespace right_sidebar;

void draw_telemetry_panel(const VehicleState& vs, MavlinkSender* sender,
                          AppSettings* settings, const char* meta)
{
    if (!s_cells_loaded) {
        if (settings) cells_load(*settings);
        s_cells_loaded = true;
    }
    ui_panel_header("TELEMETRY", meta ? meta : "CLICK A TILE TO ASSIGN");
    draw_data_grid(&vs, sender, settings);
}
