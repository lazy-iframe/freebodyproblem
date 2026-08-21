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


#include "rc_binding.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>

// ── Stick bindings ───────────────────────────────────────────────────────────

const std::vector<RcStickBinding>& rc_stick_bindings()
{
    // ArduPilot's RCMAP_* defaults are AETR (1=roll, 2=pitch, 3=throttle,
    // 4=yaw), which is also PX4's RC_MAP_* default. The mode switch is the one
    // that differs in spirit: ArduPilot's FLTMODE_CH selects among six PWM
    // bands on one channel, PX4's RC_MAP_FLTMODE does the same job under a
    // different name, and both default to channel 5.
    static const std::vector<RcStickBinding> kBindings = {
        { "ROLL",      "RCMAP_ROLL",     "RC_MAP_ROLL",     1 },
        { "PITCH",     "RCMAP_PITCH",    "RC_MAP_PITCH",    2 },
        { "THROTTLE",  "RCMAP_THROTTLE", "RC_MAP_THROTTLE", 3 },
        { "YAW",       "RCMAP_YAW",      "RC_MAP_YAW",      4 },
        { "MODE",      "FLTMODE_CH",     "RC_MAP_FLTMODE",  5 },
    };
    return kBindings;
}

const char* rc_stick_param(const RcStickBinding& b, RcParamDialect dialect)
{
    return (dialect == RcParamDialect::PX4) ? b.px4 : b.ardupilot;
}

int rc_stick_throttle_index()
{
    return 2;   // matches the table above
}

// ── Auxiliary functions ──────────────────────────────────────────────────────

RcFirmware rc_firmware_for(MAV_TYPE type)
{
    switch (type) {
    case MAV_TYPE_FIXED_WING:
    case MAV_TYPE_VTOL_TAILSITTER_DUOROTOR:
    case MAV_TYPE_VTOL_TAILSITTER_QUADROTOR:
    case MAV_TYPE_VTOL_TILTROTOR:        return RcFirmware::Plane;
    case MAV_TYPE_GROUND_ROVER:
    case MAV_TYPE_SURFACE_BOAT:          return RcFirmware::Rover;
    case MAV_TYPE_SUBMARINE:             return RcFirmware::Sub;
    default:                             return RcFirmware::Copter;
    }
}

const char* rc_firmware_label(RcFirmware fw)
{
    switch (fw) {
    case RcFirmware::Plane: return "PLANE";
    case RcFirmware::Rover: return "ROVER";
    case RcFirmware::Sub:   return "SUB";
    default:                return "COPTER";
    }
}

