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

// MavlinkSender: commands to the vehicle — arming, modes, movement, calibration,
// capability and stream-rate requests, TIMESYNC.
// Every MAVLink message sent is packed and queued by MavlinkSender; the queue,
// transport and retries are in sender.cpp.

#include "backend/mavlink/sender/sender.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <mutex>
#include <mavlink/ardupilotmega/mavlink.h>
#include "frontend/source/app_log.hpp"   // gcs_log — thread-safe, see the header

using Clock = std::chrono::steady_clock;

void MavlinkSender::send_manual_control(uint8_t tsys, int16_t x, int16_t y, int16_t z,
                                        int16_t r, uint32_t buttons)
{
    mavlink_message_t msg;
    std::lock_guard<std::mutex> _tx(tx_mtx_);
    mavlink_msg_manual_control_pack_status(gcs_sysid(), GCS_COMPID, &tx_status_, &msg,
                                           tsys, x, y, z, r,
                                           (uint16_t)(buttons & 0xFFFFu),
                                           (uint16_t)(buttons >> 16),
                                           0 /* enabled_extensions */, 0, 0,
                                           0, 0, 0, 0, 0, 0);
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    const uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
    std::lock_guard<std::mutex> lk(mtx_);
    manual_frame_.assign(buf, buf + len);
    manual_pending_.target   = tsys;
    manual_pending_.x        = x;
    manual_pending_.y        = y;
    manual_pending_.z        = z;
    manual_pending_.r        = r;
    manual_pending_.buttons  = (uint16_t)(buttons & 0xFFFFu);
    manual_pending_.buttons2 = (uint16_t)(buttons >> 16);
}

void MavlinkSender::send_rc_override(uint8_t tsys, uint8_t tcomp, const uint16_t c[18])
{
    mavlink_message_t msg;
    std::lock_guard<std::mutex> _tx(tx_mtx_);
    mavlink_msg_rc_channels_override_pack_status(gcs_sysid(), GCS_COMPID, &tx_status_, &msg,
                                                 tsys, tcomp,
                                                 c[0],  c[1],  c[2],  c[3],  c[4],  c[5],
                                                 c[6],  c[7],  c[8],  c[9],  c[10], c[11],
                                                 c[12], c[13], c[14], c[15], c[16], c[17]);
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    const uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
    std::lock_guard<std::mutex> lk(mtx_);
    rc_frame_.assign(buf, buf + len);
    rc_pending_.target = tsys;
    std::copy(c, c + 18, rc_pending_.chan);
}

MavlinkSender::RcOverrideFrame MavlinkSender::rc_override_sent() const
{
    std::lock_guard<std::mutex> lk(mtx_);
    return rc_sent_;
}

MavlinkSender::ManualControlFrame MavlinkSender::manual_control_sent() const
{
    std::lock_guard<std::mutex> lk(mtx_);
    return manual_sent_;
}

void MavlinkSender::arm(uint8_t tsys, uint8_t tcomp)
{
    // MAV_CMD_COMPONENT_ARM_DISARM (400), param1=1 (arm)
    enqueue_command_long(tsys, tcomp, 400, 1.f);
}

void MavlinkSender::disarm(uint8_t tsys, uint8_t tcomp, bool force)
{
    // MAV_CMD_COMPONENT_ARM_DISARM (400), param1=0 (disarm)
    // param2=21196 is the magic number for a forced disarm, honoured in flight:
    // the motors stop, whatever the vehicle's own checks would say
    enqueue_command_long(tsys, tcomp, 400, 0.f, force ? 21196.f : 0.f);
}

void MavlinkSender::takeoff(uint8_t tsys, uint8_t tcomp, float altitude_m)
{
    // MAV_CMD_NAV_TAKEOFF (22), param7=altitude (m, relative to home)
    enqueue_command_long(tsys, tcomp, 22,
                         0.f, 0.f, 0.f, 0.f, 0.f, 0.f, altitude_m);
}

void MavlinkSender::set_mode(uint8_t tsys, uint8_t tcomp, uint32_t custom_mode)
{
    // param1 = MAV_MODE_FLAG_CUSTOM_MODE_ENABLED (1)
    // param2 = custom_mode enum value, param3 unused — the ArduPilot shape.
    set_mode_raw(tsys, tcomp, 1.f, static_cast<float>(custom_mode), 0.f);
}

