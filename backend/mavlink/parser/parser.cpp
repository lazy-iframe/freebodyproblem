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

#include "backend/mavlink/parser/parser.hpp"
#include "backend/vehicle/firmware_profile.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "backend/mavlink/signing.hpp"
#include "frontend/source/app_log.hpp"   // gcs_log — thread-safe, see the header

// ── MavlinkParser::parse ──────────────────────────────────────────────────────

int MavlinkParser::parse(const uint8_t* buf, size_t len)
{
    return framer_.feed(buf, len,
                        [this](const mavlink_message_t& msg) { handle(msg); });
}

// ── MavlinkParser::handle ─────────────────────────────────────────────────────

void MavlinkParser::handle(const mavlink_message_t& msg)
{
    // Wrong system entirely. A caller that demultiplexes by sysid never gets
    // here, but one feeding a shared bus straight in does, and this is what
    // keeps a second aircraft's telemetry out of this vehicle's state.
    if (bound_ && msg.sysid != sysid_) return;

    ++total_messages_;

    // A silence long enough to be something other than a slow heartbeat. A
    // vehicle with no streams configured still heartbeats at 1 Hz, so the
    // threshold sits clear of that; a reboot is silent for longer.
    {
        constexpr int64_t RESUME_GAP_NS = 1500000000;   // 1.5 s
        const int64_t now_ns = timesync_monotonic_ns();
        if (last_rx_ns_ != 0 && now_ns - last_rx_ns_ >= RESUME_GAP_NS) {
            resume_ns_   = now_ns;
            resume_mark_ = state_.statustext_total;
        }
        last_rx_ns_ = now_ns;
    }

    // Store last raw message per ID for the MAVLink inspector tab.
    state_.last_messages[msg.msgid] = msg;

    // Per-message-ID stats: the count, and the rate over a window (see
    // MessageStats). Until the first window closes, its count so far stands
    // in once it spans a quarter second, so a new message shows a rate within
    // a few arrivals — but not from a burst that landed in one batch, whose
    // span is next to nothing. A message back after a long silence starts
    // again, rather than showing the rate it had before.
    auto  now  = std::chrono::steady_clock::now();
    auto& stat = msg_stats_[msg.msgid];
    if (stat.count == 0 || now - stat.last_seen > std::chrono::seconds(10)) {
        stat.win_start = now;
        stat.win_count = 0;
        stat.rate_hz   = 0.0;
    } else {
        ++stat.win_count;
        const double el = std::chrono::duration<double>(now - stat.win_start).count();
        if (el >= MessageStats::RATE_WINDOW_S) {
            stat.rate_hz   = stat.win_count / el;
            stat.win_start = now;
            stat.win_count = 0;
        } else if (stat.rate_hz <= 0.0 && stat.win_count >= 3 && el >= 0.25) {
            stat.rate_hz = stat.win_count / el;
        }
    }
    stat.last_seen = now;
    ++stat.count;

    // Within the right system, most of what follows describes the vehicle's own
    // flight state and is only meaningful from the autopilot. A gimbal, camera
    // or companion computer on the same sysid has its own MAV_COMP_ID and its
    // own opinion of ATTITUDE; letting it write here is how a gimbal's pointing
    // angle ends up displayed as the aircraft's.
    //
    // Deliberately not gated below: STATUSTEXT and COMMAND_ACK are worth having
    // from any component, COMPONENT_INFORMATION is *about* another component by
    // definition, and TIMESYNC is answered for whoever asked. HEARTBEAT is
    // excluded because it is what does the binding in the first place.
    if (bound_ && msg.compid != autopilot_compid_) {
        switch (msg.msgid) {
        case MAVLINK_MSG_ID_SYS_STATUS:
        case MAVLINK_MSG_ID_GPS_RAW_INT:
        case MAVLINK_MSG_ID_ATTITUDE:
        case MAVLINK_MSG_ID_GLOBAL_POSITION_INT:
        case MAVLINK_MSG_ID_HOME_POSITION:
        case MAVLINK_MSG_ID_VFR_HUD:
        case MAVLINK_MSG_ID_EKF_STATUS_REPORT:
        case MAVLINK_MSG_ID_ESTIMATOR_STATUS:
        case MAVLINK_MSG_ID_RC_CHANNELS:
        case MAVLINK_MSG_ID_RC_CHANNELS_RAW:
        case MAVLINK_MSG_ID_PARAM_VALUE:
        case MAVLINK_MSG_ID_PARAM_EXT_ACK:
        case MAVLINK_MSG_ID_AVAILABLE_MODES:
        case MAVLINK_MSG_ID_AVAILABLE_MODES_MONITOR:
        case MAVLINK_MSG_ID_AUTOPILOT_VERSION:
        case MAVLINK_MSG_ID_MISSION_COUNT:
        case MAVLINK_MSG_ID_MISSION_ITEM_INT:
        case MAVLINK_MSG_ID_MISSION_REQUEST_INT:
        case MAVLINK_MSG_ID_MISSION_REQUEST:
        case MAVLINK_MSG_ID_MISSION_ACK:
        case MAVLINK_MSG_ID_COMMAND_LONG:
        case MAVLINK_MSG_ID_MAG_CAL_PROGRESS:
        case MAVLINK_MSG_ID_MAG_CAL_REPORT:
            return;
        default:
            break;
        }
    }

    // Decode and store fields for known message types
    switch (msg.msgid) {
    case MAVLINK_MSG_ID_HEARTBEAT: on_heartbeat(msg); break;
    case MAVLINK_MSG_ID_SYS_STATUS: on_sys_status(msg); break;
    case MAVLINK_MSG_ID_GPS_RAW_INT: on_gps_raw_int(msg); break;
    case MAVLINK_MSG_ID_TIMESYNC: on_timesync(msg); break;
    case MAVLINK_MSG_ID_ATTITUDE: on_attitude(msg); break;
    case MAVLINK_MSG_ID_HOME_POSITION: on_home_position(msg); break;
    case MAVLINK_MSG_ID_GLOBAL_POSITION_INT: on_global_position_int(msg); break;
    case MAVLINK_MSG_ID_VFR_HUD: on_vfr_hud(msg); break;
    case MAVLINK_MSG_ID_EKF_STATUS_REPORT: on_ekf_status_report(msg); break;
    case MAVLINK_MSG_ID_ESTIMATOR_STATUS: on_estimator_status(msg); break;
    case MAVLINK_MSG_ID_RC_CHANNELS: on_rc_channels(msg); break;
    case MAVLINK_MSG_ID_RC_CHANNELS_RAW: on_rc_channels_raw(msg); break;
    case MAVLINK_MSG_ID_PARAM_EXT_ACK: on_param_ext_ack(msg); break;
    case MAVLINK_MSG_ID_AVAILABLE_MODES: on_available_modes(msg); break;
    case MAVLINK_MSG_ID_AVAILABLE_MODES_MONITOR: on_available_modes_monitor(msg); break;
    case MAVLINK_MSG_ID_AUTOPILOT_VERSION: on_autopilot_version(msg); break;
    case MAVLINK_MSG_ID_COMPONENT_INFORMATION: on_component_information(msg); break;
    case MAVLINK_MSG_ID_PARAM_VALUE: on_param_value(msg); break;
    case MAVLINK_MSG_ID_MISSION_COUNT: on_mission_count(msg); break;
    case MAVLINK_MSG_ID_MISSION_ITEM_INT: on_mission_item_int(msg); break;
    case MAVLINK_MSG_ID_MISSION_REQUEST_INT: on_mission_request_int(msg); break;
    case MAVLINK_MSG_ID_MISSION_REQUEST: on_mission_request(msg); break;
    case MAVLINK_MSG_ID_MISSION_ACK: on_mission_ack(msg); break;
    case MAVLINK_MSG_ID_COMMAND_LONG: on_command_long(msg); break;
    case MAVLINK_MSG_ID_COMMAND_ACK: on_command_ack(msg); break;
    case MAVLINK_MSG_ID_MAG_CAL_PROGRESS: on_mag_cal_progress(msg); break;
    case MAVLINK_MSG_ID_MAG_CAL_REPORT: on_mag_cal_report(msg); break;
    case MAVLINK_MSG_ID_STATUSTEXT: on_statustext(msg); break;
    default:
        break;
    }
}

