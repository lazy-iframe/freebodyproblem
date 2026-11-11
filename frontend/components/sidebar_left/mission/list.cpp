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

// frontend/components/sidebar_left/mission/list.cpp
//
// The plan as a list: one row per item, opened to edit it, with the rows
// whose leg comes near the terrain marked.

#include "frontend/components/sidebar_left/mission/mission_internal.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>

namespace mission_tab {

// ── Waypoint list ─────────────────────────────────────────────────────────────
// `plan_home` is home's AMSL by the planning rule; `leg_to[i]` the leg arriving
// at item i, if any.
void draw_waypoint_list(std::optional<float> plan_home,
                        const std::vector<const MissionLeg*>& leg_to)
{
    // Reserve: spacing(4) + upload-btn(26) + spacing(4) +
    //          status-badge-line(~22) + bottom-padding(8) ≈ 64px
    const float list_h = std::max(80.0f,
                                  ImGui::GetContentRegionAvail().y - 64.0f);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, bg_child_dark());
    if (ImGui::BeginChild("##wp_list", { -1.0f, list_h }, false)) {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 4.0f, 2.0f });

        // Ensure open vector is sized
        if ((int)st().open.size() != (int)st().edit.size())
            st().open.resize(st().edit.size(), false);

        int to_delete = -1;

        if (st().edit.empty()) {
            const char*  msg = "MISSION EMPTY";
            const ImVec2 ts  = ImGui::CalcTextSize(msg);
            const ImVec2 av  = ImGui::GetContentRegionAvail();
            ImGui::SetCursorPos({ (av.x - ts.x) * 0.5f, (av.y - ts.y) * 0.5f });
            ImGui::TextDisabled("%s", msg);
        }

        for (int i = 0; i < (int)st().edit.size(); ++i) {
            MissionItem& item = st().edit[i];
            item.seq = (uint16_t)i;  // keep seq in sync with list position

            ImGui::PushID(i);

            // ── Row header (seq, cmd, alt) ────────────────────────────────────
            // The whole row, short of the delete button, toggles the item open
            // and closed.
            const float  del_w  = 18.0f;
            const ImVec2 del_sz = { del_w, ImGui::GetFontSize() + 2.0f };
            const float  row_w  = ImGui::GetContentRegionAvail().x - del_w - 4.0f;

            // A placed item also gets its height above ground, and the row
            // turns red when that is below it — or when the leg arriving at
            // it goes through the ground, amber when that only comes too near.
            char hdr[80];
            const MissionCmdMeta* m = mission_cmd_meta(item.command);
            std::optional<float> agl;
            if (m && m->show_latlon) agl = item_terrain(item, plan_home).agl;
            if (m && m->show_alt && agl)
                snprintf(hdr, sizeof(hdr), "#%u  %s  %.0fm  (ALT AGL TER ~%.0f)", i, m->name,
                         (double)item.alt, (double)*agl);
            else if (m && m->show_alt)
                snprintf(hdr, sizeof(hdr), "#%u  %s  %.0fm", i, m->name, (double)item.alt);
            else
                snprintf(hdr, sizeof(hdr), "#%u  %s", i, m ? m->name : mission_cmd_name(item.command));

            const MissionLeg* leg        = leg_to[i];
            const bool        hits       = leg && leg->c.collides();
            const bool        close_call = leg && leg->c.too_close() && !hits;
            if (hits || close_call) {
                const size_t len = strlen(hdr);
                snprintf(hdr + len, sizeof(hdr) - len, "  !");
            }
            const bool below = (agl && *agl < 0.0f) || hits;
            if (below)
                ImGui::PushStyleColor(ImGuiCol_Text, col_error());
            else if (close_call)
                ImGui::PushStyleColor(ImGuiCol_Text, col_warning());
            else if (item.is_current)
                ImGui::PushStyleColor(ImGuiCol_Text, accent_col());
            if (ImGui::Selectable(hdr, st().open[i], ImGuiSelectableFlags_AllowOverlap,
                                  { row_w, del_sz.y }))
                st().open[i] = !st().open[i];
            if (below || close_call || item.is_current)
                ImGui::PopStyleColor();

            ImGui::SameLine(0, 4);
            if (ui_solid_button("x", del_sz,
                                btn_disconnect_base(), btn_disconnect_hov()))
                to_delete = i;

            // ── Expanded fields ───────────────────────────────────────────────
            if (st().open[i]) {
                // Command selector
                ImGui::Indent(12.0f);
                ImGui::TextDisabled("Cmd");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(-1.0f);
                if (ImGui::BeginCombo("##cmd", mission_cmd_name(item.command))) {
                    for (int c = 0; c < MISSION_CMD_COUNT; ++c) {
                        bool sel = (MISSION_CMDS[c].id == item.command);
                        char opt[32];
                        snprintf(opt, sizeof(opt), "%s (%u)", MISSION_CMDS[c].name, MISSION_CMDS[c].id);
                        if (ImGui::Selectable(opt, sel)) {
                            item.command = MISSION_CMDS[c].id;
                            item.param1 = item.param2 = item.param3 = item.param4 = 0.f;
                        }
                        if (sel) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
                ImGui::Unindent(12.0f);

                draw_wp_fields(item, plan_home, leg);
            }

            ImGui::Separator();
            ImGui::PopID();
        }

        if (to_delete >= 0 && to_delete < (int)st().edit.size()) {
            st().edit.erase(st().edit.begin() + to_delete);
            st().open.erase(st().open.begin() + to_delete);
        }

        ImGui::PopStyleVar();
    }
    ImGui::EndChild();
    ImGui::PopStyleColor(); // ChildBg
}

} // namespace mission_tab
