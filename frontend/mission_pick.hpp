// Free Body Problem: A modern, simple and minimalist Ground Control Station
// Copyright (C) 2026  Azhar Tanweer

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.


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
