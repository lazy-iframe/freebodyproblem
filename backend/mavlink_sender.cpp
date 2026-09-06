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


#include "mavlink_sender.hpp"

#ifdef _WIN32
#  include <winsock2.h>
#else
#  include <sys/socket.h>
#  include <unistd.h>
#endif

// Winsock has no write() for sockets; send() is the portable spelling and works
// identically on POSIX. Serial goes through serial_write() instead — on Windows
// a port is a HANDLE, not a socket, so the two paths cannot share a call.
static inline int socket_write(int fd, const char* buf, int len)
{
#ifdef _WIN32
    return ::send(static_cast<SOCKET>(fd), buf, len, 0);
#else
    return static_cast<int>(::send(fd, buf, len, 0));
#endif
}

#include <chrono>
#include <cstdio>
#include <cstddef>
#include <cstring>
#include <bitset>
#include <mutex>
#include <mavlink/ardupilotmega/mavlink.h>
#include "../frontend/app_log.hpp"   // gcs_log — thread-safe, see the header

namespace {

// MAVLink TX channel allocation.
//
// A channel is an index into the library's global array of per-channel status,
// which is where the outgoing sequence counter lives. Handing each sender its
// own means two senders never touch the same counter; the free list exists so a
// vehicle that comes and goes over a session does not exhaust the sixteen.
//
// Channel 0 is the shared fallback once they run out. Senders on it serialise
// their packing through pack_mutex() below, because sharing a channel means
// sharing that sequence counter and an unguarded increment of it is a race.
// Callers are expected to cap vehicle count below MAX_CHANNELS so this stays
// unreachable; it exists so that exceeding the cap degrades rather than breaks.
std::mutex&                         chan_mutex()
{
    static std::mutex m;
    return m;
}

std::bitset<MavlinkSender::MAX_CHANNELS>& chan_taken()
{
    static std::bitset<MavlinkSender::MAX_CHANNELS> b;
    return b;
}

// Serialises packing for senders that had to share channel 0.
std::mutex& pack_mutex()
{
    static std::mutex m;
    return m;
}

// Held only by a sender sharing channel 0; a no-op lock otherwise.
struct PackLock {
    std::unique_lock<std::mutex> lk;
    explicit PackLock(bool shared)
    {
        if (shared) lk = std::unique_lock<std::mutex>(pack_mutex());
    }
};

// Returns the channel, or -1 when none is free and the caller must share 0.
int acquire_channel()
{
    std::lock_guard<std::mutex> lk(chan_mutex());
    auto& taken = chan_taken();
    for (size_t i = 0; i < taken.size(); ++i) {
        if (!taken.test(i)) { taken.set(i); return static_cast<int>(i); }
    }
    return -1;
}

void release_channel(uint8_t chan, bool shared)
{
    if (shared) return;
    std::lock_guard<std::mutex> lk(chan_mutex());
    auto& taken = chan_taken();
    if (chan < taken.size()) taken.reset(chan);
}

} // namespace

MavlinkSender::MavlinkSender() : MavlinkSender(acquire_channel()) {}

MavlinkSender::MavlinkSender(int acquired)
    : chan_(acquired < 0 ? uint8_t{0} : static_cast<uint8_t>(acquired))
    , shared_chan_(acquired < 0)
{
    if (shared_chan_)
        std::fprintf(stderr,
                     "[mavlink] TX channels exhausted (%d); sharing channel 0\n",
                     MAX_CHANNELS);
}

MavlinkSender::~MavlinkSender() { release_channel(chan_, shared_chan_); }


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
}

void MavlinkSender::request_autopilot_capabilities(uint8_t tsys, uint8_t tcomp)
{
    // MAV_CMD_REQUEST_AUTOPILOT_CAPABILITIES (520), param1=1 (request)
    enqueue_command_long(tsys, tcomp, 520, 1.f);
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


void MavlinkSender::request_param_list(uint8_t tsys, uint8_t tcomp)
{
    // PARAM_REQUEST_LIST (#21) — FC responds with a PARAM_VALUE stream
    mavlink_message_t msg;
    PackLock _pl(shared_chan_);
    mavlink_msg_param_request_list_pack_chan(GCS_SYSID, GCS_COMPID, chan_, &msg, tsys, tcomp);

    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    const uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);

    param_list_seq_.fetch_add(1, std::memory_order_release);

    std::lock_guard<std::mutex> lk(mtx_);
    queue_.emplace(buf, buf + len);
}

uint32_t MavlinkSender::param_list_seq() const
{
    return param_list_seq_.load(std::memory_order_acquire);
}

