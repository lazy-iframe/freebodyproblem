// Free Body Problem: A modern, simple and minimalist Ground Control Station
// Copyright (C) 2026  Azhar Tanweer

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.


#pragma once

#include <chrono>
#include <cmath>
#include <cstdint>
#include <deque>
#include <string>
#include <unordered_map>
#include <vector>

#include <mavlink/ardupilotmega/mavlink.h>

struct MissionItem {
    uint16_t seq;
    double   lat, lon;        // degrees
    float    alt;             // metres (MISSION_ITEM_INT.z)
    float    param1 = 0.f;
    float    param2 = 0.f;
    float    param3 = 0.f;
    float    param4 = 0.f;
    uint16_t command;         // MAV_CMD
    uint8_t  frame = 3;       // MAV_FRAME (3 = GLOBAL_RELATIVE_ALT)
    bool     is_current = false;
    bool     autocontinue = true;
};

struct ParamEntry {
    char     param_id[17];  // null-terminated, MAVLink param_id is 16 chars
    float    value;
    uint8_t  type;          // MAV_PARAM_TYPE
    uint16_t index;
};

struct StatusText {
    uint8_t severity;    // MAV_SEVERITY value
    char    text[51];    // null-terminated, max 50 chars
};

struct CommandAck {
    uint16_t command;
    uint8_t  result;   // MAV_RESULT: 0=ACCEPTED, else failure
};

struct MessageStats {
    uint64_t count   = 0;
    double   rate_hz = 0.0;   // exponential moving average
    std::chrono::steady_clock::time_point last_seen{};

    // Returns rate decayed by elapsed silence since the last message.
    // Prevents the displayed rate from staying non-zero when messages stop.
    double effective_rate_hz() const {
        if (count == 0 || rate_hz <= 0.0) return 0.0;
        auto   now = std::chrono::steady_clock::now();
        double dt  = std::chrono::duration<double>(now - last_seen).count();
        if (dt <= 0.0) return rate_hz;
        // Each "missed" expected interval decays the EMA by factor (1 - alpha).
        // alpha = 0.2  →  decay = 0.8 ^ (dt * rate_hz)
        double n_missed = dt * rate_hz;
        return rate_hz * std::pow(0.8, n_missed);
    }
};

struct VehicleState {
    // HEARTBEAT
    uint8_t       sysid         = 0;
    uint8_t       compid        = 0;
    MAV_TYPE      type          = MAV_TYPE_GENERIC;
    MAV_AUTOPILOT autopilot     = MAV_AUTOPILOT_GENERIC;
    MAV_STATE     mav_state     = MAV_STATE_UNINIT;
    uint32_t      custom_mode   = 0;
    bool          armed         = false;
    bool          has_heartbeat = false;

    // ATTITUDE (#30) – radians converted to degrees
    float roll = 0, pitch = 0, yaw = 0;
    bool  has_attitude = false;

    // GLOBAL_POSITION_INT (#33)
    double lat = 0, lon = 0;
    float  alt_asl = 0, alt_rel = 0;
    bool   has_global_pos = false;

    // GPS_RAW_INT (#24)
    uint8_t gps_fix_type   = 0;
    uint8_t satellites_vis = 0;
    float   hdop           = 99.99f; // horizontal dilution of precision (eph/100)
    float   vdop           = 99.99f; // vertical dilution of precision   (epv/100)
    bool    has_gps_raw    = false;

    // VFR_HUD (#74)
    float    airspeed = 0, groundspeed = 0, climb = 0;
    int16_t  heading  = 0;
    uint16_t throttle = 0;
    bool     has_vfr  = false;

    // SYS_STATUS (#1)
    float   voltage_V         = 0;
    float   current_A         = 0;
    int8_t  battery_remaining = -1;
    uint16_t load_pct         = 0;
    bool    has_sys_status    = false;

    // EKF_STATUS_REPORT (#193)
    float    ekf_velocity_variance    = 0;
    float    ekf_pos_horiz_variance   = 0;
    float    ekf_pos_vert_variance    = 0;
    float    ekf_compass_variance     = 0;
    float    ekf_terrain_alt_variance = 0;
    float    ekf_airspeed_variance    = 0;
    uint16_t ekf_flags                = 0;
    bool     has_ekf_status           = false;

