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

// frontend/source/mission_cmds.hpp
//
// The mission commands the editor knows by name: what each is called, what
// its four parameters mean, and whether it has a place and a height. Data
// only; MAV_CMD numbers as in common.xml.

#pragma once

#include <cstdint>

struct MissionCmdMeta {
    uint16_t    id;
    const char* name;
    const char* p1;        // param1 label (nullptr = unused)
    const char* p2;
    const char* p3;
    const char* p4;
    bool        show_latlon; // show Lat / Lon / Alt fields
    bool        show_alt;    // show Alt even when no lat/lon
};

inline constexpr MissionCmdMeta MISSION_CMDS[] = {
    // id    name                     p1              p2                p3              p4              ll    alt
    {  16, "WAYPOINT",          "Delay (s)",    "Accept R (m)",   "Pass R (m)",   "Yaw (°)",      true,  true  },
    {  17, "LOITER_UNLIM",      nullptr,        nullptr,          "Radius (m)",   "Hdg req",      true,  true  },
    {  18, "LOITER_TURNS",      "Turns",        "Hdg req",        "Radius (m)",   "Xtrack exit",  true,  true  },
    {  19, "LOITER_TIME",       "Time (s)",     "Hdg req",        "Radius (m)",   "Xtrack exit",  true,  true  },
    {  20, "RTL",               nullptr,        nullptr,          nullptr,        nullptr,        false, false },
    {  21, "LAND",              "Abort alt",    "Mode",           nullptr,        "Yaw (°)",      true,  true  },
    {  22, "TAKEOFF",           "Pitch min",    nullptr,          nullptr,        "Yaw (°)",      true,  true  },
    {  30, "CONT_CHANGE_ALT",   "Hdg req",      nullptr,          nullptr,        nullptr,        false, true  },
    {  31, "LOITER_TO_ALT",     "Hdg req",      "Radius (m)",     nullptr,        "Xtrack exit",  true,  true  },
    {  82, "ROI",               "ROI mode",     nullptr,          nullptr,        nullptr,        true,  true  },
    {  93, "NAV_DELAY",         "Hold (s)",     "Hour (UTC)",     "Minute (UTC)", "Second (UTC)", false, false },
    { 112, "COND_DELAY",        "Time (s)",     nullptr,          nullptr,        nullptr,        false, false },
    { 113, "COND_CHANGE_ALT",   "Rate (m/s)",   nullptr,          nullptr,        nullptr,        false, true  },
    { 114, "COND_DISTANCE",     "Dist (m)",     nullptr,          nullptr,        nullptr,        false, false },
    { 115, "COND_YAW",          "Angle (°)",    "Speed (°/s)",    "Direction",    "Relative",     false, false },
    { 176, "DO_SET_MODE",       "Mode",         "Custom mode",    nullptr,        nullptr,        false, false },
    { 177, "DO_JUMP",           "WP#",          "Repeat",         nullptr,        nullptr,        false, false },
    { 178, "DO_CHANGE_SPEED",   "Type",         "Speed (m/s)",    "Throttle %",   nullptr,        false, false },
    { 179, "DO_SET_HOME",       "Current",      nullptr,          nullptr,        nullptr,        true,  true  },
    { 181, "DO_SET_RELAY",      "Relay#",       "State",          nullptr,        nullptr,        false, false },
    { 182, "DO_REPEAT_RELAY",   "Relay#",       "Cycles",         "Dwell (s)",    nullptr,        false, false },
    { 183, "DO_SET_SERVO",      "Servo#",       "PWM (µs)",       nullptr,        nullptr,        false, false },
    { 184, "DO_REPEAT_SERVO",   "Servo#",       "PWM (µs)",       "Cycles",       "Dwell (s)",    false, false },
    { 189, "DO_LAND_START",     nullptr,        nullptr,          nullptr,        nullptr,        true,  true  },
    { 201, "DO_SET_ROI",        "ROI mode",     "WP#",            "ROI idx",      nullptr,        true,  true  },
    { 203, "DIGICAM_CONFIG",    "Exp mode",     "Shutter",        "Aperture",     "ISO",          false, false },
    { 204, "DIGICAM_CTRL",      "Session",      "Zoom step",      "Zoom type",    "Shutter cmd",  false, false },
    { 206, "MOUNT_CONFIGURE",   "Mode",         "Stabilize roll", "Stabilize tilt","Stabilize pan",false, false },
    { 207, "MOUNT_CONTROL",     "Pitch (°)",    "Roll (°)",       "Yaw (°)",      "Save",         false, false },
};
inline constexpr int MISSION_CMD_COUNT = (int)(sizeof(MISSION_CMDS) / sizeof(MISSION_CMDS[0]));

// The entry for a command, or null for one the table does not list.
const MissionCmdMeta* mission_cmd_meta(uint16_t cmd);

// Its short name, "WAYPOINT"; "CMD_<n>" for one the table does not list.
const char* mission_cmd_name(uint16_t cmd);
