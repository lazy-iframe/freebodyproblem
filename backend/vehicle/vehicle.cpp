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


#include "backend/vehicle/vehicle.hpp"
#include "backend/mission/watch.hpp"
#include "backend/vehicle/connect.hpp"
#include "backend/vehicle/identity.hpp"
#include "backend/vehicle/param_fetch.hpp"
#include "backend/vehicle/reports.hpp"
#include "backend/vehicle/stream_rates.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <unordered_map>

#include "backend/mavlink/timesync.hpp"
#include "frontend/source/app_log.hpp"   // gcs_log — thread-safe, see the header

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


// "v1_sys1", or "v1_sys1_alpha" for a vehicle with a name. The number and sysid
// stay in beside the name because they are what keeps two logs apart: nothing
// stops an operator calling two aircraft the same thing. Only what is safe in a
// file name anywhere is let through.
static std::string tlog_stem(uint32_t number, uint8_t sysid, const std::string& name)
{
    std::string s = "v" + std::to_string(number) + "_sys" + std::to_string(sysid);
    if (name.empty()) return s;
    s += '_';
    for (const char c : name) {
        const bool ok = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') ||
                        (c >= 'A' && c <= 'Z') || c == '-' || c == '_';
        s += ok ? c : '_';
    }
    return s;
}

Vehicle::Vehicle(VehicleId id, uint8_t autopilot_compid, uint32_t number, std::string name)
    : id_(id)
    , number_(number)
    , autopilot_compid_(autopilot_compid)
    , parser_(id.sysid, autopilot_compid)
    , tlog_(Tlog::open(tlog_stem(number, id.sysid, name)))
    , name_(std::move(name))
{
    sender_.set_tlog(tlog_.get());
}

Vehicle::~Vehicle()
{
    stop();
    tlog_->close();
}

std::string Vehicle::name() const
{
    std::lock_guard<std::mutex> lk(name_mtx_);
    return name_;
}