const std::vector<RcAuxOption>& rc_aux_options()
{
    // id, label, then which firmwares implement it: copter, plane, rover, sub.
    static const std::vector<RcAuxOption> kOptions = {
    {  0, "Do Nothing",                true,  true,  true,  true  },
    {  2, "FLIP",                      true,  false, false, false },
    {  3, "SIMPLE mode (Copter)",      true,  false, false, false },
    {  4, "RTL mode",                  true,  true,  true,  false },
    {  5, "Save Trim",                 true,  true,  false, false },
    {  7, "Save Waypoint",             true,  true,  false, false },
    {  9, "Camera Trigger",            true,  true,  true,  true  },
    { 10, "Rangefinder",               true,  false, false, false },
    { 11, "Fence",                     true,  true,  true,  true  },
    { 12, "ResetToArmedYaw",           true,  false, false, false },
    { 13, "SUPERSIMPLE mode",          true,  false, false, false },
    { 14, "Acro Trainer",              true,  false, false, false },
    { 15, "Sprayer",                   true,  false, false, false },
    { 16, "AUTO mode",                 true,  true,  true,  false },
    { 17, "AUTOTUNE mode",             true,  false, false, false },
    { 18, "LAND mode",                 true,  false, false, false },
    { 19, "Gripper",                   true,  true,  true,  true  },
    { 21, "Parachute Enable",          true,  false, false, false },
    { 22, "Parachute Release",         true,  true,  false, false },
    { 23, "Parachute 3-Pos Switch",    true,  false, false, false },
    { 24, "Reset Auto Mission",        true,  true,  true,  true  },
    { 25, "Attitude FF",               true,  false, false, false },
    { 26, "Attitude AccLim",           true,  false, false, false },
    { 27, "Retract Mount1",            true,  true,  true,  true  },
    { 28, "Relay 1 On/Off",            true,  true,  true,  true  },
    { 29, "Landing Gear",              true,  true,  false, false },
    { 30, "Lost Vehicle Sound",        true,  true,  true,  false },
    { 31, "Motor Emergency Stop",      true,  true,  true,  true  },
    { 32, "Motor Interlock",           true,  false, false, false },
    { 33, "BRAKE mode",                true,  false, false, false },
    { 34, "Relay 2 On/Off",            true,  true,  true,  true  },
    { 35, "Relay 3 On/Off",            true,  true,  true,  true  },
    { 36, "Relay 4 On/Off",            true,  true,  true,  true  },
    { 37, "THROW mode",                true,  false, false, false },
    { 38, "ADSB Avoidance Enable",     true,  false, false, false },
    { 39, "Precision Loiter",          true,  false, false, false },
    { 40, "Object Avoidance",          true,  true,  false, false },
    { 41, "Arm/Disarm (4.1-)",         true,  true,  true,  false },
    { 42, "SMARTRTL mode",             true,  true,  false, false },
    { 43, "Inverted Flight",           true,  true,  false, false },
    { 44, "Winch Enable",              true,  false, false, false },
    { 45, "Winch Control",             true,  false, false, false },
    { 46, "RC Override Enable",        true,  true,  true,  true  },
    { 47, "Custom Function 1",         true,  false, false, false },
    { 48, "Custom Function 2",         true,  false, false, false },
    { 49, "Custom Function 3",         true,  false, false, false },
    { 50, "Learn Cruise",              false, false, true,  false },
    { 51, "MANUAL mode",               false, true,  true,  false },
    { 52, "ACRO mode",                 false, true,  true,  false },
    { 53, "STEERING mode",             false, false, true,  false },
    { 54, "HOLD mode",                 false, false, true,  false },
    { 55, "GUIDED mode",               true,  true,  true,  false },
    { 56, "LOITER mode",               true,  true,  false, false },
    { 57, "FOLLOW mode",               true,  true,  false, false },
    { 58, "Clear Waypoints",           true,  true,  true,  true  },
    { 59, "SIMPLE mode (Rover)",       false, false, true,  false },
    { 60, "ZIGZAG mode",               true,  false, false, false },
    { 61, "ZIGZAG Save Waypoints",     true,  false, false, false },
    { 62, "Compass Learn",             true,  true,  true,  true  },
    { 63, "Sailboat Tack",             false, false, true,  false },
    { 64, "Reverse Throttle",          false, true,  false, false },
    { 65, "GPS Disable",               true,  true,  true,  true  },
    { 66, "Relay 5 On/Off",            true,  true,  true,  true  },
    { 67, "Relay 6 On/Off",            true,  true,  true,  true  },
    { 68, "STABILIZE mode",            true,  false, false, false },
    { 69, "POSHOLD mode",              true,  false, false, false },
    { 70, "ALTHOLD mode",              true,  true,  false, false },
    { 71, "FLOWHOLD mode",             true,  false, false, false },
    { 72, "CIRCLE mode",               true,  true,  true,  false },
    { 73, "DRIFT mode",                true,  false, false, false },
    { 74, "Sailboat Motor 3-Pos",      false, false, true,  false },
    { 75, "Surface Tracking",          true,  false, false, false },
    { 76, "STANDBY mode",              true,  false, false, false },
    { 77, "TAKEOFF mode",              true,  false, false, false },
    { 78, "RunCam Control",            true,  true,  true,  true  },
    { 79, "RunCam OSD Control",        true,  true,  true,  true  },
    { 80, "Viso Align",                true,  false, false, false },
    { 81, "Disarm",                    true,  true,  true,  true  },
    { 82, "Q_Assist 3-Pos Sw",         false, true,  false, false },
    { 83, "ZIGZAG Auto",               true,  false, false, false },
    { 84, "AIRMODE",                   true,  true,  false, false },
    { 85, "Generator",                 true,  true,  true,  false },
    { 86, "Non-Auto Terrain Follow",   false, true,  false, false },
    { 87, "CROW Mode Switch",          false, true,  false, false },
    { 88, "Soaring Enable",            false, true,  false, false },
    { 89, "Force Flare",               false, true,  false, false },
    { 90, "EKF Source Set",            true,  true,  true,  false },
    { 91, "Airspeed Ratio Cal",        false, true,  false, false },
    { 92, "FBWA Mode",                 false, true,  false, false },
    { 94, "VTX Power",                 true,  true,  true,  true  },
    { 95, "FBWA Taildragger",          false, true,  false, false },
    { 96, "Mode Switch Reset",         false, true,  false, false },
    { 97, "WindVane Home Dir",         false, false, true,  false },
    {102, "Camera Mode Toggle",        true,  true,  true,  true  },
    {103, "EKF Lane Switch",           true,  true,  true,  true  },
    {104, "EKF Yaw Reset",             true,  true,  true,  true  },
    {105, "GPS Disable Yaw",           true,  true,  true,  true  },
    {106, "Disable Airspeed Use",      true,  true,  false, false },
    {107, "Enable Autotuning",         true,  false, false, false },
    {108, "QRTL Mode",                 false, true,  false, false },
    {111, "Loweheiser Starter",        true,  true,  true,  true  },
    {112, "Switch External AHRS",      true,  true,  true,  true  },
    {113, "Retract Mount2",            true,  false, false, false },
    {150, "CRUISE Mode",               false, true,  false, false },
    {151, "TURTLE Mode",               true,  false, false, false },
    {152, "SIMPLE Heading Reset",      true,  false, false, false },
    {153, "ARM/DISARM (4.2+)",         true,  true,  true,  true  },
    {154, "ARM/DISARM + AIRMODE",      true,  true,  false, false },
    {155, "TRIM RC/SERVO Save",        false, true,  true,  false },
    {156, "Torqeedo Error Clear",      false, false, true,  false },
    {157, "Force FBWA Long FS",        false, true,  false, false },
    {158, "Optflow Calibration",       true,  true,  false, false },
    {159, "Force Flying State",        true,  false, false, false },
    {160, "WeatherVane Enable",        false, true,  false, false },
    {161, "Turbine Start (Heli)",      false, true,  false, false },
    {162, "Auto Throttle Notch",       true,  true,  false, false },
    {163, "Mount Yaw Lock",            true,  true,  true,  true  },
    {164, "Pause Stream Logging",      true,  true,  true,  true  },
    {165, "ARM / Motor E-Stop",        true,  true,  true,  true  },
    {166, "Camera Record Video",       true,  true,  true,  true  },
    {167, "Camera Zoom",               true,  true,  true,  true  },
    {168, "Camera Manual Focus",       true,  true,  true,  true  },
    {169, "Camera Auto Focus",         true,  true,  true,  true  },
    {170, "QSTABILIZE mode",           false, true,  false, false },
    {171, "Compass Calibration",       true,  true,  true,  false },
    {172, "Battery MPPT Enable",       true,  true,  true,  false },
    {173, "Plane AUTO Landing Abort",  false, true,  false, false },
    {174, "Camera Image Tracking",     true,  true,  true,  true  },
    {175, "Camera Lens",               true,  true,  true,  true  },
    {176, "VTOL Fwd Throttle Disable", false, true,  false, false },
    {177, "Mount LRF Enable",          true,  true,  true,  true  },
    {178, "FlightMode Pause/Resume",   true,  false, false, false },
    {179, "ICEngine Start/Stop",       false, true,  false, false },
    {180, "Autotune Test Gains",       true,  true,  false, false },
    {181, "VTOL QuickTune",            false, true,  false, false },
    {182, "AHRS AutoTrim",             true,  false, false, false },
    {183, "AUTOLAND mode",             false, true,  false, false },
    {184, "System ID",                 true,  true,  false, false },
    {185, "Mount RP Lock",             true,  true,  true,  true  },
    {186, "Mount POI Lock",            true,  true,  true,  true  },
    {201, "ROLL Input",                true,  false, false, false },
    {202, "PITCH Input",               true,  false, false, false },
    {203, "THROTTLE Input",            false, false, false, false },
    {204, "YAW Input",                 false, false, false, false },
    {207, "Mainsail",                  false, false, true,  false },
    {208, "Flap Control",              false, true,  false, false },
    {209, "Forward Throttle",          false, true,  false, false },
    {210, "Airbrakes",                 false, true,  false, false },
    {211, "Walking Robot Height",      false, false, true,  false },
    {212, "Mount1 Roll",               true,  true,  true,  true  },
    {213, "Mount1 Pitch",              true,  true,  true,  true  },
    {214, "Mount1 Yaw",                true,  true,  true,  true  },
    {215, "Mount2 Roll",               true,  true,  true,  true  },
    {216, "Mount2 Pitch",              true,  true,  true,  true  },
    {217, "Mount2 Yaw",                true,  true,  true,  true  },
    {218, "Loweheiser Throttle",       true,  true,  true,  true  },
    {219, "TX Tuning Channel",         true,  false, false, false },
    {300, "Scripting RC 1",            true,  true,  true,  true  },
    {301, "Scripting RC 2",            true,  true,  true,  true  },
    {302, "Scripting RC 3",            true,  true,  true,  true  },
    {303, "Scripting RC 4",            true,  true,  true,  true  },
    {304, "Scripting RC 5",            true,  true,  true,  true  },
    {305, "Scripting RC 6",            true,  true,  true,  true  },
    {306, "Scripting RC 7",            true,  true,  true,  true  },
    {307, "Scripting RC 8",            true,  true,  true,  true  },
    {308, "Scripting RC 9",            true,  true,  true,  true  },
    {309, "Scripting RC 10",           true,  true,  true,  true  },
    {310, "Scripting RC 11",           true,  true,  true,  true  },
    {311, "Scripting RC 12",           true,  true,  true,  true  },
    {312, "Scripting RC 13",           true,  true,  true,  true  },
    {313, "Scripting RC 14",           true,  true,  true,  true  },
    {314, "Scripting RC 15",           true,  true,  true,  true  },
    {315, "Scripting RC 16",           true,  true,  true,  true  },
    {316, "Scripting Stop/Restart",    true,  true,  true,  true  },
    };
    return kOptions;
}

