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


#include "sidebar_right.hpp"
#include "layout.hpp"
#include "theme.hpp"
#include "ui_kit.hpp"
#include "imgui.h"
#include "sidebar_left/mavlink_field_catalog_generated.hpp"
#include "../settings.hpp"
#include <vector>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <limits>
#include <algorithm>
#include <unordered_set>
#include <chrono>
#include <cctype>
#include <ctime>
#include <string>

// Height of one telemetry tile in the 3×3 grid.
static constexpr float GRID_CELL_H = 66.0f;

#define _USE_MATH_DEFINES
#include <cmath>

// ── Artificial horizon (local to this TU) ────────────────────────────────────

static void draw_artificial_horizon(float roll_deg, float pitch_deg, float yaw_deg,
                                     float airspeed_ms, int throttle_pct, bool has_vfr,
                                     float size)
{
    ImDrawList* dl   = ImGui::GetWindowDrawList();
    const ImVec2 p0  = ImGui::GetCursorScreenPos();
    const float  cx  = p0.x + size * 0.5f;
    const float  cy  = p0.y + size * 0.5f;
    const float  r   = size * 0.45f;
    const float  fh  = ImGui::GetTextLineHeight();

    const float roll_rad = roll_deg  * (float)M_PI / 180.0f;
    const float pitch_px = pitch_deg * (r / 45.0f);

    const float ux  = -sinf(roll_rad);
    const float uy  = -cosf(roll_rad);
    const float ldx = -uy, ldy = ux;

    const float hx = cx - ux * pitch_px;
    const float hy = cy - uy * pitch_px;

    dl->PushClipRect({ cx - r, cy - r }, { cx + r, cy + r }, true);

    dl->AddCircleFilled({ cx, cy }, r, ah_sky());

    {
        const float dx_c = hx - cx, dy_c = hy - cy;
        const float b    = dx_c * ldx + dy_c * ldy;
        const float disc = b * b - (dx_c * dx_c + dy_c * dy_c - r * r);

        if (disc > 0.0f) {
            const float sq  = sqrtf(disc);
            const float px1 = hx + (-b - sq) * ldx, py1 = hy + (-b - sq) * ldy;
            const float px2 = hx + (-b + sq) * ldx, py2 = hy + (-b + sq) * ldy;

            const float a1 = atan2f(py1 - cy, px1 - cx);
            const float a2 = atan2f(py2 - cy, px2 - cx);
            const float a2_ccw = (a2 < a1) ? (a2 + 2.0f * (float)M_PI) : a2;

            const float amid  = (a1 + a2_ccw) * 0.5f;
            const float mid_wx = cx + cosf(amid) * r;
            const float mid_wy = cy + sinf(amid) * r;
            const bool ccw_is_ground =
                (mid_wx - hx) * (-ux) + (mid_wy - hy) * (-uy) > 0.0f;

            if (ccw_is_ground) {
                dl->PathLineTo({ px2, py2 });
                dl->PathLineTo({ px1, py1 });
                dl->PathArcTo({ cx, cy }, r, a1, a2_ccw, 40);
            } else {
                dl->PathLineTo({ px1, py1 });
                dl->PathLineTo({ px2, py2 });
                dl->PathArcTo({ cx, cy }, r, a2_ccw, a1 + 2.0f * (float)M_PI, 40);
            }
            dl->PathFillConvex(ah_ground());
            dl->AddLine({ px1, py1 }, { px2, py2 }, ah_horizon_line(), AH_THICK_HORIZON);

        } else {
            const float center_dot = (cx - hx) * (-ux) + (cy - hy) * (-uy);
            if (center_dot > 0.0f)
                dl->AddCircleFilled({ cx, cy }, r, ah_ground());
        }
    }

    static const int marks[] = { -30, -20, -10, -5, 5, 10, 20, 30 };
    for (int deg : marks) {
        const float disp = ((float)deg - pitch_deg) * (r / 45.0f);
        if (fabsf(disp) > r * 0.92f) continue;
        const float mx   = cx + ux * disp;
        const float my   = cy + uy * disp;
        const float hl   = (deg == 5 || deg == -5) ? r * 0.12f : r * 0.22f;
        const float thick = (deg % 10 == 0) ? AH_THICK_PITCH_BOLD : AH_THICK_PITCH_THIN;
        dl->AddLine({ mx + ldx * hl, my + ldy * hl },
                    { mx - ldx * hl, my - ldy * hl },
                    ah_pitch_ladder(), thick);

        if (deg != 5 && deg != -5) {
            char buf[8];
            snprintf(buf, sizeof(buf), "%d", deg);
            const float tw  = (float)strlen(buf) * 6.0f;
            const float lx  = mx + ldx * (hl + r * 0.04f);
            const float ly  = my + ldy * (hl + r * 0.04f);
            dl->AddRectFilled({ lx - 1, ly - 1 }, { lx + tw + 1, ly + fh + 1 },
                              ah_pitch_label_bg());
            dl->AddText({ lx, ly - fh * 0.5f + 1 }, ah_pitch_label(), buf);
        }
    }

    const float ch  = r * 0.16f, gap = r * 0.06f;
    dl->AddLine({ cx - ch - gap, cy }, { cx - gap, cy }, ah_crosshair(), AH_THICK_CROSSHAIR);
    dl->AddLine({ cx + gap, cy }, { cx + ch + gap, cy }, ah_crosshair(), AH_THICK_CROSSHAIR);
    dl->AddCircleFilled({ cx, cy }, 3.0f, ah_crosshair());

    if (has_vfr) {
        char as_buf[16];
        snprintf(as_buf, sizeof(as_buf), "%.1f m/s", (double)airspeed_ms);
        const float tw = (float)strlen(as_buf) * 6.0f + 6.0f;
        const float bx = cx - r * 0.60f - tw * 0.5f;
        const float by = cy - r * 0.62f;
        dl->AddRectFilled({ bx - 2, by - 2 }, { bx + tw, by + fh + 2 },
                          ah_overlay_bg(), 2.0f);
        dl->AddText({ bx, by }, ah_airspeed_text(), as_buf);

        char thr_buf[8];
        snprintf(thr_buf, sizeof(thr_buf), "%d%%", throttle_pct);
        const float tw2 = (float)strlen(thr_buf) * 6.0f + 6.0f;
        const float bx2 = cx + r * 0.35f;
        const float by2 = cy - r * 0.62f;
        dl->AddRectFilled({ bx2 - 2, by2 - 2 }, { bx2 + tw2, by2 + fh + 2 },
                          ah_overlay_bg(), 2.0f);
        dl->AddText({ bx2, by2 }, ah_throttle_text(), thr_buf);
    }

    {
        char hdg_buf[12];
        snprintf(hdg_buf, sizeof(hdg_buf), "%.0f\xc2\xb0", (double)yaw_deg);
        const float tw  = (float)strlen(hdg_buf) * 6.0f + 6.0f;
        const float bx  = cx - tw * 0.5f;
        const float by  = cy + r * 0.65f;
        dl->AddRectFilled({ bx - 2, by - 2 }, { bx + tw, by + fh + 2 },
                          ah_overlay_bg(), 2.0f);
        dl->AddText({ bx, by }, ah_heading_text(), hdg_buf);
    }

    dl->PopClipRect();

    {
        const float arc_r = r * 1.08f;
        const float base  = -(float)M_PI / 2.0f;
        dl->PathArcTo({ cx, cy }, arc_r,
                      base - 60.0f * (float)M_PI / 180.0f,
                      base + 60.0f * (float)M_PI / 180.0f, 40);
        dl->PathStroke(ah_roll_arc(), false, AH_THICK_ROLL_ARC);

        static const float ticks[] = { -60, -30, 0, 30, 60 };
        for (float t : ticks) {
            const float a   = base + t * (float)M_PI / 180.0f;
            const float len = (t == 0) ? r * 0.10f : r * 0.06f;
            dl->AddLine({ cx + cosf(a) * arc_r,           cy + sinf(a) * arc_r },
                        { cx + cosf(a) * (arc_r - len),   cy + sinf(a) * (arc_r - len) },
                        ah_roll_tick(), AH_THICK_ROLL_TICK);

            if (t != 0.0f) {
                const float label_r = arc_r + r * 0.14f;
                char buf[8];
                snprintf(buf, sizeof(buf), "%d", (int)fabsf(t));
                const float tw = (float)strlen(buf) * 5.5f;
                const float tx = cx + cosf(a) * label_r - tw * 0.5f;
                const float ty = cy + sinf(a) * label_r - fh * 0.5f;
                dl->AddText({ tx, ty }, ah_roll_label(), buf);
            }
        }

        const float a   = base + roll_deg * (float)M_PI / 180.0f;
        const float ca  = cosf(a), sa = sinf(a);
        const float cp  = cosf(a + (float)M_PI / 2.0f);
        const float sp  = sinf(a + (float)M_PI / 2.0f);
        const float tip = arc_r + r * 0.04f, br = arc_r - r * 0.09f, hw = r * 0.04f;
        dl->AddTriangleFilled(
            { cx + ca * tip,          cy + sa * tip          },
            { cx + ca * br + cp * hw, cy + sa * br + sp * hw },
            { cx + ca * br - cp * hw, cy + sa * br - sp * hw },
            ah_roll_pointer());
    }

    dl->AddCircle({ cx, cy }, r, ah_border(), 64, AH_THICK_BORDER);

    ImGui::Dummy({ size, size });
}