void Vehicle::set_name(const std::string& name)
{
    {
        std::lock_guard<std::mutex> lk(name_mtx_);
        if (name_ == name) return;
        name_ = name;
    }
    tlog_->rename(tlog_stem(number_, id_.sysid, name));
}

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
    tlog_->add(msg);

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
    std::snprintf(c.name, sizeof(c.name), "%s", name().c_str());
    c.compid = autopilot_compid_;
    c.stale  = stale();
    {
        std::lock_guard<std::mutex> lk(snap_mtx_);
        c.type          = static_cast<uint8_t>(snap_.state.type);
        c.armed         = snap_.state.armed;
        c.has_heartbeat = snap_.state.has_heartbeat;
        c.heartbeat_signed      = snap_.state.heartbeat_signed;
        c.heartbeat_signing_gen = snap_.state.heartbeat_signing_gen;
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

// Attitude alone, the moment it arrives, between the full publishes. The
// horizon is the one thing on screen that moves with every sample, and on the
// 50 ms cadence above a sample could wait almost a whole period, or be skipped
// when two landed in one — a horizon that lagged and stepped where v0.3.1's,
// published per datagram, did not. Four scalars under the lock, so it costs
// nothing at any rate the vehicle streams.
void Vehicle::publish_attitude()
{
    const VehicleState& ps = parser_.state();
    std::lock_guard<std::mutex> lk(snap_mtx_);
    snap_.state.roll         = ps.roll;
    snap_.state.pitch        = ps.pitch;
    snap_.state.yaw          = ps.yaw;
    snap_.state.has_attitude = ps.has_attitude;
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

    bool     rates_requested = false;
    uint32_t seen_reboots    = 0;

    // Each conversation's state and timers, link-free: they say what is due,
    // and this loop sends it and logs it.
    ModeListWalk    modes      (Clock::now());   // the mode list, one index at a time
    VersionProbe    version    (Clock::now());   // AUTOPILOT_VERSION, until it arrives
    StreamRateWatch rates      (Clock::now());   // the telemetry rates asked for, kept
    ParamFetch      params     (Clock::now());   // the bulk fetch, and when to publish
    MissionWatch    plan_watch (Clock::now());   // the plan's transfers
    MissionWatch    fence_watch(Clock::now());   // the fence's, on its own list
    VehicleReports  reports(tsys);               // the log lines and tones

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

        bool attitude_in = false;
        for (const auto& msg : batch) {
            parser_.handle(msg);
            if (msg.msgid == MAVLINK_MSG_ID_ATTITUDE) attitude_in = true;
            if (msg.compid == autopilot_compid_) rates.on_message(msg.msgid);
            if (msg.msgid == MAVLINK_MSG_ID_HEARTBEAT &&
                msg.compid == autopilot_compid_)
                last_heartbeat_ns_.store(now_ns(), std::memory_order_relaxed);
        }
        batch.clear();

        // A rebooted autopilot has forgotten every message interval it was
        // asked for and goes back to its own defaults, so the connect burst is
        // run again — without it the panels would sit on the last values the
        // old boot sent.
        if (parser_.state().reboot_count != seen_reboots) {
            seen_reboots    = parser_.state().reboot_count;
            rates_requested = false;
        }

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
            const char* stack = send_connect_burst(sender_, tsys, tcomp, parser_.state());
            version.on_first_request(Clock::now());
            modes.on_probe_sent(Clock::now());
            gcs_log("[sys%u] telemetry rates configured (%s)", tsys, stack);

            // The watchdog's first window starts with the burst, so the rates
            // the vehicle ran at before it are not held against it.
            rates.restart(Clock::now());
        }

        // ── Rate watchdog ─────────────────────────────────────────────────────
        for (const StreamRateWatch::Event& e :
             rates.tick(rates_requested, sender_.requested_intervals(), Clock::now())) {
            char name[24];
            stream_name(e.msgid, name);
            if (e.kind == StreamRateWatch::Event::Kind::Back) {
                gcs_log("[sys%u] %s back at %.1f Hz", tsys, name, e.got_hz);
                continue;
            }
            sender_.request_message_interval(tsys, tcomp, e.msgid, e.interval);
            if (e.note == StreamRateWatch::Event::Note::FirstMiss)
                gcs_log("[sys%u] %s at %.1f Hz, asked for %.0f Hz \xe2\x80\x94 asking again",
                        tsys, name, e.got_hz, e.want_hz);
            else if (e.note == StreamRateWatch::Event::Note::GoingSlow)
                gcs_log("[sys%u] %s still at %.1f Hz after %d requests \xe2\x80\x94 "
                        "the vehicle or the link is holding it down; asking every %d s",
                        tsys, name, e.got_hz, StreamRateWatch::FAST_TRIES,
                        (int)StreamRateWatch::SLOW_RETRY.count());
        }

        // Firmware version, until it is in hand.
        switch (version.tick(rates_requested, parser_.state().has_fw_info, Clock::now())) {
        case VersionProbe::Ask::None:
            break;
        case VersionProbe::Ask::RequestMessage:
            sender_.request_message(tsys, tcomp, MAVLINK_MSG_ID_AUTOPILOT_VERSION);
            break;
        case VersionProbe::Ask::RequestCapabilities:
            sender_.request_autopilot_capabilities(tsys, tcomp);
            break;
        }

        // ── Command ACKs ──────────────────────────────────────────────────────
        const bool tones = tones_enabled_.load(std::memory_order_relaxed);
        for (const auto& ack : parser_.pending_acks()) {
            sender_.notify_ack(ack.command, ack.result);
            reports.command_ack(ack, tones);
        }
        parser_.clear_acks();

        for (const auto& pa : parser_.pending_param_ext_acks())
            reports.param_ext_ack(pa, tones);
        parser_.clear_param_ext_acks();

        // ── Mode list ─────────────────────────────────────────────────────────
        // Walk the vehicle's mode list one index at a time, and restart the walk
        // if AVAILABLE_MODES_MONITOR reports the list changed.
        {
            const auto& ms = parser_.state();

            if (ms.modes_dirty) {
                parser_.clear_modes_dirty();
                modes.restart();
                gcs_log("[sys%u] mode list changed — re-requesting", tsys);
            }

            const ModeListWalk::Action ma = modes.tick(ms, Clock::now());
            if (ma.kind == ModeListWalk::Action::Kind::Probe)
                sender_.request_available_modes(tsys, tcomp);
            else if (ma.kind == ModeListWalk::Action::Kind::RequestIndex)
                sender_.request_available_mode(tsys, tcomp, ma.index);

            if (ma.just_completed)
                gcs_log("[sys%u] mode list: %zu modes", tsys,
                        ms.available_modes.size());
        }

        // Drain per-item mission requests generated by MISSION_COUNT /
        // MISSION_ITEM_INT handlers (one request triggers the next).
        for (const auto& req : parser_.pending_mission_reqs()) {
            sender_.request_mission_item(req.tsys, req.tcomp, req.seq, req.type);
            (req.type == MAV_MISSION_TYPE_FENCE ? fence_watch : plan_watch)
                .on_item_requested(req.seq, Clock::now());
        }
        parser_.clear_mission_reqs();

        // Drain the vehicle's MISSION_REQUEST_INT during a mission upload.
        for (const auto& req : parser_.pending_item_reqs())
            sender_.send_mission_item_at(req.tsys, req.tcomp, req.seq, req.type);
        parser_.clear_item_reqs();

        // Answer TIMESYNC exchanges the vehicle opened. ArduPilot asks on its
        // own schedule as well as answering ours, and a request left unanswered
        // is a vehicle whose clock never lines up with ours.
        for (const auto& r : parser_.pending_timesync_replies())
            sender_.send_timesync(r.tsys, r.tcomp, r.tc1, r.ts1);
        parser_.clear_timesync_replies();

        const VehicleState& ps = parser_.state();

        reports.tick(ps, parser_.status_texts());

        // The plan and the fence, each kept moving by its own watch: completed
        // downloads acknowledged, stalled ones asked again, uploads reported.
        for (const bool fence : { false, true }) {
            const uint8_t type = fence ? MAV_MISSION_TYPE_FENCE : MAV_MISSION_TYPE_MISSION;
            MissionWatch::Progress p;
            p.generation = fence ? ps.fence_generation    : ps.mission_generation;
            p.received   = fence ? ps.fence_received      : ps.mission_received;
            p.count      = fence ? ps.fence_count         : ps.mission_count;
            p.upload     = fence ? ps.fence_upload_status : ps.upload_status;
            const MissionWatch::Actions a =
                (fence ? fence_watch : plan_watch).tick(p, Clock::now());

            reports.mission(fence, a, p, ps, tones);
            if (a.loaded)
                sender_.send_mission_ack(ps.sysid, ps.compid, type);
            if (a.re_request)
                sender_.request_mission_item(ps.sysid, ps.compid, a.re_request_seq, type);
            if (a.upload != MissionWatch::Actions::Upload::None)
                sender_.clear_upload(type);
        }

        const ParamFetch::Publish pub = params.on_state(ps, Clock::now());
        const bool republish_params      = pub.republish;
        const bool params_just_completed = pub.just_completed;

        // Publish on a fixed cadence rather than on every message, but never
        // hold back a change the operator is waiting on: a completed parameter
        // fetch goes out immediately.
        const double since_pub = std::chrono::duration<double>(
                                     Clock::now() - last_publish).count();
        if (since_pub >= PUBLISH_INTERVAL_S || republish_params ||
            params_just_completed)
        {
            publish(params.active(), republish_params);
            last_publish = Clock::now();
        } else if (attitude_in) {
            publish_attitude();
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

        // ── Bulk parameter fetch: open, retransmit, give up ───────────────────
        if (params.open_if_requested(sender_.param_list_seq(), Clock::now()))
            parser_.clear_params();   // count this fetch from zero

        const ParamFetch::Actions pa = params.tick(ps, parser_.params(), Clock::now());
        switch (pa.kind) {
        case ParamFetch::Actions::Kind::None:
            break;
        case ParamFetch::Actions::Kind::Abandoned:
            gcs_log("[sys%u] param fetch abandoned at %u/%u — no reply in "
                    "%.0f s; press FETCH ALL to retry", tsys,
                    (unsigned)ps.params_received, (unsigned)ps.param_count,
                    ParamFetch::STALL_S * ParamFetch::GIVEUP_ROUNDS);
            break;
        case ParamFetch::Actions::Kind::RequestList:
            sender_.request_param_list(ps.sysid, ps.compid);
            params.adopt_list_seq(sender_.param_list_seq());
            gcs_log("[sys%u] param fetch stalled at %u/%u — "
                    "re-requesting full list (%d missing)", tsys,
                    (unsigned)ps.params_received,
                    (unsigned)ps.param_count, (int)pa.missing_count);
            break;
        case ParamFetch::Actions::Kind::RequestReads:
            for (uint16_t idx : pa.missing)
                sender_.request_param_read(ps.sysid, ps.compid, (int16_t)idx);
            gcs_log("[sys%u] param fetch stalled at %u/%u — "
                    "re-requesting %d missing", tsys,
                    (unsigned)ps.params_received,
                    (unsigned)ps.param_count, (int)pa.missing_count);
            break;
        }
    }

    // A last publish so the UI does not keep showing a half-updated snapshot
    // from whenever the cadence last fired.
    publish(params.active(), true);
}
