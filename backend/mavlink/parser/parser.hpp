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

#include <chrono>
#include <cmath>
#include <cstdint>
#include <deque>
#include <string>
#include <unordered_map>
#include <vector>

#include <mavlink/ardupilotmega/mavlink.h>

#include "backend/mavlink/framer.hpp"
#include "backend/mavlink/timesync.hpp"
#include "backend/mission/download.hpp"
#include "backend/vehicle/state.hpp"

class MavlinkParser {
public:
    // Whether a message is a heartbeat from a real vehicle: an autopilot, not a
    // gimbal, camera, companion computer or another ground station. Used by
    // Fleet before any parser exists for that sysid, so that even discovery's
    // one decode happens here.
    static bool announces_vehicle(const mavlink_message_t& msg);

    // Two ways to come by an identity.
    //
    // The default constructor discovers it: the first heartbeat from a real
    // autopilot binds this parser to that sysid/compid, which is what a single
    // link feeding a single vehicle wants and is how this class has always
    // behaved. The two-argument form is told up front, for a link that
    // demultiplexes several vehicles and has already decided which one this is.
    MavlinkParser() = default;
    MavlinkParser(uint8_t sysid, uint8_t autopilot_compid)
        : sysid_(sysid), autopilot_compid_(autopilot_compid), bound_(true) {}

    // The system this parser speaks for, once known. Zero until the first
    // autopilot heartbeat when the identity was discovered rather than given.
    uint8_t sysid()            const { return sysid_;            }
    uint8_t autopilot_compid() const { return autopilot_compid_; }
    bool    bound()            const { return bound_;            }

    // Feed a raw UDP datagram. Returns number of complete messages parsed.
    //
    // Convenience for callers that own a whole byte stream to themselves. Where
    // one link feeds several vehicles the framing belongs to the link — one
    // MavlinkFramer per byte stream, its output routed by sysid — and those
    // callers use handle() directly.
    int parse(const uint8_t* buf, size_t len);

    // Feed one already-framed message. The routing decision of which parser a
    // message belongs to is the caller's; this only interprets it.
    void handle(const mavlink_message_t& msg);

    const VehicleState& state() const { return state_; }
    const std::unordered_map<std::string, ParamEntry>& params() const { return params_; }
    const std::unordered_map<uint32_t, MessageStats>& msg_stats() const { return msg_stats_; }

    uint64_t total_messages() const { return total_messages_;          }
    uint64_t total_bytes()    const { return framer_.total_bytes();    }
    uint64_t parse_errors()   const { return framer_.parse_errors();   }

    // ACK queue — drained each frame by main.cpp, forwarded to MavlinkSender
    const std::vector<CommandAck>& pending_acks() const { return ack_queue_; }
    void clear_acks() { ack_queue_.clear(); }

    // PARAM_EXT_ACK queue — drained by the link thread, same shape as the
    // command ACKs above. Only ever non-empty when something has actually sent
    // a PARAM_EXT_SET; autopilots that only speak the classic parameter
    // protocol never put anything here.
    const std::vector<ParamExtAck>& pending_param_ext_acks() const { return param_ext_acks_; }
    void clear_param_ext_acks() { param_ext_acks_.clear(); }

    // Acknowledge a mode-list change after the link thread has re-requested it.
    void clear_modes_dirty() { state_.modes_dirty = false; }

    // Drop what we hold, so a new PARAM_REQUEST_LIST is counted from zero.
    // Without it the second fetch of a session starts at params_received ==
    // param_count and so is already "finished": no progress, and no gap-filling
    // if it stalls. The published copy in main.cpp is left alone and only
    // replaced when the fetch completes, so the table keeps showing the old
    // values while the new ones come in rather than blanking.
    void clear_params()
    {
        params_.clear();
        state_.params_received = 0;
    }

    // Mission download request queue — drained by link thread after each parse()
    // `type` is the MAV_MISSION_TYPE the item belongs to: mission or fence.
    struct MissionReq { uint8_t tsys; uint8_t tcomp; uint16_t seq; uint8_t type; };
    const std::vector<MissionReq>& pending_mission_reqs() const { return mission_reqs_; }
    void clear_mission_reqs() { mission_reqs_.clear(); }

    // Mission upload item request queue — FC sends MISSION_REQUEST_INT during upload
    // Link thread drains this and calls MavlinkSender::send_mission_item_at()
    struct ItemReq { uint8_t tsys; uint8_t tcomp; uint16_t seq; uint8_t type; };
    const std::vector<ItemReq>& pending_item_reqs() const { return item_reqs_; }
    void clear_item_reqs() { item_reqs_.clear(); }

    // Status text log — rolling window of the last MAX_STATUS_TEXTS messages
    const std::deque<StatusText>& status_texts() const { return status_texts_; }

    // TIMESYNC. The exchange needs a sender and this class has none, so the
    // link thread opens each round with begin_request() and drains any reply
    // the vehicle's own requests earned, exactly as it drains the mission
    // queues above. Link thread only.
    TimeSync& timesync() { return timesync_; }
    const std::vector<TimeSync::Reply>& pending_timesync_replies() const
    { return timesync_replies_; }
    void clear_timesync_replies() { timesync_replies_.clear(); }

