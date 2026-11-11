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

// frontend/components/sidebar_left/mission/add.cpp
//
// Adding items: the grid of common commands and MORE for the rest. A command
// with a position arms a map pick; one without is appended at once.

#include "frontend/components/sidebar_left/mission/mission_internal.hpp"
#include "frontend/source/app_log.hpp"
#include <cstdio>

namespace mission_tab {

// ── Add grid ─────────────────────────────────────────────────────────────────
//
// The commands a mission is mostly made of, one button each. A command with a
// position arms a map click — its button reads LOC? until the map is clicked
// there — and one without is appended at once. MORE opens the rest of
// MISSION_CMDS as the same kind of grid.
struct QuickCmd { uint16_t id; const char* label; };
static constexpr QuickCmd QUICK_CMDS[] = {
    {  16, "WAYPOINT"   }, {  22, "TAKEOFF"    }, {  21, "LAND"     },
    {  20, "RTL"        }, {  17, "LOITER"     }, {  19, "LOIT TIME"},
    {  18, "LOIT TURNS" }, { 201, "ROI"        }, { 178, "SPEED"    },
    {  93, "DELAY"      }, { 177, "JUMP"       },
};
static constexpr int QUICK_COUNT = (int)(sizeof(QUICK_CMDS) / sizeof(QUICK_CMDS[0]));
static constexpr int GRID_COLS   = 3;

static bool is_quick(uint16_t cmd)
{
    for (const QuickCmd& q : QUICK_CMDS)
        if (q.id == cmd) return true;
    return false;
}

// Altitude for a new item: the last one given, else a safe default.
static constexpr float DEFAULT_ALT_M = 20.0f;

// Append a new item. `lat`/`lon` are where the map was clicked, or 0/0 for a
// command that has no position; `alt` is the height the 3D map set, if it did.
void append_item(uint16_t cmd, double lat, double lon,
                 std::optional<float> alt)
{
    MissionItem ni{};
    ni.seq     = (uint16_t)st().edit.size();
    ni.command = cmd;
    ni.frame   = 3; // MAV_FRAME_GLOBAL_RELATIVE_ALT
    ni.lat     = lat;
    ni.lon     = lon;
    ni.alt     = DEFAULT_ALT_M;
    for (int k = (int)st().edit.size() - 1; k >= 0; --k) {
        const MissionItem& prev = st().edit[k];
        const MissionCmdMeta* pm = mission_cmd_meta(prev.command);
        if (!pm || pm->show_alt || pm->show_latlon) { ni.alt = prev.alt; break; }
    }
    if (alt) ni.alt = *alt;
    // The new item opens and every other one closes: what was just added is
    // what is about to be filled in, and the list stays one item tall else.
    st().edit.push_back(ni);
    st().open.assign(st().edit.size() - 1, false);
    st().open.push_back(true);
    st().has_edit = true;      // protect manual edits from auto-sync
    st().cleared  = false;
}

// One button of the add grid. A positional command arms the map and reads
// LOC? on the active plate while it waits; pressing it again disarms.
// Returns true when it closed over a popup's worth of choice.
static bool add_button(MissionPickState* pick, uint16_t cmd, const char* label,
                       ImVec2 size)
{
    const MissionCmdMeta* m      = mission_cmd_meta(cmd);
    const bool     placed = m && m->show_latlon;
    const bool     armed  = pick && pick->add_command == cmd;

    char id[48];
    snprintf(id, sizeof(id), "%s###add%u", armed ? "LOC?" : label, (unsigned)cmd);
    if (!ui_grid_button(id, size, armed)) {
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
            ImGui::SetTooltip(placed ? "%s \xe2\x80\x94 then click the map to place it"
                                     : "%s \xe2\x80\x94 added to the end of the list",
                              m ? m->name : label);
        return false;
    }

    if (armed) {
        pick->add_command = 0;
    } else if (placed && pick) {
        pick->add_command  = cmd;
        pick->add_label    = m->name;
    } else {
        append_item(cmd, 0.0, 0.0);
        if (pick) pick->add_command = 0;
    }
    return true;
}

void draw_add_grid(MissionPickState* pick)
{
    const float gap   = 4.0f;
    const float col_w = (ImGui::GetContentRegionAvail().x - gap * (GRID_COLS - 1)) / GRID_COLS;
    const ImVec2 sz   = { col_w, 26.0f };

    for (int i = 0; i < QUICK_COUNT; ++i) {
        if (i % GRID_COLS != 0) ImGui::SameLine(0, gap);
        add_button(pick, QUICK_CMDS[i].id, QUICK_CMDS[i].label, sz);
    }

    // MORE: everything else, lit while one of those waits for its LOC.
    if (QUICK_COUNT % GRID_COLS != 0) ImGui::SameLine(0, gap);
    const bool more_armed = pick && pick->add_command != 0 && !is_quick(pick->add_command);
    if (ui_grid_button(more_armed ? "LOC?###more" : "MORE...###more", sz, more_armed)) {
        if (more_armed) pick->add_command = 0;
        else            ImGui::OpenPopup("##more_cmds");
    }

    ui_push_dialog_style();
    if (ImGui::BeginPopup("##more_cmds")) {
        constexpr float POP_W = 300.0f;
        ui_dialog_title("ADD MISSION ITEM", POP_W);
        const float pw = (POP_W - gap) * 0.5f;
        int n = 0;
        for (int c = 0; c < MISSION_CMD_COUNT; ++c) {
            if (is_quick(MISSION_CMDS[c].id)) continue;
            if (n++ % 2 != 0) ImGui::SameLine(0, gap);
            if (add_button(pick, MISSION_CMDS[c].id, MISSION_CMDS[c].name, { pw, 24.0f }))
                ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    ui_pop_dialog_style();
}

} // namespace mission_tab