void MavlinkSender::set_mode_raw(uint8_t tsys, uint8_t tcomp,
                                 float base_mode, float custom_mode,
                                 float custom_submode)
{
    // MAV_CMD_DO_SET_MODE (176)
    enqueue_command_long(tsys, tcomp, 176, base_mode, custom_mode, custom_submode);
}

void MavlinkSender::return_to_launch(uint8_t tsys, uint8_t tcomp)
{
    // MAV_CMD_NAV_RETURN_TO_LAUNCH (20)
    enqueue_command_long(tsys, tcomp, 20);
}

void MavlinkSender::reposition(uint8_t tsys, uint8_t tcomp,
                               double lat_deg, double lon_deg, float altitude_m)
{
    mavlink_message_t msg;
    std::lock_guard<std::mutex> _tx(tx_mtx_);
    mavlink_msg_command_int_pack_status(
        gcs_sysid(), GCS_COMPID, &tx_status_, &msg,
        tsys, tcomp,
        MAV_FRAME_GLOBAL_RELATIVE_ALT_INT,
        MAV_CMD_DO_REPOSITION,
        0,          // current
        0,          // autocontinue
        -1.0f,      // ground speed: -1 leaves the vehicle's default alone
        0.0f,       // bitmask
        0.0f,       // loiter radius: 0 is the vehicle's own
        NAN,        // yaw: NaN holds the current heading
        (int32_t)std::lround(lat_deg * 1e7),
        (int32_t)std::lround(lon_deg * 1e7),
        altitude_m);

    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    const uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
    std::lock_guard<std::mutex> lk(mtx_);
    queue_.emplace(buf, buf + len);
}

void MavlinkSender::goto_position(uint8_t tsys, uint8_t tcomp,
                                  double lat_deg, double lon_deg,
                                  float altitude_m)
{
    // Everything but the three position fields is masked off. Bits 0-2 are the
    // position, 3-5 velocity, 6-8 acceleration, 9 force, 10 yaw, 11 yaw rate;
    // a set bit means "ignore this field", so the position bits are the only
    // ones left clear.
    constexpr uint16_t TYPE_MASK = 0x0DF8;   // 0b0000110111111000

    // time_boot_ms is the sender's own uptime, and ArduPilot does not act on
    // it. Measured from first use rather than from process start because the
    // field is only ever read as a monotonic stamp.
    static const auto t0 = std::chrono::steady_clock::now();
    const uint32_t boot_ms = (uint32_t)std::chrono::duration_cast<
        std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count();

    mavlink_message_t msg;
    std::lock_guard<std::mutex> _tx(tx_mtx_);
    mavlink_msg_set_position_target_global_int_pack_status(
        gcs_sysid(), GCS_COMPID, &tx_status_, &msg,
        boot_ms, tsys, tcomp,
        MAV_FRAME_GLOBAL_RELATIVE_ALT_INT,
        TYPE_MASK,
        (int32_t)std::lround(lat_deg * 1e7),
        (int32_t)std::lround(lon_deg * 1e7),
        altitude_m,
        0.f, 0.f, 0.f,      // vx, vy, vz          — ignored by the mask
        0.f, 0.f, 0.f,      // afx, afy, afz       — ignored by the mask
        0.f, 0.f);          // yaw, yaw_rate       — ignored by the mask

    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    const uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
    std::lock_guard<std::mutex> lk(mtx_);
    queue_.emplace(buf, buf + len);
}

void MavlinkSender::command_long(uint8_t tsys, uint8_t tcomp, uint16_t cmd,
                                 float p1, float p2, float p3,
                                 float p4, float p5, float p6, float p7)
{
    // Deliberately unvalidated: the caller names the MAV_CMD and owns what it
    // means. Every command still lands in the same queue, so ACK feedback and
    // flash state work for these exactly as for the named commands.
    enqueue_command_long(tsys, tcomp, cmd, p1, p2, p3, p4, p5, p6, p7);
}