    void print_stats();

private:
    // Framing for the parse() convenience path only. A link that feeds several
    // vehicles frames upstream of here and never touches this one.
    MavlinkFramer                              framer_;
    std::unordered_map<uint32_t, MessageStats> msg_stats_;
    VehicleState                               state_;
    std::unordered_map<std::string, ParamEntry> params_;

    // Which system and component this parser speaks for.
    //
    // This replaces what used to be called the source lock. The lock was set on
    // the first autopilot heartbeat and meant to keep stray traffic from
    // gimbals and companion computers — common on a USB serial bus — out of the
    // vehicle's state. It only ever worked for three message types: HEARTBEAT
    // and the two RC_CHANNELS variants re-checked it, while ATTITUDE,
    // GLOBAL_POSITION_INT, PARAM_VALUE, the mission messages and everything
    // else wrote state_ no matter who sent them. A second vehicle on the same
    // link therefore interleaved its telemetry into this one silently.
    //
    // The gate now lives at the top of handle(), where it covers every message
    // rather than three, and is expressed as two separate questions: is this
    // the right system, and within it, is this the autopilot rather than some
    // other component announcing itself.
    uint8_t sysid_            = 0;
    uint8_t autopilot_compid_ = 0;
    bool    bound_            = false;
    std::vector<CommandAck>                    ack_queue_;
    std::vector<ParamExtAck>                   param_ext_acks_;
    std::vector<MissionReq>                    mission_reqs_;
    MissionDownload                            mission_dl_;  // the plan's download
    MissionDownload                            fence_dl_;    // the fence's download
    std::vector<ItemReq>                       item_reqs_;
    std::deque<StatusText>                     status_texts_;
    TimeSync                                   timesync_;
    std::vector<TimeSync::Reply>               timesync_replies_;
    static constexpr size_t MAX_STATUS_TEXTS = 200;

    uint64_t total_messages_ = 0;

    // One handler per message, called from handle(). Grouped by file:
    // parser.cpp, telemetry.cpp, params.cpp, modes.cpp, commands.cpp, mission.cpp.
    // parser.cpp
    void on_heartbeat(const mavlink_message_t& msg);
    void on_sys_status(const mavlink_message_t& msg);
    void on_timesync(const mavlink_message_t& msg);
    void on_statustext(const mavlink_message_t& msg);
    // telemetry.cpp
    void on_gps_raw_int(const mavlink_message_t& msg);
    void on_attitude(const mavlink_message_t& msg);
    void on_home_position(const mavlink_message_t& msg);
    void on_global_position_int(const mavlink_message_t& msg);
    void on_vfr_hud(const mavlink_message_t& msg);
    void on_ekf_status_report(const mavlink_message_t& msg);
    void on_estimator_status(const mavlink_message_t& msg);
    void on_rc_channels(const mavlink_message_t& msg);
    void on_rc_channels_raw(const mavlink_message_t& msg);
    // params.cpp
    void on_param_ext_ack(const mavlink_message_t& msg);
    void on_param_value(const mavlink_message_t& msg);
    // modes.cpp
    void on_available_modes(const mavlink_message_t& msg);
    void on_available_modes_monitor(const mavlink_message_t& msg);
    void on_autopilot_version(const mavlink_message_t& msg);
    void on_component_information(const mavlink_message_t& msg);
    // commands.cpp
    void on_command_long(const mavlink_message_t& msg);
    void on_command_ack(const mavlink_message_t& msg);
    void on_mag_cal_progress(const mavlink_message_t& msg);
    void on_mag_cal_report(const mavlink_message_t& msg);
    // mission.cpp
    void on_mission_count(const mavlink_message_t& msg);
    void on_mission_item_int(const mavlink_message_t& msg);
    void on_mission_request_int(const mavlink_message_t& msg);
    void on_mission_request(const mavlink_message_t& msg);
    void on_mission_ack(const mavlink_message_t& msg);

    // A download's progress into state_, and its list once every item is in.
    void publish_download_progress(bool fence);
    void finish_download(bool fence);

    // Reboot detection — see VehicleState::reboot_count.
    //
    // The vehicle's boot clock as last read, from any message that carries it.
    // Going backwards by more than jitter can explain is the vehicle restarting.
    void note_boot_ms(uint32_t boot_ms, int64_t now_ns);
    uint32_t last_boot_ms_  = 0;
    bool     have_boot_ms_  = false;

    // Where the stream last resumed after a silence, and how many STATUSTEXTs
    // had arrived by then. A reboot is always a silence followed by a new boot,
    // but it is noticed only when the first message carrying the new clock
    // arrives — and the new boot's first STATUSTEXTs can beat it. The resume
    // point is where the new boot's messages actually begin.
    int64_t  last_rx_ns_    = 0;
    int64_t  resume_ns_     = 0;
    uint32_t resume_mark_   = 0;
};
