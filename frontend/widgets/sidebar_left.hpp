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
#include <unordered_map>
#include <string>
#include "../../backend/mavlink_parser.hpp"
#include "../../backend/mavlink_sender.hpp"
#include "../../backend/connection.hpp"
#include "../settings.hpp"
#include "../mission_pick.hpp"

// Filled for one frame when the user presses Connect or Disconnect.
struct ConnectionRequest {
    ConnType type    = ConnType::UDP;
    char     host[64]   = "0.0.0.0"; // UDP bind host / TCP remote host
    int      port       = 14550;
    char     device[64] = {};         // serial device path
    int      baud       = 57600;
    bool     requested   = false;     // true for one frame when Connect is pressed
    bool     disconnect  = false;     // true for one frame when Disconnect is pressed
};

void draw_sidebar_left(MavlinkSender* sender, const VehicleState* vs,
                       ConnectionRequest* conn_out, LinkStatus link_status,
                       const std::unordered_map<std::string, ParamEntry>* params,
                       AppSettings* settings,
                       const std::unordered_map<uint32_t, MessageStats>* msg_stats,
                       uint64_t total_messages,
                       uint64_t total_bytes,
                       uint64_t parse_errors,
                       MissionPickState* pick = nullptr);
