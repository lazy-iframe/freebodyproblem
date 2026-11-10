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

// MavlinkParser: flight telemetry — position, attitude, estimator health, RC input.
// Every MAVLink message received is decoded by MavlinkParser; handle() in
// parser.cpp dispatches here.

#include "backend/mavlink/parser/parser.hpp"
#include "backend/vehicle/firmware_profile.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "backend/mavlink/signing.hpp"
#include "frontend/source/app_log.hpp"   // gcs_log — thread-safe, see the header

void MavlinkParser::on_gps_raw_int(const mavlink_message_t& msg)
{
    mavlink_gps_raw_int_t gps;
    mavlink_msg_gps_raw_int_decode(&msg, &gps);
    state_.gps_fix_type   = gps.fix_type;
    state_.satellites_vis = gps.satellites_visible;
    state_.hdop           = (gps.eph == UINT16_MAX) ? 99.99f : gps.eph * 0.01f;
    state_.vdop           = (gps.epv == UINT16_MAX) ? 99.99f : gps.epv * 0.01f;
    state_.has_gps_raw    = true;
}

void MavlinkParser::on_attitude(const mavlink_message_t& msg)
{
    mavlink_attitude_t att;
    mavlink_msg_attitude_decode(&msg, &att);
    constexpr float RAD2DEG = 180.0f / static_cast<float>(M_PI);
    state_.roll        = att.roll  * RAD2DEG;
    state_.pitch       = att.pitch * RAD2DEG;
    state_.yaw         = att.yaw   * RAD2DEG;
    state_.has_attitude = true;
    note_boot_ms(att.time_boot_ms, timesync_monotonic_ns());
}

void MavlinkParser::on_home_position(const mavlink_message_t& msg)
{
    mavlink_home_position_t hp;
    mavlink_msg_home_position_decode(&msg, &hp);
    state_.home_lat     = hp.latitude  / 1e7;
    state_.home_lon     = hp.longitude / 1e7;
    state_.home_alt_asl = hp.altitude  / 1000.0f;
    state_.has_home     = true;
}

void MavlinkParser::on_global_position_int(const mavlink_message_t& msg)
{
    mavlink_global_position_int_t gp;
    mavlink_msg_global_position_int_decode(&msg, &gp);
    state_.lat           = gp.lat / 1e7;
    state_.lon           = gp.lon / 1e7;
    state_.alt_asl       = gp.alt          / 1000.0f;
    state_.alt_rel       = gp.relative_alt  / 1000.0f;
    state_.has_global_pos = true;
    note_boot_ms(gp.time_boot_ms, timesync_monotonic_ns());
}

void MavlinkParser::on_vfr_hud(const mavlink_message_t& msg)
{
    mavlink_vfr_hud_t vfr;
    mavlink_msg_vfr_hud_decode(&msg, &vfr);
    state_.airspeed    = vfr.airspeed;
    state_.groundspeed = vfr.groundspeed;
    state_.heading     = vfr.heading;
    state_.throttle    = vfr.throttle;
    state_.climb       = vfr.climb;
    state_.has_vfr     = true;
}

void MavlinkParser::on_ekf_status_report(const mavlink_message_t& msg)
{
    mavlink_ekf_status_report_t ekf;
    mavlink_msg_ekf_status_report_decode(&msg, &ekf);
    state_.ekf_velocity_variance    = ekf.velocity_variance;
    state_.ekf_pos_horiz_variance   = ekf.pos_horiz_variance;
    state_.ekf_pos_vert_variance    = ekf.pos_vert_variance;
    state_.ekf_compass_variance     = ekf.compass_variance;
    state_.ekf_terrain_alt_variance = ekf.terrain_alt_variance;
    state_.ekf_airspeed_variance    = ekf.airspeed_variance;
    state_.ekf_flags                = ekf.flags;
    state_.ekf_source_msgid         = MAVLINK_MSG_ID_EKF_STATUS_REPORT;
    state_.has_ekf_status           = true;
}

