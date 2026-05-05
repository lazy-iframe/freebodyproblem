// Free Body Problem: A modern, simple and minimalist Ground Control Station
// Copyright (C) 2026  Azhar Tanweer
// Contact: azhar.tanweer404@gmail.com

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

#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <winsock2.h>
#else
#  include <netinet/in.h>
#endif

#include <chrono>
#include <cstdint>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <vector>

#include "mavlink_parser.hpp"  // for MissionItem

// Visual feedback state for a sent command
enum class CmdFlashState { Normal, Pending, Accepted, Rejected };

// MavlinkSender — thread-safe outbound MAVLink command queue.
//
// Usage:
//   UI thread  : call arm(), disarm(), takeoff(), etc. — enqueues a serialised frame
//   Recv thread: call flush(fd, dest) after each recvfrom — drains queue via sendto
//
// GCS identity follows MAVLink convention: sysid=255, compid=190 (MissionPlanner).

class MavlinkSender {
public:
    static constexpr uint8_t GCS_SYSID  = 255;
    static constexpr uint8_t GCS_COMPID = 190;

    // ── High-level commands ───────────────────────────────────────────────────

    void arm   (uint8_t target_sysid, uint8_t target_compid);
    void disarm(uint8_t target_sysid, uint8_t target_compid, bool force = false);

    // Climb to altitude_m (metres, relative to home) after takeoff
    void takeoff(uint8_t target_sysid, uint8_t target_compid, float altitude_m);

    // ArduCopter/Plane custom_mode values (e.g. STABILIZE=0, ALT_HOLD=2, GUIDED=4)
    void set_mode(uint8_t target_sysid, uint8_t target_compid, uint32_t custom_mode);

    void return_to_launch(uint8_t target_sysid, uint8_t target_compid);

    // MAV_CMD_DO_AUX_FUNCTION (218): trigger any ArduPilot auxiliary function.
    // function: RCx_OPTION value (e.g. 32 = motor interlock)
    // switch_pos: 0 = LOW, 1 = MIDDLE, 2 = HIGH
    void do_aux_function(uint8_t target_sysid, uint8_t target_compid,
                         uint16_t function, uint8_t switch_pos);

    // MAV_CMD_DO_SET_SERVO (183): set a servo output channel directly.
    // servo_num: 1-based servo output number
    // pwm_us: pulse width in microseconds (typical range 1000–2000)
    void do_set_servo(uint8_t target_sysid, uint8_t target_compid,
                      uint8_t servo_num, uint16_t pwm_us);

    // Request a specific MAVLink message at a fixed interval.
    // Sends MAV_CMD_SET_MESSAGE_INTERVAL (511).
    // interval_us: microseconds between messages (-1 = disable, 0 = autopilot default)
    void request_message_interval(uint8_t target_sysid, uint8_t target_compid,
                                  uint32_t message_id, int32_t interval_us);

    // Request AUTOPILOT_VERSION (#148) — call once after first contact.
    // Triggers a COMMAND_ACK then an AUTOPILOT_VERSION response.
    void request_autopilot_capabilities(uint8_t target_sysid, uint8_t target_compid);

    // Request all parameters (PARAM_REQUEST_LIST #21).
    // FC responds with a stream of PARAM_VALUE (#22) messages.
    void request_param_list(uint8_t target_sysid, uint8_t target_compid);

    // Re-request a single parameter by index (PARAM_REQUEST_READ #20).
    // Used to fill gaps caused by UDP packet loss during a bulk fetch.
    void request_param_read(uint8_t target_sysid, uint8_t target_compid,
                            int16_t param_index);

    // Write a single parameter (PARAM_SET #23).
    // FC echoes the accepted value back as a PARAM_VALUE message.
    void set_param(uint8_t target_sysid, uint8_t target_compid,
                   const char* param_id, float value, uint8_t param_type);

    // ── Mission protocol ─────────────────────────────────────────────────────

    // Send MISSION_REQUEST_LIST (#43) — FC responds with MISSION_COUNT (#44).
    void request_mission_list(uint8_t target_sysid, uint8_t target_compid);

    // Send MISSION_REQUEST_INT (#51) for a specific sequence number.
    // FC responds with MISSION_ITEM_INT (#73).
    void request_mission_item(uint8_t target_sysid, uint8_t target_compid,
                              uint16_t seq);

    // Send MISSION_ACK (#47) to signal successful receipt of all items.
    void send_mission_ack(uint8_t target_sysid, uint8_t target_compid);

    // Begin mission upload — sends MISSION_COUNT and stores the item vector.
    // FC will then send MISSION_REQUEST_INT per item; link thread serves
    // each request by calling send_mission_item_at().
    void start_upload(uint8_t target_sysid, uint8_t target_compid,
                      std::vector<MissionItem> items);

    // Send MISSION_ITEM_INT for seq from the stored upload vector.
    // Called by the link thread when parser.pending_item_reqs() is drained.
    void send_mission_item_at(uint8_t target_sysid, uint8_t target_compid,
                              uint16_t seq);

    // Discard stored upload items (call after MISSION_ACK or abort).
    void clear_upload();

    // True while upload items are stored (cleared by clear_upload()).
    bool upload_active() const;

    // ── Transport ─────────────────────────────────────────────────────────────

    // Drain all queued frames via sendto() (UDP).
    void flush(int fd, const sockaddr_in& dest);

    // Drain all queued frames via write() (TCP socket or serial fd).
    void flush_stream(int fd);

    bool has_pending() const;

    // ── ACK feedback ──────────────────────────────────────────────────────────

    // Called from recv thread when COMMAND_ACK arrives
    void notify_ack(uint16_t command, uint8_t result);

    // Called from UI thread to colour a button; returns Normal when no entry
    // Flash duration: ~600 ms for Accepted/Rejected, then returns to Normal
    CmdFlashState query_flash(uint16_t cmd) const;

private:
    // Serialise a COMMAND_LONG and push onto the queue
    void enqueue_command_long(uint8_t tsys, uint8_t tcomp, uint16_t cmd,
                              float p1 = 0.f, float p2 = 0.f, float p3 = 0.f,
                              float p4 = 0.f, float p5 = 0.f, float p6 = 0.f,
                              float p7 = 0.f);

    struct CmdState {
        CmdFlashState state = CmdFlashState::Normal;
        std::chrono::steady_clock::time_point changed_at{};
    };

    mutable std::mutex                      mtx_;
    std::queue<std::vector<uint8_t>>        queue_;
    mutable std::unordered_map<uint16_t, CmdState> cmd_states_;

    std::vector<MissionItem>                upload_items_;
};
