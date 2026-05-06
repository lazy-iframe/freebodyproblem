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


#include "mavlink_parser.hpp"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>

// ── MavlinkParser::parse ──────────────────────────────────────────────────────

int MavlinkParser::parse(const uint8_t* buf, size_t len)
{
    total_bytes_ += len;

    int parsed = 0;
    mavlink_message_t msg;

    for (size_t i = 0; i < len; ++i) {
        uint8_t result = mavlink_parse_char(MAVLINK_COMM_0, buf[i], &msg, &ch_status_);
        if (result == MAVLINK_FRAMING_OK) {
            handle_message(msg);
            ++parsed;
        } else if (result == MAVLINK_FRAMING_BAD_CRC ||
                   result == MAVLINK_FRAMING_BAD_SIGNATURE) {
            ++parse_errors_;
        }
    }

    return parsed;
}

// ── MavlinkParser::handle_message ─────────────────────────────────────────────

void MavlinkParser::handle_message(const mavlink_message_t& msg)
{
    ++total_messages_;

    // Store last raw message per ID for the MAVLink inspector tab.
    state_.last_messages[msg.msgid] = msg;

    // Update per-message-ID stats with an EMA rate estimate
    auto  now  = std::chrono::steady_clock::now();
    auto& stat = msg_stats_[msg.msgid];
    if (stat.count > 0) {
        double dt = std::chrono::duration<double>(now - stat.last_seen).count();
        if (dt > 0.0) {
            // Cap instantaneous rate to 500 Hz to prevent burst-arrival spikes
            // from inflating the EMA when buffered messages are processed at once.
            constexpr double kMaxInstHz = 500.0;
            double inst_hz = std::min(1.0 / dt, kMaxInstHz);
            stat.rate_hz = 0.2 * inst_hz + 0.8 * stat.rate_hz;
        }
    }
    stat.last_seen = now;
    ++stat.count;

    // Decode and store fields for known message types
    switch (msg.msgid) {

    case MAVLINK_MSG_ID_HEARTBEAT: {
        mavlink_heartbeat_t hb;
        mavlink_msg_heartbeat_decode(&msg, &hb);

        // Ignore GCS-type and non-autopilot heartbeats (gimbals, cameras,
        // companion computers). They use MAV_AUTOPILOT_INVALID and their
        // custom_mode field is meaningless for flight-mode display.
        if (hb.autopilot == MAV_AUTOPILOT_INVALID) break;

        // Lock onto the first real autopilot we hear from and ignore all
        // others. On a shared serial bus multiple sysids are possible.
        if (locked_sysid_ == 0) {
            locked_sysid_  = msg.sysid;
            locked_compid_ = msg.compid;
        } else if (msg.sysid != locked_sysid_ || msg.compid != locked_compid_) {
            break;
        }

        state_.sysid         = msg.sysid;
        state_.compid        = msg.compid;
        state_.type          = static_cast<MAV_TYPE>(hb.type);
        state_.autopilot     = static_cast<MAV_AUTOPILOT>(hb.autopilot);
        state_.mav_state     = static_cast<MAV_STATE>(hb.system_status);
        state_.custom_mode   = hb.custom_mode;
        state_.armed         = (hb.base_mode & MAV_MODE_FLAG_SAFETY_ARMED) != 0;
        state_.has_heartbeat = true;
        break;
    }

    case MAVLINK_MSG_ID_SYS_STATUS: {
        mavlink_sys_status_t ss;
        mavlink_msg_sys_status_decode(&msg, &ss);
        state_.voltage_V         = ss.voltage_battery / 1000.0f;
        state_.current_A         = ss.current_battery / 100.0f;
        state_.battery_remaining = static_cast<int8_t>(ss.battery_remaining);
        state_.load_pct          = ss.load / 10;
        state_.has_sys_status    = true;
        break;
    }

    case MAVLINK_MSG_ID_GPS_RAW_INT: {
        mavlink_gps_raw_int_t gps;
        mavlink_msg_gps_raw_int_decode(&msg, &gps);
        state_.gps_fix_type   = gps.fix_type;
        state_.satellites_vis = gps.satellites_visible;
        state_.hdop           = (gps.eph == UINT16_MAX) ? 99.99f : gps.eph * 0.01f;
        state_.vdop           = (gps.epv == UINT16_MAX) ? 99.99f : gps.epv * 0.01f;
        state_.has_gps_raw    = true;
        break;
    }

    case MAVLINK_MSG_ID_ATTITUDE: {
        mavlink_attitude_t att;
        mavlink_msg_attitude_decode(&msg, &att);
        constexpr float RAD2DEG = 180.0f / static_cast<float>(M_PI);
        state_.roll        = att.roll  * RAD2DEG;
        state_.pitch       = att.pitch * RAD2DEG;
        state_.yaw         = att.yaw   * RAD2DEG;
        state_.has_attitude = true;
        break;
    }

    case MAVLINK_MSG_ID_GLOBAL_POSITION_INT: {
        mavlink_global_position_int_t gp;
        mavlink_msg_global_position_int_decode(&msg, &gp);
        state_.lat           = gp.lat / 1e7;
        state_.lon           = gp.lon / 1e7;
        state_.alt_asl       = gp.alt          / 1000.0f;
        state_.alt_rel       = gp.relative_alt  / 1000.0f;
        state_.has_global_pos = true;
        break;
    }

    case MAVLINK_MSG_ID_VFR_HUD: {
        mavlink_vfr_hud_t vfr;
        mavlink_msg_vfr_hud_decode(&msg, &vfr);
        state_.airspeed    = vfr.airspeed;
        state_.groundspeed = vfr.groundspeed;
        state_.heading     = vfr.heading;
        state_.throttle    = vfr.throttle;
        state_.climb       = vfr.climb;
        state_.has_vfr     = true;
        break;
    }

    case MAVLINK_MSG_ID_EKF_STATUS_REPORT: {
        mavlink_ekf_status_report_t ekf;
        mavlink_msg_ekf_status_report_decode(&msg, &ekf);
        state_.ekf_velocity_variance    = ekf.velocity_variance;
        state_.ekf_pos_horiz_variance   = ekf.pos_horiz_variance;
        state_.ekf_pos_vert_variance    = ekf.pos_vert_variance;
        state_.ekf_compass_variance     = ekf.compass_variance;
        state_.ekf_terrain_alt_variance = ekf.terrain_alt_variance;
        state_.ekf_airspeed_variance    = ekf.airspeed_variance;
        state_.ekf_flags                = ekf.flags;
        state_.has_ekf_status           = true;
        break;
    }

    case MAVLINK_MSG_ID_AUTOPILOT_VERSION: {
        mavlink_autopilot_version_t av;
        mavlink_msg_autopilot_version_decode(&msg, &av);
        // Version packed as major(bits 31:24) minor(23:16) patch(15:8) type(7:0)
        const uint8_t major = (av.flight_sw_version >> 24) & 0xFF;
        const uint8_t minor = (av.flight_sw_version >> 16) & 0xFF;
        const uint8_t patch = (av.flight_sw_version >>  8) & 0xFF;
        snprintf(state_.fw_version, sizeof(state_.fw_version),
                 "%u.%u.%u", major, minor, patch);
        // flight_custom_version: 8 bytes → 16 hex chars
        snprintf(state_.fw_hash, sizeof(state_.fw_hash),
                 "%02x%02x%02x%02x%02x%02x%02x%02x",
                 av.flight_custom_version[0], av.flight_custom_version[1],
                 av.flight_custom_version[2], av.flight_custom_version[3],
                 av.flight_custom_version[4], av.flight_custom_version[5],
                 av.flight_custom_version[6], av.flight_custom_version[7]);
        state_.has_fw_info = true;
        break;
    }

    case MAVLINK_MSG_ID_PARAM_VALUE: {
        mavlink_param_value_t pv;
        mavlink_msg_param_value_decode(&msg, &pv);
        ParamEntry entry{};
        // param_id is 16 chars, not necessarily null-terminated
        std::memcpy(entry.param_id, pv.param_id, 16);
        entry.param_id[16] = '\0';
        entry.value = pv.param_value;
        entry.type  = pv.param_type;
        entry.index = pv.param_index;
        params_[entry.param_id]     = entry;
        state_.param_count          = pv.param_count;
        state_.params_received      = (uint16_t)params_.size();
        ++state_.params_generation;
        break;
    }

    case MAVLINK_MSG_ID_MISSION_COUNT: {
        mavlink_mission_count_t mc;
        mavlink_msg_mission_count_decode(&msg, &mc);
        state_.mission.clear();
        state_.mission.resize(mc.count);
        state_.mission_count    = mc.count;
        state_.mission_received = 0;
        state_.has_mission      = false;
        if (mc.count > 0)
            mission_reqs_.push_back({msg.sysid, msg.compid, 0});
        break;
    }

    case MAVLINK_MSG_ID_MISSION_ITEM_INT: {
        mavlink_mission_item_int_t mi;
        mavlink_msg_mission_item_int_decode(&msg, &mi);
        if (mi.seq < state_.mission_count) {
            MissionItem& item   = state_.mission[mi.seq];
            item.seq            = mi.seq;
            item.lat            = mi.x / 1e7;
            item.lon            = mi.y / 1e7;
            item.alt            = mi.z;
            item.param1         = mi.param1;
            item.param2         = mi.param2;
            item.param3         = mi.param3;
            item.param4         = mi.param4;
            item.command        = mi.command;
            item.frame          = mi.frame;
            item.is_current     = (mi.current != 0);
            item.autocontinue   = (mi.autocontinue != 0);
            ++state_.mission_received;
            if (state_.mission_received < state_.mission_count)
                mission_reqs_.push_back({msg.sysid, msg.compid,
                                         (uint16_t)(mi.seq + 1)});
            else
                state_.has_mission = true;
        }
        break;
    }

    case MAVLINK_MSG_ID_MISSION_REQUEST_INT: {
        // FC is requesting a mission item from GCS (upload protocol, new style)
        mavlink_mission_request_int_t mr;
        mavlink_msg_mission_request_int_decode(&msg, &mr);
        if (mr.seq == 0)
            state_.upload_status = VehicleState::UploadStatus::InProgress;
        item_reqs_.push_back({msg.sysid, msg.compid, mr.seq});
        break;
    }

    case MAVLINK_MSG_ID_MISSION_REQUEST: {
        // FC is requesting a mission item from GCS (upload protocol, legacy style)
        // Some ArduPilot versions send this instead of MISSION_REQUEST_INT.
        mavlink_mission_request_t mr;
        mavlink_msg_mission_request_decode(&msg, &mr);
        if (mr.seq == 0)
            state_.upload_status = VehicleState::UploadStatus::InProgress;
        item_reqs_.push_back({msg.sysid, msg.compid, mr.seq});
        break;
    }

    case MAVLINK_MSG_ID_MISSION_ACK: {
        // FC acknowledges completed upload
        mavlink_mission_ack_t ma;
        mavlink_msg_mission_ack_decode(&msg, &ma);
        state_.upload_ack_result = ma.type;
        state_.upload_status = (ma.type == MAV_MISSION_ACCEPTED)
                             ? VehicleState::UploadStatus::Accepted
                             : VehicleState::UploadStatus::Failed;
        break;
    }

    case MAVLINK_MSG_ID_COMMAND_ACK: {
        mavlink_command_ack_t ack;
        mavlink_msg_command_ack_decode(&msg, &ack);
        ack_queue_.push_back({ack.command, ack.result});
        break;
    }

    case MAVLINK_MSG_ID_STATUSTEXT: {
        mavlink_statustext_t st;
        mavlink_msg_statustext_decode(&msg, &st);
        StatusText entry{};
        entry.severity = st.severity;
        std::memcpy(entry.text, st.text, 50);
        entry.text[50] = '\0';
        if (status_texts_.size() >= MAX_STATUS_TEXTS)
            status_texts_.pop_front();
        status_texts_.push_back(entry);
        printf("\033[33m[STATUS] %s\033[0m\n", entry.text);
        fflush(stdout);
        break;
    }

    default:
        break;
    }
}

