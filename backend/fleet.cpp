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


#include "fleet.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>

#include "mavlink_framer.hpp"
#include "../frontend/app_log.hpp"

using Clock = std::chrono::steady_clock;

namespace {

constexpr double CONNECT_TIMEOUT_S = 10.0;

// How long a link that has been talking may go completely silent before it is
// declared lost. Generous on purpose: an autopilot busy with a sensor
// calibration answers slowly, and cutting a calibration short because the
// vehicle spent a few seconds thinking would be worse than noticing late.
// Even at ArduPilot's 1 Hz heartbeat this is fifteen missed beats — silence
// this long is a link that has gone, not one that is busy.
constexpr double LINK_SILENCE_TIMEOUT_S = 15.0;

} // namespace


Fleet::~Fleet() { disconnect_all(); }

uint32_t Fleet::connect(const LinkConfig& cfg)
{
    const uint32_t id = next_link_id_.fetch_add(1, std::memory_order_relaxed);

    auto slot   = std::make_shared<LinkSlot>();
    slot->link  = std::make_shared<Link>(id, cfg);
    slot->running.store(true, std::memory_order_relaxed);

    last_link_id_.store(id, std::memory_order_relaxed);

    if (!slot->link->open()) {
        // Keep the slot around so the UI can read why it failed, but do not
        // start a thread on a handle that was never opened.
        slot->running.store(false, std::memory_order_relaxed);
        std::lock_guard<std::mutex> lk(mtx_);
        links_.push_back(slot);
        return 0;
    }

    // Listed and started under the same lock. Between the two a disconnect
    // would find the slot, see a thread that is not yet joinable, join nothing
    // and close the transport — leaving this call to start a thread on a closed
    // link and, worse, to destroy a std::thread that is still running. The new
    // thread's first act that needs this lock is routing, which happens after a
    // read, so holding it across the spawn costs nothing.
    std::lock_guard<std::mutex> lk(mtx_);
    links_.push_back(slot);
    slot->thread = std::thread(&Fleet::rx_loop, this, slot);
    return id;
}

void Fleet::disconnect(uint32_t link_id)
{
    std::shared_ptr<LinkSlot> slot;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        auto it = std::find_if(links_.begin(), links_.end(),
                               [&](const std::shared_ptr<LinkSlot>& s) {
                                   return s->link && s->link->id() == link_id;
                               });
        if (it == links_.end()) return;
        slot = *it;
        links_.erase(it);
    }

    // Stop the reader first. After this nothing pushes into a vehicle inbox and
    // nothing reaches for a vehicle's sender, which is what makes it safe to
    // take the vehicles down next.
    slot->running.store(false, std::memory_order_relaxed);
    if (slot->thread.joinable()) slot->thread.join();
    slot->link->close();

    std::vector<std::shared_ptr<Vehicle>> doomed;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        for (auto it = vehicles_.begin(); it != vehicles_.end(); ) {
            if (it->first.link_id == link_id) {
                doomed.push_back(it->second);
                it = vehicles_.erase(it);
            } else {
                ++it;
            }
        }
    }
    for (auto& v : doomed) v->stop();

    // The active vehicle may have been one of them.
    {
        std::lock_guard<std::mutex> lk(active_mtx_);
        if (active_.link_id == link_id) active_ = VehicleId{};
    }
    if (!active()) {
        auto rest = vehicles();
        if (!rest.empty()) set_active(rest.front()->id());
    }
    retune_tones();
}

void Fleet::disconnect_all()
{
    std::vector<uint32_t> ids;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        for (auto& s : links_)
            if (s->link) ids.push_back(s->link->id());
    }
    for (uint32_t id : ids) disconnect(id);

    // Links that failed to open never had a thread and hold no vehicles.
    std::lock_guard<std::mutex> lk(mtx_);
    links_.clear();
    vehicles_.clear();
}

// ── Routing ───────────────────────────────────────────────────────────────────

