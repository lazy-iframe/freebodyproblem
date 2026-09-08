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
#include "../../backend/vehicle.hpp"
#include "../goto_target.hpp"
#include "../mission_pick.hpp"
#include <string>
#include <vector>

// Set the tile URL template and attribution string.  Safe to call from the main
// thread at any time; workers pick up the new host on their next request.
// URL format: "https://host/path/{z}/{x}/{y}" — {z}/{x}/{y} are substituted per tile.
// Some providers use {z}/{y}/{x} ordering (e.g. Esri) — match your provider.
void map_view_set_tile_source(const std::string& url, const std::string& attribution);

// One aircraft as the map draws it. Built by the render loop, which is already
// snapshotting every vehicle each frame, so this costs no extra locking.
//
// The whole fleet is drawn, not only the vehicle the panels are bound to: an
// operator flying three aircraft needs to see where the other two are, and a
// map that hides them is the one thing a moving map must not do.
struct MapVehicle {
    VehicleId id;                 // keys this vehicle's trail
    uint32_t  number  = 0;        // Fleet's display number — the "(2)" in the label
    uint8_t   sysid   = 0;
    double    lat     = 0.0;
    double    lon     = 0.0;
    bool      has_pos = false;
    float     heading = 0.0f;
    bool      has_hdg = false;
    bool      active  = false;    // the vehicle every other panel is showing

    // HOME_POSITION. Where this aircraft returns to, drawn as a helipad.
    double    home_lat = 0.0;
    double    home_lon = 0.0;
    bool      has_home = false;
};

// Record one position sample on the bound vehicle's trail.
//
// Called once per vehicle per frame by the render loop, not from the draw
// below: the trail is a record of where the aircraft went, and an aircraft
// that flew while the operator was watching another one still went there. A
// trail that only grew while its vehicle was on screen would draw a straight
// line across every switch.
//
// Samples are thinned by distance, so a vehicle sitting still costs nothing
// and the point count follows ground track rather than frame rate.
void map_track_pump(const VehicleState& vs);

// Drop the bound vehicle's trail. The map's own right-click menu offers this;
// declared here because the trail is public state, not the menu's.
void map_track_clear();

// Starts the tile-fetch thread on first call; must be paired with a single
// call to map_view_shutdown() before the OpenGL context is destroyed.
void map_view_shutdown();

// Render the slippy-map panel.  Call once per frame from the GL (main) thread.
//   lat/lon   – vehicle position in degrees (ignored when has_pos is false)
//   has_pos   – false while GPS fix is absent
//   heading   – vehicle heading in degrees [0–360), clockwise from north
//   has_hdg   – false when heading is unavailable (shows dot marker instead of arrow)
//   win_*     – screen-space rectangle for the ImGui window slot
//   mission   – fetched mission waypoint list; nullptr = no overlay
//   pick      – optional shared pick state; when active_index >= 0 the map
//               captures the next left-click as a lat/lon for that waypoint
//   alt_rel   – altitude above home in metres, shown in the A/C overlay
//   gs        – ground speed in m/s, shown in the A/C overlay
//   go        – optional GO HERE state; when present a right-click opens the
//               map's context menu and a confirmed target is written back for
//               the caller to send
//   fleet     – every vehicle to draw, the active one among them. nullptr or
//               empty falls back to drawing the lat/lon above on its own, which
//               is what the scalar arguments are still for: the overlay, the
//               follow-the-vehicle centring, and callers with one aircraft
void draw_map_view(double lat, double lon, bool has_pos,
                   float heading, bool has_hdg,
                   float win_x, float win_y, float win_w, float win_h,
                   const std::vector<MissionItem>* mission = nullptr,
                   MissionPickState* pick = nullptr,
                   float alt_rel = 0.0f, float gs = 0.0f,
                   GotoTargetState* go = nullptr,
                   const std::vector<MapVehicle>* fleet = nullptr);