// ── Discovery ─────────────────────────────────────────────────────────────────

bool MavlinkParser::announces_vehicle(const mavlink_message_t& msg)
{
    if (msg.msgid != MAVLINK_MSG_ID_HEARTBEAT) return false;

    mavlink_heartbeat_t hb;
    mavlink_msg_heartbeat_decode(&msg, &hb);

    // Gimbals, cameras and companion computers heartbeat too, with
    // MAV_AUTOPILOT_INVALID. So does another GCS on the same network, and so do
    // our own datagrams coming back on a loopback UDP bind. None of them is a
    // vehicle, and creating one for them would put a phantom in the switcher.
    //
    // By what it says it is, and never by comparing its sysid with this GCS's own: that
    // is a setting, and a vehicle on the same number must not vanish.
    if (hb.autopilot == MAV_AUTOPILOT_INVALID) return false;
    if (hb.type      == MAV_TYPE_GCS)          return false;
    return true;
}

// ── Per-message handlers ────────────────────────────────────────────────────────

void MavlinkParser::on_heartbeat(const mavlink_message_t& msg)
{
    mavlink_heartbeat_t hb;
    mavlink_msg_heartbeat_decode(&msg, &hb);

    // Every component, before the filter below drops all but the autopilot.
    // A heartbeat is the only thing a component announces itself with, so
    // this is the whole of the discovery step.
    {
        int slot = -1;
        for (int i = 0; i < state_.component_count; ++i)
            if (state_.components[i].sysid  == msg.sysid &&
                state_.components[i].compid == msg.compid) { slot = i; break; }

        if (slot < 0 && state_.component_count < VehicleState::MAX_COMPONENTS)
            slot = state_.component_count++;

        // Past MAX_COMPONENTS the newcomer is dropped rather than evicting
        // one: the table is a list of what is present, and a bus with
        // seventeen components on it has bigger problems than this panel.
        if (slot >= 0) {
            auto& c = state_.components[slot];
            c.sysid     = msg.sysid;
            c.compid    = msg.compid;
            c.type      = hb.type;
            c.autopilot = hb.autopilot;
            ++c.heartbeats;
        }
    }

    // Ignore GCS-type and non-autopilot heartbeats (gimbals, cameras,
    // companion computers). They use MAV_AUTOPILOT_INVALID and their
    // custom_mode field is meaningless for flight-mode display.
    if (hb.autopilot == MAV_AUTOPILOT_INVALID) return;

    // Bind to the first real autopilot heard from, when the identity was
    // not handed to us. Afterwards a heartbeat from any other component on
    // this system updates the roster above but not the state below — it is
    // not this vehicle's autopilot talking.
    if (!bound_) {
        sysid_            = msg.sysid;
        autopilot_compid_ = msg.compid;
        bound_            = true;
    } else if (msg.sysid != sysid_ || msg.compid != autopilot_compid_) {
        return;
    }

    state_.sysid         = msg.sysid;
    state_.compid        = msg.compid;
    state_.type          = static_cast<MAV_TYPE>(hb.type);
    state_.autopilot     = static_cast<MAV_AUTOPILOT>(hb.autopilot);
    state_.mav_state     = static_cast<MAV_STATE>(hb.system_status);
    state_.custom_mode   = hb.custom_mode;
    state_.armed         = (hb.base_mode & MAV_MODE_FLAG_SAFETY_ARMED) != 0;
    {
        // Said once a change, for the log. Going from signed to not is the one to
        // look at: a vehicle told to stop, or something else using its sysid.
        const bool is_signed = (msg.incompat_flags & MAVLINK_IFLAG_SIGNED) != 0;
        if (state_.has_heartbeat && is_signed != state_.heartbeat_signed)
            gcs_log(is_signed ? "sys%u: heartbeats now signed"
                              : "sys%u: heartbeats NO LONGER SIGNED", (unsigned)msg.sysid);
        state_.heartbeat_signed      = is_signed;
        state_.heartbeat_signing_gen = signing::generation();
    }
    state_.has_heartbeat = true;
    ++state_.heartbeat_count;
}

