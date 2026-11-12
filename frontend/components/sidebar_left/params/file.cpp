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

// frontend/components/sidebar_left/params/file.cpp
//
// Parameter files: LOAD reads one, compares it with the vehicle and offers
// the differences for review before staging any; SAVE writes what the list
// shows. Both name their file through the app's file browser.

#include "frontend/components/sidebar_left/params/params_internal.hpp"
#include "frontend/components/others/file_dialog.hpp"
#include "frontend/source/param_file.hpp"
#include "backend/vehicle/sensor_inventory.hpp"
#include <cctype>
#include <cstdio>
#include <string>
#include <vector>

namespace params_tab {

// What the file's comment header claims about the vehicle it came off. Only
// what the link has actually told us — a header is documentation, and a guessed
// one is worse than a missing one.
static ParamFileHeader file_header(const VehicleState* vs)
{
    ParamFileHeader h;
    if (!vs) return h;

    h.sysid = vs->sysid ? vs->sysid : 1;

    // The names the connection panel shows, the vehicle's capitalised as a
    // heading is. Left out for a value the table does not know.
    if (const char* stack = mav_autopilot_name(vs->autopilot)) h.stack = stack;
    if (const char* type = mav_vehicle_type_name(vs->type)) {
        h.vehicle = type;
        h.vehicle[0] = (char)std::toupper((unsigned char)h.vehicle[0]);
    }

    if (vs->has_fw_info) {
        h.version  = vs->fw_version;
        h.git_hash = vs->fw_hash;
    }
    return h;
}

static constexpr const char* PARAM_FILTERS = "Parameter files (*.params){.params},All files{.*}";
static const FileDialogSpec  LOAD_DLG = { "param_load_dlg", "LOAD PARAMETERS", PARAM_FILTERS,
                                          "parameters.params", /*for_save=*/false };
static const FileDialogSpec  SAVE_DLG = { "param_save_dlg", "SAVE PARAMETERS", PARAM_FILTERS,
                                          "parameters.params", /*for_save=*/true };

void draw_param_file(const VehicleState* vs,
                     const std::unordered_map<std::string, ParamEntry>* params,
                     uint8_t tsys, uint8_t tcomp)
{
    const auto& sorted_ids  = st().sorted_ids;
    auto&       edited_vals = st().edited_vals;

    // ── Parameter file ────────────────────────────────────────────────────────
    //
    // Both buttons name their file through the browser rather than off a typed
    // path: a .params file lives wherever the operator keeps their configs, and
    // an absolute path is a thing to find, not to remember and retype. What is
    // kept between presses is only the last file touched, which seeds the
    // browser so a save after a load lands next to what was loaded.
    static std::string s_param_path = "parameters.params";

    ImGui::TextDisabled("PARAM FILE");
    {
        const float half = (ImGui::GetContentRegionAvail().x - 4.0f) * 0.5f;

        if (ui_grid_button("LOAD", { half, 24.0f }))
            file_dialog_open(LOAD_DLG, s_param_path);

        ImGui::SameLine(0, 4);

        if (ui_grid_button("SAVE", { -1.0f, 24.0f }))
            file_dialog_open(SAVE_DLG, s_param_path);
    }

    // ── The two browsers ──────────────────────────────────────────────────────
    //
    // Polled every frame whether or not their button was pressed — Display()
    // returns immediately for a key that is not the open dialog, so only one
    // of these ever does anything, and only one can be open at a time.
    //
    // The work each button used to do inline happens here instead, once the
    // operator has named a file. Nothing is read or written until they do.
    if (file_dialog_poll(LOAD_DLG.key, s_param_path)) {
        // Reads the file and opens the picker below — it neither stages nor
        // writes on its own. A parameter file is a whole vehicle's worth of
        // settings, and pulling one in wholesale is rarely what is wanted.
        std::vector<ParamFileRow> rows;
        std::string err;
        st().load_rows.clear();
        st().load_matched = st().load_unknown = st().load_skipped = 0;

        if (!param_file_load(s_param_path, &rows, &st().load_skipped, &err)) {
            gcs_log("param load failed: %s", err.c_str());
        } else {
            // Only differences are offered. A value the vehicle already
            // holds is not a change to review, and listing 1300 of them
            // would bury the twelve that matter.
            for (const ParamFileRow& r : rows) {
                auto it = params->find(r.id);
                if (it == params->end()) { ++st().load_unknown; continue; }
                if (r.value == it->second.value) { ++st().load_matched; continue; }
                st().load_rows.push_back(LoadRow{ r.id, r.value,
                                               it->second.value, true });
            }

            if (st().load_rows.empty()) {
                gcs_log("%s: nothing to change (%d match, %d not on vehicle, %d bad lines)",
                        s_param_path.c_str(), st().load_matched,
                        st().load_unknown, st().load_skipped);
            } else {
                // The picker is submitted further down this same function,
                // so it opens on this frame rather than the next one.
                ImGui::OpenPopup("##param_load_pick");
            }
        }
    }

    if (file_dialog_poll(SAVE_DLG.key, s_param_path)) {
        // Writes what the list shows, pending edits included — the file is
        // the configuration you are looking at, not a snapshot of the
        // vehicle. The log says so whenever the two differ.
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
                    (int)rows.size(), s_param_path.c_str(), pending);
        } else {
            gcs_log("saved %d params to %s", (int)rows.size(),
                    s_param_path.c_str());
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
        for (const LoadRow& r : st().load_rows)
            if (r.selected) ++selected;

        ImGui::TextDisabled("%s", s_param_path.c_str());
        ImGui::PushTextWrapPos(0.0f);
        ImGui::TextDisabled("%d differ \xe2\x80\x94 %d already match, %d not on vehicle, %d bad lines",
                            (int)st().load_rows.size(), st().load_matched,
                            st().load_unknown, st().load_skipped);
        ImGui::PopTextWrapPos();
        ImGui::Spacing();

        // Bulk selection, with the running count beside it so the APPLY figure
        // below is never a surprise.
        constexpr float SEL_W = 130.0f;
        if (ui_grid_button("SELECT ALL", { SEL_W, 24.0f }))
            for (LoadRow& r : st().load_rows) r.selected = true;
        ImGui::SameLine(0, 4);
        if (ui_grid_button("DESELECT ALL", { SEL_W, 24.0f }))
            for (LoadRow& r : st().load_rows) r.selected = false;
        ImGui::SameLine(0, 8);
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(accent_col(), "%d of %d",
                           selected, (int)st().load_rows.size());

        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_ChildBg, bg_param_list());
        ImGui::PushStyleColor(ImGuiCol_Border,  col_separator());
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, FRAME_BORDER_NORMAL);
        if (ImGui::BeginChild("##load_list", { PICK_W, PICK_H }, true)) {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 4.0f, 3.0f });

            const float row_w  = ImGui::GetContentRegionAvail().x;
            const float from_x = row_w - 190.0f;   // old value column
            const float to_x   = row_w -  90.0f;   // new value column

            for (size_t i = 0; i < st().load_rows.size(); ++i) {
                LoadRow& r = st().load_rows[i];
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
            for (const LoadRow& r : st().load_rows) {
                if (!r.selected) continue;
                edited_vals[r.id] = r.file_val;
                ++staged;
            }
            // Staged, not written: they join the pending edits as if typed by
            // hand, and WRITE ALL remains the deliberate second act.
            gcs_log("staged %d parameter%s from %s \xe2\x80\x94 press WRITE ALL to send",
                    staged, staged == 1 ? "" : "s", s_param_path.c_str());
            st().load_rows.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndDisabled();

        ImGui::SameLine(0, BGAP);
        if (ui_grid_button("CANCEL##load", { BW, UI_DIALOG_BH }) ||
            ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
            st().load_rows.clear();
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
    ui_pop_dialog_style();
}

} // namespace params_tab