std::shared_ptr<Vehicle> Fleet::route(const mavlink_message_t& msg, uint32_t link_id)
{
    const VehicleId vid{link_id, msg.sysid};

    {
        std::lock_guard<std::mutex> lk(mtx_);
        auto it = vehicles_.find(vid);
        if (it != vehicles_.end()) return it->second;
    }

    // Discovery is heartbeat-gated. A vehicle announces itself exactly once per
    // second and in exactly one way, so anything else arriving from a sysid we
    // have never heard of is a fragment of a conversation we are not part of.
    if (msg.msgid != MAVLINK_MSG_ID_HEARTBEAT) return nullptr;

    mavlink_heartbeat_t hb;
    mavlink_msg_heartbeat_decode(&msg, &hb);

    // Gimbals, cameras and companion computers heartbeat too, with
    // MAV_AUTOPILOT_INVALID. So does another GCS on the same network, and so do
    // our own datagrams coming back on a loopback UDP bind. None of them is a
    // vehicle, and creating one for them would put a phantom in the switcher.
    if (hb.autopilot == MAV_AUTOPILOT_INVALID)      return nullptr;
    if (msg.sysid    == MavlinkSender::GCS_SYSID)   return nullptr;

    std::shared_ptr<Vehicle> created;
    bool first = false;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        auto it = vehicles_.find(vid);
        if (it != vehicles_.end()) return it->second;   // raced with another link

        if (vehicles_.size() >= MAX_VEHICLES) {
            static bool warned = false;
            if (!warned) {
                warned = true;
                gcs_log("vehicle limit (%zu) reached — ignoring sys%u",
                        MAX_VEHICLES, (unsigned)msg.sysid);
            }
            return nullptr;
        }

        // The same sysid already known on a different link. Legitimate when one
        // aircraft is reachable two ways, and a misconfiguration when it is two
        // aircraft that both shipped as sysid 1 — worth saying out loud either
        // way, because the switcher is about to show two entries.
        for (const auto& [other_id, other] : vehicles_) {
            if (other_id.sysid == msg.sysid) {
                gcs_log("sys%u is now on two links — if that is two aircraft, "
                        "give them distinct SYSID_THISMAV", (unsigned)msg.sysid);
                break;
            }
        }

        // Lowest free number. A number is free exactly when no live vehicle
        // holds it, so a scan of the fleet is the whole bookkeeping — no free
        // list to fall out of step with the map it describes. MAX_VEHICLES is
        // 15, and this runs once per discovery.
        uint32_t number = 1;
        while (std::any_of(vehicles_.begin(), vehicles_.end(),
                           [number](const auto& kv) {
                               return kv.second->number() == number;
                           }))
            ++number;

        created = std::make_shared<Vehicle>(vid, msg.compid, number);
        vehicles_[vid] = created;
        first = (vehicles_.size() == 1);
    }

    created->start();
    gcs_log("vehicle discovered: (%u) sys%u comp%u",
            (unsigned)created->number(), (unsigned)msg.sysid,
            (unsigned)msg.compid);

    if (first) set_active(vid);
    retune_tones();
    return created;
}

// ── Link thread ───────────────────────────────────────────────────────────────

void Fleet::rx_loop(std::shared_ptr<LinkSlot> slot)
{
    Link& link = *slot->link;

    MavlinkFramer framer;
    uint8_t       buf[2048];

    auto link_start = Clock::now();
    auto last_rx    = Clock::now();
    bool had_data   = false;

    // Vehicles seen on this link, so the flush below does not walk the whole
    // fleet. Held by shared_ptr: a vehicle removed elsewhere must stay alive
    // until this thread is done reaching for its sender.
    std::vector<std::shared_ptr<Vehicle>> mine;

    while (slot->running.load(std::memory_order_relaxed)) {
        int         n = 0;
        sockaddr_in src{};
        bool        src_valid = false;

        const RxResult rx = link.read(buf, sizeof(buf), n, src, src_valid);

        if (rx == RxResult::Closed) {
            gcs_log("%s: closed by the vehicle", link.name().c_str());
            link.set_status(LinkStatus::Timeout);
            break;
        }
        if (rx == RxResult::Failed) {
            // The usual cause is the port disappearing underneath us: an
            // autopilot rebooting takes its USB serial device with it, which is
            // exactly what happens after a calibration that ends in a reboot.
            gcs_log("%s: read failed", link.name().c_str());
            link.set_status(LinkStatus::Error);
            break;
        }

        if (rx == RxResult::Data && n > 0) {
            last_rx = Clock::now();
            if (!had_data) {
                had_data = true;
                link.set_status(LinkStatus::Connected);
                gcs_log("%s: link established", link.name().c_str());
            }

            framer.feed(buf, (size_t)n, [&](const mavlink_message_t& msg) {
                // Vehicles already seen on this link are remembered here rather
                // than looked up under the fleet lock. Routing runs once per
                // message, and taking a shared lock at that rate to answer a
                // question whose answer almost never changes would put the link
                // thread in contention with every UI frame.
                std::shared_ptr<Vehicle> v;
                for (const auto& c : mine)
                    if (c->sysid() == msg.sysid) { v = c; break; }

                if (!v) {
                    v = route(msg, link.id());
                    if (!v) return;
                    mine.push_back(v);
                }

                if (src_valid) v->set_peer(src);
                v->set_link_counters(framer.total_bytes(), framer.parse_errors());
                v->deliver(msg);
            });
        } else if (!had_data) {
            const double elapsed =
                std::chrono::duration<double>(Clock::now() - link_start).count();
            if (elapsed >= CONNECT_TIMEOUT_S) {
                gcs_log("%s: connection timeout after %.0f s",
                        link.name().c_str(), elapsed);
                link.set_status(LinkStatus::Timeout);
                break;
            }
        } else {
            // A link that was talking and has stopped. Nothing here reports an
            // error — UDP in particular cannot, since there is no connection to
            // fail — so silence is the only evidence there is, and without this
            // the panel would go on showing a live link and a frozen counter
            // until somebody thought to reconnect.
            const double quiet =
                std::chrono::duration<double>(Clock::now() - last_rx).count();
            if (quiet >= LINK_SILENCE_TIMEOUT_S) {
                gcs_log("%s: silent for %.0f s \xe2\x80\x94 giving up",
                        link.name().c_str(), quiet);
                link.set_status(LinkStatus::Timeout);
                break;
            }
        }

        // Drain what the vehicles queued. The transport handle belongs to this
        // thread, so this is the only place frames can actually leave.
        for (auto& v : mine) {
            sockaddr_in peer{};
            const bool  has_peer = v->peer(peer);
            link.flush_to(v->sender(), has_peer ? &peer : nullptr);
        }
    }

    link.close();
}

