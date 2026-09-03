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

#include "mavlink_parser.hpp"   // ParamEntry

// ─────────────────────────────────────────────────────────────────────────────
// What sensors the vehicle actually has, read out of its parameters.
//
// There is no MAVLink message that says "this vehicle has two compasses and
// three IMUs". Both stacks record it in parameters instead, as a device ID per
// instance — COMPASS_DEV_ID2, INS_ACC3_ID, CAL_MAG1_ID — which is zero when
// that slot is empty and otherwise packs the bus, the address and the chip:
//
//     bits  0-2   bus type   (I2C, SPI, DroneCAN, …)
//     bits  3-7   bus number
//     bits  8-15  address on that bus
//     bits 16-23  device type — the chip itself
//
// So the inventory is a parameter read, and needs the parameter table fetched
// before it can say anything. Nothing here sends: it is given the table the
// PARAMS tab already holds and returns what is in it.
//
// The chip names are a lookup table of the device types in common use. An
// unknown one is not an error and not a missing sensor — it is a chip this
// table has not been taught, and it shows as its number.
// ─────────────────────────────────────────────────────────────────────────────

struct SensorDevice {
    std::string param;          // the parameter it was read from
    int         instance = 0;   // 1-based, as the operator counts them

    uint32_t    devid    = 0;
    uint8_t     bus_type = 0;
    uint8_t     bus      = 0;
    uint8_t     address  = 0;
    uint8_t     devtype  = 0;

    const char* bus_name     = "";   // "I2C", "SPI", … never null
    const char* devtype_name = "";   // "" when the type is not in the table

    // Whether the vehicle is using it, where a parameter says so. `has_use`
    // false means the stack has no such parameter for this sensor — not that
    // the sensor is disabled.
    bool has_use  = false;
    bool in_use   = false;

    // Mounted off the board, where a parameter says so. Compasses only.
    bool has_external = false;
    bool external     = false;
};

enum class SensorKind { Compass, Accel, Gyro };

// Every populated instance of `kind`, in instance order. Empty when the
// parameter table has not been fetched, or holds none of the names — which is
// the same answer to the panel: nothing to show yet.
std::vector<SensorDevice> sensor_inventory(
    const std::unordered_map<std::string, ParamEntry>* params, SensorKind kind);

// Names for the identifiers a component announces itself with. nullptr when the
// value is not one this knows, so the caller shows the number rather than this
// inventing a name for it.
const char* mav_component_name(uint8_t compid);
const char* mav_autopilot_name(uint8_t autopilot);
const char* mav_vehicle_type_name(uint8_t type);
