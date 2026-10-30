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


// frontend/mission_alt.hpp
//
// Where a mission item is in height. Its altitude means what its frame says:
// absolute is AMSL already, relative is above home, terrain frames are above
// the ground under it. The mission tab and the 3D map both turn it into AMSL
// here, so the number in the list and the marker in the air agree.

#pragma once
#include "../backend/mavlink_parser.hpp"
#include <optional>

// Home's altitude AMSL: HOME_POSITION, else the home slot of a downloaded
// mission. Neither before the vehicle has told us.
inline std::optional<float> home_amsl(const VehicleState* vs)
{
    if (!vs) return std::nullopt;
    if (vs->has_home)         return vs->home_alt_asl;
    if (vs->has_mission_home) return vs->mission_home.alt;
    return std::nullopt;
}

// Does this frame put the item somewhere on the earth's surface?
inline bool mission_frame_is_global(uint8_t frame)
{
    switch (frame) {
    case MAV_FRAME_GLOBAL:
    case MAV_FRAME_GLOBAL_INT:
    case MAV_FRAME_GLOBAL_RELATIVE_ALT:
    case MAV_FRAME_GLOBAL_RELATIVE_ALT_INT:
    case MAV_FRAME_GLOBAL_TERRAIN_ALT:
    case MAV_FRAME_GLOBAL_TERRAIN_ALT_INT:
        return true;
    default:
        return false;
    }
}

// The item's altitude in metres AMSL. `home` is home's AMSL, needed for the
// relative frames; `ground` the terrain under the item, for the terrain ones.
// nullopt when the one its frame needs is missing, or for a local frame.
inline std::optional<float> mission_item_amsl(const MissionItem& item,
                                              std::optional<float> home,
                                              std::optional<float> ground)
{
    switch (item.frame) {
    case MAV_FRAME_GLOBAL:
    case MAV_FRAME_GLOBAL_INT:
        return item.alt;
    case MAV_FRAME_GLOBAL_RELATIVE_ALT:
    case MAV_FRAME_GLOBAL_RELATIVE_ALT_INT:
        if (home) return *home + item.alt;
        return std::nullopt;
    case MAV_FRAME_GLOBAL_TERRAIN_ALT:
    case MAV_FRAME_GLOBAL_TERRAIN_ALT_INT:
        if (ground) return *ground + item.alt;
        return std::nullopt;
    default:
        return std::nullopt;
    }
}
