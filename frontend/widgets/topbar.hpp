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
#include <string>
#include <unordered_map>
#include <vector>
#include "../../backend/mavlink_parser.hpp"
#include "../../backend/mavlink_sender.hpp"
#include "../../backend/connection.hpp"
#include "../../backend/vehicle.hpp"
#include "flight_target.hpp"

void draw_topbar(const VehicleState& vs,
                 const std::unordered_map<uint32_t, MessageStats>& stats,
                 uint64_t total_messages,
                 uint64_t total_bytes,
                 uint64_t parse_errors,
                 MavlinkSender* sender,
                 LinkStatus link_status,
                 bool* close_requested,
                 // The callsign chip is the vehicle switcher: it already shows
                 // which system is on screen, so clicking it to choose a
                 // different one puts the control where the operator is already
                 // looking. `vehicles` is the whole fleet; `*selected_out` is
                 // filled in only on the frame a row is clicked.
                 const std::vector<VehicleChip>& vehicles,
                 VehicleId  active,
                 VehicleId* selected_out,
                 // In the SWARM view, the checked vehicles. ARM and INTERLOCK
                 // then stop being annunciators of one vehicle and become
                 // buttons that act on all of these; the cards show the state.
                 // Null everywhere else.
                 const std::vector<FlightTarget>* swarm_targets = nullptr);

// The vehicle's current flight mode as the topbar shows it: its own
// AVAILABLE_MODES name when it published one, its stack's table otherwise.
std::string mode_display_name(const VehicleState& vs);

// The interlock latch the topbar keeps for a vehicle: HIGH after the operator
// last set it high. The vehicle never reports interlock, so this is what the
// GCS sent, not what the vehicle did.
bool interlock_latched(VehicleId id);
