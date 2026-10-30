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


// frontend/terrain_clearance.hpp
//
// How close a planned path comes to the ground, by the terrain data of
// frontend/terrain.hpp: a mission's legs, and the straight line GO HERE asks
// the vehicle to fly. Both are judged against one operator-set clearance;
// closer than that warns, through the ground is a collision.
//
// A leg is taken as the vehicle flies it: a straight line from one point to
// the next, the height changing evenly along it — except between two items
// in a terrain frame, which the autopilot flies at its height above the
// ground, so that leg is as close as the lower of the two.
//
// Results are cached by the leg's two ends, so asking every frame costs a
// lookup; a leg checked while its terrain was still loading is checked
// again until it has all of it.

#pragma once
#include "../backend/mavlink_parser.hpp"
#include <optional>
#include <vector>

// The clearance the operator wants kept, metres; from settings.
float terrain_clearance_m();
void  set_terrain_clearance_m(float m);

struct Clearance {
    bool   complete = false;   // every sample had terrain; else a lower bound on nothing
    bool   has_min  = false;   // at least one sample did
    float  min_m    = 0.0f;    // the least height above the ground along it
    double lat = 0.0, lon = 0.0;   // where that is
    float  path_amsl = 0.0f;       // and the path's own height there

    bool collides() const { return has_min && min_m < 0.0f; }
    bool too_close() const { return has_min && min_m < terrain_clearance_m(); }
};

// A straight path between two points at heights AMSL.
Clearance path_clearance(double lat0, double lon0, double amsl0,
                         double lat1, double lon1, double amsl1);

// Home's AMSL for planning: what the vehicle reported (`reported`), else the
// ground under its home position, else under the first placed item — so a
// plan made with no vehicle connected still stands on the ground it starts
// from. One rule for the mission tab, the 3D view and GO HERE, so their
// numbers agree.
std::optional<float> plan_home_amsl(std::optional<float> reported,
                                    const double* home_lat, const double* home_lon,
                                    const std::vector<MissionItem>* mission);

// One leg of a mission: from the item at index `from` to the one at `to`,
// both indices into the mission vector.
struct MissionLeg {
    int       from = 0, to = 0;
    Clearance c;
};

// Every leg between consecutive placed items (local frames and items with no
// position are stepped over). `home` is home's AMSL, for the relative frames;
// a leg whose ends cannot be put in height without it is left out.
std::vector<MissionLeg> mission_legs(const std::vector<MissionItem>& mission,
                                     std::optional<float> home);