const char* rc_aux_option_label(int value)
{
    if (value < 0) return nullptr;
    for (const RcAuxOption& o : rc_aux_options())
        if (o.value == (uint16_t)value) return o.label;
    return nullptr;
}

bool rc_aux_param(int channel_1based, RcParamDialect dialect,
                  char* out, size_t out_len)
{
    if (dialect != RcParamDialect::ArduPilot) return false;
    if (channel_1based < 1 || channel_1based > RcChannels::MAX_CHANNELS) return false;
    if (!out || out_len == 0) return false;

    snprintf(out, out_len, "RC%d_OPTION", channel_1based);
    return true;
}

// ── Flight-mode switch slots ─────────────────────────────────────────────────

const std::vector<RcModeSlot>& rc_mode_slots()
{
    // ArduPilot's mode-switch bands, from RC_Channel's six-position read. They
    // are contiguous and the outer two are open-ended, so every pulse lands in
    // exactly one slot.
    static const std::vector<RcModeSlot> kSlots = {
        { 1, "FLTMODE1",    0, 1230 },
        { 2, "FLTMODE2", 1231, 1360 },
        { 3, "FLTMODE3", 1361, 1490 },
        { 4, "FLTMODE4", 1491, 1620 },
        { 5, "FLTMODE5", 1621, 1749 },
        { 6, "FLTMODE6", 1750, 65535 },
    };
    return kSlots;
}

