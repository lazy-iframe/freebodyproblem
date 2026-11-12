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

// The map's position trails, per vehicle: kept here, drawn by both views.

#include "frontend/components/map/map_internal.hpp"
#include "lib/geo.hpp"
#include <cmath>
#include <deque>

namespace map_detail {

TrackRegistry g_tracks;

} // namespace map_detail

using namespace map_detail;

// ── Position trail ────────────────────────────────────────────────────────────
//
// Where each vehicle has been, kept per vehicle so switching the callsign chip
// shows that aircraft's track and not a line drawn between the two.

namespace {

// Thinning distance. Below this the aircraft has not moved enough to be worth a
// point — GPS noise on a vehicle standing still would otherwise fill the trail
// with a smudge the size of its own position error.
constexpr double TRACK_MIN_STEP_M = 3.0;

// Trail length. At 3 m spacing this is 18 km of ground track, past which the
// oldest point is dropped for each new one. 6000 points is ~96 KB per vehicle
// and one polyline call, so the cap is about the trail staying readable rather
// than about the memory.
constexpr size_t TRACK_MAX_POINTS = 6000;

} // namespace

void map_track_pump(const VehicleState& vs)
{
    VehicleTrack& track = g_tracks.by_vehicle[ui_bound_vehicle()];
    if (vs.has_heartbeat) {
        if (track.armed_known && !track.armed && vs.armed) track.pts.clear();
        track.armed       = vs.armed;
        track.armed_known = true;
    }

    if (!vs.has_global_pos)              return;
    // A fix the vehicle has not got yet reads as the origin, and a trail
    // through Null Island is worse than no trail.
    if (vs.lat == 0.0 && vs.lon == 0.0)  return;

    auto& pts = track.pts;
    // Height counts as movement too: the 3D view draws the climb out of a
    // vertical takeoff, which never moves the aircraft across the ground.
    if (!pts.empty() &&
        geo_rough_distance_m(pts.back().lat, pts.back().lon, vs.lat, vs.lon)
            < TRACK_MIN_STEP_M &&
        std::fabs(pts.back().alt_amsl - vs.alt_asl) < TRACK_MIN_STEP_M)
        return;

    pts.push_back({ vs.lat, vs.lon, vs.alt_asl });
    if (pts.size() > TRACK_MAX_POINTS) pts.pop_front();
}

void map_track_clear()
{
    g_tracks.by_vehicle[ui_bound_vehicle()].pts.clear();
}

const std::deque<TrackPoint>* map_track_of(VehicleId id)
{
    const auto it = g_tracks.by_vehicle.find(id);
    return it == g_tracks.by_vehicle.end() ? nullptr : &it->second.pts;
}
