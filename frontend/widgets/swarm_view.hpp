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

#include <memory>
#include <vector>

#include "../../backend/mavlink_parser.hpp"
#include "../../backend/vehicle.hpp"
#include "../settings.hpp"
#include "layout.hpp"
#include "flight_target.hpp"

// ── SWARM view ────────────────────────────────────────────────────────────────
//
// The whole fleet on one screen, and the one place a command goes to several
// vehicles at once. A card per vehicle down the left half — checkbox, attitude,
// estimator bars, the numbers that say whether it is healthy — and across the
// top of the right half the FLIGHT section aimed at the checked vehicles, the
// telemetry grid of the focused one, and the fleet event log. The map below
// them is the centre view's, moved into this layout.
//
// Checking and selecting are separate on purpose. The checkbox decides who a
// command reaches; a click on the card body selects the vehicle (topbar,
// telemetry grid, GO HERE on the map), marked by a green frame. A double click
// opens it as a large card floating over the grid, and a click on that puts
// it back. Neither touches the checkbox, so looking at an aircraft can never
// add it to a batch command.

// One vehicle as this view draws it. Built by the render loop from the snapshot
// it is already taking of every vehicle, so nothing here locks.
struct SwarmVehicle {
    std::shared_ptr<Vehicle> vehicle;   // held so its sender outlives the frame
    VehicleState             state;
    uint32_t                 number = 0;
    bool                     online = false;   // link up and heartbeat fresh
};

// Size the screen for this frame's fleet: the card column's width from the
// number of vehicles, the band's height from their mode lists. Call before
// draw_center_view(), which places the map from the result.
void swarm_view_layout(const std::vector<SwarmVehicle>& fleet);
const SwarmLayout& swarm_layout();

// The checked vehicles, in display-number order: who the FLIGHT section and
// the topbar's ARM and INTERLOCK buttons reach while this view is up.
std::vector<FlightTarget> swarm_checked_targets(const std::vector<SwarmVehicle>& fleet);

// Is this vehicle in the command set? For the map, which rings checked vehicles.
bool swarm_is_checked(VehicleId id);

// Everything below the centre header except the map. `fleet` in display-number
// order. `*focus_out` is set only on the frame a card is clicked.
void draw_swarm_view(const std::vector<SwarmVehicle>& fleet,
                     const VehicleState& focused_vs,
                     MavlinkSender*      focused_sender,
                     VehicleId           focused_id,
                     AppSettings*        settings,
                     VehicleId*          focus_out);
