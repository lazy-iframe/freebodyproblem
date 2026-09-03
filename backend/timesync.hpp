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

#include <cstdint>

// ─────────────────────────────────────────────────────────────────────────────
// TIMESYNC (#111) — the vehicle's own clock, read across the link.
//
// The exchange is two messages. We send tc1 = 0, ts1 = our monotonic clock;
// the vehicle replies with tc1 = its clock at the moment it answered and our
// ts1 echoed back untouched. Taking the midpoint of the round trip as the
// instant the vehicle's reading applies cancels most of the transport latency,
// and what is left is the offset between the two clocks:
//
//     offset = tc1 - (ts1 + now) / 2
//
// Add that to our monotonic clock at any later moment and the result is what
// the vehicle's clock reads right now — the same domain as the time_boot_ms
// carried by ATTITUDE, GLOBAL_POSITION_INT and the rest, i.e. time since the
// flight controller booted. ArduPilot answers in nanoseconds since boot, so
// the offset is nanoseconds and the arithmetic never has to round.
//
// This class is the exchange and nothing else: no socket, no thread, no clock
// of its own. Callers hand it the readings and a `now`, and it hands back the
// reply that owes to be sent. It lives on the link thread; the offset it
// produces is copied into VehicleState for the UI to add to its own clock, so
// the displayed time ticks at frame rate rather than at the sync rate.
// ─────────────────────────────────────────────────────────────────────────────

// Monotonic nanoseconds, the local side of the exchange. Not wall-clock time
// and not tied to any epoch — only differences mean anything. Every caller must
// use this one function, or the offset is against a clock nobody else reads.
int64_t timesync_monotonic_ns();

class TimeSync {
public:
    // A reply we owe the vehicle, when it was the one to open an exchange.
    // Per the spec a reply is addressed to the requester rather than broadcast,
    // hence the ids.
    struct Reply {
        uint8_t tsys  = 0;
        uint8_t tcomp = 0;
        int64_t tc1   = 0;   // our clock, at the moment we answered
        int64_t ts1   = 0;   // the requester's ts1, echoed back unchanged
    };

    // Opens an exchange: returns the ts1 to send alongside tc1 = 0, and
    // remembers it so the matching reply can be told from anything else.
    int64_t begin_request(int64_t now_ns);

    // Feeds a received TIMESYNC. Returns true and fills `out` when the vehicle
    // was the one asking and we owe it a reply; returns false otherwise,
    // having updated the offset if this was the answer to our own request.
    bool on_timesync(int64_t tc1, int64_t ts1,
                     uint8_t sender_sysid, uint8_t sender_compid,
                     int64_t now_ns, Reply* out);

    // Until an exchange completes there is no offset, and the reading it would
    // produce is not a slow clock — it is the wrong clock entirely. Callers
    // must check this before using offset_ns() for anything a person reads.
    bool    synced()    const { return synced_; }
    int64_t offset_ns() const { return offset_ns_; }

private:
    int64_t offset_ns_   = 0;
    // ts1 of the request still outstanding; 0 when there is none.
    int64_t pending_ts1_ = 0;
    bool    synced_      = false;
};
