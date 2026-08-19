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


#include "sidebar_internal.hpp"
#include "../sidebar_themes.hpp"
#include "../param_meta.hpp"
#include "../../app_log.hpp"
#include "../../param_file.hpp"
#include "imgui.h"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <cstdio>
#include <string>
#include <unordered_map>
#include <vector>

// ── Parameter files ───────────────────────────────────────────────────────────

// What the file's comment header claims about the vehicle it came off. Only
// what the link has actually told us — a header is documentation, and a guessed
// one is worse than a missing one.
static ParamFileHeader file_header(const VehicleState* vs)
{
    ParamFileHeader h;
    if (!vs) return h;

    h.sysid = vs->sysid ? vs->sysid : 1;

    if (vs->autopilot == MAV_AUTOPILOT_ARDUPILOTMEGA) h.stack = "ArduPilot";
    else if (vs->autopilot == MAV_AUTOPILOT_PX4)      h.stack = "PX4";

    switch (vs->type) {
    case MAV_TYPE_QUADROTOR:      h.vehicle = "Quadrotor";   break;
    case MAV_TYPE_HEXAROTOR:      h.vehicle = "Hexarotor";   break;
    case MAV_TYPE_OCTOROTOR:      h.vehicle = "Octorotor";   break;
    case MAV_TYPE_TRICOPTER:      h.vehicle = "Tricopter";   break;
    case MAV_TYPE_HELICOPTER:     h.vehicle = "Helicopter";  break;
    case MAV_TYPE_FIXED_WING:     h.vehicle = "Fixed Wing";  break;
    case MAV_TYPE_GROUND_ROVER:   h.vehicle = "Ground Rover";break;
    case MAV_TYPE_SURFACE_BOAT:   h.vehicle = "Boat";        break;
    case MAV_TYPE_SUBMARINE:      h.vehicle = "Submarine";   break;
    case MAV_TYPE_ANTENNA_TRACKER:h.vehicle = "Antenna Tracker"; break;
    default: break;
    }

    if (vs->has_fw_info) {
        h.version  = vs->fw_version;
        h.git_hash = vs->fw_hash;
    }
    return h;
}

// One line of a loaded file, waiting to be accepted or dropped. Only parameters
// the vehicle actually has and whose value differs get this far.
struct LoadRow {
    std::string id;
    float       file_val;
    float       vehicle_val;
    bool        selected;
};

// The pending review. Lives across frames because the popup does — cleared when
// the operator applies it or walks away.
static std::vector<LoadRow> s_load_rows;
static int s_load_matched = 0;   // in the file, already equal on the vehicle
static int s_load_unknown = 0;   // in the file, not on this vehicle
static int s_load_skipped = 0;   // unparseable lines

