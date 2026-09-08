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

#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <winsock2.h>
#else
#  include <netinet/in.h>
#endif

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <vector>

#include "connection.hpp"      // for SerialHandle
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

    // How many senders can exist at once — one per vehicle, since each needs a
    // MAVLink TX channel of its own. See the note on chan_ below.
    static constexpr int MAX_CHANNELS = MAVLINK_COMM_NUM_BUFFERS;

    // Takes the next free TX channel. Throws nothing: past MAX_CHANNELS the
    // sender shares channel 0, which is safe (the pack mutex covers it) and only
    // costs a shared sequence counter. Callers that care cap vehicle count
    // instead of relying on that.
    MavlinkSender();
    ~MavlinkSender();

    MavlinkSender(const MavlinkSender&)            = delete;
    MavlinkSender& operator=(const MavlinkSender&) = delete;

    // ── High-level commands ───────────────────────────────────────────────────

    void arm   (uint8_t target_sysid, uint8_t target_compid);
    void disarm(uint8_t target_sysid, uint8_t target_compid, bool force = false);

    // Climb to altitude_m (metres, relative to home) after takeoff
    void takeoff(uint8_t target_sysid, uint8_t target_compid, float altitude_m);

    // ArduCopter/Plane custom_mode values (e.g. STABILIZE=0, ALT_HOLD=2, GUIDED=4)
    void set_mode(uint8_t target_sysid, uint8_t target_compid, uint32_t custom_mode);

    void return_to_launch(uint8_t target_sysid, uint8_t target_compid);

    // ── Guided flight ─────────────────────────────────────────────────────────

    // SET_POSITION_TARGET_GLOBAL_INT (#86) — "fly to this point and hold it".
    //
    // A stream message rather than a command: there is no COMMAND_ACK, and a
    // vehicle that is not in GUIDED discards it silently. Callers check the
    // mode themselves, since the wire says nothing either way.
    //
    // altitude_m is metres above home (MAV_FRAME_GLOBAL_RELATIVE_ALT_INT), the
    // same datum the takeoff altitude and the A/C overlay use. The type_mask
    // ignores velocity, acceleration, yaw and yaw rate, leaving position the
    // only field the vehicle is asked to honour — the fields the mask ignores
    // are still sent as zero, and a vehicle that mistook one for a demand
    // would be commanded to stop dead.
    void goto_position(uint8_t target_sysid, uint8_t target_compid,
                       double lat_deg, double lon_deg, float altitude_m);

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

    // Ask for AVAILABLE_MODES (#435) at mode_index 0. The spec calls this the
    // "all modes" form, but ArduPilot answers it with index 1 alone — its value
    // is the number_modes field, which tells the caller how far to enumerate.
    // Flight stacks predating the standard modes protocol do not answer at all,
    // so callers must tolerate an empty mode list.
    void request_available_modes(uint8_t target_sysid, uint8_t target_compid);

    // Ask for one specific 1-based mode_index. This is the only form ArduPilot
    // honours per-mode, so a full list means walking 1..number_modes.
    void request_available_mode(uint8_t target_sysid, uint8_t target_compid,
                                uint8_t mode_index);

    // Send any MAV_CMD as a COMMAND_LONG. The named commands above cover what
    // the GCS itself issues; this is the escape hatch plugins (plugins/) and
    // payload-specific code use to reach commands the UI does not model.
    void command_long(uint8_t target_sysid, uint8_t target_compid, uint16_t cmd,
                      float p1 = 0.f, float p2 = 0.f, float p3 = 0.f,
                      float p4 = 0.f, float p5 = 0.f, float p6 = 0.f,
                      float p7 = 0.f);

    // MAV_CMD_REQUEST_MESSAGE (512) — ask a component to send one message now.
    // The general form of the two named requests above; used for the ones a
    // vehicle only sends when asked, such as AUTOPILOT_VERSION (148) and
    // COMPONENT_INFORMATION (395).
    //
    // Addressed like everything else here, which for this command matters: a
    // compass or IMU with its own MAV_COMP_ID answers for itself, and asking
    // the autopilot gets the autopilot's answer.
    void request_message(uint8_t target_sysid, uint8_t target_compid,
                         uint16_t message_id);

    // ── Sensor calibration ───────────────────────────────────────────────────

    // MAV_CMD_PREFLIGHT_CALIBRATION (241). One sensor per message: the command
    // takes a parameter per sensor and the spec is explicit that all the others
    // must be zero, so this is the raw form and the named helpers below are the
    // only shapes the GCS itself sends.
    void preflight_calibration(uint8_t target_sysid, uint8_t target_compid,
                               float gyro = 0.f, float mag = 0.f, float baro = 0.f,
                               float rc = 0.f, float accel = 0.f, float airspeed = 0.f,
                               float esc = 0.f);

    // Begin the six-position accelerometer calibration: PREFLIGHT_CALIBRATION
    // with the accelerometer parameter at 1.
    //
    // The vehicle then drives the exchange. It asks for each orientation twice
    // over — once as a STATUSTEXT for the operator to read, once as a
    // MAV_CMD_ACCELCAL_VEHICLE_POS command carrying the position as an enum —
    // and waits for send_accelcal_vehicle_pos() before moving to the next.
    void calibrate_accelerometer(uint8_t target_sysid, uint8_t target_compid);

    // Begin a gyroscope calibration: PREFLIGHT_CALIBRATION with the gyro
    // parameter at 1.
    //
    // Unlike the accelerometer there is no exchange to follow — the vehicle
    // samples its own gyros while it sits still, then answers with the
    // COMMAND_ACK for this command. It must not be moved while it does.
    void calibrate_gyroscope(uint8_t target_sysid, uint8_t target_compid);

    // Begin a magnetometer calibration the way ArduPilot expects one:
    // MAV_CMD_DO_START_MAG_CAL (42424), the command written for this job.
    //
    // Its parameters, and the ones chosen here:
    //   param1  bitmask of compasses; 0 is "every one you can start"
    //   param2  retry on failure         — 1, so a bad run is retried onboard
    //   param3  autosave                 — 0, so the offsets wait to be accepted
    //   param4  delay before starting    — 0
    //   param5  autoreboot               — 0, never: rebooting is the operator's
    //
    // autosave is deliberately off. The vehicle calculates the offsets and holds
    // them, and nothing is written until accept_mag_cal() below — so a fit the
    // operator can see and dislike is one they can still walk away from.
    void start_mag_cal(uint8_t target_sysid, uint8_t target_compid);

    // MAV_CMD_DO_ACCEPT_MAG_CAL (42425) — commit the offsets from the run that
    // just finished. param1 is a bitmask of compasses; 0 is all of them, which
    // is what a calibration of all of them accepts.
    void accept_mag_cal(uint8_t target_sysid, uint8_t target_compid);

    // MAV_CMD_DO_CANCEL_MAG_CAL (42426) — stop a running compass calibration
    // and keep the old offsets. Its own command: the all-zero
    // PREFLIGHT_CALIBRATION below is how the accelerometer routine is
    // withdrawn, and would leave this one running.
    void cancel_mag_cal(uint8_t target_sysid, uint8_t target_compid);

    // Begin a magnetometer calibration the portable way: PREFLIGHT_CALIBRATION
    // with the magnetometer parameter at 1.
    //
    // For PX4, which has no DO_START_MAG_CAL. ArduPilot accepts this too and
    // starts the same routine, but with autosave on and nothing to accept, so
    // the panel uses start_mag_cal() there and keeps this for the other stack.
    void calibrate_magnetometer(uint8_t target_sysid, uint8_t target_compid);

    // Abandon a calibration in progress: PREFLIGHT_CALIBRATION with every
    // parameter zero, which is how a GCS withdraws the request.
    void cancel_calibration(uint8_t target_sysid, uint8_t target_compid);

    // MAV_CMD_ACCELCAL_VEHICLE_POS (42429) — "the vehicle is now in this
    // position". `position` is an ACCELCAL_VEHICLE_POS value.
    void send_accelcal_vehicle_pos(uint8_t target_sysid, uint8_t target_compid,
                                   uint32_t position);

    // MAV_CMD_PREFLIGHT_REBOOT_SHUTDOWN (246), param1=1 — restart the
    // autopilot. ArduPilot stores new accelerometer offsets but keeps flying on
    // the ones it booted with, so a calibration is not in force until this.
    //
    // Expect no reply and expect the link to drop: the vehicle stops answering
    // immediately, and over USB it takes its serial device with it.
    void reboot_autopilot(uint8_t target_sysid, uint8_t target_compid);

    // Request all parameters (PARAM_REQUEST_LIST #21).
    // FC responds with a stream of PARAM_VALUE (#22) messages.
    void request_param_list(uint8_t target_sysid, uint8_t target_compid);

    // Counter bumped every time a PARAM_REQUEST_LIST is queued, by whoever
    // queues it — the PARAMS tab, a plugin, or the link thread's own retry.
    //
    // The link thread watches it to know a bulk fetch is genuinely in flight,
    // which cannot be inferred from the PARAM_VALUEs arriving. A vehicle
    // volunteers a PARAM_VALUE for every parameter it changes, and this GCS
    // causes such a change itself: SET_MESSAGE_INTERVAL writes the vehicle's
    // stream-rate parameters, so configuring telemetry rates on connect draws
    // back an unsolicited PARAM_VALUE whose param_count field is the vehicle's
    // whole parameter total. Counting arrivals alone, that is indistinguishable
    // from a fetch that stalled one message in.
    uint32_t param_list_seq() const;

    // Re-request a single parameter by index (PARAM_REQUEST_READ #20).
    // Used to fill gaps caused by UDP packet loss during a bulk fetch.
    void request_param_read(uint8_t target_sysid, uint8_t target_compid,
                            int16_t param_index);

    // Write a single parameter (PARAM_SET #23).
    // FC echoes the accepted value back as a PARAM_VALUE message.
    void set_param(uint8_t target_sysid, uint8_t target_compid,
                   const char* param_id, float value, uint8_t param_type);

    // Write a single parameter through the extended protocol (PARAM_EXT_SET
    // #323). Same shape as set_param() deliberately — a caller swaps one for
    // the other and changes nothing else — but `param_type` is a
    // MAV_PARAM_EXT_TYPE rather than a MAV_PARAM_TYPE, and the reply is a
    // PARAM_EXT_ACK (#324) naming the parameter and its verdict rather than a
    // bare PARAM_VALUE echo.
    //
    // The value is carried in the message's 128-byte payload in the type's own
    // byte representation, which is what makes this protocol able to write
    // integers a float cannot hold exactly. This signature still takes a float,
    // for symmetry with set_param() and because everything the GCS writes
    // through it is well inside float's exact-integer range; a caller needing
    // the full width of an INT64 parameter wants a different entry point.
    void set_param_ext(uint8_t target_sysid, uint8_t target_compid,
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

    // Send TIMESYNC (#111), either half of the exchange. A request is
    // tc1 = 0, ts1 = our clock, broadcast (target ids 0); a reply carries our
    // clock in tc1, the requester's ts1 back unchanged, and is addressed to
    // that requester as the spec requires. See backend/timesync.hpp — this
    // only puts the numbers on the wire.
    void send_timesync(uint8_t target_sysid, uint8_t target_compid,
                       int64_t tc1, int64_t ts1);

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

    // Drain all queued frames to a connected TCP socket.
    void flush_stream(int fd);

    // Drain all queued frames to an open serial port.
    void flush_serial(SerialHandle h);

    bool has_pending() const;

    // ── ACK feedback ──────────────────────────────────────────────────────────

    // Called from recv thread when COMMAND_ACK arrives
    void notify_ack(uint16_t command, uint8_t result);

    // Called from UI thread to colour a button; returns Normal when no entry
    // Flash duration: ~600 ms for Accepted/Rejected, then returns to Normal
    CmdFlashState query_flash(uint16_t cmd) const;

private:
    // Takes an already-acquired channel index, or -1 when none was free. The
    // public constructor delegates here so both channel members are set from a
    // single acquisition rather than from a shared scratch variable.
    explicit MavlinkSender(int acquired);

    // Serialise a COMMAND_LONG and push onto the queue
    void enqueue_command_long(uint8_t tsys, uint8_t tcomp, uint16_t cmd,
                              float p1 = 0.f, float p2 = 0.f, float p3 = 0.f,
                              float p4 = 0.f, float p5 = 0.f, float p6 = 0.f,
                              float p7 = 0.f);

    struct CmdState {
        CmdFlashState state = CmdFlashState::Normal;
        std::chrono::steady_clock::time_point changed_at{};
    };

    // This sender's MAVLink TX channel.
    //
    // Every mavlink_msg_*_pack() funnels through mavlink_finalize_message(),
    // which is hardcoded to MAVLINK_COMM_0 and increments *that* channel's
    // sequence counter — a global. Two senders packing at once would therefore
    // race, and worse, would share one sequence number: each vehicle counts the
    // gaps in the GCS's sequence to estimate link quality, so a shared counter
    // reads to every vehicle as heavy packet loss. A channel each fixes both.
    const uint8_t                           chan_;

    // True when channels ran out and this sender had to share channel 0. Packing
    // then goes through a process-wide mutex, since the sequence counter behind
    // that channel is no longer ours alone.
    const bool                              shared_chan_;

    mutable std::mutex                      mtx_;
    std::queue<std::vector<uint8_t>>        queue_;
    std::atomic<uint32_t>                   param_list_seq_ { 0 };
    mutable std::unordered_map<uint16_t, CmdState> cmd_states_;

    std::vector<MissionItem>                upload_items_;
};