void MavlinkParser::on_sys_status(const mavlink_message_t& msg)
{
    mavlink_sys_status_t ss;
    mavlink_msg_sys_status_decode(&msg, &ss);
    state_.voltage_V         = ss.voltage_battery / 1000.0f;
    state_.current_A         = ss.current_battery / 100.0f;
    state_.battery_remaining = static_cast<int8_t>(ss.battery_remaining);
    state_.load_pct          = ss.load / 10;
    state_.has_sys_status    = true;
}

void MavlinkParser::on_timesync(const mavlink_message_t& msg)
{
    mavlink_timesync_t ts;
    mavlink_msg_timesync_decode(&msg, &ts);
    TimeSync::Reply reply;
    const int64_t now_ns     = timesync_monotonic_ns();
    const int64_t old_offset = timesync_.offset_ns();
    if (timesync_.on_timesync(ts.tc1, ts.ts1, msg.sysid, msg.compid,
                              now_ns, &reply))
        timesync_replies_.push_back(reply);
    state_.time_offset_ns = timesync_.offset_ns();
    state_.has_timesync   = timesync_.synced();

    // The reboot check that does not depend on any stream being
    // configured — a rebooted vehicle has forgotten the message intervals
    // asked of it, but it still answers TIMESYNC. Only a freshly completed
    // round trip is a reading; between them the offset is the old boot's.
    if (timesync_.synced() && timesync_.offset_ns() != old_offset) {
        const int64_t boot_ns = now_ns + timesync_.offset_ns();
        if (boot_ns > 0) note_boot_ms((uint32_t)(boot_ns / 1000000), now_ns);
    }
}

