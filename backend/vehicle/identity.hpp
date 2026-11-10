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

// Finding out what the vehicle is: its mode list and its firmware version.
// Neither is streamed, so each is asked for, and asked again when the answer
// is lost. The parser's half of the same conversation is parser/modes.cpp.
//
// Holds no link and sends nothing. Vehicle's protocol loop feeds these the
// published state and sends what they say is due through MavlinkSender.

#include <chrono>
#include <cstdint>

#include "backend/vehicle/state.hpp"

// AVAILABLE_MODES enumeration. The spec's "request index 0 for all modes" form
// is not what ArduPilot implements — it replies with index 1 alone — so the
// list has to be walked one index at a time. The index-0 probe is still worth
// sending first because its number_modes field is how we learn how far to
// walk. Sweeps are repeated to refill gaps left by packet loss; when they run
// out the UI falls back to its built-in tables.
class ModeListWalk {
public:
    using Clock = std::chrono::steady_clock;

    static constexpr int    MAX_SWEEPS = 3;
    static constexpr int    MAX_PROBES = 3;
    static constexpr double REQ_S      = 0.10;  // pacing between index requests
    static constexpr double PROBE_S    = 2.0;   // retry interval for the probe

    explicit ModeListWalk(Clock::time_point now) : last_req_(now) {}

    // The connect burst sent the index-0 probe.
    void on_probe_sent(Clock::time_point now);

    // AVAILABLE_MODES_MONITOR says the list changed: walk it again from the top.
    void restart();

    struct Action {
        enum class Kind { None, Probe, RequestIndex };
        Kind    kind           = Kind::None;
        uint8_t index          = 0;       // RequestIndex
        bool    just_completed = false;   // the whole list is in, said once
    };
    Action tick(const VehicleState& ms, Clock::time_point now);

private:
    Clock::time_point last_req_;
    int  cursor_ = 0;      // highest index requested this sweep
    int  sweeps_ = 0;
    int  probes_ = 0;
    bool logged_ = false;
};

// AUTOPILOT_VERSION — the firmware version the parameter reference is matched
// against, and the board the connection tab names. Neither stack streams it,
// and one request is easily lost, so it is asked for until it arrives. Through
// MAV_CMD_REQUEST_MESSAGE first: the common set marks
// REQUEST_AUTOPILOT_CAPABILITIES deprecated in its favour, and PX4 after v1.17
// no longer answers the old command. The old one goes out on alternate retries
// for a stack that predates the new.
class VersionProbe {
public:
    using Clock = std::chrono::steady_clock;

    static constexpr int    MAX_PROBES = 6;
    static constexpr double PROBE_S    = 2.0;

    explicit VersionProbe(Clock::time_point now) : last_req_(now) {}

    // The connect burst sent the first request (REQUEST_MESSAGE). A reboot
    // reruns the burst, which starts the count again.
    void on_first_request(Clock::time_point now);

    enum class Ask { None, RequestMessage, RequestCapabilities };
    // `burst_sent`: the connect burst has gone out for this boot.
    Ask tick(bool burst_sent, bool has_fw_info, Clock::time_point now);

private:
    Clock::time_point last_req_;
    int               probes_ = 0;
};
