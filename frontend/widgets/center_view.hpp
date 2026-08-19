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

class MavlinkSender;

// `sender` is handed to the plugin rail beside the video, which is where user
// plugins reach the vehicle from.
void draw_center_view(const VehicleState& vs, MavlinkSender* sender,
                      MissionPickState* pick = nullptr);
void center_view_shutdown();   // stop video + delete GL texture — call before context destroy

// True while the feed is fullscreen — the second press of the VIDEO mode
// button. The panels it displaces (both sidebars) skip drawing themselves.
bool center_view_video_fullscreen();

// The context handed to plugins — vehicle, link and live camera state. Built
// here because this is where the camera state lives, and shared so the rail's
// buttons and the feed's click handlers see exactly the same picture.
// `slot` is left at -1; the rail sets it per button.
PluginContext center_view_plugin_context(const VehicleState& vs,
                                         MavlinkSender* sender);
