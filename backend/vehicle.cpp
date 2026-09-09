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


#include "vehicle.hpp"
#include "firmware_profile.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <unordered_set>

#include "timesync.hpp"
#include "../frontend/app_log.hpp"   // gcs_log — thread-safe, see the header
#include "../frontend/audio.hpp"     // gcs_tone

using Clock = std::chrono::steady_clock;

namespace {

// A vehicle that has not been heard from in this long is shown as stale. The
// value matches the link's own silence timeout, and for the same reason given
// there: an autopilot busy with a sensor calibration answers slowly, and
// fifteen missed heartbeats is a vehicle that has gone rather than one that is
// thinking.
constexpr double VEHICLE_STALE_S = 15.0;

// How often the published snapshot is refreshed. The old link thread republished
// on every datagram, which was affordable when there was one vehicle and one
// copy. A snapshot carries the parameter table and the inspector's cache of the
// last raw message per ID — the latter alone is a few hundred bytes times the
// number of distinct message IDs seen — and at 20 Hz per vehicle the UI still
// updates faster than anyone can read it.
constexpr double PUBLISH_INTERVAL_S = 0.05;

int64_t now_ns()
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               Clock::now().time_since_epoch()).count();
}

} // namespace


Vehicle::Vehicle(VehicleId id, uint8_t autopilot_compid, uint32_t number)
    : id_(id)
    , number_(number)
    , autopilot_compid_(autopilot_compid)
    , parser_(id.sysid, autopilot_compid)
{}

Vehicle::~Vehicle() { stop(); }

void Vehicle::start()
{
    if (running_.exchange(true)) return;
    thread_ = std::thread(&Vehicle::loop, this);
}

void Vehicle::stop()
{
    if (!running_.exchange(false)) {
        if (thread_.joinable()) thread_.join();
        return;
    }
    inbox_cv_.notify_all();
    if (thread_.joinable()) thread_.join();
}

// ── Link thread side ──────────────────────────────────────────────────────────

void Vehicle::deliver(const mavlink_message_t& msg)
{
    bool first_drop = false;
    {
        std::lock_guard<std::mutex> lk(inbox_mtx_);
        if (inbox_.size() >= INBOX_MAX) {
            // Drop the oldest rather than the newest: what a vehicle is doing
            // now matters more than what it was doing four thousand messages
            // ago, and a backlog this deep is stale by definition.
            inbox_.erase(inbox_.begin());
            first_drop =
                inbox_dropped_.fetch_add(1, std::memory_order_relaxed) == 0;
        }
        inbox_.push_back(msg);
    }
    inbox_cv_.notify_one();

    // Outside the lock: this writes to the log and to stderr, and the link
    // thread should not hold a vehicle's inbox shut while it does. Said once
    // rather than per message — a wedged protocol thread would otherwise turn a
    // silent data loss into a flooded log, and what the operator needs to know
    // is that it happened, not how often.
    if (first_drop)
        gcs_log("[sys%u] falling behind — telemetry is being dropped",
                (unsigned)id_.sysid);
}

void Vehicle::set_peer(const sockaddr_in& addr)
{
    std::lock_guard<std::mutex> lk(peer_mtx_);
    peer_     = addr;
    peer_set_ = true;
}

bool Vehicle::peer(sockaddr_in& out) const
{
    std::lock_guard<std::mutex> lk(peer_mtx_);
    if (!peer_set_) return false;
    out = peer_;
    return true;
}

void Vehicle::set_link_counters(uint64_t bytes, uint64_t parse_errors)
{
    link_bytes_.store(bytes, std::memory_order_relaxed);
    link_parse_errors_.store(parse_errors, std::memory_order_relaxed);
}

// ── UI thread side ────────────────────────────────────────────────────────────

void Vehicle::snapshot(VehicleSnapshot& out) const
{
    std::lock_guard<std::mutex> lk(snap_mtx_);
    out = snap_;
}

