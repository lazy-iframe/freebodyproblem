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

#include "mavlink_framer.hpp"
#include "timesync.hpp"

// One entry from AVAILABLE_MODES (#435). Flight stacks enumerate their own mode
// list, which is the only portable way to know what a given vehicle supports —
// custom_mode numbering differs between ArduCopter, ArduPlane, Rover and Sub.
struct FlightModeInfo {
    uint8_t     mode_index    = 0;   // 1-based; the vehicle's own ordering
    uint32_t    custom_mode   = 0;
    uint8_t     standard_mode = 0;   // MAV_STANDARD_MODE, 0 when custom-only
    uint32_t    properties    = 0;   // MAV_MODE_PROPERTY bitmask
    std::string name;                // mode_name, as reported by the vehicle

    // MAV_MODE_PROPERTY_NOT_USER_SELECTABLE — must not be offered as a button.
    bool user_selectable() const { return (properties & 2u) == 0u; }
    // MAV_MODE_PROPERTY_ADVANCED
    bool advanced() const { return (properties & 1u) != 0u; }
};

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

// Raw receiver pulse widths, from RC_CHANNELS (#65) or RC_CHANNELS_RAW (#35).
//
// Both carry the same thing — what the receiver handed the autopilot, before
// trim, reversal or deadzone — and differ only in reach: #65 carries 18
// channels plus the receiver's own channel count, #35 carries 8 and a port
// number. #65 is preferred wherever a vehicle sends it; #35 is the fallback for
// stacks that predate it.
//
// A calibration has to read these and not RC_CHANNELS_SCALED, which has already
// had the very trim and reversal we are trying to measure applied to it.
struct RcChannels {
    static constexpr int MAX_CHANNELS = 18;

    uint16_t chan[MAX_CHANNELS] = {};
    uint8_t  count      = 0;          // channels the receiver reports; may exceed MAX_CHANNELS
    uint8_t  rssi       = UINT8_MAX;  // 0-254, UINT8_MAX = unknown
    bool     from_raw   = false;      // true while only RC_CHANNELS_RAW has been seen
    bool     valid      = false;      // at least one message decoded
    uint32_t generation = 0;          // ticks on every update; a sweep watches this

    // Channels this vehicle actually carries, clamped to what the struct holds.
    // chancount is documented as 0 when no RC is being received, so a zero there
    // is not "no channels in the message" — it is no receiver.
    int usable_count() const {
        if (count == 0) return 0;
        const int n = (int)count;
        return n < MAX_CHANNELS ? n : MAX_CHANNELS;
    }

    // 1-based, matching the RCn_* parameter names and every RC UI in the field.
    uint16_t at(int channel_1based) const {
        if (channel_1based < 1 || channel_1based > MAX_CHANNELS) return 0;
        return chan[channel_1based - 1];
    }

    bool has_rssi() const { return rssi != UINT8_MAX; }
};

// PARAM_EXT_ACK (#324) — the extended parameter protocol's reply to a
// PARAM_EXT_SET. Unlike PARAM_SET, which is answered with a plain PARAM_VALUE
// echo that is indistinguishable from any other, this one names the parameter
// and says outright whether the write took.
struct ParamExtAck {
    char    param_id[17];  // null-terminated
    char    value[129];    // the payload as received, null-terminated for display
    uint8_t param_type;    // MAV_PARAM_EXT_TYPE
    uint8_t result;        // PARAM_ACK: 0=ACCEPTED, 3=IN_PROGRESS, else failure
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
    // Ticks once per HEARTBEAT from the locked autopilot. The UI beeps on the
    // change rather than on a clock, so the cue is the vehicle's pulse and not
    // the ground station's: heartbeats stop, beeping stops.
    uint32_t      heartbeat_count = 0;

    // ATTITUDE (#30) – radians converted to degrees
    float roll = 0, pitch = 0, yaw = 0;
    bool  has_attitude = false;

    // GLOBAL_POSITION_INT (#33)
    double lat = 0, lon = 0;
    float  alt_asl = 0, alt_rel = 0;
    bool   has_global_pos = false;

    // HOME_POSITION (#242)
    //
    // Where the vehicle will return to, and the datum every "relative"
    // altitude in this GCS is relative to. Sent when the vehicle sets or moves
    // home, and streamed slowly on request — see the connect burst.
    double home_lat = 0, home_lon = 0;
    float  home_alt_asl = 0;          // metres above mean sea level
    bool   has_home = false;

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