void MavlinkSender::request_param_read(uint8_t tsys, uint8_t tcomp,
                                        int16_t param_index)
{
    // PARAM_REQUEST_READ (#20) — empty param_id means "look up by index"
    mavlink_message_t msg;
    PackLock _pl(shared_chan_);
    mavlink_msg_param_request_read_pack_chan(GCS_SYSID, GCS_COMPID, chan_, &msg,
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
    PackLock _pl(shared_chan_);
    mavlink_msg_param_set_pack_chan(GCS_SYSID, GCS_COMPID, chan_, &msg,
                                    tsys, tcomp, param_id, value, param_type);

    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    const uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);

    std::lock_guard<std::mutex> lk(mtx_);
    queue_.emplace(buf, buf + len);
}

// PARAM_EXT_SET carries its value as raw bytes rather than a float, so the
// caller's number has to be narrowed to the parameter's declared type first and
// then copied out little-endian — MAVLink is little-endian on the wire, and the
// payload is opaque bytes that no field-level byte swap will touch.
static void encode_param_ext_value(char out[128], float value, uint8_t ext_type)
{
    std::memset(out, 0, 128);

    auto put = [&](const void* src, size_t n) { std::memcpy(out, src, n); };

    switch (ext_type) {
    case MAV_PARAM_EXT_TYPE_UINT8:  { uint8_t  v = (uint8_t) value; put(&v, 1); break; }
    case MAV_PARAM_EXT_TYPE_INT8:   { int8_t   v = (int8_t)  value; put(&v, 1); break; }
    case MAV_PARAM_EXT_TYPE_UINT16: { uint16_t v = (uint16_t)value; put(&v, 2); break; }
    case MAV_PARAM_EXT_TYPE_INT16:  { int16_t  v = (int16_t) value; put(&v, 2); break; }
    case MAV_PARAM_EXT_TYPE_UINT32: { uint32_t v = (uint32_t)value; put(&v, 4); break; }
    case MAV_PARAM_EXT_TYPE_INT32:  { int32_t  v = (int32_t) value; put(&v, 4); break; }
    case MAV_PARAM_EXT_TYPE_UINT64: { uint64_t v = (uint64_t)value; put(&v, 8); break; }
    case MAV_PARAM_EXT_TYPE_INT64:  { int64_t  v = (int64_t) value; put(&v, 8); break; }
    case MAV_PARAM_EXT_TYPE_REAL64: { double   v = (double)  value; put(&v, 8); break; }
    case MAV_PARAM_EXT_TYPE_CUSTOM:
        // CUSTOM is an opaque string the component defines for itself. There is
        // no meaning to give a float here, so the payload stays zeroed rather
        // than inventing a text encoding the receiver never agreed to.
        break;
    case MAV_PARAM_EXT_TYPE_REAL32:
    default:                        { float    v =           value; put(&v, 4); break; }
    }
}

void MavlinkSender::set_param_ext(uint8_t tsys, uint8_t tcomp,
                                   const char* param_id, float value,
                                   uint8_t param_type)
{
    char payload[128];
    encode_param_ext_value(payload, value, param_type);

    mavlink_message_t msg;
    PackLock _pl(shared_chan_);
    mavlink_msg_param_ext_set_pack_chan(GCS_SYSID, GCS_COMPID, chan_, &msg,
                                        tsys, tcomp, param_id, payload, param_type);

    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    const uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);

    std::lock_guard<std::mutex> lk(mtx_);
    queue_.emplace(buf, buf + len);
}

void MavlinkSender::request_mission_list(uint8_t tsys, uint8_t tcomp)
{
    mavlink_message_t msg;
    PackLock _pl(shared_chan_);
    mavlink_msg_mission_request_list_pack_chan(GCS_SYSID, GCS_COMPID, chan_, &msg,
                                                tsys, tcomp, MAV_MISSION_TYPE_MISSION);
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    const uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
    std::lock_guard<std::mutex> lk(mtx_);
    queue_.emplace(buf, buf + len);
}

void MavlinkSender::request_mission_item(uint8_t tsys, uint8_t tcomp, uint16_t seq)
{
    mavlink_message_t msg;
    PackLock _pl(shared_chan_);
    mavlink_msg_mission_request_int_pack_chan(GCS_SYSID, GCS_COMPID, chan_, &msg,
                                              tsys, tcomp, seq, MAV_MISSION_TYPE_MISSION);
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    const uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
    std::lock_guard<std::mutex> lk(mtx_);
    queue_.emplace(buf, buf + len);
}

void MavlinkSender::send_mission_ack(uint8_t tsys, uint8_t tcomp)
{
    mavlink_message_t msg;
    PackLock _pl(shared_chan_);
    mavlink_msg_mission_ack_pack_chan(GCS_SYSID, GCS_COMPID, chan_, &msg,
                                       tsys, tcomp,
                                       MAV_MISSION_ACCEPTED,
                                       MAV_MISSION_TYPE_MISSION,
                                       0);
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    const uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
    std::lock_guard<std::mutex> lk(mtx_);
    queue_.emplace(buf, buf + len);
}

