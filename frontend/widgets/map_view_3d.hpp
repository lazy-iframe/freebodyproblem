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


// frontend/widgets/map_view_3d.hpp
//
// The map in 3D: the same imagery tiles draped over the terrain heights of
// frontend/terrain.hpp, seen from a camera that orbits a point on the ground.
// Drawn by draw_map_view() in place of the flat map when its 3D button is on,
// inside the same window and from the same inputs, so nothing outside the map
// knows which of the two is showing.
//
// New mission items are placed here in 3D; dragging waypoints and drawing
// fences stay on the flat map, which the map returns to when either starts.

#pragma once
#include "map_view.hpp"
#include "imgui.h"
#include <vector>

// Where the camera looks, shared with the 2D map so switching between them
// stays on the same spot: the map's centre, and whether it follows the active
// vehicle. A pan in 3D moves the centre and stops following, as in 2D.
struct Map3DCenter {
    double& lat;
    double& lon;
    bool&   follow;
};

// What the 3D view draws. `fleet` holds at least the active vehicle;
// `mission` may be null. With `pick` armed for a mission item (LOC? on the
// mission tab), the view places it: a click on the ground for where, then
// the pointer slides it up and down its vertical line and a second click
// sets the height. The result goes back through `pick` as the 2D map's does.
//
// With `go`, a right-click on the view picks a GO HERE point the same way —
// a right-click on the ground for where, another for the height — and
// map3d_take_goto() then hands it to the map for its GO HERE menu. The
// standing target, once sent, is drawn from `go` too.
struct Map3DScene {
    const std::vector<MapVehicle>*  fleet   = nullptr;
    const std::vector<MissionItem>* mission = nullptr;
    MissionPickState*               pick    = nullptr;
    const GotoTargetState*          go      = nullptr;
    // The geofence zones to stand as walls: the fence editor's plan while it
    // has one, else the active vehicle's as downloaded. May be null. The
    // parameter cylinder comes from the active vehicle in `fleet`.
    const FencePlan*                fence   = nullptr;
};

// False when the driver lacks what the 3D view needs (OpenGL 3.3 shaders and
// framebuffers), so the map can leave its 3D button off. Main thread only.
bool map3d_available();

// Draw the 3D view into the rectangle p0..p0+size of the current window, and
// handle the pointer when `hovered`: left-drag orbits, right-drag pans, the
// wheel zooms. Returns false if the view could not draw at all this frame.
bool map3d_draw(ImDrawList* dl, ImVec2 p0, ImVec2 size, bool hovered,
                Map3DCenter center, const Map3DScene& scene);

// Vertical exaggeration, for the map's button beside 3D to show and step
// through; kept for the session.
float map3d_exaggeration();
void  map3d_cycle_exaggeration();

// Imagery draped on the terrain, or off for the bare elevation mesh in olive —
// the shape of the ground with nothing over it. Off, no imagery is fetched.
// For the map's TEX button; kept for the session.
bool map3d_textures();
void map3d_toggle_textures();

// The way the camera looks, clockwise from north — for the north pointer.
float map3d_yaw_deg();

// The GO HERE point picked in 3D, once its height has been clicked: where,
// and how high above home. True once per point; the map opens its menu there.
bool map3d_take_goto(double& lat, double& lon, float& rel_alt);

// A GO HERE point is being picked: Esc belongs to it.
bool map3d_picking_goto();

// Release the GL objects. Call before the context goes.
void map3d_shutdown();
