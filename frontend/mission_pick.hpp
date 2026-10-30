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
#include "../backend/geofence.hpp"

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

    // The 3D map places an item's height too: set with pick_done, the
    // altitude above home (GLOBAL_RELATIVE_ALT, the frame new items take).
    // Left false by the 2D map, where the item gets the altitude before it.
    bool   has_picked_alt = false;
    float  picked_alt     = 0.0f;

    // EDIT on the mission tab: the map lets any waypoint be dragged to a new
    // position. map_view writes the moved coordinates straight into
    // edit_mission, so the list follows the drag as it happens.
    bool   drag_edit = false;

    // Pointer to the live editable mission vector in tab_mission (static lifetime).
    // Updated every frame by tab_mission so map_view renders the current edit state
    // (live edits) instead of the last-fetched vs.mission — and, in EDIT, moves
    // waypoints in it.
    std::vector<MissionItem>* edit_mission = nullptr;

    // ── Geofence drawing (the mission tab's FENCE view) ──────────────────────
    //
    // fence_draw != None arms the map the way add_command does: each click
    // comes back through pick_done / picked_lat / picked_lon, and the tab
    // builds the shape. A polygon collects its vertices in fence_pts; a circle
    // takes its centre there, then a second click for its radius.
    // fence_inclusion is which kind the shape will be.
    enum class FenceDraw : uint8_t { None, Polygon, Circle };
    FenceDraw                fence_draw      = FenceDraw::None;
    bool                     fence_inclusion = true;
    std::vector<FenceVertex> fence_pts;

    // Set by map_view with pick_done when the click landed on a polygon's first
    // vertex: close the polygon rather than add a vertex there.
    bool fence_close = false;

    // EDIT in the FENCE view: polygon vertices and circle centres can be
    // dragged. map_view writes into edit_fence, which the tab points at its
    // plan every frame (null when the FENCE view has nothing to draw) — the
    // map draws it in preference to the vehicle's own fence.
    bool       fence_edit = false;
    FencePlan* edit_fence = nullptr;

    bool fence_drawing() const { return fence_draw != FenceDraw::None; }
    // A click on the map is wanted: for a mission item or for a fence shape.
    bool picking() const { return add_command != 0 || fence_drawing(); }

    void cancel_fence_draw() { fence_draw = FenceDraw::None; fence_pts.clear(); }
};
