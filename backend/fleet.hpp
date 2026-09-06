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

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "link.hpp"
#include "vehicle.hpp"

// Every link, every vehicle, and the threads that drive them.
//
// One thread per link reads and frames bytes; one thread per vehicle interprets
// them. Framing has to happen per link because a message's sysid is not knowable
// until it is framed — but framing is a CRC and a copy, while the work that
// follows it is the 640-case dispatch, the parameter table, the mission state
// and the protocol timers, and that part runs per vehicle and in parallel.
class Fleet {
public:
    // One channel per vehicle sender is the binding constraint, and the null
    // sender the UI falls back on takes one of them.
    static constexpr size_t MAX_VEHICLES = MavlinkSender::MAX_CHANNELS - 1;

    ~Fleet();

    // Opens a link and starts its thread. Returns the link id, or 0 on failure.
    uint32_t connect(const LinkConfig& cfg);

    // Stops one link and forgets the vehicles that were only reachable on it.
    void disconnect(uint32_t link_id);
    void disconnect_all();

    // ── UI thread ─────────────────────────────────────────────────────────────

    std::vector<std::shared_ptr<Vehicle>> vehicles() const;
    std::vector<VehicleChip>              chips()    const;
    std::vector<LinkInfo>                 links()    const;

    std::shared_ptr<Vehicle> active() const;
    void                     set_active(VehicleId id);
    VehicleId                active_id() const;

    // What the topbar shows. The active vehicle's link where there is one, and
    // otherwise the most recently opened link — so "Connecting" and "Error"
    // still reach the screen before any vehicle exists to attribute them to.
    LinkStatus display_status() const;

private:
    struct LinkSlot {
        std::shared_ptr<Link> link;
        std::thread           thread;
        std::atomic<bool>     running{false};
    };

    void rx_loop(std::shared_ptr<LinkSlot> slot);

    // Called on a link thread when a message arrives for a sysid with no vehicle
    // yet. Creates one only for a heartbeat from a real autopilot.
    std::shared_ptr<Vehicle> route(const mavlink_message_t& msg, uint32_t link_id);

    void retune_tones();   // only the active vehicle makes noise

    mutable std::mutex mtx_;
    std::vector<std::shared_ptr<LinkSlot>> links_;
    std::unordered_map<VehicleId, std::shared_ptr<Vehicle>, VehicleIdHash> vehicles_;

    std::atomic<uint32_t> next_link_id_{1};
    std::atomic<uint32_t> last_link_id_{0};

    mutable std::mutex active_mtx_;
    VehicleId          active_{};
};
