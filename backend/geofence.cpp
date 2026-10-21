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


#include "geofence.hpp"

#include <cmath>

namespace {

// MAV_FRAME_GLOBAL: fence items carry no altitude that means anything, and
// both stacks take this frame for them.
constexpr uint8_t FENCE_FRAME = 0;

MissionItem fence_item(uint16_t cmd, double lat, double lon, float p1)
{
    MissionItem it{};
    it.command      = cmd;
    it.frame        = FENCE_FRAME;
    it.lat          = lat;
    it.lon          = lon;
    it.alt          = 0.0f;
    it.param1       = p1;
    it.autocontinue = false;
    return it;
}

} // namespace

bool fence_shape_valid(const FenceShape& s)
{
    return s.kind == FenceShape::Kind::Polygon ? s.pts.size() >= 3
                                               : s.radius_m > 0.0f;
}

std::vector<MissionItem> fence_to_items(const FencePlan& plan)
{
    std::vector<MissionItem> out;
    if (plan.has_return) {
        MissionItem rp = plan.return_point;
        rp.command = FENCE_CMD_RETURN_POINT;
        out.push_back(rp);
    }
    for (const FenceShape& s : plan.shapes) {
        if (!fence_shape_valid(s)) continue;
        if (s.kind == FenceShape::Kind::Polygon) {
            const uint16_t cmd = s.inclusion ? FENCE_CMD_POLY_INCLUSION
                                             : FENCE_CMD_POLY_EXCLUSION;
            for (const FenceVertex& v : s.pts)
                out.push_back(fence_item(cmd, v.lat, v.lon, (float)s.pts.size()));
        } else {
            const uint16_t cmd = s.inclusion ? FENCE_CMD_CIRC_INCLUSION
                                             : FENCE_CMD_CIRC_EXCLUSION;
            out.push_back(fence_item(cmd, s.center.lat, s.center.lon, s.radius_m));
        }
    }
    for (size_t i = 0; i < out.size(); ++i) {
        out[i].seq        = (uint16_t)i;
        out[i].is_current = false;
    }
    return out;
}

FencePlan fence_from_items(const std::vector<MissionItem>& items)
{
    FencePlan plan;
    size_t i = 0;
    while (i < items.size()) {
        const MissionItem& it = items[i];
        switch (it.command) {
        case FENCE_CMD_RETURN_POINT:
            plan.has_return   = true;
            plan.return_point = it;
            ++i;
            break;

        case FENCE_CMD_POLY_INCLUSION:
        case FENCE_CMD_POLY_EXCLUSION: {
            const size_t n = (size_t)std::lround(it.param1);
            bool ok = n >= 3 && i + n <= items.size();
            for (size_t k = 0; ok && k < n; ++k)
                ok = items[i + k].command == it.command &&
                     (size_t)std::lround(items[i + k].param1) == n;
            if (!ok) { ++i; break; }   // malformed: skip this item, resync on the next
            FenceShape s;
            s.kind      = FenceShape::Kind::Polygon;
            s.inclusion = it.command == FENCE_CMD_POLY_INCLUSION;
            for (size_t k = 0; k < n; ++k)
                s.pts.push_back({ items[i + k].lat, items[i + k].lon });
            plan.shapes.push_back(std::move(s));
            i += n;
            break;
        }

        case FENCE_CMD_CIRC_INCLUSION:
        case FENCE_CMD_CIRC_EXCLUSION: {
            FenceShape s;
            s.kind      = FenceShape::Kind::Circle;
            s.inclusion = it.command == FENCE_CMD_CIRC_INCLUSION;
            s.center    = { it.lat, it.lon };
            s.radius_m  = it.param1;
            if (fence_shape_valid(s)) plan.shapes.push_back(s);
            ++i;
            break;
        }

        default:
            ++i;   // not a fence command; nothing to show for it
            break;
        }
    }
    return plan;
}