void draw_tab_params(MavlinkSender* sender, const VehicleState* vs,
                     const std::unordered_map<std::string, ParamEntry>* params)
{
    const bool    connected = (vs && vs->has_heartbeat);
    const uint8_t tsys      = connected ? vs->sysid  : 1;
    const uint8_t tcomp     = connected ? vs->compid : 1;

    ImGui::Spacing();
    ImGui::TextColored(accent_col(), "PARAMETERS");
    if (!connected) {
        ImGui::SameLine(0, 6);
        ImGui::TextColored(col_no_link_muted(), "(no link)");
    }
    themed_sep();
    ImGui::Spacing();

    ImGui::BeginDisabled(!connected);
    if (ui_grid_button("FETCH ALL", { -1.0f, 26.0f }) && connected) {
        sender->request_param_list(tsys, tcomp);
        gcs_log("requesting parameter list");
    }
    ImGui::EndDisabled();

    if (vs && vs->param_count > 0) {
        ImGui::Spacing();
        const uint16_t received = vs->params_received;
        const uint16_t total    = vs->param_count;
        if (received < total) {
            char prog_lbl[32];
            snprintf(prog_lbl, sizeof(prog_lbl), "%u / %u", received, total);
            ImGui::ProgressBar((float)received / total, { -1.0f, 12.0f }, prog_lbl);
        } else {
            ImGui::TextDisabled("%u params loaded", total);
        }
    }

    ImGui::Spacing();

    if (!params || params->empty()) {
        ImGui::TextDisabled("Press \"Fetch All\" to load parameters.");
        return;
    }

    // Sorted IDs — rebuilt once per fetch session when count changes
    static std::vector<std::string> sorted_ids;
    static size_t last_param_count = 0;
    if (params->size() != last_param_count) {
        sorted_ids.clear();
        sorted_ids.reserve(params->size());
        for (auto& [k, _] : *params)
            sorted_ids.push_back(k);
        std::sort(sorted_ids.begin(), sorted_ids.end());
        last_param_count = params->size();
    }

    // Pending edits — initialised to current FC value on first appearance
    static std::unordered_map<std::string, float> edited_vals;

    // Rows the operator has moved away from the vehicle's value, in name order
    // so the write goes out and logs in the same order the list shows.
    // Rebuilt every frame: the vehicle echoes each accepted write back as a
    // PARAM_VALUE, which is what makes a row stop being dirty, so this has to
    // follow the live map rather than a flag set at edit time.
    std::vector<const ParamEntry*> dirty;
    for (const std::string& id : sorted_ids) {
        auto ev = edited_vals.find(id);
        if (ev == edited_vals.end()) continue;
        auto it = params->find(id);
        if (it != params->end() && it->second.value != ev->second)
            dirty.push_back(&it->second);
    }
    const int dirty_n = (int)dirty.size();

    // ── Write all ─────────────────────────────────────────────────────────────
    //
    // Confirmed rather than immediate: a single WRITE is one considered change,
    // where this one sends everything staged — including a whole file loaded a
    // moment ago, which the operator may not have read line by line.
    ImGui::BeginDisabled(!connected || dirty_n == 0);
    {
        char lbl[48];
        snprintf(lbl, sizeof(lbl), "WRITE ALL (%d)###write_all", dirty_n);
        const bool hit = dirty_n > 0
            ? ui_solid_button(lbl, { -1.0f, 26.0f },
                              btn_write_base(), btn_write_hov())
            : ui_grid_button(lbl, { -1.0f, 26.0f });
        if (hit) ImGui::OpenPopup("##confirm_write_all");
    }
    ImGui::EndDisabled();

    {
        char q[96];
        snprintf(q, sizeof(q), "WRITE %d CHANGED PARAMETER%s TO THE VEHICLE?",
                 dirty_n, dirty_n == 1 ? "" : "S");
        if (ui_confirm_popup("##confirm_write_all", "WRITE ALL", q,
                             "WRITE ALL", btn_write_base()) == UiConfirm::Confirmed) {
            int sent = 0;
            for (const ParamEntry* p : dirty) {
                auto ev = edited_vals.find(p->param_id);
                if (ev == edited_vals.end()) continue;
                sender->set_param(tsys, tcomp, p->param_id, ev->second, p->type);
                ++sent;
            }
            // No retry behind this: each write is echoed back as a PARAM_VALUE,
            // so anything the vehicle missed stays visibly dirty in the list and
            // the operator can press again.
            gcs_log("writing %d parameters \xe2\x80\x94 rows clear as each is echoed back",
                    sent);
        }
    }

    ImGui::Spacing();

    // ── Parameter file ────────────────────────────────────────────────────────
    static char s_param_path[256] = "parameters.params";

    ImGui::TextDisabled("PARAM FILE");
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputTextWithHint("##param_path", "parameters.params",
                             s_param_path, sizeof(s_param_path));

    {
        const float half = (ImGui::GetContentRegionAvail().x - 4.0f) * 0.5f;

        // LOAD reads the file and opens the picker below — it neither stages nor
        // writes on its own. A parameter file is a whole vehicle's worth of
        // settings, and pulling one in wholesale is rarely what is wanted.
        if (ui_grid_button("LOAD", { half, 24.0f })) {
            std::vector<ParamFileRow> rows;
            std::string err;
            s_load_rows.clear();
            s_load_matched = s_load_unknown = s_load_skipped = 0;

            if (!param_file_load(s_param_path, &rows, &s_load_skipped, &err)) {
                gcs_log("param load failed: %s", err.c_str());
            } else {
                // Only differences are offered. A value the vehicle already
                // holds is not a change to review, and listing 1300 of them
                // would bury the twelve that matter.
                for (const ParamFileRow& r : rows) {
                    auto it = params->find(r.id);
                    if (it == params->end()) { ++s_load_unknown; continue; }
                    if (r.value == it->second.value) { ++s_load_matched; continue; }
                    s_load_rows.push_back(LoadRow{ r.id, r.value,
                                                   it->second.value, true });
                }

                if (s_load_rows.empty()) {
                    gcs_log("%s: nothing to change (%d match, %d not on vehicle, %d bad lines)",
                            s_param_path, s_load_matched, s_load_unknown, s_load_skipped);
                } else {
                    ImGui::OpenPopup("##param_load_pick");
                }
            }
        }

        ImGui::SameLine(0, 4);

        // SAVE writes what the list shows, pending edits included — the file is
        // the configuration you are looking at, not a snapshot of the vehicle.
        // The log says so whenever the two differ.
        if (ui_grid_button("SAVE", { -1.0f, 24.0f })) {
            std::vector<ParamFileRow> rows;
            rows.reserve(sorted_ids.size());
            int pending = 0;
            for (const std::string& id : sorted_ids) {
                auto it = params->find(id);
                if (it == params->end()) continue;
                const ParamEntry& p = it->second;

                float v  = p.value;
                auto  ev = edited_vals.find(id);
                if (ev != edited_vals.end() && ev->second != p.value) {
                    v = ev->second;
                    ++pending;
                }
                rows.push_back(ParamFileRow{ id, v, p.type, tsys, tcomp });
            }

            std::string err;
            if (!param_file_save(s_param_path, rows, file_header(vs), &err)) {
                gcs_log("param save failed: %s", err.c_str());
            } else if (pending > 0) {
                gcs_log("saved %d params to %s (%d are unwritten edits)",
                        (int)rows.size(), s_param_path, pending);
            } else {
                gcs_log("saved %d params to %s", (int)rows.size(), s_param_path);
            }
        }
    }

    // ── Load picker ───────────────────────────────────────────────────────────
    //
    // Everything the file would change, one row each, checked by default. The
    // operator's job here is to uncheck what they did not come for — a config
    // file from another airframe carries plenty that should not follow it over.
    ui_push_dialog_style();
    if (ImGui::BeginPopupModal("##param_load_pick", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
        constexpr float PICK_W = 420.0f;
        constexpr float PICK_H = 320.0f;

        ui_dialog_title("PARAMETERS FROM FILE", PICK_W);

        int selected = 0;
        for (const LoadRow& r : s_load_rows)
            if (r.selected) ++selected;

        ImGui::TextDisabled("%s", s_param_path);
        ImGui::TextDisabled("%d differ \xe2\x80\x94 %d already match, %d not on vehicle, %d bad lines",
                            (int)s_load_rows.size(), s_load_matched,
                            s_load_unknown, s_load_skipped);
        ImGui::Spacing();

        // Bulk selection, with the running count beside it so the APPLY figure
        // below is never a surprise.
        constexpr float SEL_W = 130.0f;
        if (ui_grid_button("SELECT ALL", { SEL_W, 24.0f }))
            for (LoadRow& r : s_load_rows) r.selected = true;
        ImGui::SameLine(0, 4);
        if (ui_grid_button("DESELECT ALL", { SEL_W, 24.0f }))
            for (LoadRow& r : s_load_rows) r.selected = false;
        ImGui::SameLine(0, 8);
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(accent_col(), "%d of %d",
                           selected, (int)s_load_rows.size());

        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_ChildBg, bg_param_list());
        ImGui::PushStyleColor(ImGuiCol_Border,  col_separator());
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, FRAME_BORDER_NORMAL);
        if (ImGui::BeginChild("##load_list", { PICK_W, PICK_H }, true)) {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 4.0f, 3.0f });

            const float row_w  = ImGui::GetContentRegionAvail().x;
            const float from_x = row_w - 190.0f;   // old value column
            const float to_x   = row_w -  90.0f;   // new value column

            for (size_t i = 0; i < s_load_rows.size(); ++i) {
                LoadRow& r = s_load_rows[i];
                ImGui::PushID((int)i);

                ImGui::Checkbox("##sel", &r.selected);
                ImGui::SameLine(0, 6);
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted(r.id.c_str());

                // Vehicle value, then the file's — read left to right as the
                // change that would be staged.
                ImGui::SameLine(from_x);
                ImGui::TextDisabled("%.6g", (double)r.vehicle_val);
                ImGui::SameLine(to_x - 14.0f);
                ImGui::TextDisabled("\xe2\x86\x92");
                ImGui::SameLine(to_x);
                ImGui::TextColored(accent_col(), "%.6g", (double)r.file_val);

                ImGui::PopID();
            }
            ImGui::PopStyleVar();  // ItemSpacing
        }
        ImGui::EndChild();
        ImGui::PopStyleVar();      // FrameBorderSize
        ImGui::PopStyleColor(2);   // Border, ChildBg

        ImGui::Spacing();

        constexpr float BW = 150.0f, BGAP = 8.0f;
        ui_dialog_row(BW * 2.0f + BGAP);

        char apply_lbl[40];
        snprintf(apply_lbl, sizeof(apply_lbl), "STAGE %d###apply_load", selected);

        ImGui::BeginDisabled(selected == 0);
        if (ui_solid_button(apply_lbl, { BW, UI_DIALOG_BH },
                            btn_write_base(), btn_write_hov())) {
            int staged = 0;
            for (const LoadRow& r : s_load_rows) {
                if (!r.selected) continue;
                edited_vals[r.id] = r.file_val;
                ++staged;
            }
            // Staged, not written: they join the pending edits as if typed by
            // hand, and WRITE ALL remains the deliberate second act.
            gcs_log("staged %d parameter%s from %s \xe2\x80\x94 press WRITE ALL to send",
                    staged, staged == 1 ? "" : "s", s_param_path);
            s_load_rows.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndDisabled();

        ImGui::SameLine(0, BGAP);
        if (ui_grid_button("CANCEL##load", { BW, UI_DIALOG_BH }) ||
            ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
            s_load_rows.clear();
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
    ui_pop_dialog_style();

    ImGui::Spacing();
    themed_sep();
    ImGui::Spacing();

    // Search filter
    static char param_search[64] = {};
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputTextWithHint("##param_search", "Search...",
                             param_search, sizeof(param_search));
    ImGui::Spacing();

    // Upper-cased search string for case-insensitive filtering
    char search_upper[64] = {};
    for (int i = 0; i < 63 && param_search[i]; ++i)
        search_upper[i] = (char)toupper((unsigned char)param_search[i]);

    const float avail_h = ImGui::GetContentRegionAvail().y;
    const float val_w   = 150.0f;
    const float btn_w   = 86.0f;

    ImGui::PushStyleColor(ImGuiCol_ChildBg, bg_param_list());
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,   FRAME_ROUNDING_SM);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, FRAME_BORDER_NORMAL);
    ImGui::PushStyleColor(ImGuiCol_Border, col_separator());
    if (ImGui::BeginChild("##params_list", { -1.0f, avail_h }, false)) {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 2.0f, 3.0f });
        const float row_w  = ImGui::GetContentRegionAvail().x;
        const float name_w = row_w - val_w - btn_w - 8.0f;

        for (const std::string& id : sorted_ids) {
            auto it = params->find(id);
            if (it == params->end()) continue;
            const ParamEntry& p = it->second;

            // Filter
            if (search_upper[0] != '\0') {
                char id_upper[17] = {};
                for (int i = 0; i < 16 && p.param_id[i]; ++i)
                    id_upper[i] = (char)toupper((unsigned char)p.param_id[i]);
                if (strstr(id_upper, search_upper) == nullptr) continue;
            }

            ImGui::PushID(p.param_id);

            const ParamMeta* meta = get_param_meta(p.param_id);

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(p.param_id);

            if (meta && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
                ImGui::BeginTooltip();
                ImGui::TextColored(accent_col(), "%s", p.param_id);
                ImGui::Separator();
                ImGui::PushTextWrapPos(320.0f);
                ImGui::TextUnformatted(meta->short_desc);
                ImGui::PopTextWrapPos();
                ImGui::Spacing();
                if (meta->units)
                    ImGui::Text("Units:   %s", meta->units);
                ImGui::Text("Default: %.4g", meta->default_val);
                if (meta->widget == ParamWidgetType::Float &&
                    meta->min_val != meta->max_val)
                    ImGui::Text("Range:   %.4g  \xe2\x80\x93  %.4g",
                                meta->min_val, meta->max_val);
                if (meta->reboot_required)
                    ImGui::TextColored(col_reboot(), "! Reboot required");
                ImGui::EndTooltip();
            }

            ImGui::SameLine(name_w);

            auto [ev_it, inserted] = edited_vals.emplace(id, p.value);
            float& edit_val = ev_it->second;

            if (meta && meta->widget == ParamWidgetType::Enum) {
                const char* preview = "Unknown";
                int32_t cur_int = (int32_t)edit_val;
                for (int e = 0; e < meta->enum_count; ++e) {
                    if (meta->enum_entries[e].value == cur_int) {
                        preview = meta->enum_entries[e].label;
                        break;
                    }
                }
                ImGui::SetNextItemWidth(val_w);
                if (ImGui::BeginCombo("##v", preview, ImGuiComboFlags_HeightLargest)) {
                    for (int e = 0; e < meta->enum_count; ++e) {
                        bool sel = (meta->enum_entries[e].value == cur_int);
                        if (ImGui::Selectable(meta->enum_entries[e].label, sel))
                            edit_val = (float)meta->enum_entries[e].value;
                        if (sel) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

            } else if (meta && meta->widget == ParamWidgetType::Bitmask) {
                int32_t bitmask_val = (int32_t)edit_val;
                char btn_lbl[24];
                snprintf(btn_lbl, sizeof(btn_lbl), "0x%04X###bm", bitmask_val);
                if (ui_grid_button(btn_lbl, { val_w, 0.0f }))
                    ImGui::OpenPopup("##bitmask_popup");

                ui_push_dialog_style();
                if (ImGui::BeginPopup("##bitmask_popup")) {
                    ui_dialog_title(p.param_id, 240.0f);
                    for (int b = 0; b < meta->bitmask_count; ++b) {
                        bool set = (bitmask_val >> meta->bitmask_bits[b].bit) & 1;
                        if (ImGui::Checkbox(meta->bitmask_bits[b].label, &set)) {
                            if (set) bitmask_val |=  (1 << meta->bitmask_bits[b].bit);
                            else     bitmask_val &= ~(1 << meta->bitmask_bits[b].bit);
                            edit_val = (float)bitmask_val;
                        }
                    }
                    ImGui::Spacing();
                    char val_line[48];
                    snprintf(val_line, sizeof(val_line), "0x%04X  (%d)",
                             bitmask_val, bitmask_val);
                    ui_dialog_text(val_line, ui_col_accent());
                    ImGui::EndPopup();
                }
                ui_pop_dialog_style();

            } else {
                ImGui::SetNextItemWidth(val_w);
                ImGui::InputFloat("##v", &edit_val, 0.0f, 0.0f, "%.4g");
            }

            ImGui::SameLine(0, 4);

            const bool dirty = (edit_val != p.value);
            ImGui::BeginDisabled(!connected || !dirty);
            // Amber only once the field differs from the vehicle's value —
            // an unedited row stays in the neutral well like every other button.
            const bool wrote = dirty
                ? ui_solid_button("WRITE", { btn_w, 0.0f },
                                  btn_write_base(), btn_write_hov())
                : ui_grid_button("WRITE", { btn_w, 0.0f });
            if (wrote) {
                sender->set_param(tsys, tcomp, p.param_id, edit_val, p.type);
                gcs_log("writing %s = %.6g", p.param_id, (double)edit_val);
            }
            ImGui::EndDisabled();

            ImGui::PopID();
        }
        ImGui::PopStyleVar(); // ItemSpacing
    }
    ImGui::EndChild();
    ImGui::PopStyleColor(); // Border
    ImGui::PopStyleVar(2);  // FrameRounding, FrameBorderSize
    ImGui::PopStyleColor(); // ChildBg
}
