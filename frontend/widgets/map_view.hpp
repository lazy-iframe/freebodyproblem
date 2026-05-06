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
