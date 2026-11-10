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

// The bulk parameter fetch, from this GCS's side: when one is open, when it has
// stalled and what to ask again, when to give up — and, for publishing, when
// the table has changed and when a fetch has just completed.
//
// A fetch follows the PARAM_REQUEST_LIST this GCS sent, never the PARAM_VALUEs
// coming back; see MavlinkSender::param_list_seq(). Everything here only runs
// inside a fetch somebody actually asked for.
//
// Holds no link and sends nothing. Vehicle's protocol loop feeds it the
// published state and carries out what it returns through MavlinkSender.

#include <chrono>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "backend/vehicle/state.hpp"

class ParamFetch {
public:
    using Clock = std::chrono::steady_clock;

    // Retransmit after a stall. Individual PARAM_REQUEST_READs are for filling
    // the handful of gaps packet loss leaves at the end of a fetch, and are the
    // wrong tool for a fetch that never got going: a vehicle queues them in a
    // small fixed-depth buffer — ArduPilot's is 20 — and silently drops the
    // rest, so asking for a thousand missing indices at once gets about twenty
    // answered and a thousand discarded, every round, forever.
    // PARAM_REQUEST_LIST goes down the vehicle's separate streaming path
    // instead, which is paced against its own link budget and drops nothing; it
    // is why pressing FETCH ALL completes a fetch that the gap-filling never
    // could.
    static constexpr double STALL_S       = 2.0;
    static constexpr int    READ_BURST    = 16;  // <= a vehicle's request queue depth
    static constexpr int    GIVEUP_ROUNDS = 5;   // ~10 s with nothing arriving

    explicit ParamFetch(Clock::time_point now) : last_param_time_(now) {}

    // ── Publishing ───────────────────────────────────────────────────────────
    //
    // Completion is an edge; republishing the table is not. Both used to hang
    // off the same test, which was wrong once a fetch had finished:
    // params_generation ticks on every PARAM_VALUE, and after completion the
    // ones that keep arriving — the echo of a write, or the counters a vehicle
    // saves as it runs — all arrive with params_received still equal to
    // param_count. Every one of them therefore read as a fresh completion and
    // logged "parameters loaded" again.
    struct Publish {
        bool republish      = false;   // the table is complete and has changed
        bool just_completed = false;   // a fetch finished this pass
    };
    // Once per pass, before publishing. Also notes PARAM_VALUE progress for the
    // stall timer.
    Publish on_state(const VehicleState& ps, Clock::time_point now);

    // ── The fetch ────────────────────────────────────────────────────────────

    // A PARAM_REQUEST_LIST went out — from the PARAMS tab, a plugin, or our own
    // retry — when `list_seq` moved. True when that opens a fetch: the caller
    // clears the parser's table, so this one is counted from zero.
    bool open_if_requested(uint32_t list_seq, Clock::time_point now);

    struct Actions {
        enum class Kind {
            None,
            RequestList,    // ask for the whole list again; then adopt_list_seq()
            RequestReads,   // ask for each index in `missing`
            Abandoned,      // out of rounds: the fetch is closed
        };
        Kind                  kind          = Kind::None;
        std::vector<uint16_t> missing;          // RequestReads only
        size_t                missing_count = 0;
    };
    Actions tick(const VehicleState& ps,
                 const std::unordered_map<std::string, ParamEntry>& params,
                 Clock::time_point now);

    // Our own RequestList is the same fetch continuing, not a new one: adopting
    // its sequence number keeps open_if_requested() from resetting the give-up
    // counter and leaving this looping until the link drops.
    void adopt_list_seq(uint32_t seq) { seen_list_seq_ = seq; }

    bool active() const { return active_; }

private:
    // Publishing
    uint32_t          last_params_gen_  = 0;
    uint32_t          known_params_gen_ = 0;
    bool              complete_latch_   = false;
    Clock::time_point last_param_time_;

    // The fetch
    uint32_t seen_list_seq_ = 0;
    bool     active_        = false;
    int      stall_rounds_  = 0;
    uint16_t stall_mark_    = 0;
};
