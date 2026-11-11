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

// frontend/components/sidebar_left/tab_params.cpp
//
// The PARAMS tab: fetching, the searchable list with its staged edits, and
// WRITE ALL / DISCARD ALL. The row editors, the info popup and the parameter
// files are in params/.

#include "frontend/components/sidebar_left/params/params_internal.hpp"
#include "frontend/source/app_log.hpp"
#include "frontend/source/audio.hpp"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

namespace params_tab {

static VehicleUiState<ParamsTabState> s_state;
ParamsTabState& st() { return *s_state; }

} // namespace params_tab

using namespace params_tab;

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
        char prog_lbl[32];
        snprintf(prog_lbl, sizeof(prog_lbl), "%u / %u", received, total);

        if (vs->param_fetch_active && received < total) {
            const float frac = (float)received / total;
            ui_progress_bar(prog_lbl, frac, 12.0f);
            // Ticks alongside the bar, speeding up as it fills. A full fetch is
            // a thousand-odd parameters and the better part of a minute, most
            // of it spent looking somewhere other than this panel.
            gcs_progress("params", frac);
        } else if (received >= total) {
            ImGui::TextDisabled("%u params loaded", total);
        } else {
            // Parameters we hold without having asked for them: a vehicle sends
            // a PARAM_VALUE whenever one of its parameters changes, and the
            // stream rates this GCS sets on connect are parameters. That is a
            // count of what we happen to know, not a fetch in progress, so it
            // gets a number and no bar.
            ImGui::TextDisabled("%u of %u known", received, total);
        }
    }

    ImGui::Spacing();

    if (!params || params->empty()) {
        ImGui::TextDisabled("Press \"Fetch All\" to load parameters.");
        return;
    }

    // Sorted IDs — rebuilt once per fetch session when count changes
    auto& sorted_ids       = st().sorted_ids;
    auto& last_param_count = st().last_param_count;
    if (params->size() != last_param_count) {
        sorted_ids.clear();
        sorted_ids.reserve(params->size());
        for (auto& [k, _] : *params)
            sorted_ids.push_back(k);
        std::sort(sorted_ids.begin(), sorted_ids.end());
        last_param_count = params->size();
    }

    // Values the operator has staged: typed into a row, picked from an enum or
    // bitmask, or loaded from a parameter file. A row appears here only once it
    // has actually been moved, and leaves again as soon as the vehicle's value
    // agrees with it — which is what an accepted write looks like.
    //
    // Not every row seeded with the vehicle's value on sight, which is what
    // this used to do. A seeded row is indistinguishable from an edited one, so
    // any parameter the *vehicle* changed on its own afterwards became a
    // pending edit that nobody made: the seed stayed at the old value while the
    // live one moved. Vehicles do this routinely — ArduPilot saves its
    // STAT_RUNTIME / STAT_FLTTIME counters as it goes, and every save is
    // broadcast as a PARAM_VALUE — so a fetch and no typing at all could leave
    // WRITE ALL offering to write one parameter back. Offering, worse, to write
    // the *stale* value over the vehicle's newer one.
    auto& edited_vals = st().edited_vals;

    const ParamDocSource* docs = param_docs_for(vs);
    const bool heli    = vs && vs->type == MAV_TYPE_HELICOPTER;
    // The release whose page fits the firmware, from AUTOPILOT_VERSION; until
    // that arrives, -1, which the lookup reads as the newest.
    const int  fw_doc  = (docs && vs->has_fw_info)
                       ? param_doc_version_for(*docs, vs->fw_version) : -1;

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

    // Staged values outside their documented range, for WRITE ALL to mention.
    int dirty_oor = 0;
    for (const ParamEntry* p : dirty) {
        const ParamMeta* meta = docs ? get_param_meta(*docs, p->param_id, heli, fw_doc) : nullptr;
        if (param_out_of_range(meta, param_effective_kind(meta, p->type),
                               edited_vals[p->param_id]))
            ++dirty_oor;
    }

    // ── Write all / discard all ───────────────────────────────────────────────
    //
    // Both confirmed rather than immediate: a single WRITE is one considered
    // change, where these two act on everything staged — including a whole file
    // loaded a moment ago, which the operator may not have read line by line.
    // Discarding is local and touches no vehicle, but there is no undo behind
    // it and a staged file can be hundreds of rows, so it asks too. The per-row
    // cross is the unconfirmed way out, because it can only lose one value.
    const float half_w = (ImGui::GetContentRegionAvail().x - 4.0f) * 0.5f;

    ImGui::BeginDisabled(!connected || dirty_n == 0);
    {
        char lbl[48];
        snprintf(lbl, sizeof(lbl), "WRITE ALL (%d)###write_all", dirty_n);
        const bool hit = dirty_n > 0
            ? ui_solid_button(lbl, { half_w, 26.0f },
                              btn_write_base(), btn_write_hov())
            : ui_grid_button(lbl, { half_w, 26.0f });
        if (hit) ImGui::OpenPopup("##confirm_write_all");
    }
    ImGui::EndDisabled();

    ImGui::SameLine(0, 4);

    // Not gated on `connected`, unlike WRITE ALL: throwing away your own edits
    // is something you can always do, and being unable to clear them because
    // the link happened to drop would be its own annoyance.
    ImGui::BeginDisabled(dirty_n == 0);
    {
        char lbl[48];
        snprintf(lbl, sizeof(lbl), "DISCARD (%d)###discard_all", dirty_n);
        const bool hit = dirty_n > 0
            ? ui_solid_button(lbl, { half_w, 26.0f },
                              btn_disconnect_base(), btn_disconnect_hov())
            : ui_grid_button(lbl, { half_w, 26.0f });
        if (hit) ImGui::OpenPopup("##confirm_discard_all");
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered() && dirty_n > 0)
        ImGui::SetTooltip("Discard all staged changes — the vehicle is not touched");

    {
        char q[96];
        snprintf(q, sizeof(q), "DISCARD %d STAGED CHANGE%s?",
                 dirty_n, dirty_n == 1 ? "" : "S");
        if (ui_confirm_popup("##confirm_discard_all", "DISCARD CHANGES", q,
                             "DISCARD", btn_disconnect_base()) == UiConfirm::Confirmed) {
            const int n = (int)edited_vals.size();
            edited_vals.clear();
            gcs_log("discarded %d staged param change%s", n, n == 1 ? "" : "s");
        }
    }

    {
        char q[160];
        int  qn = snprintf(q, sizeof(q), "WRITE %d CHANGED PARAMETER%s TO THE VEHICLE?",
                           dirty_n, dirty_n == 1 ? "" : "S");
        if (dirty_oor > 0 && qn > 0 && (size_t)qn < sizeof(q))
            snprintf(q + qn, sizeof(q) - qn, "\n%d %s OUTSIDE THE DOCUMENTED RANGE.",
                     dirty_oor, dirty_oor == 1 ? "IS" : "ARE");
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

    draw_param_file(vs, params, tsys, tcomp);

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

    // Which page the rows are read against, so a mismatch is visible before
    // anyone opens an "i".
    if (docs) {
        char lbl[16];
        param_doc_version_label(*docs, fw_doc >= 0 ? fw_doc : (int)docs->version_count - 1,
                                lbl, sizeof(lbl));
        ImGui::PushTextWrapPos(0.0f);
        if (fw_doc >= 0)
            ImGui::TextDisabled("Docs: %s %s reference (firmware %s)",
                                docs->name, lbl, vs->fw_version);
        else
            ImGui::TextDisabled("Docs: %s %s reference until the firmware reports its version",
                                docs->name, lbl);
        ImGui::PopTextWrapPos();
    } else if (vs && vs->has_heartbeat) {
        ImGui::PushTextWrapPos(0.0f);
        ImGui::TextDisabled("Docs: none for this vehicle (ArduPilot Copter and PX4 only)");
        ImGui::PopTextWrapPos();
    }
    ImGui::Spacing();

    const float avail_h = ImGui::GetContentRegionAvail().y;
    const float val_w   = 150.0f;
    // The info button and WRITE share what WRITE alone used to take, for the
    // same reason the discard cross comes out of the value column below.
    const float btn_w   = 86.0f;
    const float info_w  = 22.0f;
    const float write_w = btn_w - info_w - 4.0f;
    // Discard-one cross, and the input width left once it is taken out. The
    // cross comes out of the value column rather than being added to the row:
    // the name column is already the one under pressure — left_w is 24% of the
    // window, so on a small display it has barely room for a 16-character
    // parameter ID — and widening the row would come out of exactly that.
    // Reserved on every row, drawn or not, so the WRITE column does not jump
    // sideways as rows are edited and cleared.
    const float x_w     = 22.0f;
    const float in_w    = val_w - x_w - 4.0f;

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

            int doc = -1;
            const ParamMeta* meta = docs
                ? get_param_meta(*docs, p.param_id, heli, fw_doc, &doc) : nullptr;
            const ParamKind  kind = param_effective_kind(meta, p.type);
            const bool       ro   = meta && meta->has(PARAM_FLAG_READONLY);

            // The editor drawn: the parameter's own, unless it was switched to
            // manual entry, which edits an enum or bitmask as the plain number
            // the vehicle stores.
            bool manual = (kind == ParamKind::Enum || kind == ParamKind::Bitmask) &&
                          st().manual.count(id) > 0;
            const ParamKind ui_kind = manual ? param_effective_kind(nullptr, p.type) : kind;

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(p.param_id);

            // Just the name on hover; the whole entry is behind "i".
            if (meta && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted(meta->display_name);
                if (meta->units) ImGui::TextDisabled("%s", meta->units);
                ImGui::EndTooltip();
            }

            ImGui::SameLine(name_w);

            // What the row shows: the staged value if there is one, otherwise
            // the vehicle's. A plain local, not a reference into the map —
            // nothing is inserted until an edit below actually happens. ImGui
            // keeps its own buffer while a field is being typed into, so the
            // backing float only has to survive the frame.
            auto  ev       = edited_vals.find(id);
            float edit_val = (ev != edited_vals.end()) ? ev->second : p.value;
            bool  edited   = false;

            ImGui::BeginDisabled(ro);
            edited = draw_value_editor(p, meta, kind, ui_kind, manual, edit_val, in_w);
            ImGui::EndDisabled();

            // Stage on edit; unstage once the vehicle agrees again, which is
            // both how an accepted write clears a row and how typing a value
            // back to what it was clears it.
            if (edited) {
                edited_vals[id] = edit_val;
            } else if (ev != edited_vals.end() && ev->second == p.value) {
                edited_vals.erase(ev);
            }

            bool dirty = (edit_val != p.value);

            // Discard this one row. Unconfirmed — it can lose exactly one value,
            // and the value it restores is sitting right there on the vehicle.
            ImGui::SameLine(0, 4);
            if (dirty) {
                if (ui_grid_button("\xc3\x97" "###discard_one", { x_w, 0.0f })) {
                    edited_vals.erase(id);
                    // Restore the row in this same frame, so the WRITE button
                    // beside it does not stay lit until the next one.
                    edit_val = p.value;
                    dirty    = false;
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Discard this change (back to %.4g)",
                                      (double)p.value);
            } else {
                ImGui::Dummy({ x_w, ImGui::GetFrameHeight() });
            }

            ImGui::SameLine(0, 4);

            // Everything the reference says about this parameter. Live with or
            // without a link — reading documentation needs no vehicle.
            if (ui_grid_button("i###info", { info_w, 0.0f }))
                ImGui::OpenPopup("##param_info");
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
                ImGui::SetTooltip("Parameter info");

            ui_push_dialog_style();
            if (ImGui::BeginPopup("##param_info")) {
                const bool was = manual;
                draw_param_info(p, meta, kind, edit_val, docs, fw_doc, doc,
                                (kind == ParamKind::Enum || kind == ParamKind::Bitmask)
                                    ? &manual : nullptr);
                if (manual != was) {
                    if (manual) st().manual.insert(id);
                    else        st().manual.erase(id);
                }
                ImGui::EndPopup();
            }
            ui_pop_dialog_style();

            ImGui::SameLine(0, 4);

            ImGui::BeginDisabled(!connected || !dirty || ro);
            // Amber only once the field differs from the vehicle's value —
            // an unedited row stays in the neutral well like every other button.
            const bool wrote = dirty
                ? ui_solid_button("WRITE", { write_w, 0.0f },
                                  btn_write_base(), btn_write_hov())
                : ui_grid_button("WRITE", { write_w, 0.0f });
            if (wrote) {
                sender->set_param(tsys, tcomp, p.param_id, edit_val, p.type);
                if (param_out_of_range(meta, kind, edit_val)) {
                    char rng[64];
                    param_format_range(meta, rng, sizeof(rng));
                    gcs_log("writing %s = %.6g (outside documented range %s)",
                            p.param_id, (double)edit_val, rng);
                } else
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
