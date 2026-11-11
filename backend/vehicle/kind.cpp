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

// backend/vehicle/kind.cpp

#include "backend/vehicle/kind.hpp"
#include "backend/vehicle/state.hpp"

#include <cstring>

VehicleKind vehicle_kind_from_type(MAV_TYPE type)
{
    switch (type) {
    case MAV_TYPE_QUADROTOR:
    case MAV_TYPE_HEXAROTOR:
    case MAV_TYPE_OCTOROTOR:
    case MAV_TYPE_TRICOPTER:
    case MAV_TYPE_DECAROTOR:
    case MAV_TYPE_DODECAROTOR:
    case MAV_TYPE_COAXIAL:
    case MAV_TYPE_HELICOPTER:
        return VehicleKind::Copter;

    // A Copter BiCopter reports VTOL_TAILSITTER_DUOROTOR too; only the banner
    // tells the two apart. Plane is the likelier of them, and the only one
    // that reports the rest.
    case MAV_TYPE_FIXED_WING:
    case MAV_TYPE_VTOL_TAILSITTER_DUOROTOR:
    case MAV_TYPE_VTOL_TAILSITTER_QUADROTOR:
    case MAV_TYPE_VTOL_TILTROTOR:
    case MAV_TYPE_VTOL_FIXEDROTOR:
    case MAV_TYPE_VTOL_TAILSITTER:
    case MAV_TYPE_VTOL_TILTWING:
    // Values only Q_MAV_TYPE gives: no other ArduPilot firmware reports them.
    case MAV_TYPE_FLAPPING_WING:
    case MAV_TYPE_KITE:
    case MAV_TYPE_FREE_BALLOON:
    case MAV_TYPE_ROCKET:
        return VehicleKind::Plane;

    case MAV_TYPE_GROUND_ROVER:
    case MAV_TYPE_SURFACE_BOAT:
        return VehicleKind::Rover;

    case MAV_TYPE_SUBMARINE:       return VehicleKind::Sub;
    case MAV_TYPE_AIRSHIP:         return VehicleKind::Blimp;
    case MAV_TYPE_ANTENNA_TRACKER: return VehicleKind::Tracker;
    default:                       return VehicleKind::Unknown;
    }
}

VehicleKind vehicle_kind_from_banner(const char* text)
{
    // THISFIRMWARE, then " V<version>". The "APM:" names are the ones releases
    // before 4.0 used.
    static const struct { const char* name; VehicleKind kind; } BANNERS[] = {
        { "ArduCopter",         VehicleKind::Copter  },
        { "APM:Copter",         VehicleKind::Copter  },
        { "ArduPlane",          VehicleKind::Plane   },
        { "APM:Plane",          VehicleKind::Plane   },
        { "ArduRover",          VehicleKind::Rover   },
        { "APM:Rover",          VehicleKind::Rover   },
        { "ArduSub",            VehicleKind::Sub     },
        { "Blimp",              VehicleKind::Blimp   },
        { "AntennaTracker",     VehicleKind::Tracker },
        { "APM:AntennaTracker", VehicleKind::Tracker },
    };
    if (!text) return VehicleKind::Unknown;
    for (const auto& b : BANNERS) {
        const size_t n = std::strlen(b.name);
        if (std::strncmp(text, b.name, n) == 0 &&
            text[n] == ' ' && (text[n + 1] == 'V' || text[n + 1] == 'v'))
            return b.kind;
    }
    return VehicleKind::Unknown;
}

VehicleKind vehicle_kind(const VehicleState& vs)
{
    if (vs.banner_kind != VehicleKind::Unknown) return vs.banner_kind;
    return vehicle_kind_from_type(vs.type);
}

const char* vehicle_firmware_name(const VehicleState& vs)
{
    if (vs.autopilot == MAV_AUTOPILOT_PX4) return "PX4";
    if (vs.autopilot != MAV_AUTOPILOT_ARDUPILOTMEGA) return "Unknown";
    switch (vehicle_kind(vs)) {
    case VehicleKind::Copter:  return "ArduCopter";
    case VehicleKind::Plane:   return "ArduPlane";
    case VehicleKind::Rover:   return "ArduRover";
    case VehicleKind::Sub:     return "ArduSub";
    case VehicleKind::Blimp:   return "Blimp";
    case VehicleKind::Tracker: return "AntennaTracker";
    default:                   return "ArduPilot";
    }
}
