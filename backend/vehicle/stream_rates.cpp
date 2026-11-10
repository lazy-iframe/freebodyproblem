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
#include "backend/vehicle/stream_rates.hpp"

#include <cstdio>

#include <mavlink/ardupilotmega/mavlink.h>

void StreamRateWatch::restart(Clock::time_point now)
{
    counts_.clear();
    total_        = 0;
    window_start_ = now;
    watch_.clear();
}

std::vector<StreamRateWatch::Event>
StreamRateWatch::tick(bool burst_sent, const std::unordered_map<uint32_t, int32_t>& requested,
                      Clock::time_point now)
{
    std::vector<Event> out;
    const double win = std::chrono::duration<double>(now - window_start_).count();
    if (!burst_sent || win < WINDOW_S) return out;

    if (total_ > 0)
        for (const auto& [id, us] : requested) {
            if (us <= 0) continue;
            if (id == MAVLINK_MSG_ID_HOME_POSITION ||
                id == MAVLINK_MSG_ID_MAG_CAL_PROGRESS ||
                id == MAVLINK_MSG_ID_MAG_CAL_REPORT) continue;
            const double want = 1e6 / us;
            const double got  = counts_[id] / win;
            Watch&       w    = watch_[id];
            if (got >= SHORT_FRAC * want) {
                if (w.misses > 0)
                    out.push_back({Event::Kind::Back, Event::Note::None, id, us, got, want});
                w = {};
                continue;
            }
            if (now < w.next_try) continue;
            ++w.misses;
            Event e{Event::Kind::AskAgain, Event::Note::None, id, us, got, want};
            if (w.misses == 1)               e.note = Event::Note::FirstMiss;
            else if (w.misses == FAST_TRIES) e.note = Event::Note::GoingSlow;
            out.push_back(e);
            if (w.misses >= FAST_TRIES) w.next_try = now + SLOW_RETRY;
        }
    counts_.clear();
    total_        = 0;
    window_start_ = now;
    return out;
}

const char* stream_name(uint32_t id, char (&name)[24])
{
    switch (id) {
    case MAVLINK_MSG_ID_ATTITUDE:            snprintf(name, sizeof(name), "ATTITUDE");            break;
    case MAVLINK_MSG_ID_VFR_HUD:             snprintf(name, sizeof(name), "VFR_HUD");             break;
    case MAVLINK_MSG_ID_GLOBAL_POSITION_INT: snprintf(name, sizeof(name), "GLOBAL_POSITION_INT"); break;
    case MAVLINK_MSG_ID_SYS_STATUS:          snprintf(name, sizeof(name), "SYS_STATUS");          break;
    case MAVLINK_MSG_ID_GPS_RAW_INT:         snprintf(name, sizeof(name), "GPS_RAW_INT");         break;
    case MAVLINK_MSG_ID_EKF_STATUS_REPORT:   snprintf(name, sizeof(name), "EKF_STATUS_REPORT");   break;
    case MAVLINK_MSG_ID_ESTIMATOR_STATUS:    snprintf(name, sizeof(name), "ESTIMATOR_STATUS");    break;
    case MAVLINK_MSG_ID_RC_CHANNELS:         snprintf(name, sizeof(name), "RC_CHANNELS");         break;
    default:                                 snprintf(name, sizeof(name), "msg %u", (unsigned)id);
    }
    return name;
}
