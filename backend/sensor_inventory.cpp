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


#include "sensor_inventory.hpp"

#include <cmath>
#include <cstdio>

namespace {

using ParamMap = std::unordered_map<std::string, ParamEntry>;

const ParamEntry* find(const ParamMap* params, const std::string& name)
{
    if (!params) return nullptr;
    auto it = params->find(name);
    return (it == params->end()) ? nullptr : &it->second;
}

// A device ID is at most 24 bits, so it survives the float a parameter is
// carried in exactly — every value below 2^24 does. Rounded rather than
// truncated: a float that has been through a text field can arrive as
// 4849919.9998.
uint32_t param_devid(const ParamEntry* p)
{
    if (!p || !(p->value > 0.0f)) return 0;
    const double v = std::floor((double)p->value + 0.5);
    if (v <= 0.0 || v > 16777215.0) return 0;
    return (uint32_t)v;
}

// AP_HAL::Device::BusType, which PX4's device ID uses the same way.
const char* bus_type_name(uint8_t t)
{
    switch (t) {
    case 1: return "I2C";
    case 2: return "SPI";
    case 3: return "DroneCAN";
    case 4: return "SITL";
    case 5: return "MSP";
    case 6: return "Serial";
    case 7: return "QSPI";
    default: return "unknown bus";
    }
}

// AP_Compass_Backend::DevTypes. The numbers are the vehicle's, not MAVLink's,
// and a value missing here means only that this table has not been taught it.
const char* compass_devtype_name(uint8_t t)
{
    switch (t) {
    case 0x01: return "HMC5883 (old)";
    case 0x02: return "LSM303D";
    case 0x04: return "AK8963";
    case 0x05: return "BMM150";
    case 0x06: return "LSM9DS1";
    case 0x07: return "HMC5883";
    case 0x08: return "LIS3MDL";
    case 0x09: return "AK09916";
    case 0x0A: return "IST8310";
    case 0x0B: return "ICM20948";
    case 0x0C: return "MMC3416";
    case 0x0D: return "QMC5883L";
    case 0x0E: return "MAG3110";
    case 0x0F: return "SITL";
    case 0x10: return "IST8308";
    case 0x11: return "RM3100";
    case 0x12: return "RM3100";
    case 0x13: return "MMC5883";
    case 0x14: return "AK09918";
    case 0x15: return "AK09915";
    case 0x16: return "QMC5883P";
    case 0x17: return "BMM350";
    case 0x18: return "IIS2MDC";
    default:   return "";
    }
}

// AP_InertialSensor_Backend::DevTypes. Accelerometers and gyros share the
// numbering: the same chip provides both, and the ID names the chip.
const char* imu_devtype_name(uint8_t t)
{
    switch (t) {
    case 0x09: return "BMI160";
    case 0x10: return "L3G4200D";
    case 0x11: return "LSM303D";
    case 0x12: return "BMA180";
    case 0x13: return "MPU6000";
    case 0x16: return "MPU9250";
    case 0x17: return "IIS328DQ";
    case 0x18: return "LSM9DS1";
    case 0x21: return "MPU6000";
    case 0x22: return "L3GD20";
    case 0x24: return "MPU9250";
    case 0x25: return "I3G4250D";
    case 0x26: return "LSM9DS1";
    case 0x27: return "ICM20789";
    case 0x28: return "ICM20689";
    case 0x29: return "BMI055";
    case 0x2A: return "SITL";
    case 0x2B: return "BMI088";
    case 0x2C: return "ICM20948";
    case 0x2D: return "ICM20648";
    case 0x2E: return "ICM20649";
    case 0x2F: return "ICM20602";
    case 0x30: return "ICM20601";
    case 0x31: return "ADIS1647x";
    case 0x32: return "serial IMU";
    case 0x33: return "ICM40609";
    case 0x34: return "ICM42688";
    case 0x35: return "ICM42605";
    case 0x36: return "ICM40605";
    case 0x37: return "IIM42652";
    case 0x38: return "BMI270";
    case 0x39: return "BMI085";
    case 0x3A: return "ICM42670";
    case 0x3B: return "ICM45686";
    case 0x3C: return "SCHA63T";
    case 0x3D: return "IIM42653";
    default:   return "";
    }
}

// The parameter names each stack uses, per instance. Both are tried for every
// vehicle rather than switched on the autopilot type from the heartbeat: a
// parameter table either has the name in it or it does not, which is a surer
// test than what the vehicle calls itself.
struct Naming {
    // %d is the 1-based instance for ArduPilot, which drops it on the first —
    // COMPASS_DEV_ID, then COMPASS_DEV_ID2 — and the 0-based one for PX4,
    // which never drops it. Handled by the two builders below.
    const char* ap_id_first;    // ArduPilot, instance 1
    const char* ap_id_rest;     // ArduPilot, instances 2+ (printf, %d)
    const char* ap_use_first;   // may be null
    const char* ap_use_rest;
    const char* px4_id;         // PX4 (printf, %d, 0-based)
};

const Naming kCompass = {
    "COMPASS_DEV_ID", "COMPASS_DEV_ID%d",
    "COMPASS_USE",    "COMPASS_USE%d",
    "CAL_MAG%d_ID",
};
const Naming kAccel = {
    "INS_ACC_ID", "INS_ACC%d_ID",
    nullptr,      nullptr,
    "CAL_ACC%d_ID",
};
const Naming kGyro = {
    "INS_GYR_ID", "INS_GYR%d_ID",
    nullptr,      nullptr,
    "CAL_GYRO%d_ID",
};

// ArduPilot's compass external flag, whose name is abbreviated past instance 1
// in a way nothing else here is: COMPASS_EXTERNAL, COMPASS_EXTERN2.
std::string compass_external_name(int instance)
{
    if (instance <= 1) return "COMPASS_EXTERNAL";
    char buf[24];
    snprintf(buf, sizeof(buf), "COMPASS_EXTERN%d", instance);
    return buf;
}

std::string formatted(const char* fmt, int n)
{
    char buf[24];
    snprintf(buf, sizeof(buf), fmt, n);
    return buf;
}

void decode_into(SensorDevice& d, SensorKind kind)
{
    d.bus_type =  d.devid        & 0x07;
    d.bus      = (d.devid >> 3)  & 0x1F;
    d.address  = (d.devid >> 8)  & 0xFF;
    d.devtype  = (d.devid >> 16) & 0xFF;

    d.bus_name     = bus_type_name(d.bus_type);
    d.devtype_name = (kind == SensorKind::Compass) ? compass_devtype_name(d.devtype)
                                                   : imu_devtype_name(d.devtype);
}

// The most instances either stack exposes. ArduPilot allows eight compasses and
// PX4 four of everything; stopping here costs nothing and bounds the loop.
constexpr int kMaxInstances = 8;

} // namespace

