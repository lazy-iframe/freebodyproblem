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


#include "mavlink_sender.hpp"

#ifdef _WIN32
#  include <winsock2.h>
#else
#  include <sys/socket.h>
#  include <unistd.h>
#endif

#include <chrono>
#include <mavlink/ardupilotmega/mavlink.h>

using Clock = std::chrono::steady_clock;

// ── High-level command methods ────────────────────────────────────────────────

void MavlinkSender::arm(uint8_t tsys, uint8_t tcomp)
{
    // MAV_CMD_COMPONENT_ARM_DISARM (400), param1=1 (arm)
    enqueue_command_long(tsys, tcomp, 400, 1.f);
}

void MavlinkSender::disarm(uint8_t tsys, uint8_t tcomp, bool force)
{
    // MAV_CMD_COMPONENT_ARM_DISARM (400), param1=0 (disarm)
    // param2=21196 is the ArduPilot magic number for a forced disarm in-flight
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
    // MAV_CMD_DO_SET_MODE (176)
    // param1 = MAV_MODE_FLAG_CUSTOM_MODE_ENABLED (1)
    // param2 = custom_mode enum value (autopilot-specific)
    enqueue_command_long(tsys, tcomp, 176,
                         1.f, static_cast<float>(custom_mode));
}

void MavlinkSender::return_to_launch(uint8_t tsys, uint8_t tcomp)
{
    // MAV_CMD_NAV_RETURN_TO_LAUNCH (20)
    enqueue_command_long(tsys, tcomp, 20);
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

void MavlinkSender::request_autopilot_capabilities(uint8_t tsys, uint8_t tcomp)
{
    // MAV_CMD_REQUEST_AUTOPILOT_CAPABILITIES (520), param1=1 (request)
    enqueue_command_long(tsys, tcomp, 520, 1.f);
}


void MavlinkSender::request_param_list(uint8_t tsys, uint8_t tcomp)
{
    // PARAM_REQUEST_LIST (#21) — FC responds with a PARAM_VALUE stream
    mavlink_message_t msg;
    mavlink_msg_param_request_list_pack(GCS_SYSID, GCS_COMPID, &msg, tsys, tcomp);

    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    const uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);

    std::lock_guard<std::mutex> lk(mtx_);
    queue_.emplace(buf, buf + len);
}

void MavlinkSender::request_param_read(uint8_t tsys, uint8_t tcomp,
                                        int16_t param_index)
{
    // PARAM_REQUEST_READ (#20) — empty param_id means "look up by index"
    mavlink_message_t msg;
    mavlink_msg_param_request_read_pack(GCS_SYSID, GCS_COMPID, &msg,
                                        tsys, tcomp, "", param_index);
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    const uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
    std::lock_guard<std::mutex> lk(mtx_);
    queue_.emplace(buf, buf + len);
}

void MavlinkSender::set_param(uint8_t tsys, uint8_t tcomp,
                               const char* param_id, float value, uint8_t param_type)
{
    // PARAM_SET (#23) — FC echoes the accepted value back as PARAM_VALUE
    mavlink_message_t msg;
    mavlink_msg_param_set_pack(GCS_SYSID, GCS_COMPID, &msg,
                               tsys, tcomp, param_id, value, param_type);

    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    const uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);

    std::lock_guard<std::mutex> lk(mtx_);
    queue_.emplace(buf, buf + len);
}

void MavlinkSender::request_mission_list(uint8_t tsys, uint8_t tcomp)
{
    mavlink_message_t msg;
    mavlink_msg_mission_request_list_pack(GCS_SYSID, GCS_COMPID, &msg,
                                           tsys, tcomp, MAV_MISSION_TYPE_MISSION);
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    const uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
    std::lock_guard<std::mutex> lk(mtx_);
    queue_.emplace(buf, buf + len);
}

void MavlinkSender::request_mission_item(uint8_t tsys, uint8_t tcomp, uint16_t seq)
{
    mavlink_message_t msg;
    mavlink_msg_mission_request_int_pack(GCS_SYSID, GCS_COMPID, &msg,
                                         tsys, tcomp, seq, MAV_MISSION_TYPE_MISSION);
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    const uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
    std::lock_guard<std::mutex> lk(mtx_);
    queue_.emplace(buf, buf + len);
}

void MavlinkSender::send_mission_ack(uint8_t tsys, uint8_t tcomp)
{
    mavlink_message_t msg;
    mavlink_msg_mission_ack_pack(GCS_SYSID, GCS_COMPID, &msg,
                                  tsys, tcomp,
                                  MAV_MISSION_ACCEPTED,
                                  MAV_MISSION_TYPE_MISSION,
                                  0);
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    const uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
    std::lock_guard<std::mutex> lk(mtx_);
    queue_.emplace(buf, buf + len);
}

void MavlinkSender::start_upload(uint8_t tsys, uint8_t tcomp,
                                  std::vector<MissionItem> items)
{
    mavlink_message_t msg;
    mavlink_msg_mission_count_pack(GCS_SYSID, GCS_COMPID, &msg,
                                   tsys, tcomp,
                                   (uint16_t)items.size(),
                                   MAV_MISSION_TYPE_MISSION,
                                   0 /* opaque_id */);
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    const uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);

    std::lock_guard<std::mutex> lk(mtx_);
    queue_.emplace(buf, buf + len);
    upload_items_ = std::move(items);
}

