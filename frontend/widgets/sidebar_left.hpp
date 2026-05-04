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