// The portable twin of the above, and what PX4 sends. Its ratios go into
// the same fields so nothing downstream has to know which arrived.
//
// #193 wins if a vehicle somehow sends both: it exists only on ArduPilot,
// so hearing it at all settles which stack is talking, and letting the two
// alternate would swing the bars between two different scales. No firmware
// profile needed for that — the message's own existence is the evidence.
void MavlinkParser::on_estimator_status(const mavlink_message_t& msg)
{
    if (state_.ekf_source_msgid == MAVLINK_MSG_ID_EKF_STATUS_REPORT) return;

    mavlink_estimator_status_t est;
    mavlink_msg_estimator_status_decode(&msg, &est);
    state_.ekf_velocity_variance    = est.vel_ratio;
    state_.ekf_pos_horiz_variance   = est.pos_horiz_ratio;
    state_.ekf_pos_vert_variance    = est.pos_vert_ratio;
    state_.ekf_compass_variance     = est.mag_ratio;
    // Kept as sent, NaN included. PX4 reports NaN — not zero — for an
    // estimate it is not running: on a stock x500 hagl_ratio and tas_ratio
    // both arrive NaN because there is no rangefinder and no airspeed
    // sensor. That is worth preserving rather than flattening, because it
    // is the one thing that tells "not estimated" apart from "perfect",
    // which a zero cannot. The bars check for it.
    state_.ekf_terrain_alt_variance = est.hagl_ratio;
    state_.ekf_airspeed_variance    = est.tas_ratio;
    state_.ekf_flags                = est.flags;   // ESTIMATOR_STATUS_FLAGS here
    state_.ekf_source_msgid         = MAVLINK_MSG_ID_ESTIMATOR_STATUS;
    state_.has_ekf_status           = true;
}

void MavlinkParser::on_rc_channels(const mavlink_message_t& msg)
{
    // The gate at the top of handle() covers this now. It used to be one of
    // only three places that checked, and the reason it was worth checking
    // here in particular still holds: RC is the one stream a second node on
    // a shared bus plausibly also publishes — a companion computer
    // forwarding its own receiver, a second autopilot on a serial splitter
    // — and a calibration sweep that quietly mixed two receivers together
    // would write the union of both as one channel's range.
    mavlink_rc_channels_t rc;
    mavlink_msg_rc_channels_decode(&msg, &rc);

    const uint16_t src[RcChannels::MAX_CHANNELS] = {
        rc.chan1_raw,  rc.chan2_raw,  rc.chan3_raw,  rc.chan4_raw,
        rc.chan5_raw,  rc.chan6_raw,  rc.chan7_raw,  rc.chan8_raw,
        rc.chan9_raw,  rc.chan10_raw, rc.chan11_raw, rc.chan12_raw,
        rc.chan13_raw, rc.chan14_raw, rc.chan15_raw, rc.chan16_raw,
        rc.chan17_raw, rc.chan18_raw,
    };
    std::memcpy(state_.rc.chan, src, sizeof(src));

    state_.rc.count    = rc.chancount;
    state_.rc.rssi     = rc.rssi;
    state_.rc.from_raw = false;
    state_.rc.valid    = true;
    ++state_.rc.generation;
}

void MavlinkParser::on_rc_channels_raw(const mavlink_message_t& msg)
{
    // Strictly the fallback. A vehicle sending both would otherwise have its
    // 18-channel picture overwritten eight channels at a time, and the
    // upper channels — where every aux switch lives — would flicker between
    // their real value and zero at whatever rate the two streams interleave.
    if (state_.rc.valid && !state_.rc.from_raw) return;

    mavlink_rc_channels_raw_t rc;
    mavlink_msg_rc_channels_raw_decode(&msg, &rc);

    // RC_CHANNELS_RAW is per-port: port 0 is channels 1-8, port 1 is 9-16.
    // Most receivers only ever fill port 0, but honouring the field costs
    // nothing and is what makes the second bank land in the right slots.
    const int base = (int)rc.port * 8;
    if (base < 0 || base >= RcChannels::MAX_CHANNELS) return;

    const uint16_t src[8] = {
        rc.chan1_raw, rc.chan2_raw, rc.chan3_raw, rc.chan4_raw,
        rc.chan5_raw, rc.chan6_raw, rc.chan7_raw, rc.chan8_raw,
    };
    const int n = std::min(8, RcChannels::MAX_CHANNELS - base);
    for (int i = 0; i < n; ++i)
        state_.rc.chan[base + i] = src[i];

    // No chancount in this message. The highest port seen carrying a live
    // pulse is the best available stand-in, and never shrinks within a link
    // — a channel that goes to zero is a lost channel, not a smaller radio.
    const uint8_t reach = static_cast<uint8_t>(base + n);
    if (reach > state_.rc.count) state_.rc.count = reach;

    state_.rc.rssi     = rc.rssi;
    state_.rc.from_raw = true;
    state_.rc.valid    = true;
    ++state_.rc.generation;
}
