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


#pragma once
#include <vector>
#include "../backend/mavlink_parser.hpp"

// Passed by pointer to both draw_sidebar_left and draw_map_view each frame.
// Carries two-way state for the "click map to place waypoint" workflow.
struct MissionPickState {
    // active_index >= 0  →  waiting for a map click to assign lat/lon to that waypoint.
    // Set by tab_mission when the user presses a "Pick" button.
    // Cleared by tab_mission after it consumes pick_done (or on Escape / cancel).
    // map_view reads it but does NOT clear it.
    int    active_index = -1;

    // Set by map_view after a successful click.
    // tab_mission reads active_index to know which item to update, writes the
    // coords, then clears both pick_done and active_index.
    bool   pick_done  = false;
    double picked_lat = 0.0;
    double picked_lon = 0.0;

    // Pointer to the live editable mission vector in tab_mission (static lifetime).
    // Updated every frame by tab_mission so map_view renders the current edit state
    // (live edits) instead of the last-fetched vs.mission.
    const std::vector<MissionItem>* edit_mission = nullptr;
};