    // Estimator health, from whichever message the stack reports it in:
    // ArduPilot's own EKF_STATUS_REPORT (#193, ardupilotmega.xml) or the
    // portable ESTIMATOR_STATUS (#230, common.xml) that PX4 sends.
    //
    // The six fields line up one-to-one, but they are not the same quantity —
    // ArduPilot reports normalised variances, PX4 innovation test ratios — so
    // the panel drawing them has to say which. See FirmwareProfile.
    float    ekf_velocity_variance    = 0;
    float    ekf_pos_horiz_variance   = 0;
    float    ekf_pos_vert_variance    = 0;
    float    ekf_compass_variance     = 0;
    float    ekf_terrain_alt_variance = 0;
    float    ekf_airspeed_variance    = 0;

    // Which of the two filled the fields above: 0 none, 193, or 230. Not
    // decoration — it is the only thing that says what they mean, and it is
    // what keeps a vehicle sending both from flickering between two scales.
    uint32_t ekf_source_msgid         = 0;

    // The bitmask that came with it, and the one field the two messages do NOT
    // agree on: EKF_STATUS_FLAGS when ekf_source_msgid is 193,
    // ESTIMATOR_STATUS_FLAGS when it is 230. Nothing reads this today; nothing
    // may start to without checking the source first.
    uint16_t ekf_flags                = 0;
    bool     has_ekf_status           = false;

    // RC_CHANNELS (#65) / RC_CHANNELS_RAW (#35)
    RcChannels rc;

    // TIMESYNC (#111) — how far the vehicle's boot clock runs ahead of our
    // monotonic one, in nanoseconds. Add it to timesync_monotonic_ns() to get
    // the vehicle's clock at that instant; that is a live reading rather than a
    // stale snapshot, so the UI can tick a clock from it at frame rate without
    // waiting on the next exchange. Meaningless until has_timesync.
    int64_t time_offset_ns = 0;
    bool    has_timesync   = false;

    // STATUSTEXT (#253) — every one ever received on this link, not the size of
    // the rolling window that holds them. A consumer that wants only the new
    // ones cannot count entries in that window: it drops its oldest at 200, so
    // an index into it silently points at a different message afterwards.
    uint32_t statustext_total = 0;

    // MAV_CMD_ACCELCAL_VEHICLE_POS (42429), arriving as a COMMAND_LONG from the
    // vehicle. During an accelerometer calibration ArduPilot sends this
    // alongside the human-readable STATUSTEXT, and it is the authoritative half
    // of the pair: an enum rather than a sentence, and the only place SUCCESS
    // and FAILED are reported as values rather than prose.
    //
    // `accelcal_seq` ticks on every one received, including a repeat of the
    // same position — the vehicle re-sends if it thinks the GCS missed one, and
    // a caller watching only the value would take the repeat for silence.
    uint32_t accelcal_pos = 0;   // ACCELCAL_VEHICLE_POS, 0 when none seen
    uint32_t accelcal_seq = 0;

    // COMMAND_ACK for MAV_CMD_PREFLIGHT_CALIBRATION (241) — the vehicle's
    // verdict on a calibration it was asked to run. Latched here rather than
    // read from the ACK queue because that queue is drained on the link thread
    // and cleared every pass, and the panel that cares runs a frame later.
    //
    // `calib_ack_seq` ticks on every one received, the same as accelcal_seq
    // above and for the same reason: two runs in a row can end the same way,
    // and a caller watching only the result would take the second for silence.
    uint8_t  calib_ack_result = 255;  // MAV_RESULT; 255 when none seen
    uint32_t calib_ack_seq    = 0;

    // COMMAND_ACK for the compass calibration commands — DO_START_MAG_CAL
    // (42424), DO_ACCEPT_MAG_CAL (42425) and DO_CANCEL_MAG_CAL (42426).
    //
    // Separate from calib_ack_* above because they are separate commands: a
    // vehicle refusing to start a compass calibration says so here, and a run
    // driven through these never touches PREFLIGHT_CALIBRATION at all.
    uint16_t magcal_ack_cmd    = 0;    // which of the three answered
    uint8_t  magcal_ack_result = 255;  // MAV_RESULT; 255 when none seen
    uint32_t magcal_ack_seq    = 0;

    // MAG_CAL_PROGRESS (#191) and MAG_CAL_REPORT (#192) — ArduPilot's account
    // of a compass calibration, one message per compass.
    //
    // Kept as a slot per compass rather than as a single latched message: the
    // vehicle interleaves several compasses and sends all their reports at
    // once, so one latch would drop every compass but the last to arrive.
    // `magcal_seq` ticks on each message, so a consumer can tell an unchanged
    // pass from a repeated one.
    static constexpr int MAX_COMPASSES = 3;
    struct MagCalCompass {
        bool    seen           = false;  // a message has arrived for this one
        uint8_t status         = 0;      // MAG_CAL_STATUS
        uint8_t completion_pct = 0;      // 0-100, from MAG_CAL_PROGRESS
        bool    reported       = false;  // MAG_CAL_REPORT arrived: it is done
        bool    autosaved      = false;  // the vehicle stored the new offsets
        float   fitness        = 0.f;    // RMS milligauss, lower is better
        // Which parts of the sphere of directions the compass has been turned
        // through: 80 bits over ArduPilot's geodesic grid, LSB-first in each
        // byte. See geodesic_grid.hpp. Only MAG_CAL_PROGRESS carries it, so it
        // keeps its last value once the compass has reported.
        uint8_t completion_mask[10] = {};
        // Value of magcal_seq when this slot was last written, so a consumer
        // can tell what belongs to the calibration it started from what a
        // previous one left behind.
        uint32_t seq           = 0;
    };
    MagCalCompass magcal[MAX_COMPASSES];
    uint32_t      magcal_seq = 0;

