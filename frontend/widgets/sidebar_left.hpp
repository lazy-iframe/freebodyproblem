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
#include <vector>
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

// Feed the RC panel's calibration the live receiver stream.
//
// Called once per frame from the render loop rather than from the RC tab's own
// draw, because a calibration sweep has to keep recording while the operator is
// looking at something else — switching to PARAMS mid-sweep must not silently
// stop measuring, and in fullscreen video the sidebar is not drawn at all.
void rc_tab_pump(const VehicleState* vs);

// Feed the SENSORS panel's calibrations — accelerometer and gyroscope.
//
// Same reason as rc_tab_pump, more sharply: a calibration is a conversation the
// vehicle is driving, and it keeps asking for positions whether or not this
// panel is the one on screen. Miss the messages and the exchange stalls with
// the vehicle waiting on an answer that will never come.
//
// `now` is a monotonic seconds clock, used to time out a gyro calibration whose
// COMMAND_ACK never arrives.
void sensors_tab_pump(const VehicleState* vs, const std::vector<StatusText>& texts,
                      double now);

void draw_sidebar_left(MavlinkSender* sender, const VehicleState* vs,
                       ConnectionRequest* conn_out, LinkStatus link_status,
                       const std::unordered_map<std::string, ParamEntry>* params,
                       AppSettings* settings,
                       const std::unordered_map<uint32_t, MessageStats>* msg_stats,
                       uint64_t total_messages,
                       uint64_t total_bytes,
                       uint64_t parse_errors,
                       MissionPickState* pick = nullptr);