// ── Data grid state ───────────────────────────────────────────────────────────

struct GridCellConfig {
    uint32_t msg_id         = UINT32_MAX;  // UINT32_MAX → unconfigured
    char     field_name[64] = {};
    char     title[48]      = {};          // short label shown at bottom of cell
};

static GridCellConfig s_cells[9];
static bool           s_cells_loaded = false;  // have we pulled from AppSettings yet?

// Copy persistent settings → runtime cells (called once on first draw).
static void cells_load(const AppSettings& settings)
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

// Set of message IDs we've already sent a rate-request for this session.
// Prevents re-spamming the FC every frame when data is absent.
static std::unordered_set<uint32_t> s_requested_msgs;

// Picker state ─────────────────────────────────────────────────────────────────
static int  s_picker_cell        = -1;
static bool s_picker_needs_open  = false;
static bool s_picker_scroll_msgs = false;  // scroll msg list to selection once
static bool s_picker_scroll_flds = false;  // scroll field list to selection once

static char     s_picker_filter[128]     = {};
static uint32_t s_picker_sel_msg         = UINT32_MAX;
static char     s_picker_sel_field[64]   = {};
static char     s_picker_sel_label[48]   = {};

// ── Case-insensitive substring helper ────────────────────────────────────────

static bool ci_contains(const char* haystack, const char* needle_lower)
{
    if (!needle_lower[0]) return true;
    char h[128]; h[0] = '\0';
    size_t hl = strlen(haystack);
    for (size_t k = 0; k < hl && k < 127; ++k)
        h[k] = (char)tolower((unsigned char)haystack[k]);
    h[hl < 127 ? hl : 127] = '\0';
    return strstr(h, needle_lower) != nullptr;
}