VehicleChip Vehicle::chip() const
{
    VehicleChip c;
    c.id     = id_;
    c.number = number_;
    c.compid = autopilot_compid_;
    c.stale  = stale();
    {
        std::lock_guard<std::mutex> lk(snap_mtx_);
        c.type          = static_cast<uint8_t>(snap_.state.type);
        c.armed         = snap_.state.armed;
        c.has_heartbeat = snap_.state.has_heartbeat;
    }
    return c;
}

bool Vehicle::stale() const
{
    const int64_t last = last_heartbeat_ns_.load(std::memory_order_relaxed);
    if (last == 0) return false;   // nothing heard yet is not the same as lost
    return (now_ns() - last) > static_cast<int64_t>(VEHICLE_STALE_S * 1e9);
}

void Vehicle::set_tones_enabled(bool on)
{
    tones_enabled_.store(on, std::memory_order_relaxed);
}

uint64_t Vehicle::inbox_dropped() const
{
    return inbox_dropped_.load(std::memory_order_relaxed);
}

void Vehicle::publish(bool param_fetch_active, bool republish_params)
{
    const VehicleState& ps = parser_.state();

    std::lock_guard<std::mutex> lk(snap_mtx_);
    snap_.state = ps;
    // Owned by this thread, not the parser — see the field's note.
    snap_.state.param_fetch_active = param_fetch_active;
    snap_.msg_stats      = parser_.msg_stats();
    snap_.total_messages = parser_.total_messages();
    snap_.total_bytes    = link_bytes_.load(std::memory_order_relaxed);
    snap_.parse_errors   = link_parse_errors_.load(std::memory_order_relaxed);
    snap_.status_texts.assign(parser_.status_texts().begin(),
                              parser_.status_texts().end());

    // The parameter table is republished only when it has actually changed, so
    // the table on screen keeps showing the values it has while a new fetch
    // fills in behind it rather than blanking.
    if (republish_params) {
        snap_.params            = parser_.params();
        snap_.params_generation = ps.params_generation;
    }
}

// ── Protocol thread ───────────────────────────────────────────────────────────
//
// What used to be the second half of the link thread. Every conversation a GCS
// has to keep up with one vehicle lives here: asking for stream rates, walking
// the mode list, keeping the clocks aligned, and driving a parameter fetch
// through the stalls and losses a real radio link inflicts on it.
//
// All of the state below is a function local, exactly as it was when this ran
// on the link thread. That is not laziness — it is what makes it thread-safe by
// construction: nothing here is reachable from another thread, so none of it
// needs a lock, and the only things that do are the inbox, the snapshot and the
// send queue, each with its own.