// ── UI thread ─────────────────────────────────────────────────────────────────

std::vector<std::shared_ptr<Vehicle>> Fleet::vehicles() const
{
    std::lock_guard<std::mutex> lk(mtx_);
    std::vector<std::shared_ptr<Vehicle>> out;
    out.reserve(vehicles_.size());
    for (const auto& [id, v] : vehicles_) out.push_back(v);
    std::sort(out.begin(), out.end(),
              [](const std::shared_ptr<Vehicle>& a,
                 const std::shared_ptr<Vehicle>& b) {
                  if (a->id().link_id != b->id().link_id)
                      return a->id().link_id < b->id().link_id;
                  return a->id().sysid < b->id().sysid;
              });
    return out;
}

std::vector<VehicleChip> Fleet::chips() const
{
    std::vector<VehicleChip> out;
    for (const auto& v : vehicles()) out.push_back(v->chip());

    // The vehicle knows its link's id but not its name, so the names are filled
    // in here where both are in scope.
    std::lock_guard<std::mutex> lk(mtx_);
    for (auto& c : out) {
        for (const auto& s : links_) {
            if (s->link && s->link->id() == c.id.link_id) {
                std::snprintf(c.link_name, sizeof(c.link_name), "%s",
                              s->link->name().c_str());
                break;
            }
        }
    }
    return out;
}

std::vector<LinkInfo> Fleet::links() const
{
    std::lock_guard<std::mutex> lk(mtx_);
    std::vector<LinkInfo> out;
    out.reserve(links_.size());
    for (const auto& s : links_) {
        if (!s->link) continue;
        LinkInfo li;
        li.id     = s->link->id();
        li.name   = s->link->name();
        li.status = s->link->status();
        for (const auto& [vid, v] : vehicles_)
            if (vid.link_id == li.id) ++li.vehicles;
        out.push_back(li);
    }
    return out;
}

std::shared_ptr<Vehicle> Fleet::active() const
{
    VehicleId want;
    {
        std::lock_guard<std::mutex> lk(active_mtx_);
        want = active_;
    }
    if (!want.valid()) return nullptr;

    std::lock_guard<std::mutex> lk(mtx_);
    auto it = vehicles_.find(want);
    return it == vehicles_.end() ? nullptr : it->second;
}

VehicleId Fleet::active_id() const
{
    std::lock_guard<std::mutex> lk(active_mtx_);
    return active_;
}

void Fleet::set_active(VehicleId id)
{
    {
        std::lock_guard<std::mutex> lk(active_mtx_);
        if (active_ == id) return;
        active_ = id;
    }
    retune_tones();
}

void Fleet::retune_tones()
{
    const VehicleId a = active_id();
    for (const auto& v : vehicles())
        v->set_tones_enabled(v->id() == a);
}

LinkStatus Fleet::display_status() const
{
    if (auto v = active()) {
        std::lock_guard<std::mutex> lk(mtx_);
        for (const auto& s : links_)
            if (s->link && s->link->id() == v->link_id())
                return s->link->status();
    }

    const uint32_t last = last_link_id_.load(std::memory_order_relaxed);
    std::lock_guard<std::mutex> lk(mtx_);
    for (const auto& s : links_)
        if (s->link && s->link->id() == last)
            return s->link->status();
    return LinkStatus::Idle;
}

