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

// backend/vehicle/kind.hpp
//
// Which ArduPilot firmware a vehicle runs: Copter, Plane, Rover, Sub, Blimp or
// AntennaTracker. Mode numbers, the RCn_OPTION table and the parameter
// reference all differ between them, so everything that asks goes through
// here rather than through its own switch on the HEARTBEAT type.
//
// The HEARTBEAT type alone cannot always say. What each firmware puts there
// (GCS_<vehicle>::frame_type() in the ArduPilot source):
//
//   ArduCopter      QUADROTOR, HEXAROTOR, OCTOROTOR, TRICOPTER, DECAROTOR,
//                   DODECAROTOR, COAXIAL (single and coax copters), HELICOPTER
//                   (every heli frame), and VTOL_TAILSITTER_DUOROTOR for the
//                   BiCopter frame (FRAME_CLASS 10, AP_MotorsTailsitter)
//   ArduPlane       FIXED_WING, QuadPlanes included, unless Q_MAV_TYPE says
//                   otherwise. That parameter takes QUADROTOR, COAXIAL,
//                   HELICOPTER, GROUND_ROVER, SURFACE_BOAT and SUBMARINE among
//                   its values, so a Plane can claim to be any of the others.
//   ArduRover       GROUND_ROVER, or SURFACE_BOAT for FRAME_CLASS 2
//   ArduSub         SUBMARINE
//   Blimp           AIRSHIP
//   AntennaTracker  ANTENNA_TRACKER
//
// What does settle it is the banner: the STATUSTEXT ArduPilot sends at boot,
// on every PARAM_REQUEST_LIST and on MAV_CMD_DO_SEND_BANNER, which opens with
// the firmware's name ("ArduCopter V4.7.1 (…)", from THISFIRMWARE in each
// vehicle's version.h). The connect burst asks for it. Until it arrives the
// type is read as the table above reads it, which is right for every vehicle
// except a Copter BiCopter, which is taken for a tailsitter QuadPlane, and a
// Plane with Q_MAV_TYPE set.

#pragma once

#include <mavlink/ardupilotmega/mavlink.h>

struct VehicleState;

enum class VehicleKind : uint8_t { Unknown, Copter, Plane, Rover, Sub, Blimp, Tracker };

// From the HEARTBEAT type alone, by the table above. Unknown for a type no
// ArduPilot firmware reports.
VehicleKind vehicle_kind_from_type(MAV_TYPE type);

// From a STATUSTEXT, if it is an ArduPilot banner; Unknown for any other text.
VehicleKind vehicle_kind_from_banner(const char* text);

// The banner's answer once one has been heard, the type's until then.
VehicleKind vehicle_kind(const VehicleState& vs);

// "ArduCopter", "ArduPlane", …, "PX4", or "Unknown": the firmware's name, as
// the connection panel and the log show it.
const char* vehicle_firmware_name(const VehicleState& vs);