std::vector<SensorDevice> sensor_inventory(const ParamMap* params, SensorKind kind)
{
    std::vector<SensorDevice> out;
    if (!params || params->empty()) return out;

    const Naming& n = (kind == SensorKind::Compass) ? kCompass
                    : (kind == SensorKind::Accel)   ? kAccel
                                                    : kGyro;

    for (int i = 1; i <= kMaxInstances; ++i) {
        const std::string id_name = (i == 1) ? std::string(n.ap_id_first)
                                             : formatted(n.ap_id_rest, i);

        const ParamEntry* id = find(params, id_name);
        std::string       used_name = id_name;

        // PX4 counts from zero and never drops the number, so instance i lives
        // under i-1 there.
        if (!id) {
            const std::string px4 = formatted(n.px4_id, i - 1);
            id = find(params, px4);
            if (id) used_name = px4;
        }

        // A parameter that is absent means this stack does not have the slot;
        // one that is zero means the slot is there and empty. Neither is a
        // sensor, and neither is worth a row.
        const uint32_t devid = param_devid(id);
        if (devid == 0) continue;

        SensorDevice d;
        d.param    = used_name;
        d.instance = i;
        d.devid    = devid;
        decode_into(d, kind);

        if (n.ap_use_first) {
            const std::string use_name = (i == 1) ? std::string(n.ap_use_first)
                                                  : formatted(n.ap_use_rest, i);
            if (const ParamEntry* u = find(params, use_name)) {
                d.has_use = true;
                d.in_use  = (u->value != 0.0f);
            }
        }

        if (kind == SensorKind::Compass) {
            if (const ParamEntry* e = find(params, compass_external_name(i))) {
                d.has_external = true;
                d.external     = (e->value != 0.0f);
            }
        }

        out.push_back(d);
    }

    return out;
}

// ── Component identifiers ────────────────────────────────────────────────────

const char* mav_component_name(uint8_t compid)
{
    // MAV_COMPONENT, abridged to what turns up on a vehicle. Note there is no
    // MAV_COMP_ID for a compass or a barometer: sensors on the autopilot's own
    // buses are not components and do not have their own IDs. 200-202 are the
    // ones for an IMU that speaks MAVLink in its own right.
    switch (compid) {
    case 0:   return "broadcast";
    case 1:   return "autopilot";
    case 100: case 101: case 102: case 103:
    case 104: case 105:            return "camera";
    case 154: case 171: case 172:
    case 173: case 174: case 175:  return "gimbal";
    case 155: return "log";
    case 156: return "ADS-B";
    case 157: return "OSD";
    case 158: return "peripheral";
    case 160: return "FLARM";
    case 161: return "parachute";
    case 169: return "winch";
    case 180: case 181:            return "battery";
    case 189: return "MAVCAN";
    case 190: return "ground station";
    case 191: case 192: case 193:
    case 194:                      return "onboard computer";
    case 195: return "path planner";
    case 196: return "obstacle avoidance";
    case 197: return "visual odometry";
    case 198: return "pairing manager";
    case 200: return "IMU";
    case 201: return "IMU 2";
    case 202: return "IMU 3";
    case 220: return "GPS";
    case 221: return "GPS 2";
    case 240: return "UDP bridge";
    case 241: return "UART bridge";
    case 242: return "tunnel node";
    case 250: return "system control";
    default:  return nullptr;
    }
}

const char* mav_autopilot_name(uint8_t autopilot)
{
    switch (autopilot) {
    case 0:  return "generic";
    case 3:  return "ArduPilot";
    case 8:  return "invalid";      // MAV_AUTOPILOT_INVALID — not an autopilot
    case 12: return "PX4";
    default: return nullptr;
    }
}

const char* mav_vehicle_type_name(uint8_t type)
{
    switch (type) {
    case 1:  return "fixed wing";
    case 2:  return "quadrotor";
    case 3:  return "coaxial";
    case 4:  return "helicopter";
    case 5:  return "antenna tracker";
    case 6:  return "GCS";
    case 10: return "ground rover";
    case 11: return "surface boat";
    case 12: return "submarine";
    case 13: return "hexarotor";
    case 14: return "octorotor";
    case 15: return "tricopter";
    case 18: return "VTOL (duorotor)";
    case 19: return "VTOL (quadrotor)";
    case 20: return "VTOL (tiltrotor)";
    case 26: return "gimbal";
    case 27: return "ADS-B";
    case 29: return "camera";
    case 30: return "charging station";
    case 31: return "onboard controller";
    default: return nullptr;
    }
}