void MavlinkSender::do_aux_function(uint8_t tsys, uint8_t tcomp,
                                     uint16_t function, uint8_t switch_pos)
{
    // MAV_CMD_DO_AUX_FUNCTION (218)
    // param1 = aux function number, param2 = switch position (0=LOW,1=MID,2=HIGH)
    enqueue_command_long(tsys, tcomp, 218,
                         (float)function, (float)switch_pos);
}

void MavlinkSender::do_set_servo(uint8_t tsys, uint8_t tcomp,
                                  uint8_t servo_num, uint16_t pwm_us)
{
    // MAV_CMD_DO_SET_SERVO (183)
    // param1 = servo number (1-based), param2 = PWM pulse width in microseconds
    enqueue_command_long(tsys, tcomp, 183,
                         (float)servo_num, (float)pwm_us);
}

void MavlinkSender::request_message(uint8_t tsys, uint8_t tcomp, uint16_t message_id)
{
    // MAV_CMD_REQUEST_MESSAGE (512), param1 = message id.
    enqueue_command_long(tsys, tcomp, 512, (float)message_id);
}

// ── Sensor calibration ───────────────────────────────────────────────────────

void MavlinkSender::preflight_calibration(uint8_t tsys, uint8_t tcomp,
                                          float gyro, float mag, float baro,
                                          float rc, float accel, float airspeed,
                                          float esc)
{
    // MAV_CMD_PREFLIGHT_CALIBRATION (241)
    enqueue_command_long(tsys, tcomp, 241,
                         gyro, mag, baro, rc, accel, airspeed, esc);
}

void MavlinkSender::calibrate_accelerometer(uint8_t tsys, uint8_t tcomp)
{
    // param5 = 1 is ArduPilot's full six-position calibration, the one that
    // talks back and waits to be told when the airframe has been moved. The
    // other accepted values calibrate without a conversation — 2 levels the
    // board where it sits, 4 is the single-position form — and neither drives
    // the exchange this GCS implements.
    preflight_calibration(tsys, tcomp, 0.f, 0.f, 0.f, 0.f, 1.f);
}

void MavlinkSender::calibrate_gyroscope(uint8_t tsys, uint8_t tcomp)
{
    // param1 = 1, and every other parameter zero. ArduPilot also accepts 3 here
    // for the gyro's temperature calibration, which is a different and far
    // longer procedure than the one this panel drives.
    preflight_calibration(tsys, tcomp, 1.f);
}

void MavlinkSender::start_mag_cal(uint8_t tsys, uint8_t tcomp)
{
    // MAV_CMD_DO_START_MAG_CAL (42424): all compasses, retry on, autosave off,
    // no delay, no autoreboot. See the header for why autosave is off.
    enqueue_command_long(tsys, tcomp, 42424, 0.f, 1.f, 0.f, 0.f, 0.f);
}

void MavlinkSender::accept_mag_cal(uint8_t tsys, uint8_t tcomp)
{
    // MAV_CMD_DO_ACCEPT_MAG_CAL (42425), param1 = 0 for every compass.
    enqueue_command_long(tsys, tcomp, 42425);
}

void MavlinkSender::cancel_mag_cal(uint8_t tsys, uint8_t tcomp)
{
    // MAV_CMD_DO_CANCEL_MAG_CAL (42426), param1 = 0 for every compass.
    enqueue_command_long(tsys, tcomp, 42426);
}

void MavlinkSender::calibrate_magnetometer(uint8_t tsys, uint8_t tcomp)
{
    // param2 = 1, and every other parameter zero. The PX4 path.
    preflight_calibration(tsys, tcomp, 0.f, 1.f);
}

void MavlinkSender::cancel_calibration(uint8_t tsys, uint8_t tcomp)
{
    preflight_calibration(tsys, tcomp);   // every parameter zero
}

void MavlinkSender::send_accelcal_vehicle_pos(uint8_t tsys, uint8_t tcomp,
                                              uint32_t position)
{
    // MAV_CMD_ACCELCAL_VEHICLE_POS (42429), position in param1.
    //
    // Sent as a plain COMMAND_LONG through the same queue as everything else,
    // so it inherits the ACK plumbing: the vehicle answers each one, and a
    // position it did not accept shows up as a rejection rather than as a
    // calibration that quietly stops advancing.
    enqueue_command_long(tsys, tcomp, 42429, (float)position);
}

