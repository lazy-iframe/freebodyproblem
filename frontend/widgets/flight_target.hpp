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

#include "../../backend/mavlink_parser.hpp"
#include "../../backend/vehicle.hpp"

class MavlinkSender;

// One vehicle a command goes to, when a command can go to several: the FLIGHT
// section and, in the SWARM view, the topbar's ARM and INTERLOCK buttons.
// `number` is Fleet's display number, used only to name the vehicle in the log
// and in headings; `id` is for per-vehicle UI state such as the interlock latch.
struct FlightTarget {
    MavlinkSender*      sender = nullptr;
    const VehicleState* vs     = nullptr;
    uint32_t            number = 0;
    VehicleId           id{};
};
