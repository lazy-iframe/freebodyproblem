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
// Carries two-way state for placing and moving waypoints on the map.
struct MissionPickState {
    // add_command != 0  →  waiting for a map click to append a new item with
    // this MAV_CMD there. Set by the mission tab's command grid, whose button
    // reads "LOC?" meanwhile; cleared by tab_mission once it has added the item.
    // add_label names it in the map's hint banner.
    uint16_t    add_command = 0;
    const char* add_label   = nullptr;

    // Set by map_view after a successful click. tab_mission adds the item
    // there, then clears pick_done and add_command; map_view never clears them.
    bool   pick_done  = false;
    double picked_lat = 0.0;
    double picked_lon = 0.0;

    // EDIT on the mission tab: the map lets any waypoint be dragged to a new
    // position. map_view writes the moved coordinates straight into
    // edit_mission, so the list follows the drag as it happens.
    bool   drag_edit = false;

    // Pointer to the live editable mission vector in tab_mission (static lifetime).
    // Updated every frame by tab_mission so map_view renders the current edit state
    // (live edits) instead of the last-fetched vs.mission — and, in EDIT, moves
    // waypoints in it.
    std::vector<MissionItem>* edit_mission = nullptr;

    bool picking() const { return add_command != 0; }
};
