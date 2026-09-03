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


#include "timesync.hpp"

#include <chrono>

int64_t timesync_monotonic_ns()
{
    using namespace std::chrono;
    return duration_cast<nanoseconds>(
               steady_clock::now().time_since_epoch()).count();
}

int64_t TimeSync::begin_request(int64_t now_ns)
{
    pending_ts1_ = now_ns;
    return now_ns;
}

bool TimeSync::on_timesync(int64_t tc1, int64_t ts1,
                           uint8_t sender_sysid, uint8_t sender_compid,
                           int64_t now_ns, Reply* out)
{
    if (tc1 == 0) {
        // The vehicle is opening an exchange of its own. Answer with our clock
        // and its ts1 untouched — it does the same arithmetic at its end.
        if (out) {
            out->tsys  = sender_sysid;
            out->tcomp = sender_compid;
            out->tc1   = now_ns;
            out->ts1   = ts1;
        }
        return true;
    }

    // Only the echo of the ts1 we actually sent is the answer to our request.
    // ArduPilot emits TIMESYNC unprompted as well, and its clock restarts at
    // zero on reboot; without this check one of those readings could be taken
    // for a round trip and the offset would be wrong by the whole uptime.
    if (ts1 == 0 || ts1 != pending_ts1_)
        return false;
    pending_ts1_ = 0;

    // Midpoint of the round trip: the instant the vehicle's tc1 applies, as
    // near as this end can tell. What is left in the estimate is half the
    // difference between the two legs' latencies.
    offset_ns_ = tc1 - (ts1 + now_ns) / 2;
    synced_    = true;
    return false;
}