    // AUTOPILOT_VERSION (#148)
    char     fw_version[32] = {};   // e.g. "4.3.7"
    char     fw_hash[17]    = {};   // first 8 bytes of flight_custom_version as hex
    bool     has_fw_info    = false;

    // PARAM_VALUE (#22) — progress counters; map lives in MavlinkParser::params_
    uint16_t param_count      = 0;  // total parameter count reported by FC
    uint16_t params_received  = 0;  // number of PARAM_VALUE messages received so far
    uint32_t params_generation = 0; // incremented on every PARAM_VALUE (incl. write echoes)

    // Mission download (MISSION_COUNT / MISSION_ITEM_INT)
    std::vector<MissionItem> mission;
    uint16_t mission_count    = 0;
    uint16_t mission_received = 0;
    bool     has_mission      = false;

    // Last raw MAVLink message received per message ID.
    // Used only by the MAVLink inspector tab for display; never read by app logic.
    // Decoded on demand via mavlink_display_draw() so there is no per-message
    // parsing overhead unless the inspector is open and a message is selected.
    std::unordered_map<uint32_t, mavlink_message_t> last_messages;

    // Mission upload status (set when FC sends MISSION_ACK after upload)
    enum class UploadStatus { Idle, InProgress, Accepted, Failed };
    UploadStatus upload_status     = UploadStatus::Idle;
    uint8_t      upload_ack_result = 255; // MAV_MISSION_RESULT
};

class MavlinkParser {
public:
    // Feed a raw UDP datagram. Returns number of complete messages parsed.
    int parse(const uint8_t* buf, size_t len);

    const VehicleState& state() const { return state_; }
    const std::unordered_map<std::string, ParamEntry>& params() const { return params_; }
    const std::unordered_map<uint32_t, MessageStats>& msg_stats() const { return msg_stats_; }

    uint64_t total_messages() const { return total_messages_; }
    uint64_t total_bytes()    const { return total_bytes_;    }
    uint64_t parse_errors()   const { return parse_errors_;  }

    // ACK queue — drained each frame by main.cpp, forwarded to MavlinkSender
    const std::vector<CommandAck>& pending_acks() const { return ack_queue_; }
    void clear_acks() { ack_queue_.clear(); }

    // Mission download request queue — drained by link thread after each parse()
    struct MissionReq { uint8_t tsys; uint8_t tcomp; uint16_t seq; };
    const std::vector<MissionReq>& pending_mission_reqs() const { return mission_reqs_; }
    void clear_mission_reqs() { mission_reqs_.clear(); }

    // Mission upload item request queue — FC sends MISSION_REQUEST_INT during upload
    // Link thread drains this and calls MavlinkSender::send_mission_item_at()
    struct ItemReq { uint8_t tsys; uint8_t tcomp; uint16_t seq; };
    const std::vector<ItemReq>& pending_item_reqs() const { return item_reqs_; }
    void clear_item_reqs() { item_reqs_.clear(); }

    // Status text log — rolling window of the last MAX_STATUS_TEXTS messages
    const std::deque<StatusText>& status_texts() const { return status_texts_; }

    void print_stats();

private:
    void handle_message(const mavlink_message_t& msg);

    mavlink_status_t                           ch_status_{};
    std::unordered_map<uint32_t, MessageStats> msg_stats_;
    VehicleState                               state_;
    std::unordered_map<std::string, ParamEntry> params_;

    // Source lock — set on the first heartbeat from a real autopilot.
    // All subsequent state updates are only accepted from this sysid/compid.
    // Prevents stray heartbeats from gimbals, companion computers, etc.
    // (common on USB serial) from corrupting custom_mode and armed state.
    uint8_t locked_sysid_  = 0;
    uint8_t locked_compid_ = 0;
    std::vector<CommandAck>                    ack_queue_;
    std::vector<MissionReq>                    mission_reqs_;
    std::vector<ItemReq>                       item_reqs_;
    std::deque<StatusText>                     status_texts_;
    static constexpr size_t MAX_STATUS_TEXTS = 200;

    uint64_t total_messages_ = 0;
    uint64_t total_bytes_    = 0;
    uint64_t parse_errors_   = 0;
};