void MavlinkSender::send_timesync(uint8_t tsys, uint8_t tcomp,
                                  int64_t tc1, int64_t ts1)
{
    mavlink_message_t msg;
    PackLock _pl(shared_chan_);
    mavlink_msg_timesync_pack_chan(GCS_SYSID, GCS_COMPID, chan_, &msg,
                                   tc1, ts1, tsys, tcomp);
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    const uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
    std::lock_guard<std::mutex> lk(mtx_);
    queue_.emplace(buf, buf + len);
}

void MavlinkSender::start_upload(uint8_t tsys, uint8_t tcomp,
                                  std::vector<MissionItem> items)
{
    mavlink_message_t msg;
    PackLock _pl(shared_chan_);
    mavlink_msg_mission_count_pack_chan(GCS_SYSID, GCS_COMPID, chan_, &msg,
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
    PackLock _pl(shared_chan_);
    mavlink_msg_mission_item_int_pack_chan(
        GCS_SYSID, GCS_COMPID, chan_, &msg,
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
    PackLock _pl(shared_chan_);
    mavlink_msg_command_long_pack_chan(
        GCS_SYSID, GCS_COMPID, chan_, &msg,
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

// A frame that could not be handed to the transport at all stays queued for the
// next pass rather than being thrown away.
//
// Both the serial port and the sockets now refuse a write instead of blocking
// on one, which is what keeps a busy autopilot from taking the link thread down
// with it. That turns "the write blocked" into "the write did nothing", and
// popping the frame anyway would quietly lose commands exactly when the link is
// under strain — a calibration confirmation dropped there leaves the vehicle
// waiting for an answer that was never sent.
//
// A partial write is different and is not retried: the transport already has
// the first half of that frame, so re-sending the whole thing would put a
// malformed message on the wire. The frame is dropped and the parser at the far
// end discards the fragment on its checksum.
static bool frame_sent(int written, std::size_t size)
{
    return written > 0 || size == 0;
}

void MavlinkSender::flush_stream(int fd)
{
    std::lock_guard<std::mutex> lk(mtx_);
    while (!queue_.empty()) {
        const auto& frame = queue_.front();
        const int n = socket_write(fd,
                                   reinterpret_cast<const char*>(frame.data()),
                                   static_cast<int>(frame.size()));
        if (!frame_sent(n, frame.size())) break;
        queue_.pop();
    }
}

void MavlinkSender::flush_serial(SerialHandle h)
{
    std::lock_guard<std::mutex> lk(mtx_);
    while (!queue_.empty()) {
        const auto& frame = queue_.front();
        const int n = serial_write(h, frame.data(), frame.size());
        if (!frame_sent(n, frame.size())) break;
        queue_.pop();
    }
}

void MavlinkSender::flush(int fd, const sockaddr_in& dest)
{
    std::lock_guard<std::mutex> lk(mtx_);
    while (!queue_.empty()) {
        const auto& frame = queue_.front();
        const int n = (int)sendto(fd,
                                  reinterpret_cast<const char*>(frame.data()),
                                  static_cast<int>(frame.size()),
                                  0,
                                  reinterpret_cast<const sockaddr*>(&dest),
                                  sizeof(dest));
        if (!frame_sent(n, frame.size())) break;
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
    PackLock _pl(shared_chan_);
    mavlink_msg_command_long_pack_chan(
        GCS_SYSID, GCS_COMPID, chan_, &msg,
        tsys, tcomp,
        cmd,
        0,              // confirmation (0 = first transmission)
        p1, p2, p3, p4, p5, p6, p7
    );

    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    const uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);

    // Compass calibration is traced end to end while it is being brought up:
    // what went out, with the parameters it actually carried, so the log can be
    // read against what the vehicle answers. See mavlink_parser.cpp for the
    // receiving half and for the switch that quietens both.
    if (cmd == 42424 || cmd == 42425 || cmd == 42426 || cmd == 241) {
        const char* which = cmd == 241   ? "PREFLIGHT_CALIBRATION"
                          : cmd == 42424 ? "DO_START_MAG_CAL"
                          : cmd == 42425 ? "DO_ACCEPT_MAG_CAL"
                                         : "DO_CANCEL_MAG_CAL";
        gcs_log("magcal tx: %s (%u) to %u/%u "
                "p1=%g p2=%g p3=%g p4=%g p5=%g p6=%g p7=%g",
                which, (unsigned)cmd, (unsigned)tsys, (unsigned)tcomp,
                (double)p1, (double)p2, (double)p3, (double)p4,
                (double)p5, (double)p6, (double)p7);
    }

    std::lock_guard<std::mutex> lk(mtx_);
    queue_.emplace(buf, buf + len);
    cmd_states_[cmd] = { CmdFlashState::Pending, Clock::now() };
}
