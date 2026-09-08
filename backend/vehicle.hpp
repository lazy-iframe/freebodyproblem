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
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "mavlink_parser.hpp"
#include "mavlink_sender.hpp"

// Which vehicle, on which link.
//
// Keyed by the pair rather than by sysid alone. Two aircraft that both ship
// with the factory default SYSID_THISMAV of 1 are a routine sight — two SITL
// instances, two boards fresh off the bench — and merging them into one object
// because they agree on a number would interleave two aircraft's telemetry with
// no way to tell from the screen. The cost of the pair is that one aircraft
// reachable over two links (a radio and a USB cable) appears twice; that is
// visible and explicable, which the alternative failure is not. Collapsing the
// two into one vehicle is a later phase, and wants a real identity to key on —
// the board UID from AUTOPILOT_VERSION — rather than a guess.
struct VehicleId {
    uint32_t link_id = 0;
    uint8_t  sysid   = 0;

    bool operator==(const VehicleId& o) const
    { return link_id == o.link_id && sysid == o.sysid; }
    bool operator!=(const VehicleId& o) const { return !(*this == o); }
    bool valid() const { return sysid != 0; }
};

struct VehicleIdHash {
    size_t operator()(const VehicleId& v) const
    { return (static_cast<size_t>(v.link_id) << 8) ^ v.sysid; }
};

// Everything the UI reads for one vehicle, copied out in a single lock.
struct VehicleSnapshot {
    VehicleState                                state;
    std::unordered_map<std::string, ParamEntry> params;
    uint32_t                                    params_generation = 0;
    std::unordered_map<uint32_t, MessageStats>  msg_stats;
    std::vector<StatusText>                     status_texts;

    // Messages are this vehicle's; bytes and framing errors belong to the link
    // it arrived on and are the same for every vehicle sharing that link.
    uint64_t total_messages = 0;
    uint64_t total_bytes    = 0;
    uint64_t parse_errors   = 0;
};

// The small, cheap view the vehicle switcher draws. Deliberately not a
// VehicleSnapshot: the switcher renders one row per vehicle every frame, and a
// snapshot carries the parameter table and the inspector's raw-message cache.
struct VehicleChip {
    VehicleId id;
    uint32_t  number        = 0;   // Fleet's display number; see Vehicle::number()
    uint8_t   compid        = 0;
    uint8_t   type          = 0;   // MAV_TYPE
    bool      armed         = false;
    bool      has_heartbeat = false;
    bool      stale         = false;

    // Which link this one came in on, e.g. "udp:0.0.0.0:14560".
    //
    // Not decoration. Two aircraft that both shipped as sysid 1 produce two rows
    // reading "SYS1", and without the link beside them the operator is choosing
    // between two identical entries. Filled by Fleet, which is what knows the
    // link names; a fixed buffer rather than a std::string because this is built
    // fresh for every vehicle on every frame.
    char      link_name[48] = {};
};

// One vehicle, with the thread that talks to it.
//
// The link this vehicle was heard on owns the socket and does the framing; it
// hands whole messages here and drains the outbound queue. Everything between
// those two points — interpreting messages, and the protocol conversations that
// a GCS has to keep up (stream rates, the mode list, clock sync, the parameter
// fetch and its retransmits) — runs on this object's own thread, so several
// vehicles make progress at once and a slow conversation with one cannot stall
// the reading of another.
class Vehicle {
public:
    Vehicle(VehicleId id, uint8_t autopilot_compid, uint32_t number);
    ~Vehicle();

    Vehicle(const Vehicle&)            = delete;
    Vehicle& operator=(const Vehicle&) = delete;

    void start();
    void stop();                                  // idempotent; joins the thread

    // ── Link thread side ──────────────────────────────────────────────────────

    // Hand over one framed message. Never blocks on the protocol thread.
    void deliver(const mavlink_message_t& msg);

    // The address this vehicle's datagrams come from. Meaningless on serial and
    // TCP, where the transport already knows where it is pointed.
    void set_peer(const sockaddr_in& addr);
    bool peer(sockaddr_in& out) const;

    // Link-wide counters, mirrored into this vehicle's snapshot for display.
    void set_link_counters(uint64_t bytes, uint64_t parse_errors);

    // ── UI thread side ────────────────────────────────────────────────────────

    void        snapshot(VehicleSnapshot& out) const;
    VehicleChip chip() const;

    MavlinkSender& sender()        { return sender_; }
    VehicleId      id()      const { return id_; }

    // The short number the operator sees beside this vehicle on the map, "(2)".
    // A sysid is what the aircraft calls itself and is not usable as a name —
    // two airframes fresh off the bench both answer to 1 — so Fleet hands out a
    // number of its own on discovery, lowest free one first, fixed for the life
    // of the object. Lowest-free rather than ever-increasing so a fleet of three
    // reads 1, 2, 3 rather than 1, 4, 7 after a morning of reconnects.
    uint32_t       number()  const { return number_; }
    uint8_t        sysid()   const { return id_.sysid; }
    uint8_t        compid()  const { return autopilot_compid_; }
    // Fixed for the object's life: a vehicle is identified by the link it was
    // heard on as well as its sysid, so it never moves between links. Losing the
    // link destroys the vehicle, and hearing the same aircraft again after a
    // reconnect creates a new one that runs its connect burst from scratch.
    uint32_t       link_id() const { return id_.link_id; }

    // No heartbeat for longer than the silence timeout. Not a reason to destroy
    // the vehicle — a radio that blips during a calibration must not take the
    // calibration with it — only to say so on screen.
    bool stale() const;

    // Only the vehicle the operator is looking at makes noise. Three aircraft
    // answering a connect burst at once would otherwise beep over each other,
    // which teaches the operator to stop listening.
    void set_tones_enabled(bool on);

    // Messages dropped because the protocol thread fell behind. Should stay
    // zero; a non-zero value means the inbox bound was reached.
    uint64_t inbox_dropped() const;

private:
    void loop();
    void publish(bool param_fetch_active, bool republish_params);

    const VehicleId id_;
    const uint32_t  number_;
    const uint8_t   autopilot_compid_;

    MavlinkParser parser_;
    MavlinkSender sender_;

    // Inbox — link thread pushes, protocol thread drains.
    mutable std::mutex             inbox_mtx_;
    std::condition_variable        inbox_cv_;
    std::vector<mavlink_message_t> inbox_;
    std::atomic<uint64_t>          inbox_dropped_{0};

    // A bound so a wedged protocol thread cannot let a link thread grow this
    // without limit. At the rates a vehicle streams, this is many seconds of
    // backlog: reaching it means something is already badly wrong, and dropping
    // the oldest is the least bad answer.
    static constexpr size_t INBOX_MAX = 4096;

    // Published snapshot — protocol thread writes, UI thread reads.
    mutable std::mutex snap_mtx_;
    VehicleSnapshot    snap_;

    mutable std::mutex peer_mtx_;
    sockaddr_in        peer_{};
    bool               peer_set_ = false;

    std::atomic<uint64_t> link_bytes_{0};
    std::atomic<uint64_t> link_parse_errors_{0};
    std::atomic<int64_t>  last_heartbeat_ns_{0};
    std::atomic<bool>     tones_enabled_{true};
    std::atomic<bool>     running_{false};

    std::thread thread_;
};
