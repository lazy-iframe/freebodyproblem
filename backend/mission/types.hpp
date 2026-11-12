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

// The two things a mission-protocol list carries that the rest of the GCS needs
// to name: one item of a plan or fence, and the parameter-set fence beside them.

#include <cstdint>

struct MissionItem {
    uint16_t seq;
    double   lat, lon;        // degrees
    float    alt;             // metres (MISSION_ITEM_INT.z)
    float    param1 = 0.f;
    float    param2 = 0.f;
    float    param3 = 0.f;
    float    param4 = 0.f;
    uint16_t command;         // MAV_CMD
    uint8_t  frame = 3;       // MAV_FRAME (3 = GLOBAL_RELATIVE_ALT)
    bool     is_current = false;
    bool     autocontinue = true;
};

// The simple, parameter-set geofence: a cylinder around home. What the vehicle
// reported of it, from PARAM_VALUE — requested on connect, and echoed by the
// vehicle whenever one of them is written. ArduPilot and PX4 name it
// differently; both fill the same fields, and `px4` says which.
struct FenceParams {
    bool  px4         = false;
    bool  has_enable  = false;  bool  enabled  = false;  // FENCE_ENABLE (PX4: GF_ACTION != 0)
    bool  has_type    = false;  uint32_t type  = 0;      // FENCE_TYPE bitmask
    bool  has_radius  = false;  float radius_m = 0.0f;   // FENCE_RADIUS / GF_MAX_HOR_DIST
    bool  has_alt_max = false;  float alt_max_m = 0.0f;  // FENCE_ALT_MAX / GF_MAX_VER_DIST
    bool  has_alt_min = false;  float alt_min_m = 0.0f;  // FENCE_ALT_MIN
    bool  has_action  = false;  int   action   = 0;      // FENCE_ACTION / GF_ACTION

    // FENCE_TYPE bits (ArduPilot).
    static constexpr uint32_t TYPE_ALT_MAX = 1u << 0;
    static constexpr uint32_t TYPE_CIRCLE  = 1u << 1;
    static constexpr uint32_t TYPE_POLYGON = 1u << 2;
    static constexpr uint32_t TYPE_ALT_MIN = 1u << 3;

    // Known to be switched off. The map draws such a fence dotted and unfilled,
    // and the 3D view puts up no walls for it.
    bool disabled() const { return has_enable && !enabled; }

    // A horizontal cylinder is configured: a radius, and on ArduPilot the
    // circle bit set. Whether it is enforced is `enabled`, separately.
    bool circle_configured() const {
        if (!has_radius || radius_m <= 0.0f) return false;
        return px4 || !has_type || (type & TYPE_CIRCLE);
    }
    // The ceiling the cylinder is drawn with, when one is configured.
    bool ceiling_configured() const {
        if (!has_alt_max || alt_max_m <= 0.0f) return false;
        return px4 || !has_type || (type & TYPE_ALT_MAX);
    }
};
