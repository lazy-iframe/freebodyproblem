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

// The connect burst: what this GCS asks of a vehicle once it knows which stack
// it is talking to — telemetry rates, the firmware version, the mode list, and
// the parameter-set fence. Sent again after a reboot, which forgets every
// interval it was asked for.

#include <cstdint>

#include "backend/mavlink/sender/sender.hpp"
#include "backend/vehicle/state.hpp"

// Queues the burst on `sender`, addressed to (tsys, tcomp). `vs` must already
// hold a heartbeat: the autopilot decides what is asked for. Returns the
// firmware profile's name, for the log.
const char* send_connect_burst(MavlinkSender& sender, uint8_t tsys, uint8_t tcomp,
                               const VehicleState& vs);
