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
#include "../../backend/mavlink_parser.hpp"
#include "../mission_pick.hpp"
#include <string>
#include <vector>

// Set the tile URL template and attribution string.  Safe to call from the main
// thread at any time; workers pick up the new host on their next request.
// URL format: "https://host/path/{z}/{x}/{y}" — {z}/{x}/{y} are substituted per tile.
// Some providers use {z}/{y}/{x} ordering (e.g. Esri) — match your provider.
void map_view_set_tile_source(const std::string& url, const std::string& attribution);

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
void draw_map_view(double lat, double lon, bool has_pos,
                   float heading, bool has_hdg,
                   float win_x, float win_y, float win_w, float win_h,
                   const std::vector<MissionItem>* mission = nullptr,
                   MissionPickState* pick = nullptr);
