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

#include <mavlink/ardupilotmega/mavlink.h>

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// MAVLink 2 message signing: this ground station's key, and its one clock.
//
// One key per connection, as the C library holds one per link. A connection is known by
// its settings, "tcp:192.168.1.1:5760", so its key is still there after a reconnect or a
// restart. Nothing on a connection is signed or checked until it has a key.
//
// The key is kept in a file of its own, readable only by this user, and not in the
// settings. Nothing outside this module sees it except the framers that check with it
// and the SETUP_SIGNING that hands it to a vehicle: the UI has a fingerprint.
//
// Why frames are signed as they leave and not as they are packed: a vehicle throws
// away anything whose timestamp is not newer than the last it had from that stream,
// and frames here are packed on one thread and sent later on another, the stick and
// RC frames ahead of older ones already queued. Signed when packed they would reach
// the vehicle out of order, and be taken for replays.
//
// Why one clock for all of it, across keys: a vehicle's stream is (sysid, compid, link
// id), and every sender here is 255/190. On a shared link each vehicle hears the frames
// meant for the others, so senders with clocks of their own would look like replays.
// ─────────────────────────────────────────────────────────────────────────────

namespace signing {

// Reads the file at `path`, which need not exist, and writes there from then on.
void init(const std::string& path);

// Writes the clock out, at most once a minute; saved so that it never runs backwards
// across a restart, even if the system clock does. Cheap to call every frame.
void tick();
void save_now();

// One connection's key and settings. Lives as long as the program, whatever happens to
// the link, so it is safe to hold on to from any thread.
struct LinkKey;

// The one for the connection of this name, made if there is none yet. Saved only once
// it has something in it.
LinkKey* for_link(const std::string& link_name);

// A key set from a passphrase, by SHA-256, as MAVProxy and Mission Planner do: the same
// passphrase there gives the same key. False if it is empty.
bool set_passphrase(LinkKey* link, const std::string& passphrase);
void forget_key(LinkKey* link);
bool active(const LinkKey* link);

// Eight hex digits that tell keys apart without giving anything of them away.
// Empty with no key.
std::string fingerprint(LinkKey* link);

// A passphrase with 256 bits in it, from the system's secure random source.
// Empty if that fails.
std::string generate_passphrase();

// Keys kept by name, to give to other connections. In the same file, and no more visible:
// what comes back out of here to show is the name and the fingerprint.
struct SavedKey {
    std::string name;
    std::string fingerprint;
};
std::vector<SavedKey> saved_keys();                     // by name

// Keeps this connection's key under `name`, over any of that name. False with no key, or
// no name.
bool save_key(LinkKey* link, const std::string& name);
bool has_saved_key(const std::string& name);
void delete_saved_key(const std::string& name);

// Gives this connection the key saved under `name`. False if there is none.
bool use_saved_key(LinkKey* link, const std::string& name);

// The name this connection's key is saved under. Empty if it is not.
std::string saved_name(LinkKey* link);

// Dropping unsigned frames on this connection, once its vehicles are meant to be
// signing. Off by default: a vehicle with no key yet must stay in view to be given one.
bool require_signed(const LinkKey* link);
void set_require_signed(LinkKey* link, bool on);

// 10 us units since 2015-01-01 UTC: the system clock, but always past the last one
// handed out or heard from a vehicle.
uint64_t next_timestamp();
void     observe_timestamp(uint64_t ts);

// What a connection signs with. Stable across reconnects and restarts: a vehicle holds a
// fixed number of streams until it reboots (16 on ArduPilot), and each new link id takes
// another. Differs between installs, so two ground stations on one vehicle do not share
// a stream, whose timestamps would collide.
uint8_t link_id(const LinkKey* link);

// `frame` as it goes on the wire, signed, in `out`. False if it is to go as it is: no
// link, no key, or not a MAVLink 2 frame.
bool sign_frame(LinkKey* link, const std::vector<uint8_t>& frame, std::vector<uint8_t>& out);

// SETUP_SIGNING's key field: this key, or false with none. Only for that message.
bool copy_key(LinkKey* link, uint8_t out[32]);

// For the framers. Each generation changes with the key: the link's with its own, the
// other with any.
uint64_t                   generation();
uint64_t                   link_generation(const LinkKey* link);
mavlink_signing_streams_t* streams();        // shared by all links, as replays are not
std::mutex&                streams_mutex();  // per link; hold while a frame is checked

// Whether a frame that did not pass may still be looked at. Unsigned ones may, for the
// framer to rule on; one whose signature failed never may.
bool accept_unverified(const mavlink_status_t* status, uint32_t msgid);

} // namespace signing