    // AUTOPILOT_VERSION (#148)
    char     fw_version[32] = {};   // e.g. "4.3.7"
    char     fw_hash[17]    = {};   // first 8 bytes of flight_custom_version as hex
    bool     has_fw_info    = false;
    uint16_t vendor_id      = 0;    // USB vendor of the board, 0 when unreported
    uint16_t product_id     = 0;    // USB product of the board
    uint32_t board_version  = 0;
    uint64_t capabilities   = 0;    // MAV_PROTOCOL_CAPABILITY bitmask
    uint64_t board_uid      = 0;    // 0 when the board does not report one

    // COMPONENT_INFORMATION (#395) — where a component keeps its metadata, not
    // the metadata itself: the reply carries MAVLink FTP URIs to JSON files
    // onboard, and nothing this GCS can show without fetching them. Kept so the
    // panel can say the component answered, and say where it pointed.
    char     comp_info_uri[101]  = {};
    uint8_t  comp_info_compid    = 0;   // who answered
    bool     has_comp_info       = false;

    // Every component heard from, autopilot or not — HEARTBEAT (#0) is the only
    // announcement a component makes, and a compass or IMU that sends its own
    // is a separate MAV_COMP_ID on the same system.
    //
    // Recorded before the autopilot filter that the rest of this state goes
    // through: the point here is what else is on the bus, which is exactly what
    // that filter throws away.
    static constexpr int MAX_COMPONENTS = 16;
    struct Component {
        uint8_t  sysid      = 0;
        uint8_t  compid     = 0;
        uint8_t  type       = 0;   // MAV_TYPE
        uint8_t  autopilot  = 0;   // MAV_AUTOPILOT
        uint32_t heartbeats = 0;
    };
    Component components[MAX_COMPONENTS];
    int       component_count = 0;

    // PARAM_VALUE (#22) — progress counters; map lives in MavlinkParser::params_
    uint16_t param_count      = 0;  // total parameter count reported by FC
    uint16_t params_received  = 0;  // number of PARAM_VALUE messages received so far
    uint32_t params_generation = 0; // incremented on every PARAM_VALUE (incl. write echoes)

    // True only while a PARAM_REQUEST_LIST this GCS sent is still being
    // answered. Stamped by the link thread, not by the parser: the parser sees
    // PARAM_VALUEs and cannot tell a bulk fetch from the single unsolicited one
    // a vehicle sends whenever a parameter changes — including the stream-rate
    // parameters our own SET_MESSAGE_INTERVALs write on connect. Without this,
    // "1 of 1387 received" reads as a stalled fetch that nobody ever started.
    bool     param_fetch_active = false;

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

    // AVAILABLE_MODES (#435) — populated after the link requests the mode list.
    // Sorted by the vehicle's mode_index. Empty when the flight stack does not
    // implement the standard modes protocol, in which case the UI falls back to
    // its built-in ArduPilot tables.
    std::vector<FlightModeInfo> available_modes;
    uint8_t  modes_expected  = 0;      // number_modes field; 0 until first reply
    bool     modes_complete  = false;  // every mode_index seen

    // True once mode_index has been received. ArduPilot answers a request for
    // index 0 ("all modes" in the spec) with index 1 only, so the link thread
    // has to walk 1..modes_expected itself and needs to know what is missing.
    bool has_mode_index(uint8_t idx) const {
        for (const auto& m : available_modes)
            if (m.mode_index == idx) return true;
        return false;
    }
    // AVAILABLE_MODES_MONITOR (#437). A change of seq means the mode list is
    // stale and must be re-requested.
    uint8_t  modes_seq       = 0;
    bool     has_modes_seq   = false;
    bool     modes_dirty     = false;  // set on seq change, cleared by the link thread
};

class MavlinkParser {
public:
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
    std::vector<ItemReq>                       item_reqs_;
    std::deque<StatusText>                     status_texts_;
    TimeSync                                   timesync_;
    std::vector<TimeSync::Reply>               timesync_replies_;
    static constexpr size_t MAX_STATUS_TEXTS = 200;

    uint64_t total_messages_ = 0;
};
