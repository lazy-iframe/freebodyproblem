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

// ── Geofences ─────────────────────────────────────────────────────────────────
//
// A vehicle can be fenced two ways, and this GCS shows both:
//
//   The simple fence, set by parameters: a cylinder around home — a radius and
//   a ceiling (ArduPilot FENCE_RADIUS / FENCE_ALT_MAX, PX4 GF_MAX_HOR_DIST /
//   GF_MAX_VER_DIST). Nothing to upload; FenceParams below is what the vehicle
//   reported, for the map to draw.
//
//   The uploaded fence: inclusion and exclusion zones, each a polygon or a
//   circle, sent over the mission protocol as its own list
//   (MAV_MISSION_TYPE_FENCE). On the wire a polygon is one item per vertex,
//   every one of them carrying the vertex count; a circle is one item. FencePlan
//   is the shapes, and fence_to_items / fence_from_items convert.

#include "mavlink_parser.hpp"   // MissionItem

#include <cstdint>
#include <vector>

struct FenceVertex { double lat = 0.0, lon = 0.0; };

struct FenceShape {
    enum class Kind : uint8_t { Polygon, Circle };
    Kind kind      = Kind::Polygon;
    bool inclusion = true;              // vehicle must stay inside / outside

    std::vector<FenceVertex> pts;       // Polygon: its vertices, in order
    FenceVertex center;                 // Circle
    float       radius_m = 0.0f;        // Circle
};

struct FencePlan {
    std::vector<FenceShape> shapes;

    // MAV_CMD_NAV_FENCE_RETURN_POINT, where ArduPilot Plane flies on a breach.
    // Not edited here, but carried through: a fence fetched with one and
    // uploaded again keeps it.
    bool        has_return = false;
    MissionItem return_point{};
};

// MAV_CMD values for fence items (common.xml).
constexpr uint16_t FENCE_CMD_RETURN_POINT   = 5000;
constexpr uint16_t FENCE_CMD_POLY_INCLUSION = 5001;
constexpr uint16_t FENCE_CMD_POLY_EXCLUSION = 5002;
constexpr uint16_t FENCE_CMD_CIRC_INCLUSION = 5003;
constexpr uint16_t FENCE_CMD_CIRC_EXCLUSION = 5004;

// The plan as the list the vehicle takes, numbered from 0.
std::vector<MissionItem> fence_to_items(const FencePlan& plan);

// Back from a downloaded list. A polygon whose vertices disagree on their
// count, or that runs off the end of the list, is dropped rather than drawn
// as something the vehicle does not hold.
FencePlan fence_from_items(const std::vector<MissionItem>& items);

// A polygon needs three vertices and a circle a radius before it fences
// anything; the editor does not upload one that does not.
bool fence_shape_valid(const FenceShape& s);
