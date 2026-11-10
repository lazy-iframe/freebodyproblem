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

// One list's download, as the mission protocol hands it over: MISSION_COUNT,
// then one MISSION_ITEM_INT per request, each request sent by the reply to the
// last. The plan and the fence each have one.
//
// Holds no link and sends nothing. MavlinkParser decodes the messages and feeds
// them here; Vehicle sends the requests it says are due.

#include <cstdint>
#include <vector>

#include "backend/mission/types.hpp"

class MissionDownload {
public:
    // MISSION_COUNT: starts the list over at `count` items. True when there is
    // an item to ask for (seq 0); false when the list is empty, which is a
    // complete download already.
    bool begin(uint16_t count);

    // MISSION_ITEM_INT. Items are requested one at a time, so only the one asked
    // for is taken. A duplicate — the reply to a re-request that crossed the
    // original on the wire — would otherwise count twice, end the download one
    // item early and leave a blank where the last item belongs. True when taken.
    bool on_item(const MissionItem& item);

    uint16_t received() const { return received_; }
    uint16_t count()    const { return count_; }
    bool     complete() const { return received_ >= count_; }

    // The finished list, numbered from 0 as the operator sees it. With
    // `strip_home`, the first item is the firmware's home slot rather than part
    // of the plan: it is handed back through `home` and `has_home`, and the
    // rest are numbered without it.
    std::vector<MissionItem> take(bool strip_home, MissionItem* home = nullptr,
                                  bool* has_home = nullptr);

private:
    std::vector<MissionItem> rx_;
    uint16_t                 received_ = 0;
    uint16_t                 count_    = 0;
};
