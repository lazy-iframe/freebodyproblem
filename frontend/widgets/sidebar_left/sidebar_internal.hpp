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
// Internal declarations shared between sidebar_left.cpp (orchestrator)
// and the per-tab translation units. Not part of the public API.
#include "../sidebar_left.hpp"  // ConnectionRequest, LinkStatus, VehicleState, ParamEntry, AppSettings
#include <unordered_map>
#include <string>
#include <cstdint>

void draw_tab_connection(const VehicleState* vs,
                         ConnectionRequest* conn_out,
                         LinkStatus link_status,
                         AppSettings* settings);

void draw_tab_flight(MavlinkSender* sender, const VehicleState* vs);

void draw_tab_params(MavlinkSender* sender, const VehicleState* vs,
                     const std::unordered_map<std::string, ParamEntry>* params);

void draw_tab_themes(AppSettings* settings);

void draw_tab_mission(MavlinkSender* sender, const VehicleState* vs,
                      MissionPickState* pick);

void draw_tab_mavlink(MavlinkSender* sender,
                      const VehicleState* vs,
                      const std::unordered_map<uint32_t, MessageStats>* stats,
                      uint64_t total_messages,
                      uint64_t total_bytes,
                      uint64_t parse_errors);
