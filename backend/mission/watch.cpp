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
#include "backend/mission/watch.hpp"

void MissionWatch::on_item_requested(uint16_t seq, Clock::time_point now)
{
    last_progress_ = now;
    if (seq == 0) retries_ = 0;
}

MissionWatch::Actions MissionWatch::tick(const Progress& p, Clock::time_point now)
{
    Actions a;

    if (p.generation != seen_generation_) {
        seen_generation_ = p.generation;
        a.loaded = true;
        retries_ = 0;
    }

    if (p.received < p.count && retries_ < MAX_RETRIES &&
        now - last_progress_ > ITEM_TIMEOUT) {
        ++retries_;
        last_progress_ = now;
        if (retries_ == MAX_RETRIES) {
            a.stalled = true;
        } else {
            a.re_request     = true;
            a.re_request_seq = p.received;
        }
    }

    if (p.upload != last_upload_) {
        last_upload_ = p.upload;
        if (p.upload == VehicleState::UploadStatus::Accepted)
            a.upload = Actions::Upload::Accepted;
        else if (p.upload == VehicleState::UploadStatus::Failed)
            a.upload = Actions::Upload::Failed;
    }
    return a;
}
