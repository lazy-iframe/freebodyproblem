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
#include "backend/mission/download.hpp"

#include <utility>

bool MissionDownload::begin(uint16_t count)
{
    rx_.assign(count, MissionItem{});
    count_    = count;
    received_ = 0;
    return count > 0;
}

bool MissionDownload::on_item(const MissionItem& item)
{
    if (received_ >= count_ || item.seq != received_ || item.seq >= rx_.size())
        return false;
    rx_[item.seq] = item;
    ++received_;
    return true;
}

std::vector<MissionItem> MissionDownload::take(bool strip_home, MissionItem* home,
                                               bool* has_home)
{
    std::vector<MissionItem> items = std::move(rx_);
    rx_.clear();

    if (has_home) *has_home = false;
    if (strip_home && !items.empty()) {
        if (home)     *home     = items.front();
        if (has_home) *has_home = true;
        items.erase(items.begin());
    }
    for (size_t i = 0; i < items.size(); ++i) items[i].seq = (uint16_t)i;
    return items;
}