void Vehicle::loop()
{
    const uint8_t tsys  = id_.sysid;
    const uint8_t tcomp = autopilot_compid_;

    bool rates_requested = false;

    // AVAILABLE_MODES enumeration. The spec's "request index 0 for all modes"
    // form is not what ArduPilot implements — it replies with index 1 alone —
    // so the list has to be walked one index at a time. The index-0 probe is
    // still worth sending first because its number_modes field is how we learn
    // how far to walk. Sweeps are repeated to refill gaps left by packet loss;
    // when they run out the UI falls back to its built-in tables.
    auto     modes_last_req = Clock::now();
    int      modes_cursor   = 0;      // highest index requested this sweep
    int      modes_sweeps   = 0;
    int      modes_probes   = 0;
    bool     modes_logged   = false;  // one-shot completion log
    constexpr int    MODES_MAX_SWEEPS = 3;
    constexpr int    MODES_MAX_PROBES = 3;
    constexpr double MODES_REQ_S      = 0.10;  // pacing between index requests
    constexpr double MODES_PROBE_S    = 2.0;   // retry interval for the probe

    uint32_t last_params_gen       = 0;
    uint32_t known_params_gen      = 0;   // thread-local; avoids shared reads
    bool     params_complete_latch = false;
    auto     last_param_time       = Clock::now();

    // Bulk parameter fetch. `param_fetch_active` follows the PARAM_REQUEST_LIST
    // this GCS sent, never the PARAM_VALUEs coming back — see the note on
    // MavlinkSender::param_list_seq(). Everything below only runs inside a fetch
    // somebody actually asked for.
    uint32_t seen_param_list_seq = 0;
    bool     param_fetch_active  = false;
    int      param_stall_rounds  = 0;
    uint16_t param_stall_mark    = 0;

    size_t   last_status_count = 0;
    bool     logged_heartbeat  = false;
    bool     logged_fw_info    = false;
    bool     logged_mission    = false;
    bool     logged_timesync   = false;
    VehicleState::UploadStatus last_upload_status = VehicleState::UploadStatus::Idle;

    // Far enough back that the first exchange goes out as soon as the thread
    // starts rather than five seconds into the vehicle's life.
    auto last_timesync_req = Clock::now() - std::chrono::hours(1);
    auto last_publish      = Clock::now() - std::chrono::hours(1);

    std::vector<mavlink_message_t> batch;

    while (running_.load(std::memory_order_relaxed)) {
        // Wait for work, but never for long: the mode sweep, the clock sync and
        // the parameter stall detector are all timers, and they have to keep
        // ticking on a vehicle that has gone quiet.
        {
            std::unique_lock<std::mutex> lk(inbox_mtx_);
            if (inbox_.empty()) {
                inbox_cv_.wait_for(lk, std::chrono::milliseconds(50), [this] {
                    return !inbox_.empty() ||
                           !running_.load(std::memory_order_relaxed);
                });
            }
            batch.swap(inbox_);
            inbox_.clear();
        }
        if (!running_.load(std::memory_order_relaxed)) break;

        for (const auto& msg : batch) {
            parser_.handle(msg);
            if (msg.msgid == MAVLINK_MSG_ID_HEARTBEAT &&
                msg.compid == autopilot_compid_)
                last_heartbeat_ns_.store(now_ns(), std::memory_order_relaxed);
        }
        batch.clear();

        // ── Connect burst ─────────────────────────────────────────────────────
        //
        // Addressed to this vehicle rather than to the hardcoded (1,1) the link
        // thread used. That was harmless with one vehicle whose sysid was almost
        // always 1, and wrong the moment a second one exists.
        //
        // Gated on a heartbeat having been parsed, not just on the vehicle
        // existing. This loop's first pass can come from the 50 ms wait_for
        // timeout with an empty batch, and until a heartbeat lands the parser
        // does not know which stack this is — which now decides what the burst
        // asks for. Costs at most one heartbeat interval.
        if (!rates_requested && parser_.state().has_heartbeat) {
            rates_requested = true;

            const FirmwareProfile& prof = firmware_profile(parser_.state().autopilot);

            sender_.request_message_interval(tsys, tcomp,  30,  50000); // ATTITUDE            @ 20 Hz
            sender_.request_message_interval(tsys, tcomp,  74, 100000); // VFR_HUD             @ 10 Hz
            sender_.request_message_interval(tsys, tcomp,  33, 200000); // GLOBAL_POSITION_INT @  5 Hz
            sender_.request_message_interval(tsys, tcomp,   1, 500000); // SYS_STATUS          @  2 Hz
            sender_.request_message_interval(tsys, tcomp,  24, 500000); // GPS_RAW_INT         @  2 Hz
            // Estimator health @ 5 Hz — EKF_STATUS_REPORT (#193) on ArduPilot,
            // ESTIMATOR_STATUS (#230) on PX4, which has no #193 to give.
            sender_.request_message_interval(tsys, tcomp,
                                             prof.estimator_msgid(), 200000);
            // HOME_POSITION at 0.5 Hz. A vehicle announces home when it sets or
            // moves it, but home is usually already set by the time a GCS
            // connects — and that announcement is long gone. Streaming it
            // slowly costs nothing and means the map has home from the first
            // couple of seconds, however late the operator joined.
            sender_.request_message_interval(tsys, tcomp, 242, 2000000); // HOME_POSITION       @0.5 Hz
            // RC_CHANNELS at 10 Hz. Fast enough that a calibration sweep catches
            // a stick's true stop rather than wherever it happened to be
            // sampled, and that the RC panel's bars track the sticks instead of
            // stepping after them.
            sender_.request_message_interval(tsys, tcomp,  65, 100000); // RC_CHANNELS         @ 10 Hz
            sender_.request_autopilot_capabilities(tsys, tcomp);
            sender_.request_available_modes(tsys, tcomp);
            modes_last_req = Clock::now();
            ++modes_probes;
            gcs_log("[sys%u] telemetry rates configured (%s)", tsys, prof.name());
        }

        // ── Command ACKs ──────────────────────────────────────────────────────
        for (const auto& ack : parser_.pending_acks()) {
            sender_.notify_ack(ack.command, ack.result);
            const char* cname = nullptr;
            switch (ack.command) {
            case 400: cname = "arm/disarm";   break;
            case  22: cname = "takeoff";      break;
            case  20: cname = "RTL";          break;
            case 176: cname = "mode change";  break;
            // DO_SET_STANDARD_MODE — the other way a mode gets set, and the
            // only one that reaches PX4's standard modes.
            case 262: cname = "mode change";  break;
            case 218: cname = "aux function"; break;
            case 192: cname = "reposition";   break;
            default:  break;
            }
            // Tone only for commands with a name above, i.e. the ones a person
            // pressed a button for. The rate and capability requests this GCS
            // fires on connect ACK too, and a burst of beeps at every connect
            // teaches the operator to ignore the sound. Gated on this being the
            // vehicle on screen for the same reason, one fleet-sized step up.
            if (cname && tones_enabled_.load(std::memory_order_relaxed))
                gcs_tone(ack.result == 0 ? GcsTone::Success : GcsTone::Failure);

            if (cname) {
                const char* result = (ack.result == 0) ? "accepted"
                                   : (ack.result == 1) ? "temp. rejected"
                                   : (ack.result == 2) ? "denied"
                                   : (ack.result == 4) ? "failed"
                                                       : "rejected";
                gcs_log("[sys%u] ack: %s \xe2\x86\x92 %s", tsys, cname, result);
            }
        }
        parser_.clear_acks();

        // PARAM_EXT_ACK — the extended parameter protocol's reply. Unlike a
        // PARAM_SET, which is answered with a PARAM_VALUE echo the table picks
        // up on its own, this one goes nowhere unless it is logged: it names the
        // parameter and says outright whether the write took.
        for (const auto& pa : parser_.pending_param_ext_acks()) {
            const char* verdict = (pa.result == PARAM_ACK_ACCEPTED)          ? "accepted"
                                : (pa.result == PARAM_ACK_VALUE_UNSUPPORTED) ? "value unsupported"
                                : (pa.result == PARAM_ACK_FAILED)            ? "failed"
                                : (pa.result == PARAM_ACK_IN_PROGRESS)       ? "in progress"
                                                                             : "rejected";
            gcs_log("[sys%u] param_ext: %s \xe2\x86\x92 %s", tsys, pa.param_id, verdict);
            if (pa.result != PARAM_ACK_IN_PROGRESS &&
                tones_enabled_.load(std::memory_order_relaxed))
                gcs_tone(pa.result == PARAM_ACK_ACCEPTED ? GcsTone::Success
                                                         : GcsTone::Failure);
        }
        parser_.clear_param_ext_acks();

        // ── Mode list ─────────────────────────────────────────────────────────
        // Walk the vehicle's mode list one index at a time, and restart the walk
        // if AVAILABLE_MODES_MONITOR reports the list changed.
        {
            const auto& ms = parser_.state();

            if (ms.modes_dirty) {
                parser_.clear_modes_dirty();
                modes_cursor = 0;
                modes_sweeps = 0;
                modes_probes = 0;
                modes_logged = false;
                gcs_log("[sys%u] mode list changed — re-requesting", tsys);
            }

            const double since = std::chrono::duration<double>(
                                     Clock::now() - modes_last_req).count();

            if (!ms.modes_complete && modes_sweeps < MODES_MAX_SWEEPS) {
                if (ms.modes_expected == 0) {
                    // No number_modes yet: either the probe was lost or the
                    // firmware does not implement the protocol at all.
                    if (since >= MODES_PROBE_S && modes_probes < MODES_MAX_PROBES) {
                        sender_.request_available_modes(tsys, tcomp);
                        modes_last_req = Clock::now();
                        ++modes_probes;
                    }
                } else if (since >= MODES_REQ_S) {
                    // Next index we have not received yet. Indices already in
                    // hand are skipped, so a retry sweep only re-asks for the
                    // genuine gaps.
                    int next = 0;
                    for (int i = modes_cursor + 1; i <= ms.modes_expected; ++i) {
                        if (!ms.has_mode_index(static_cast<uint8_t>(i))) {
                            next = i;
                            break;
                        }
                    }
                    if (next != 0) {
                        sender_.request_available_mode(tsys, tcomp,
                                                       static_cast<uint8_t>(next));
                        modes_cursor   = next;
                        modes_last_req = Clock::now();
                    } else {
                        // Reached the end with gaps still open — sweep again.
                        modes_cursor = 0;
                        ++modes_sweeps;
                    }
                }
            }

            if (ms.modes_complete && !modes_logged) {
                modes_logged = true;
                gcs_log("[sys%u] mode list: %zu modes", tsys,
                        ms.available_modes.size());
            }
        }

        // Drain per-item mission requests generated by MISSION_COUNT /
        // MISSION_ITEM_INT handlers (one request triggers the next).
        for (const auto& req : parser_.pending_mission_reqs())
            sender_.request_mission_item(req.tsys, req.tcomp, req.seq);
        parser_.clear_mission_reqs();

        // Drain the vehicle's MISSION_REQUEST_INT during a mission upload.
        for (const auto& req : parser_.pending_item_reqs())
            sender_.send_mission_item_at(req.tsys, req.tcomp, req.seq);
        parser_.clear_item_reqs();

        // Answer TIMESYNC exchanges the vehicle opened. ArduPilot asks on its
        // own schedule as well as answering ours, and a request left unanswered
        // is a vehicle whose clock never lines up with ours.
        for (const auto& r : parser_.pending_timesync_replies())
            sender_.send_timesync(r.tsys, r.tcomp, r.tc1, r.ts1);
        parser_.clear_timesync_replies();

        const VehicleState& ps = parser_.state();

        if (!logged_timesync && ps.has_timesync) {
            logged_timesync = true;
            const int64_t up_s = (timesync_monotonic_ns() + ps.time_offset_ns)
                                 / 1000000000;
            gcs_log("[sys%u] clock synced \xe2\x80\x94 up %02d:%02d:%02d", tsys,
                    (int)(up_s / 3600), (int)((up_s / 60) % 60), (int)(up_s % 60));
        }

        // Log new vehicle STATUSTEXT messages to the bottom bar.
        {
            const auto& sts = parser_.status_texts();
            static const char* const kSev[] = {
                "EMRG","ALRT","CRIT","ERR","WARN","NOTE","INFO","DEBUG"
            };
            for (size_t i = last_status_count; i < sts.size(); ++i)
                gcs_log("[sys%u/%s] %s", tsys, kSev[sts[i].severity & 7], sts[i].text);
            last_status_count = sts.size();
        }

        if (ps.has_heartbeat && !logged_heartbeat) {
            logged_heartbeat = true;
            gcs_log("[sys%u] heartbeat: sys=%d comp=%d", tsys,
                    (int)ps.sysid, (int)ps.compid);
        }

        if (ps.has_fw_info && !logged_fw_info) {
            logged_fw_info = true;
            const char* ap = "unknown";
            if (ps.autopilot == MAV_AUTOPILOT_ARDUPILOTMEGA) {
                switch (ps.type) {
                case MAV_TYPE_FIXED_WING:   ap = "ArduPlane";  break;
                case MAV_TYPE_GROUND_ROVER: ap = "ArduRover";  break;
                case MAV_TYPE_SUBMARINE:    ap = "ArduSub";    break;
                default:                    ap = "ArduCopter"; break;
                }
            } else if (ps.autopilot == MAV_AUTOPILOT_PX4) {
                ap = "PX4";
            }
            gcs_log("[sys%u] firmware: %s %s (%.8s)", tsys, ap,
                    ps.fw_version, ps.fw_hash);
        }

        if (ps.has_mission && !logged_mission) {
            logged_mission = true;
            gcs_log("[sys%u] mission loaded: %u waypoints", tsys,
                    (unsigned)ps.mission_count);
            sender_.send_mission_ack(ps.sysid, ps.compid);
        }

        if (ps.upload_status != last_upload_status) {
            last_upload_status = ps.upload_status;
            const bool tone = tones_enabled_.load(std::memory_order_relaxed);
            if (ps.upload_status == VehicleState::UploadStatus::Accepted) {
                gcs_log("[sys%u] mission upload accepted", tsys);
                if (tone) gcs_tone(GcsTone::Success);
                sender_.clear_upload();
            } else if (ps.upload_status == VehicleState::UploadStatus::Failed) {
                gcs_log("[sys%u] mission upload failed (result=%u)", tsys,
                        (unsigned)ps.upload_ack_result);
                if (tone) gcs_tone(GcsTone::Failure);
                sender_.clear_upload();
            }
        }

        // Track last-param-arrival time for the retransmit logic below.
        if (ps.params_generation != last_params_gen) {
            last_params_gen = ps.params_generation;
            last_param_time = Clock::now();
        }

        // Completion is an edge; republishing the table is not. Both used to
        // hang off the same test, which was wrong once a fetch had finished:
        // params_generation ticks on every PARAM_VALUE, and after completion the
        // ones that keep arriving — the echo of a write, or the counters a
        // vehicle saves as it runs — all arrive with params_received still equal
        // to param_count. Every one of them therefore read as a fresh completion
        // and logged "parameters loaded" again.
        const bool params_complete = ps.param_count > 0 &&
                                     ps.params_received >= ps.param_count;
        const bool params_changed  = ps.params_generation != known_params_gen;
        const bool params_just_completed = params_complete && !params_complete_latch;
        params_complete_latch = params_complete;

        const bool republish_params = params_complete && params_changed;
        if (republish_params) known_params_gen = ps.params_generation;

        // Publish on a fixed cadence rather than on every message, but never
        // hold back a change the operator is waiting on: a completed parameter
        // fetch goes out immediately.
        const double since_pub = std::chrono::duration<double>(
                                     Clock::now() - last_publish).count();
        if (since_pub >= PUBLISH_INTERVAL_S || republish_params ||
            params_just_completed)
        {
            publish(param_fetch_active, republish_params);
            last_publish = Clock::now();
        }

        if (params_just_completed)
            gcs_log("[sys%u] parameters loaded: %u params", tsys,
                    (unsigned)ps.param_count);

        // ── Clock sync ────────────────────────────────────────────────────────
        // One exchange every few seconds, not one and done: the first can be
        // dropped by a vehicle still booting, and repeating keeps the offset from
        // drifting as the two clocks run at slightly different rates.
        //
        // Addressed, where the link thread broadcast it. A broadcast request on
        // a link carrying several vehicles draws a reply from each of them into
        // one TimeSync exchange, and the offset it computes is then an average
        // of unrelated clocks.
        {
            constexpr double TIMESYNC_INTERVAL_S = 5.0;
            const double since = std::chrono::duration<double>(
                                     Clock::now() - last_timesync_req).count();
            if (since >= TIMESYNC_INTERVAL_S) {
                last_timesync_req = Clock::now();
                const int64_t ts1 =
                    parser_.timesync().begin_request(timesync_monotonic_ns());
                sender_.send_timesync(tsys, tcomp, 0, ts1);
            }
        }

        // ── Bulk parameter fetch: arm, retransmit, give up ────────────────────
        {
            // A PARAM_REQUEST_LIST went out — from the PARAMS tab, a plugin, or
            // our own retry below. That, and only that, opens a fetch.
            const uint32_t list_seq = sender_.param_list_seq();
            if (list_seq != seen_param_list_seq) {
                seen_param_list_seq = list_seq;
                parser_.clear_params();   // count this fetch from zero
                param_fetch_active  = true;
                param_stall_rounds  = 0;
                param_stall_mark    = 0;
                last_param_time     = Clock::now();
            }

            if (param_fetch_active && ps.param_count > 0 &&
                ps.params_received >= ps.param_count)
                param_fetch_active = false;   // complete

            // Retransmit after a stall. Individual PARAM_REQUEST_READs are for
            // filling the handful of gaps packet loss leaves at the end of a
            // fetch, and are the wrong tool for a fetch that never got going: a
            // vehicle queues them in a small fixed-depth buffer — ArduPilot's is
            // 20 — and silently drops the rest, so asking for a thousand missing
            // indices at once gets about twenty answered and a thousand
            // discarded, every round, forever. PARAM_REQUEST_LIST goes down the
            // vehicle's separate streaming path instead, which is paced against
            // its own link budget and drops nothing; it is why pressing FETCH
            // ALL completes a fetch that the gap-filling never could.
            constexpr double PARAM_STALL_S       = 2.0;
            constexpr int    PARAM_READ_BURST    = 16;  // <= a vehicle's request queue depth
            constexpr int    PARAM_GIVEUP_ROUNDS = 5;   // ~10 s with nothing arriving

            if (param_fetch_active && ps.param_count > 0 &&
                ps.params_received < ps.param_count)
            {
                const double stall = std::chrono::duration<double>(
                                         Clock::now() - last_param_time).count();
                if (stall >= PARAM_STALL_S) {
                    // Rounds are only counted as lost when nothing at all
                    // arrived, so a slow link retries as long as it is moving.
                    if (ps.params_received == param_stall_mark) ++param_stall_rounds;
                    else                                        param_stall_rounds = 0;
                    param_stall_mark = ps.params_received;

                    if (param_stall_rounds >= PARAM_GIVEUP_ROUNDS) {
                        gcs_log("[sys%u] param fetch abandoned at %u/%u — no reply in "
                                "%.0f s; press FETCH ALL to retry", tsys,
                                (unsigned)ps.params_received, (unsigned)ps.param_count,
                                PARAM_STALL_S * PARAM_GIVEUP_ROUNDS);
                        param_fetch_active = false;
                    } else {
                        std::unordered_set<uint16_t> have;
                        have.reserve(parser_.params().size());
                        for (const auto& [key, e] : parser_.params())
                            have.insert(e.index);

                        std::vector<uint16_t> missing;
                        for (uint16_t idx = 0; idx < ps.param_count; ++idx)
                            if (have.find(idx) == have.end())
                                missing.push_back(idx);

                        if ((int)missing.size() > PARAM_READ_BURST) {
                            sender_.request_param_list(ps.sysid, ps.compid);
                            // Our own retry is the same fetch continuing, not a
                            // new one: adopt the sequence number so the arming
                            // branch above does not reset the give-up counter and
                            // leave this looping until the link drops.
                            seen_param_list_seq = sender_.param_list_seq();
                            gcs_log("[sys%u] param fetch stalled at %u/%u — "
                                    "re-requesting full list (%d missing)", tsys,
                                    (unsigned)ps.params_received,
                                    (unsigned)ps.param_count, (int)missing.size());
                        } else {
                            for (uint16_t idx : missing)
                                sender_.request_param_read(ps.sysid, ps.compid,
                                                           (int16_t)idx);
                            gcs_log("[sys%u] param fetch stalled at %u/%u — "
                                    "re-requesting %d missing", tsys,
                                    (unsigned)ps.params_received,
                                    (unsigned)ps.param_count, (int)missing.size());
                        }
                    }
                    last_param_time = Clock::now(); // rate-limit to once per 2 s
                }
            }
        }
    }

    // A last publish so the UI does not keep showing a half-updated snapshot
    // from whenever the cadence last fired.
    publish(param_fetch_active, true);
}
