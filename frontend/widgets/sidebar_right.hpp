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
#include <cstdint>
#include <unordered_map>
#include <vector>
#include "../../backend/mavlink_parser.hpp"
#include "../../backend/mavlink_sender.hpp"
#include "../settings.hpp"

// Take in whatever this vehicle has said since the last call, for the fleet
// event log both panels below draw. Called once per vehicle per frame by the
// render loop, not from the draw: the log carries the whole fleet, so it must
// see vehicles that are not the one on screen.
//
// `number` is Fleet's display number, which is what tags the line.
void event_log_pump(const VehicleState& vs,
                    const std::vector<StatusText>& status_texts,
                    uint32_t number);

// The event log shows every vehicle, so neither of these takes a message list —
// they read the fleet log, and it does not follow the callsign chip.
void draw_sidebar_right(const VehicleState& vs,
                        MavlinkSender* sender,
                        AppSettings* settings);

// The corner block the fullscreen map shows in place of the right sidebar:
// attitude and the event log, semi-transparent, in the map's top-right corner.
// Call it after draw_center_view() — later submission is what puts it over the
// map — and only while center_view_map_fullscreen() is true.
void draw_map_overlay(const VehicleState& vs);