void MavlinkParser::on_statustext(const mavlink_message_t& msg)
{
    mavlink_statustext_t st;
    mavlink_msg_statustext_decode(&msg, &st);
    StatusText entry{};
    entry.severity = st.severity;
    std::memcpy(entry.text, st.text, 50);
    entry.text[50] = '\0';
    if (status_texts_.size() >= MAX_STATUS_TEXTS)
        status_texts_.pop_front();
    status_texts_.push_back(entry);
    ++state_.statustext_total;
    // ArduPilot's banner names the firmware, which the HEARTBEAT type cannot
    // always do; see vehicle/kind.hpp.
    const VehicleKind banner = vehicle_kind_from_banner(entry.text);
    if (banner != VehicleKind::Unknown) state_.banner_kind = banner;
    printf("\033[33m[STATUS] %s\033[0m\n", entry.text);
    fflush(stdout);
}

// ── Reboot detection ──────────────────────────────────────────────────────────

void MavlinkParser::note_boot_ms(uint32_t boot_ms, int64_t now_ns)
{
    // Clear of anything the transport can reorder or the TIMESYNC estimate can
    // be off by, and far short of any real uptime a reboot takes away — an
    // autopilot is not back up and streaming in under three seconds.
    constexpr uint32_t REBOOT_BACKSTEP_MS = 3000;

    if (have_boot_ms_ && boot_ms + REBOOT_BACKSTEP_MS < last_boot_ms_) {
        // Where the old boot's STATUSTEXTs end. The last resume is the right
        // answer only if it belongs to this reboot: the TIMESYNC path can take
        // a few seconds after the stream resumes to notice, but not half a
        // minute. Failing that, everything already received is the old boot's.
        constexpr int64_t RESUME_WINDOW_NS = 30000000000;   // 30 s
        const bool resume_is_this_one = resume_ns_ != 0 &&
                                        now_ns - resume_ns_ <= RESUME_WINDOW_NS;

        ++state_.reboot_count;
        state_.reboot_statustext_mark = resume_is_this_one ? resume_mark_
                                                           : state_.statustext_total;
        gcs_log("[sys%u] vehicle rebooted (boot clock %.1f s \xe2\x86\x92 %.1f s)",
                (unsigned)sysid_, last_boot_ms_ / 1000.0, boot_ms / 1000.0);
    }
    last_boot_ms_ = boot_ms;
    have_boot_ms_ = true;
}

// ── MavlinkParser::print_stats ────────────────────────────────────────────────

void MavlinkParser::print_stats()
{
    // Clear screen and move cursor to top-left
    printf("\033[2J\033[H");

    printf("=== MAVLink Stats  (msgs: %llu  bytes: %llu  errors: %llu) ===\n",
           (unsigned long long)total_messages_,
           (unsigned long long)framer_.total_bytes(),
           (unsigned long long)framer_.parse_errors());

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
