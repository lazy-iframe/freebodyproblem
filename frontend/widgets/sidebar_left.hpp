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
#include "../../backend/vehicle.hpp"
#include "../../backend/link.hpp"
#include "../settings.hpp"
#include "../mission_pick.hpp"
#include "flight_target.hpp"

// Filled for one frame when the user presses Connect or Disconnect.
//
// Connect no longer implies replacing whatever link is already open: several
// links can be live at once, each carrying its own vehicles. Disconnect
// therefore has to say *which* one, which is what disconnect_link_id is for.
struct ConnectionRequest {
    ConnType type    = ConnType::UDP;
    char     host[64]   = "0.0.0.0"; // UDP bind host / TCP remote host
    int      port       = 14550;
    char     device[64] = {};         // serial device path
    int      baud       = 57600;
    bool     requested   = false;     // true for one frame when Connect is pressed
    bool     disconnect  = false;     // true for one frame when Disconnect is pressed
    uint32_t disconnect_link_id = 0;  // which link to drop; 0 means all of them

    // Set for one frame when a reboot has been confirmed in the VEHICLES list.
    // Named rather than implied by the active vehicle: the list reboots any
    // aircraft in the fleet, not only the one on screen.
    VehicleId reboot_vehicle{};
};

// Feed the RC panel's calibration the live receiver stream.
//
// Called once per frame from the render loop rather than from the RC tab's own
// draw, because a calibration sweep has to keep recording while the operator is
// looking at something else — switching to PARAMS mid-sweep must not silently
// stop measuring, and in fullscreen video the sidebar is not drawn at all.
// Both pumps run for EVERY vehicle in the fleet each frame, not only the one on
// screen. That is the same reason they exist at all, one step up: a calibration
// is a conversation the vehicle is driving, and it keeps asking whether or not
// its panel — or now its aircraft — is the one being looked at. Miss the
// messages and the exchange stalls with the vehicle waiting on an answer that
// will never come.
//
// Bind the vehicle with ui_bind_vehicle() before drawing a panel or pumping it;
// see widgets/vehicle_ui_state.hpp.
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
                       const std::vector<LinkInfo>& links,
                       const std::vector<VehicleChip>& vehicles,
                       VehicleId active_vehicle,
                       MissionPickState* pick = nullptr);

// ── Pieces of the FLIGHT tab the SWARM view reuses ────────────────────────────

// TAKEOFF and the flight mode grid, sent to every target at once. Modes are
// matched between vehicles by name and grouped by flight stack; each vehicle
// gets its own mode number, encoded the way its stack expects.
//
// `name_targets` puts the target list in the heading and in the log, for a
// caller where who the buttons reach is a choice rather than a given.
void draw_flight_section(const std::vector<FlightTarget>& targets, bool name_targets);

// The height draw_flight_section() would take for these targets, from the top
// of its heading to the bottom of its last mode row, at an item spacing of
// `spacing_y`. Lets a layout size itself to the mode list before drawing it.
float flight_section_height(const std::vector<FlightTarget>& targets, float spacing_y);

// The six estimator bars. `bar_h` is the bar's height without its label;
// `small_labels` draws the labels in the micro face for a narrow row.
void draw_ekf_bars(const VehicleState& vs, float bar_h = 56.0f,
                   bool small_labels = false);
