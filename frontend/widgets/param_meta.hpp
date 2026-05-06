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
#include <cstring>
#include <string>
#include <unordered_map>

// ── Parameter widget / metadata types ────────────────────────────────────────

enum class ParamWidgetType { Float, Enum, Bitmask };

struct ParamEnumEntry  { int32_t value; const char* label; };
struct ParamBitmaskBit { int     bit;   const char* label; };

struct ParamMeta {
    const char*              short_desc;
    const char*              units;           // nullptr = dimensionless
    float                    default_val;
    float                    min_val;
    float                    max_val;
    bool                     reboot_required;
    ParamWidgetType          widget;
    const ParamEnumEntry*    enum_entries;    // set for Enum, else nullptr
    int                      enum_count;
    const ParamBitmaskBit*   bitmask_bits;   // set for Bitmask, else nullptr
    int                      bitmask_count;
};

// ── Static enum / bitmask tables ─────────────────────────────────────────────

namespace detail {

// SERIAL*_BAUD  (ArduPilot serial baud enum)
static constexpr ParamEnumEntry kSerialBaud[] = {
    {  1, "1200"   }, {  2, "2400"   }, {  4, "4800"   }, {  9, "9600"   },
    { 19, "19200"  }, { 38, "38400"  }, { 57, "57600"  }, {111, "111100" },
    {115, "115200" }, {230, "230400" }, {256, "256000"  }, {460, "460800" },
    {500, "500000" }, {921, "921600" }, {1500,"1500000" },
};

// SERIAL*_PROTOCOL
static constexpr ParamEnumEntry kSerialProtocol[] = {
    { -1, "None (disabled)"     },
    {  1, "MAVLink1"            },
    {  2, "MAVLink2"            },
    {  3, "Frsky D"             },
    {  4, "Frsky SPort"         },
    {  5, "GPS"                 },
    {  7, "Alexmos Gimbal"      },
    {  8, "SToRM32 Gimbal"      },
    {  9, "Rangefinder"         },
    { 10, "FrSky SPort Passthrough" },
    { 11, "Lidar360"            },
    { 13, "Beacon"              },
    { 14, "Volz Servo"          },
    { 15, "SBus Servo"          },
    { 16, "ESC Telemetry"       },
    { 17, "Devo Telemetry"      },
    { 18, "OpticalFlow"         },
    { 19, "RobotisServo"        },
    { 20, "NMEA Output"         },
    { 21, "WindVane"            },
    { 22, "SLCAN"               },
    { 23, "RCIN"                },
    { 24, "EFI Serial"          },
    { 25, "LTM"                 },
    { 26, "RunCam"              },
    { 27, "HottTelem"           },
    { 28, "Scripting"           },
    { 29, "Crossfire VTX"       },
    { 30, "Generator"           },
    { 32, "Winch"               },
    { 33, "MSP"                 },
    { 42, "AIS"                 },
    { 43, "CoDevESC"            },
    { 44, "Serial LED"          },
    { 45, "HNTCH"               },
};

// FRAME_CLASS (ArduCopter)
static constexpr ParamEnumEntry kFrameClass[] = {
    {  0, "Undefined"     },
    {  1, "Quad"          },
    {  2, "Hexa"          },
    {  3, "Octa"          },
    {  4, "OctaQuad"      },
    {  5, "Y6"            },
    {  6, "Heli"          },
    {  7, "Tri"           },
    {  8, "SingleCopter"  },
    {  9, "CoaxCopter"    },
    { 10, "BiCopter"      },
    { 11, "Heli_Dual"     },
    { 12, "DodecaHexa"    },
    { 13, "HeliQuad"      },
    { 14, "Deca"          },
    { 15, "Scripting"     },
    { 16, "6DoF"          },
    { 17, "DynamicSSC"    },
};

// FRAME_TYPE (ArduCopter)
static constexpr ParamEnumEntry kFrameType[] = {
    { 0, "Plus"       },
    { 1, "X"          },
    { 2, "V"          },
    { 3, "H"          },
    { 4, "V-Tail"     },
    { 5, "A-Tail"     },
    { 10, "Y"         },
    { 11, "Maiden"    },
    { 12, "Coax"      },
    { 13, "BetaFlightX" },
    { 14, "DJIX"      },
    { 15, "ClockwiseX" },
    { 18, "BetaFlightXReversed" },
};

// GPS_TYPE / GPS_TYPE2
static constexpr ParamEnumEntry kGpsType[] = {
    {  0, "None"         },
    {  1, "AUTO"         },
    {  2, "uBlox"        },
    {  5, "NMEA"         },
    {  6, "SiRF"         },
    {  7, "HIL"          },
    {  8, "SwiftNav"     },
    {  9, "DroneCAN"     },
    { 10, "SBF"          },
    { 11, "GSOF"         },
    { 13, "ERB"          },
    { 14, "MAV"          },
    { 15, "NOVA"         },
    { 16, "HemisphereNMEA" },
    { 17, "uBlox-MovingBaseline-Base"   },
    { 18, "uBlox-MovingBaseline-Rover"  },
    { 19, "MSP"          },
    { 20, "AllyStar"     },
    { 21, "ExternalAHRS" },
    { 22, "DroneCAN-MovingBaseline-Base"  },
    { 23, "DroneCAN-MovingBaseline-Rover" },
    { 24, "UnicoreNMEA"  },
    { 25, "UnicoreMovingBaselineNMEA" },
};

// BATT_MONITOR
static constexpr ParamEnumEntry kBattMonitor[] = {
    {  0, "Disabled"                  },
    {  3, "Analog Voltage Only"       },
    {  4, "Analog Voltage and Current"},
    {  5, "Solo"                      },
    {  6, "Bebop"                     },
    {  7, "SMBus-Generic"             },
    {  8, "DroneCAN-BatteryInfo"      },
    {  9, "ESC"                       },
    { 10, "Sum Of Selected Monitors"  },
    { 11, "FuelFlow"                  },
    { 12, "FuelLevelPWM"              },
    { 13, "SMBUS-SUI3"                },
    { 14, "SMBUS-SUI6"                },
    { 15, "NeoDesign"                 },
    { 16, "SMBus-Maxell"              },
    { 17, "Generator-Elec"            },
    { 18, "Generator-Fuel"            },
    { 19, "Rotoye"                    },
    { 20, "MPPT"                      },
    { 21, "INA2xx"                    },
    { 22, "LTC2946"                   },
    { 23, "Torqeedo"                  },
    { 24, "FuelLevelAnalog"           },
    { 25, "Synthetic Current/Analog Voltage" },
    { 26, "INA239_SPI"                },
    { 27, "EFI"                       },
    { 28, "AD7091R5"                  },
    { 29, "Scripting"                 },
};

// FS_THR_ENABLE (Throttle failsafe)
static constexpr ParamEnumEntry kFsThrottle[] = {
    { 0, "Disabled"       },
    { 1, "RTL"            },
    { 2, "Continue Mission (Auto)" },
    { 3, "Land"           },
    { 4, "SmartRTL or RTL" },
    { 5, "SmartRTL or Land" },
    { 6, "BRAKE or Land"  },
    { 7, "Parachute"      },
};

// FS_BATT_ENABLE (Battery failsafe)
static constexpr ParamEnumEntry kFsBattery[] = {
    { 0, "Disabled"         },
    { 1, "Land"             },
    { 2, "RTL"              },
    { 3, "SmartRTL"         },
    { 4, "SmartRTL or Land" },
    { 5, "Terminate"        },
};

// ARMING_RUDDER
static constexpr ParamEnumEntry kArmingRudder[] = {
    { 0, "Disabled"    },
    { 1, "Arm Only"    },
    { 2, "Arm/Disarm"  },
};

// RSSI_TYPE
static constexpr ParamEnumEntry kRssiType[] = {
    { 0, "Disabled"            },
    { 1, "Analog Pin"          },
    { 2, "RC Channel PWM"      },
    { 3, "Receiver Protocol"   },
    { 4, "PWM Input Pin"       },
    { 5, "TelemetryRadioRSSI"  },
};

// EK3_ENABLE
static constexpr ParamEnumEntry kEkfEnable[] = {
    { 0, "Disabled" },
    { 1, "Enabled"  },
};

// INS_FAST_SAMPLE
static constexpr ParamEnumEntry kInsFastSample[] = {
    { 0, "Disabled"         },
    { 1, "First IMU"        },
    { 3, "First Two IMUs"   },
    { 7, "All IMUs"         },
};

// COMPASS_USE / COMPASS_USE2 / COMPASS_USE3
static constexpr ParamEnumEntry kCompassUse[] = {
    { 0, "Disabled" },
    { 1, "Enabled"  },
};

// EK3_SRC1_POSXY / SRC1_VELXY etc
static constexpr ParamEnumEntry kEkfSrcPos[] = {
    { 0, "None"           },
    { 3, "GPS"            },
    { 4, "Beacon"         },
    { 6, "ExternalNav"    },
};

// MOT_PWM_TYPE (motor output type)
static constexpr ParamEnumEntry kMotPwmType[] = {
    { 0, "Normal"       },
    { 1, "OneShot"      },
    { 2, "OneShot125"   },
    { 3, "Brushed16kHz" },
    { 4, "DShot150"     },
    { 5, "DShot300"     },
    { 6, "DShot600"     },
    { 7, "DShot1200"    },
    { 8, "PWMRange"     },
};

// PILOT_TKOFF_ALT default
// (float, no enum — just metadata)

// ── Bitmask tables ────────────────────────────────────────────────────────────

// ARMING_CHECK
static constexpr ParamBitmaskBit kArmingCheck[] = {
    {  0, "All"          },
    {  1, "Barometer"    },
    {  2, "Compass"      },
    {  3, "GPS Lock"     },
    {  4, "INS"          },
    {  5, "Parameters"   },
    {  6, "RC"           },
    {  7, "Board Voltage" },
    {  8, "Battery Level" },
    {  9, "Logging"      },
    { 10, "Hardware Safety Switch" },
    { 11, "GPS Config"   },
    { 12, "System"       },
    { 13, "Mission"      },
    { 14, "Rangefinder"  },
    { 15, "Camera"       },
    { 16, "AuxAuth"      },
    { 17, "VisualOdometry" },
    { 18, "FFT"          },
};

// LOG_BITMASK
static constexpr ParamBitmaskBit kLogBitmask[] = {
    {  0, "Attitude Fast"       },
    {  1, "Attitude Medium"     },
    {  2, "GPS"                 },
    {  3, "Performance"         },
    {  4, "Control Tuning"      },
    {  5, "Navigation Tuning"   },
    {  6, "Mode"                },
    {  7, "IMU"                 },
    {  8, "Commands"            },
    {  9, "Battery"             },
    { 10, "Compass"             },
    { 11, "TECS"                },
    { 12, "Camera"              },
    { 13, "RC Input"            },
    { 14, "Sonar"               },
    { 15, "ARM/DISARM"          },
    { 16, "IMU Raw"             },
    { 17, "Vibration"           },
    { 18, "Video Stabilization" },
    { 19, "ARSP"                },
    { 20, "EKF"                 },
    { 21, "Optical Flow"        },
    { 22, "Body Frame Odometry" },
    { 23, "GPA"                 },
};

// RC_OPTIONS
static constexpr ParamBitmaskBit kRcOptions[] = {
    {  0, "Ignore Override CH"     },
    {  1, "Ignore RC Receiver"     },
    {  2, "Enable Multiple Receivers" },
    {  3, "Allow RC_OVERRIDE in Auto" },
    {  4, "Somewhat Relaxed Mode"  },
    {  5, "Enable RSSI"            },
    {  6, "Arming Check"           },
    {  7, "Allow Arming from TX"   },
    {  8, "Skip RC Cal on Boot"    },
    {  9, "Enable Camera Pan"      },
    { 10, "Send RCIN to OSD"       },
};

// INS_LOG_BAT_MASK
static constexpr ParamBitmaskBit kInsBatMask[] = {
    { 0, "IMU 1" },
    { 1, "IMU 2" },
    { 2, "IMU 3" },
};

// INS_NOTCH_FLAGS (HNotch)
static constexpr ParamBitmaskBit kHNotchFlags[] = {
    { 0, "Enable"                 },
    { 1, "Double Notch"           },
    { 2, "Dynamic Harmonic"       },
    { 3, "Update at Loop Rate"    },
    { 4, "Enable 2nd Harmonic"    },
    { 5, "Enable 3rd Harmonic"    },
    { 6, "Enable 4th Harmonic"    },
    { 7, "Enable 5th Harmonic"    },
    { 8, "Enable 6th Harmonic"    },
    { 9, "Enable 7th Harmonic"    },
    { 10, "Triple Notch"          },
};

// FENCE_TYPE
static constexpr ParamBitmaskBit kFenceType[] = {
    { 0, "Max Altitude" },
    { 1, "Circle"       },
    { 2, "Polygon"      },
    { 3, "Min Altitude" },
};

// ── Helpers ───────────────────────────────────────────────────────────────────

#define ARR(x) x, (int)(sizeof(x)/sizeof(x[0]))

// Shorthand constructors
static inline ParamMeta make_enum(const char* desc, const char* units,
                                   float def, float mn, float mx, bool reboot,
                                   const ParamEnumEntry* e, int ec)
{
    return { desc, units, def, mn, mx, reboot,
             ParamWidgetType::Enum, e, ec, nullptr, 0 };
}

static inline ParamMeta make_bitmask(const char* desc, const char* units,
                                      float def, bool reboot,
                                      const ParamBitmaskBit* b, int bc)
{
    return { desc, units, def, 0, 0, reboot,
             ParamWidgetType::Bitmask, nullptr, 0, b, bc };
}

static inline ParamMeta make_float(const char* desc, const char* units,
                                    float def, float mn, float mx, bool reboot)
{
    return { desc, units, def, mn, mx, reboot,
             ParamWidgetType::Float, nullptr, 0, nullptr, 0 };
}

// ── Main lookup table & accessor ─────────────────────────────────────────────

inline const ParamMeta* get_param_meta(const char* param_id)
{
    using Map = std::unordered_map<std::string, ParamMeta>;

    static const Map s_meta = {
        // ── Serial ────────────────────────────────────────────────────────────
        { "SERIAL_BAUD",
          make_enum("Serial port baud rate", nullptr,
                    57, 0, 1500, true,
                    ARR(kSerialBaud)) },
        { "SERIAL_PROTOCOL",
          make_enum("Serial port protocol selection", nullptr,
                    2, -1, 44, true,
                    ARR(kSerialProtocol)) },

        // ── Frame ─────────────────────────────────────────────────────────────
        { "FRAME_CLASS",
          make_enum("Vehicle frame class", nullptr,
                    1, 0, 17, true,
                    ARR(kFrameClass)) },
        { "FRAME_TYPE",
          make_enum("Vehicle frame geometry type", nullptr,
                    1, 0, 18, true,
                    ARR(kFrameType)) },

        // ── GPS ───────────────────────────────────────────────────────────────
        { "GPS_TYPE",
          make_enum("GPS 1 type", nullptr,
                    1, 0, 25, true,
                    ARR(kGpsType)) },
        { "GPS_TYPE",   // duplicate handled by digit-strip
          make_enum("GPS type", nullptr, 1, 0, 25, true, ARR(kGpsType)) },

        // ── Battery ───────────────────────────────────────────────────────────
        { "BATT_MONITOR",
          make_enum("Battery monitoring type", nullptr,
                    0, 0, 29, true,
                    ARR(kBattMonitor)) },

        // ── Failsafe ──────────────────────────────────────────────────────────
        { "FS_THR_ENABLE",
          make_enum("Throttle failsafe action", nullptr,
                    1, 0, 7, false,
                    ARR(kFsThrottle)) },
        { "FS_BATT_ENABLE",
          make_enum("Battery failsafe action", nullptr,
                    0, 0, 5, false,
                    ARR(kFsBattery)) },

        // ── Arming ────────────────────────────────────────────────────────────
        { "ARMING_RUDDER",
          make_enum("Arm/disarm with rudder", nullptr,
                    2, 0, 2, false,
                    ARR(kArmingRudder)) },
        { "ARMING_CHECK",
          make_bitmask("Arming pre-checks to perform", nullptr,
                       1, false,
                       ARR(kArmingCheck)) },

        // ── Compass ───────────────────────────────────────────────────────────
        { "COMPASS_USE",
          make_enum("Use compass 1 for yaw", nullptr,
                    1, 0, 1, false,
                    ARR(kCompassUse)) },
        { "COMPASS_USE2",
          make_enum("Use compass 2 for yaw", nullptr,
                    1, 0, 1, false,
                    ARR(kCompassUse)) },
        { "COMPASS_USE3",
          make_enum("Use compass 3 for yaw", nullptr,
                    1, 0, 1, false,
                    ARR(kCompassUse)) },
        { "COMPASS_AUTODEC",
          make_enum("Enable automatic magnetic declination", nullptr,
                    1, 0, 1, false,
                    ARR(kCompassUse)) },

        // ── EKF ───────────────────────────────────────────────────────────────
        { "EK3_ENABLE",
          make_enum("Enable EKF3", nullptr,
                    1, 0, 1, true,
                    ARR(kEkfEnable)) },
        { "EK2_ENABLE",
          make_enum("Enable EKF2 (legacy)", nullptr,
                    0, 0, 1, true,
                    ARR(kEkfEnable)) },

        // ── INS ───────────────────────────────────────────────────────────────
        { "INS_FAST_SAMPLE",
          make_enum("Fast IMU sampling bitmask", nullptr,
                    1, 0, 7, true,
                    ARR(kInsFastSample)) },
        { "INS_LOG_BAT_MASK",
          make_bitmask("IMU batch logging bitmask", nullptr,
                       0, false,
                       ARR(kInsBatMask)) },

        // ── HNotch ────────────────────────────────────────────────────────────
        { "INS_HNTCH_ENABLE",
          make_enum("Enable harmonic notch filter", nullptr,
                    0, 0, 1, false,
                    ARR(kEkfEnable)) },
        { "INS_HNTCH_FLAGS",
          make_bitmask("Harmonic notch filter options", nullptr,
                       0, false,
                       ARR(kHNotchFlags)) },
        { "INS_HNTC2_ENABLE",
          make_enum("Enable 2nd harmonic notch filter", nullptr,
                    0, 0, 1, false,
                    ARR(kEkfEnable)) },
        { "INS_HNTC2_FLAGS",
          make_bitmask("2nd harmonic notch filter options", nullptr,
                       0, false,
                       ARR(kHNotchFlags)) },

        // ── Logging ───────────────────────────────────────────────────────────
        { "LOG_BITMASK",
          make_bitmask("Logging bitmask — select what to log", nullptr,
                       0xFFFF, false,
                       ARR(kLogBitmask)) },
        { "LOG_DISARMED",
          make_enum("Log even when disarmed", nullptr,
                    0, 0, 1, false,
                    ARR(kEkfEnable)) },
        { "LOG_REPLAY",
          make_enum("Enable extended log for replay", nullptr,
                    0, 0, 1, true,
                    ARR(kEkfEnable)) },

        // ── RC ────────────────────────────────────────────────────────────────
        { "RC_OPTIONS",
          make_bitmask("RC options bitmask", nullptr,
                       0, false,
                       ARR(kRcOptions)) },
        { "RSSI_TYPE",
          make_enum("RSSI source type", nullptr,
                    0, 0, 5, false,
                    ARR(kRssiType)) },

        // ── Motors ────────────────────────────────────────────────────────────
        { "MOT_PWM_TYPE",
          make_enum("Motor PWM output type", nullptr,
                    0, 0, 8, true,
                    ARR(kMotPwmType)) },
        { "MOT_SPIN_ARM",
          make_float("Motor spin speed when armed (0–1)", nullptr,
                     0.1f, 0.0f, 0.3f, false) },
        { "MOT_SPIN_MIN",
          make_float("Motor spin minimum — below this ESC may stall", nullptr,
                     0.15f, 0.0f, 0.3f, false) },
        { "MOT_SPIN_MAX",
          make_float("Motor spin maximum (0–1)", nullptr,
                     0.95f, 0.9f, 1.0f, false) },
        { "MOT_THST_EXPO",
          make_float("Motor thrust linearisation expo (0 = linear, 1 = quadratic)", nullptr,
                     0.65f, -1.0f, 1.0f, false) },

        // ── PID — Roll ────────────────────────────────────────────────────────
        { "ATC_RAT_RLL_P",
          make_float("Roll rate controller P gain", nullptr,
                     0.135f, 0.0f, 0.9f, false) },
        { "ATC_RAT_RLL_I",
          make_float("Roll rate controller I gain", nullptr,
                     0.135f, 0.0f, 0.9f, false) },
        { "ATC_RAT_RLL_D",
          make_float("Roll rate controller D gain", nullptr,
                     0.0036f, 0.0f, 0.03f, false) },
        { "ATC_RAT_RLL_FF",
          make_float("Roll rate controller feed-forward", nullptr,
                     0.0f, 0.0f, 0.5f, false) },

        // ── PID — Pitch ───────────────────────────────────────────────────────
        { "ATC_RAT_PIT_P",
          make_float("Pitch rate controller P gain", nullptr,
                     0.135f, 0.0f, 0.9f, false) },
        { "ATC_RAT_PIT_I",
          make_float("Pitch rate controller I gain", nullptr,
                     0.135f, 0.0f, 0.9f, false) },
        { "ATC_RAT_PIT_D",
          make_float("Pitch rate controller D gain", nullptr,
                     0.0036f, 0.0f, 0.03f, false) },
        { "ATC_RAT_PIT_FF",
          make_float("Pitch rate controller feed-forward", nullptr,
                     0.0f, 0.0f, 0.5f, false) },

        // ── PID — Yaw ─────────────────────────────────────────────────────────
        { "ATC_RAT_YAW_P",
          make_float("Yaw rate controller P gain", nullptr,
                     0.18f, 0.1f, 2.5f, false) },
        { "ATC_RAT_YAW_I",
          make_float("Yaw rate controller I gain", nullptr,
                     0.018f, 0.01f, 0.9f, false) },
        { "ATC_RAT_YAW_D",
          make_float("Yaw rate controller D gain", nullptr,
                     0.0f, 0.0f, 0.02f, false) },
        { "ATC_RAT_YAW_FF",
          make_float("Yaw rate controller feed-forward", nullptr,
                     0.0f, 0.0f, 0.5f, false) },

        // ── Altitude / Climb ──────────────────────────────────────────────────
        { "PSC_ACCZ_P",
          make_float("Vertical acceleration controller P gain", nullptr,
                     0.5f, 0.1f, 1.5f, false) },
        { "PSC_ACCZ_I",
          make_float("Vertical acceleration controller I gain", nullptr,
                     1.0f, 0.0f, 3.0f, false) },
        { "PILOT_SPEED_UP",
          make_float("Pilot max climb speed", "cm/s",
                     250.0f, 10.0f, 800.0f, false) },
        { "PILOT_SPEED_DN",
          make_float("Pilot max descent speed (0 = use SPEED_UP)", "cm/s",
                     0.0f, 0.0f, 500.0f, false) },
        { "PILOT_ACCEL_Z",
          make_float("Pilot vertical acceleration", "cm/s/s",
                     250.0f, 10.0f, 500.0f, false) },

        // ── Position hold ─────────────────────────────────────────────────────
        { "WPNAV_SPEED",
          make_float("Waypoint horizontal speed", "cm/s",
                     500.0f, 20.0f, 2000.0f, false) },
        { "WPNAV_SPEED_UP",
          make_float("Waypoint climb speed", "cm/s",
                     250.0f, 10.0f, 1000.0f, false) },
        { "WPNAV_SPEED_DN",
          make_float("Waypoint descent speed", "cm/s",
                     150.0f, 10.0f, 500.0f, false) },
        { "WPNAV_ACCEL",
          make_float("Waypoint horizontal acceleration", "cm/s/s",
                     100.0f, 10.0f, 980.0f, false) },
        { "WPNAV_RADIUS",
          make_float("Waypoint acceptance radius", "cm",
                     200.0f, 5.0f, 1000.0f, false) },

        // ── Fence ─────────────────────────────────────────────────────────────
        { "FENCE_ENABLE",
          make_enum("Enable geofence", nullptr,
                    0, 0, 1, false,
                    ARR(kEkfEnable)) },
        { "FENCE_TYPE",
          make_bitmask("Fence type bitmask", nullptr,
                       7, false,
                       ARR(kFenceType)) },
        { "FENCE_ALT_MAX",
          make_float("Fence maximum altitude", "m",
                     100.0f, 10.0f, 1000.0f, false) },
        { "FENCE_RADIUS",
          make_float("Fence circle radius", "m",
                     300.0f, 30.0f, 10000.0f, false) },

        // ── Takeoff ───────────────────────────────────────────────────────────
        { "PILOT_TKOFF_ALT",
          make_float("Auto takeoff altitude above home", "cm",
                     0.0f, 0.0f, 1000.0f, false) },

        // ── RTL ───────────────────────────────────────────────────────────────
        { "RTL_ALT",
          make_float("RTL minimum altitude above home", "cm",
                     1500.0f, 200.0f, 30000.0f, false) },
        { "RTL_SPEED",
          make_float("RTL horizontal speed (0 = use WPNAV_SPEED)", "cm/s",
                     0.0f, 0.0f, 2000.0f, false) },
        { "RTL_LOIT_TIME",
          make_float("RTL loiter time before landing", "ms",
                     5000.0f, 0.0f, 60000.0f, false) },
        { "RTL_ALT_FINAL",
          make_float("RTL final descent altitude — 0 = land", "cm",
                     0.0f, -1.0f, 1000.0f, false) },
    };

    // ── Pass 1: exact match ───────────────────────────────────────────────────
    {
        auto it = s_meta.find(param_id);
        if (it != s_meta.end()) return &it->second;
    }

    // ── Pass 2: digit-strip fallback ─────────────────────────────────────────
    // Convert e.g. "SERIAL2_BAUD" → "SERIAL_BAUD"
    // Strategy: remove runs of digits that sit between letters or at name end
    std::string stripped;
    stripped.reserve(strlen(param_id));
    const char* p = param_id;
    while (*p) {
        if (std::isdigit((unsigned char)*p)) {
            // skip digit run
            while (std::isdigit((unsigned char)*p)) ++p;
        } else {
            stripped += *p++;
        }
    }

    if (stripped != param_id) {
        auto it = s_meta.find(stripped);
        if (it != s_meta.end()) return &it->second;
    }

    return nullptr;
}

#undef ARR

} // namespace detail

// Public API — forward to detail
inline const ParamMeta* get_param_meta(const char* param_id)
{
    return detail::get_param_meta(param_id);
}
