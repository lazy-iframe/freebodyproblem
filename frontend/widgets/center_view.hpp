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
#include "../../plugins/plugin_api.hpp"
#include "../mission_pick.hpp"
#include "map_view.hpp"

#include <vector>

class MavlinkSender;

// `sender` is handed to the plugin rail beside the video, which is where user
// plugins reach the vehicle from. `fleet` is every vehicle the map draws, of
// which `vs` is the active one — the sidebars and the overlay show that one,
// the map shows them all.
void draw_center_view(const VehicleState& vs, MavlinkSender* sender,
                      MissionPickState* pick = nullptr,
                      const std::vector<MapVehicle>* fleet = nullptr);
void center_view_shutdown();   // stop video + delete GL texture — call before context destroy

// True while the feed is fullscreen — the second press of the VIDEO mode
// button. The panels it displaces (both sidebars) skip drawing themselves.
bool center_view_video_fullscreen();

// True while the map is fullscreen — the second press of the MAP mode button.
// It displaces the right sidebar and the plugin rail, and keeps the left one:
// the tabs are how a mission is planned and the map is what it is planned on.
// The render loop skips the right sidebar and draws draw_map_overlay() over the
// map instead, which is where the attitude and the event log go.
bool center_view_map_fullscreen();

// The map's rectangle as the centre view last drew it. For the overlay, which
// is submitted after this panel — later submission is what puts it on top —
// and so has to be told where the map ended up.
void center_view_map_rect(float& x, float& y, float& w, float& h);

// The context handed to plugins — vehicle, link and live camera state. Built
// here because this is where the camera state lives, and shared so the rail's
// buttons and the feed's click handlers see exactly the same picture.
// `slot` is left at -1; the rail sets it per button.
PluginContext center_view_plugin_context(const VehicleState& vs,
                                         MavlinkSender* sender);
