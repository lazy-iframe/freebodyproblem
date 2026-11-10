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

// Rate watchdog: whether the telemetry this GCS asked for is arriving at the
// rate it asked for.
//
// The connect burst goes out once, and an interval asked for is not an
// interval kept: a request can be lost on the way, and anything else on the
// vehicle's channel can set the rates again — a REQUEST_DATA_STREAM from
// MAVProxy or another ground station puts every streamed message back to the
// stream's rate. So each window, every message this GCS asked a rate for that
// is arriving well short of it is asked for again. A vehicle that still does
// not keep it — a slow radio cannot carry every rate asked for — is asked every
// SLOW_RETRY after a few tries, rather than flooding the link.
//
// A window with nothing from the autopilot at all is the link down, not the
// rates, and judges nothing. Messages a vehicle sends only on an event are left
// out: HOME_POSITION before home is set, and the compass calibration's two
// outside a calibration.
//
// Holds no link and sends nothing. Vehicle's protocol loop counts arrivals into
// it and sends the requests it returns through MavlinkSender.

#include <chrono>
#include <cstdint>
#include <unordered_map>
#include <vector>

class StreamRateWatch {
public:
    using Clock = std::chrono::steady_clock;

    static constexpr double WINDOW_S   = 5.0;
    static constexpr double SHORT_FRAC = 0.7;   // under this share of the rate asked is short
    static constexpr int    FAST_TRIES = 3;     // then one try every SLOW_RETRY
    static constexpr auto   SLOW_RETRY = std::chrono::seconds(30);

    explicit StreamRateWatch(Clock::time_point now) : window_start_(now) {}

    // One message from the autopilot (not from other components on its sysid).
    void on_message(uint32_t msgid) { ++counts_[msgid]; ++total_; }

    // The connect burst went out: the first window starts with it, so the rates
    // the vehicle ran at before it are not held against it.
    void restart(Clock::time_point now);

    struct Event {
        enum class Kind {
            Back,         // was short, now at rate again
            AskAgain,     // short: send its interval again
        };
        enum class Note {
            None,
            FirstMiss,    // the first time it was found short
            GoingSlow,    // short after FAST_TRIES requests: asked every SLOW_RETRY from now
        };
        Kind     kind;
        Note     note     = Note::None;
        uint32_t msgid    = 0;
        int32_t  interval = 0;     // µs, as asked for; AskAgain resends it
        double   got_hz   = 0.0;
        double   want_hz  = 0.0;
    };

    // At the end of each window, judges every message in `requested` (id → µs,
    // MavlinkSender::requested_intervals()). Nothing before the burst.
    std::vector<Event> tick(bool burst_sent,
                            const std::unordered_map<uint32_t, int32_t>& requested,
                            Clock::time_point now);

private:
    struct Watch { int misses = 0; Clock::time_point next_try{}; };

    std::unordered_map<uint32_t, uint32_t> counts_;
    uint32_t                               total_ = 0;
    Clock::time_point                      window_start_;
    std::unordered_map<uint32_t, Watch>    watch_;
};

// A streamed message's name for the log: "ATTITUDE", or "msg 123".
const char* stream_name(uint32_t msgid, char (&buf)[24]);