void MavlinkSender::reboot_autopilot(uint8_t tsys, uint8_t tcomp)
{
    // MAV_CMD_PREFLIGHT_REBOOT_SHUTDOWN (246): param1 = 1 reboots the
    // autopilot. Every other parameter stays zero — param2 would reboot the
    // companion computer, and the higher ones select bootloader and shutdown
    // modes that this GCS has no business asking for.
    enqueue_command_long(tsys, tcomp, 246, 1.f);
    reboot_requests_.fetch_add(1, std::memory_order_relaxed);
}

void MavlinkSender::request_autopilot_capabilities(uint8_t tsys, uint8_t tcomp)
{
    // MAV_CMD_REQUEST_AUTOPILOT_CAPABILITIES (520), param1=1 (request)
    enqueue_command_long(tsys, tcomp, 520, 1.f);
}

void MavlinkSender::request_banner(uint8_t tsys, uint8_t tcomp)
{
    enqueue_command_long(tsys, tcomp, MAV_CMD_DO_SEND_BANNER);
}

void MavlinkSender::request_available_modes(uint8_t tsys, uint8_t tcomp)
{
    request_available_mode(tsys, tcomp, 0);
}

void MavlinkSender::request_available_mode(uint8_t tsys, uint8_t tcomp,
                                           uint8_t mode_index)
{
    // MAV_CMD_REQUEST_MESSAGE (512): param1 = message id, param2 = mode index.
    // DEBUG: confirms the request was queued, and to whom.
    std::fprintf(stderr, "[modes] tx REQUEST_MESSAGE(512) id=%d index=%u -> %u/%u\n",
                 (int)MAVLINK_MSG_ID_AVAILABLE_MODES, mode_index, tsys, tcomp);
    enqueue_command_long(tsys, tcomp, 512,
                         (float)MAVLINK_MSG_ID_AVAILABLE_MODES, (float)mode_index);
}

void MavlinkSender::send_timesync(uint8_t tsys, uint8_t tcomp,
                                  int64_t tc1, int64_t ts1)
{
    mavlink_message_t msg;
    std::lock_guard<std::mutex> _tx(tx_mtx_);
    mavlink_msg_timesync_pack_status(gcs_sysid(), GCS_COMPID, &tx_status_, &msg,
                                   tc1, ts1, tsys, tcomp);
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    const uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
    std::lock_guard<std::mutex> lk(mtx_);
    queue_.emplace(buf, buf + len);
}

void MavlinkSender::request_message_interval(uint8_t tsys, uint8_t tcomp,
                                              uint32_t message_id, int32_t interval_us)
{
    // MAV_CMD_SET_MESSAGE_INTERVAL = 511
    // param1 = message ID, param2 = interval in microseconds
    mavlink_message_t msg;
    std::lock_guard<std::mutex> _tx(tx_mtx_);
    mavlink_msg_command_long_pack_status(
        gcs_sysid(), GCS_COMPID, &tx_status_, &msg,
        tsys, tcomp,
        511, 0,
        (float)message_id, (float)interval_us,
        0.f, 0.f, 0.f, 0.f, 0.f
    );

    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    const uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);

    std::lock_guard<std::mutex> lk(mtx_);
    queue_.emplace(buf, buf + len);
    intervals_[message_id] = interval_us;
    // No cmd_states_ entry — stream rate requests don't need UI flash feedback
}

void MavlinkSender::request_message_interval_at_least(uint8_t tsys, uint8_t tcomp,
                                                      uint32_t message_id, int32_t interval_us)
{
    {
        std::lock_guard<std::mutex> lk(mtx_);
        const auto it = intervals_.find(message_id);
        if (it != intervals_.end() && it->second > 0 && it->second <= interval_us) return;
    }
    request_message_interval(tsys, tcomp, message_id, interval_us);
}

std::unordered_map<uint32_t, int32_t> MavlinkSender::requested_intervals() const
{
    std::lock_guard<std::mutex> lk(mtx_);
    return intervals_;
}