// Build lowercase filter once per call ────────────────────────────────────────
static void make_lower(const char* src, char* dst, size_t dstsz)
{
    size_t n = strlen(src);
    if (n >= dstsz) n = dstsz - 1;
    for (size_t k = 0; k < n; ++k)
        dst[k] = (char)tolower((unsigned char)src[k]);
    dst[n] = '\0';
}

// ── Picker helpers ────────────────────────────────────────────────────────────

// Returns unique message IDs whose fields match the filter.
// When filter is empty, returns all messages.
// When filter is set, searches field labels (not message names).
static std::vector<uint32_t> picker_unique_msgs(const char* filter)
{
    char flt[128] = {};
    make_lower(filter, flt, sizeof(flt));

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
        if (!flt[0]) {
            match = true;
        } else {
            // Scan all fields of this message for a label match
            match = false;
            for (size_t j = i; j < kMavlinkFieldCatalogCount; ++j) {
                if (kMavlinkFieldCatalog[j].msg_id != fd.msg_id) break;
                if (ci_contains(kMavlinkFieldCatalog[j].field_name, flt) ||
                    ci_contains(kMavlinkFieldCatalog[j].label,      flt)) {
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
    char flt[128] = {};
    make_lower(filter, flt, sizeof(flt));

    std::vector<const MavlinkFieldDesc*> result;
    for (size_t i = 0; i < kMavlinkFieldCatalogCount; ++i) {
        const MavlinkFieldDesc& fd = kMavlinkFieldCatalog[i];
        if (fd.msg_id != msg_id) continue;
        if (flt[0]) {
            if (!ci_contains(fd.field_name, flt) && !ci_contains(fd.label, flt))
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
            s_requested_msgs.find(s_picker_sel_msg) == s_requested_msgs.end())
        {
            sender->request_message_interval(vs->sysid, vs->compid,
                                              s_picker_sel_msg,
                                              200000 /* 5 Hz */);
            s_requested_msgs.insert(s_picker_sel_msg);
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

static void draw_data_grid(const VehicleState* vs, MavlinkSender* sender,
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
            char lbl_buf[32];
            size_t li = 0;
            for (const char* c = cell.title; *c && li < sizeof(lbl_buf) - 1; ++c)
                lbl_buf[li++] = (char)toupper((unsigned char)*c);
            lbl_buf[li] = '\0';

            ui_readout_tile(dl, p0, p1, lbl_buf, val_buf,
                            has_data ? col_val : col_no_data, hovered);

            // If this message is configured but not yet received, request it once.
            if (!has_data && sender && vs && vs->has_heartbeat &&
                s_requested_msgs.find(cell.msg_id) == s_requested_msgs.end())
            {
                sender->request_message_interval(vs->sysid, vs->compid,
                                                  cell.msg_id, 200000 /* 5 Hz */);
                s_requested_msgs.insert(cell.msg_id);
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

// ── Right sidebar ─────────────────────────────────────────────────────────────

void draw_sidebar_right(const VehicleState& vs,
                        const std::vector<StatusText>& status_texts,
                        MavlinkSender* sender,
                        AppSettings* settings)
{
    // Load cell config from settings on the first frame.
    if (!s_cells_loaded) {
        if (settings) cells_load(*settings);
        s_cells_loaded = true;
    }
    const GcsLayout l = GcsLayout::compute();

    ImGui::SetNextWindowPos ({ l.right_x, l.top    }, ImGuiCond_Always);
    ImGui::SetNextWindowSize({ l.right_w, l.total_h }, ImGuiCond_Always);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, panel_bg());
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,  { 8.0f, 8.0f });
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,    { 6.0f, 4.0f });
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar  |
        ImGuiWindowFlags_NoResize    |
        ImGuiWindowFlags_NoMove      |
        ImGuiWindowFlags_NoScrollbar;

    if (ImGui::Begin("##sidebar_right", nullptr, flags)) {
        {
            ImDrawList*  dl = ImGui::GetWindowDrawList();
            const ImVec2 wp = ImGui::GetWindowPos();
            dl->AddLine({ wp.x, wp.y }, { wp.x, wp.y + l.total_h },
                        col32_panel_border(), PANEL_BORDER_THICKNESS);
        }

        const float avail_h = ImGui::GetContentRegionAvail().y;
        const float avail_w = ImGui::GetContentRegionAvail().x;

        // HUD: ball + header + the readout strip beneath it
        const float hud_h = std::min(avail_w + 46.0f, avail_h * 0.40f);

        // Data grid: 3 rows of tiles + header + spacing
        const float grid_section_h = 3.0f * GRID_CELL_H + UI_HEADER_H + 14.0f;

        // ── Attitude ─────────────────────────────────────────────────────
        ImGui::PushStyleColor(ImGuiCol_ChildBg, panel_bg());
        if (ImGui::BeginChild("##hud", { 0.0f, hud_h }, false,
                              ImGuiWindowFlags_NoScrollbar)) {
            char hud_meta[32];
            if (vs.has_attitude)
                snprintf(hud_meta, sizeof(hud_meta), "R %+.0f\xc2\xb0  P %+.0f\xc2\xb0",
                         (double)vs.roll, (double)vs.pitch);
            else
                snprintf(hud_meta, sizeof(hud_meta), "NO DATA");
            ui_panel_header("ATTITUDE \xc2\xb7 ROLL / PITCH", hud_meta);

            const float strip_h = 40.0f;
            if (vs.has_attitude) {
                const ImVec2 avail = ImGui::GetContentRegionAvail();
                const float  hs    = std::min(avail.x, avail.y - strip_h);
                if (hs > 30.0f) {
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail.x - hs) * 0.5f);
                    draw_artificial_horizon(vs.roll, vs.pitch, vs.yaw,
                                            vs.airspeed,
                                            vs.has_vfr ? (int)vs.throttle : -1,
                                            vs.has_vfr, hs);
                }
            } else {
                const ImVec2 avail = ImGui::GetContentRegionAvail();
                ImGui::Dummy({ avail.x, std::max(0.0f, avail.y - strip_h) });
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() - avail.y * 0.5f);
                ImGui::TextDisabled("   NO ATTITUDE DATA");
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + avail.y * 0.5f
                                     - ImGui::GetTextLineHeight());
            }

            // ── GND / HDG / CLB strip ────────────────────────────────────
            {
                ImDrawList*  dl = ImGui::GetWindowDrawList();
                const ImVec2 s0 = ImGui::GetCursorScreenPos();
                const float  w  = ImGui::GetContentRegionAvail().x;
                const float  cw = w / 3.0f;

                struct Item { const char* label; char value[16]; };
                Item items[3] = { { "GND m/s", "--" }, { "HDG", "--" }, { "CLB m/s", "--" } };
                if (vs.has_vfr) {
                    snprintf(items[0].value, sizeof(items[0].value), "%.1f", (double)vs.groundspeed);
                    snprintf(items[1].value, sizeof(items[1].value), "%03d\xc2\xb0", (int)vs.heading);
                    snprintf(items[2].value, sizeof(items[2].value), "%+.1f", (double)vs.climb);
                }

                for (int i = 0; i < 3; ++i) {
                    const ImVec2 p0 = { s0.x + i * cw, s0.y };
                    const ImVec2 p1 = { p0.x + cw,     s0.y + strip_h };
                    ui_readout(dl, p0, p1, items[i].label, items[i].value,
                               vs.has_vfr ? ui_col_value() : ui_col(g_theme.col_no_link_muted),
                               UI_SZ_BODY + 2.0f);
                    if (i > 0)
                        dl->AddLine({ p0.x, p0.y + 4.0f }, { p0.x, p1.y - 4.0f },
                                    ui_col(g_theme.separator, 0.6f), 1.0f);
                }
                ImGui::Dummy({ w, strip_h });
            }
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();

        // ── Telemetry tiles ──────────────────────────────────────────────
        ImGui::PushStyleColor(ImGuiCol_ChildBg, panel_bg());
        if (ImGui::BeginChild("##datagrid", { 0.0f, grid_section_h },
                              false, ImGuiWindowFlags_NoScrollbar)) {
            ui_panel_header("TELEMETRY", "CLICK A TILE TO ASSIGN");
            draw_data_grid(&vs, sender, settings);
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();

        // ── Event log ────────────────────────────────────────────────────
        ImGui::PushStyleColor(ImGuiCol_ChildBg, panel_bg());
        if (ImGui::BeginChild("##sysmsg", { 0.0f, 0.0f },
                              false, ImGuiWindowFlags_NoScrollbar)) {
            char log_meta[24];
            snprintf(log_meta, sizeof(log_meta), "%d ENTRIES", (int)status_texts.size());
            ui_panel_header("EVENT LOG", log_meta);

            // Wall-clock stamps, captured as each message arrives.
            static std::vector<std::string> s_stamps;
            if (status_texts.size() < s_stamps.size()) s_stamps.clear();
            while (s_stamps.size() < status_texts.size()) {
                const std::time_t t  = std::time(nullptr);
                std::tm            lt{};
#ifdef _WIN32
                localtime_s(&lt, &t);
#else
                localtime_r(&t, &lt);
#endif
                char buf[16];
                snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
                         lt.tm_hour, lt.tm_min, lt.tm_sec);
                s_stamps.emplace_back(buf);
            }

            ImGui::PushStyleColor(ImGuiCol_ChildBg, bg_child_darker());
            const ImVec2 inner_sz = { 0.0f, ImGui::GetContentRegionAvail().y };
            if (ImGui::BeginChild("##sysmsg_scroll", inner_sz, false,
                                  ImGuiWindowFlags_HorizontalScrollbar)) {
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 6.0f, 3.0f });
                if (status_texts.empty()) {
                    ImGui::PushStyleColor(ImGuiCol_Text, col_no_link_muted());
                    ImGui::TextUnformatted("AWAITING VEHICLE MESSAGES");
                    ImGui::PopStyleColor();
                } else {
                    for (size_t i = 0; i < status_texts.size(); ++i) {
                        ImGui::PushStyleColor(ImGuiCol_Text, col_log());
                        ImGui::TextUnformatted(i < s_stamps.size() ? s_stamps[i].c_str()
                                                                   : "--:--:--");
                        ImGui::PopStyleColor();
                        ImGui::SameLine(0, 8);
                        ImGui::TextColored(col_status_severity(status_texts[i].severity),
                                           "%s", status_texts[i].text);
                    }
                    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                        ImGui::SetScrollHereY(1.0f);
                }
                ImGui::PopStyleVar();
            }
            ImGui::EndChild();
            ImGui::PopStyleColor();
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();
    }
    ImGui::End();

    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor();
}
