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

// Keeping one list's transfers moving: the plan's or the fence's.
//
// A download stalls for good on one lost item, since each request is sent by
// the reply to the last; this decides when to ask again for the item still
// missing, and when to give up. It also says when a download has completed and
// so wants its MISSION_ACK, and when an upload has been answered.
//
// Holds no link and sends nothing. Vehicle's protocol loop feeds it the
// published state and carries out what it returns through MavlinkSender.

#include <chrono>
#include <cstdint>

#include "backend/vehicle/state.hpp"   // VehicleState::UploadStatus

class MissionWatch {
public:
    using Clock = std::chrono::steady_clock;

    static constexpr int  MAX_RETRIES  = 5;
    static constexpr auto ITEM_TIMEOUT = std::chrono::milliseconds(1500);

    explicit MissionWatch(Clock::time_point now) : last_progress_(now) {}

    // Each item request the GCS sends for this list's download. Progress for
    // the stall timer; seq 0 is a new download and starts the retries over.
    void on_item_requested(uint16_t seq, Clock::time_point now);

    // Where the list stands, from the published VehicleState.
    struct Progress {
        uint32_t                   generation = 0;   // bumped per completed download
        uint16_t                   received   = 0;
        uint16_t                   count      = 0;
        VehicleState::UploadStatus upload     = VehicleState::UploadStatus::Idle;
    };

    struct Actions {
        // A download completed: say so, and acknowledge it. Every one is, not
        // just the first: an unacknowledged download leaves the vehicle
        // retrying the last item until its own timeout, and PX4 reports the
        // transfer as failed.
        bool     loaded         = false;
        // Ask again for item `re_request_seq`, the one still missing.
        bool     re_request     = false;
        uint16_t re_request_seq = 0;
        // Out of retries: the download is dead.
        bool     stalled        = false;
        // The vehicle answered an upload: report it and drop the stored items.
        enum class Upload { None, Accepted, Failed };
        Upload   upload         = Upload::None;
    };

    // Once per pass of the protocol loop.
    Actions tick(const Progress& p, Clock::time_point now);

private:
    uint32_t                   seen_generation_ = 0;
    int                        retries_         = 0;
    Clock::time_point          last_progress_;
    VehicleState::UploadStatus last_upload_     = VehicleState::UploadStatus::Idle;
};