int rc_mode_slot_for_pwm(uint16_t pwm)
{
    if (pwm == 0) return 0;   // no reading is not slot 1
    for (const RcModeSlot& s : rc_mode_slots())
        if (pwm >= s.pwm_lo && pwm <= s.pwm_hi) return s.index;
    return 0;
}

const std::vector<RcModeOption>& rc_copter_modes()
{
    static const std::vector<RcModeOption> kModes = {
        {  0, "Stabilize"    },
        {  1, "Acro"         },
        {  2, "AltHold"      },
        {  3, "Auto"         },
        {  4, "Guided"       },
        {  5, "Loiter"       },
        {  6, "RTL"          },
        {  7, "Circle"       },
        {  9, "Land"         },
        { 11, "Drift"        },
        { 13, "Sport"        },
        { 14, "Flip"         },
        { 15, "AutoTune"     },
        { 16, "PosHold"      },
        { 17, "Brake"        },
        { 18, "Throw"        },
        { 19, "Avoid ADSB"   },
        { 20, "Guided NoGPS" },
        { 21, "Smart RTL"    },
        { 22, "FlowHold"     },
        { 23, "Follow"       },
        { 24, "ZigZag"       },
        { 25, "SystemID"     },
        { 26, "Autorotate"   },
        { 27, "Auto RTL"     },
        { 28, "Turtle"       },
    };
    return kModes;
}

// ── Channel detection ────────────────────────────────────────────────────────

int rc_detect_moved_channel(const RcChannels& baseline, const RcChannels& live,
                            uint16_t threshold)
{
    const int n = std::min(baseline.usable_count(), live.usable_count());

    int best_ch    = 0;
    int best_delta = (int)threshold;   // must be strictly exceeded to win

    for (int i = 0; i < n; ++i) {
        const uint16_t a = baseline.chan[i];
        const uint16_t b = live.chan[i];
        if (a == 0 || b == 0) continue;   // absent in one frame — not a move

        const int delta = std::abs((int)b - (int)a);
        if (delta > best_delta) {
            best_delta = delta;
            best_ch    = i + 1;
        }
    }

    return best_ch;
}