// ── MavlinkParser::print_stats ────────────────────────────────────────────────

void MavlinkParser::print_stats()
{
    // Clear screen and move cursor to top-left
    printf("\033[2J\033[H");

    printf("=== MAVLink Stats  (msgs: %llu  bytes: %llu  errors: %llu) ===\n",
           (unsigned long long)total_messages_,
           (unsigned long long)total_bytes_,
           (unsigned long long)parse_errors_);

    const VehicleState& s = state_;

    if (s.has_heartbeat) {
        printf("\nVehicle  sys=%u comp=%u  type=%u autopilot=%u  state=%u  %s\n",
               s.sysid, s.compid, s.type, s.autopilot, s.mav_state,
               s.armed ? "ARMED" : "disarmed");
    }
    if (s.has_attitude) {
        printf("Attitude  roll=%+7.2f°  pitch=%+7.2f°  yaw=%+7.2f°\n",
               s.roll, s.pitch, s.yaw);
    }
    if (s.has_global_pos) {
        printf("Position  lat=%.6f  lon=%.6f  alt_asl=%.1f m  alt_rel=%.1f m\n",
               s.lat, s.lon, s.alt_asl, s.alt_rel);
    }
    if (s.has_gps_raw) {
        printf("GPS       fix=%u  sats=%u\n", s.gps_fix_type, s.satellites_vis);
    }
    if (s.has_vfr) {
        printf("VFR HUD   airspeed=%.1f m/s  groundspeed=%.1f m/s  "
               "heading=%d°  throttle=%u%%  climb=%.1f m/s\n",
               s.airspeed, s.groundspeed, s.heading, s.throttle, s.climb);
    }
    if (s.has_sys_status) {
        printf("Battery   %.2f V  %.1f A  %d%%  load=%u%%\n",
               s.voltage_V, s.current_A, s.battery_remaining, s.load_pct);
    }

    printf("\nMessage rates:\n");
    for (const auto& [id, stat] : msg_stats_) {
        printf("  msgid=%-5u  count=%-8llu  rate=%.1f Hz\n",
               id,
               (unsigned long long)stat.count,
               stat.effective_rate_hz());
    }

    fflush(stdout);
}