void MavlinkSender::send_mission_item_at(uint8_t tsys, uint8_t tcomp, uint16_t seq)
{
    std::lock_guard<std::mutex> lk(mtx_);
    if (seq >= upload_items_.size()) return;

    const MissionItem& it = upload_items_[seq];
    mavlink_message_t msg;
    mavlink_msg_mission_item_int_pack(
        GCS_SYSID, GCS_COMPID, &msg,
        tsys, tcomp,
        it.seq,
        it.frame,
        it.command,
        it.is_current ? 1 : 0,
        it.autocontinue ? 1 : 0,
        it.param1, it.param2, it.param3, it.param4,
        (int32_t)(it.lat * 1e7),
        (int32_t)(it.lon * 1e7),
        it.alt,
        MAV_MISSION_TYPE_MISSION
    );
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    const uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
    queue_.emplace(buf, buf + len);
}

void MavlinkSender::clear_upload()
{
    std::lock_guard<std::mutex> lk(mtx_);
    upload_items_.clear();
}

bool MavlinkSender::upload_active() const
{
    std::lock_guard<std::mutex> lk(mtx_);
    return !upload_items_.empty();
}

void MavlinkSender::request_message_interval(uint8_t tsys, uint8_t tcomp,
                                              uint32_t message_id, int32_t interval_us)
{
    // MAV_CMD_SET_MESSAGE_INTERVAL = 511
    // param1 = message ID, param2 = interval in microseconds
    mavlink_message_t msg;
    mavlink_msg_command_long_pack(
        GCS_SYSID, GCS_COMPID, &msg,
        tsys, tcomp,
        511, 0,
        (float)message_id, (float)interval_us,
        0.f, 0.f, 0.f, 0.f, 0.f
    );

    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    const uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);

    std::lock_guard<std::mutex> lk(mtx_);
    queue_.emplace(buf, buf + len);
    // No cmd_states_ entry — stream rate requests don't need UI flash feedback
}

// ── Transport ─────────────────────────────────────────────────────────────────

void MavlinkSender::flush_stream(int fd)
{
    std::lock_guard<std::mutex> lk(mtx_);
    while (!queue_.empty()) {
        const auto& frame = queue_.front();
        ::write(fd,
                reinterpret_cast<const char*>(frame.data()),
                static_cast<int>(frame.size()));
        queue_.pop();
    }
}

void MavlinkSender::flush(int fd, const sockaddr_in& dest)
{
    std::lock_guard<std::mutex> lk(mtx_);
    while (!queue_.empty()) {
        const auto& frame = queue_.front();
        sendto(fd,
               reinterpret_cast<const char*>(frame.data()),
               static_cast<int>(frame.size()),
               0,
               reinterpret_cast<const sockaddr*>(&dest),
               sizeof(dest));
        queue_.pop();
    }
}

bool MavlinkSender::has_pending() const
{
    std::lock_guard<std::mutex> lk(mtx_);
    return !queue_.empty();
}

// ── ACK feedback ─────────────────────────────────────────────────────────────

void MavlinkSender::notify_ack(uint16_t command, uint8_t result)
{
    std::lock_guard<std::mutex> lk(mtx_);
    auto& cs = cmd_states_[command];
    cs.state      = (result == MAV_RESULT_ACCEPTED)
                    ? CmdFlashState::Accepted
                    : CmdFlashState::Rejected;
    cs.changed_at = Clock::now();
}

CmdFlashState MavlinkSender::query_flash(uint16_t cmd) const
{
    static constexpr double FLASH_DURATION_S = 0.6;

    std::lock_guard<std::mutex> lk(mtx_);
    auto it = cmd_states_.find(cmd);
    if (it == cmd_states_.end())
        return CmdFlashState::Normal;

    const CmdState cs = it->second;   // copy before any potential erase
    if (cs.state == CmdFlashState::Pending) {
        double elapsed = std::chrono::duration<double>(Clock::now() - cs.changed_at).count();
        if (elapsed >= 3.0) {
            cmd_states_.erase(it);
            return CmdFlashState::Normal;
        }
        return CmdFlashState::Pending;
    }

    if (cs.state == CmdFlashState::Accepted || cs.state == CmdFlashState::Rejected) {
        double elapsed = std::chrono::duration<double>(Clock::now() - cs.changed_at).count();
        if (elapsed < FLASH_DURATION_S)
            return cs.state;
        // Flash expired — drop the entry
        cmd_states_.erase(it);
        return CmdFlashState::Normal;
    }

    return CmdFlashState::Normal;
}

// ── Private helpers ───────────────────────────────────────────────────────────

void MavlinkSender::enqueue_command_long(uint8_t tsys, uint8_t tcomp, uint16_t cmd,
                                         float p1, float p2, float p3,
                                         float p4, float p5, float p6, float p7)
{
    mavlink_message_t msg;
    mavlink_msg_command_long_pack(
        GCS_SYSID, GCS_COMPID,
        &msg,
        tsys, tcomp,
        cmd,
        0,              // confirmation (0 = first transmission)
        p1, p2, p3, p4, p5, p6, p7
    );

    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    const uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);

    std::lock_guard<std::mutex> lk(mtx_);
    queue_.emplace(buf, buf + len);
    cmd_states_[cmd] = { CmdFlashState::Pending, Clock::now() };
}
