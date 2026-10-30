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


#include "terrain_clearance.hpp"
#include "mission_alt.hpp"
#include "terrain.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <unordered_map>

#ifndef M_PI
#  define M_PI 3.14159265358979323846
#endif

namespace {

float g_clearance_m = 30.0f;

// About the terrain's own spacing; finer would only re-read the same pixels.
constexpr double STEP_M      = 25.0;
constexpr int    MAX_SAMPLES = 200;   // a long leg is sampled coarser rather than slower

double rough_metres(double lat0, double lon0, double lat1, double lon1)
{
    constexpr double M_PER_DEG = 111320.0;
    const double dn = (lat1 - lat0) * M_PER_DEG;
    const double de = (lon1 - lon0) * M_PER_DEG * std::cos(0.5 * (lat0 + lat1) * M_PI / 180.0);
    return std::sqrt(dn * dn + de * de);
}

// The cache, keyed by the path's six numbers exactly. Cleared, not pruned,
// when it grows: a mission is a few dozen legs, and the GO HERE line a few
// more as the altitude field is typed into.
struct Key {
    double v[6];
    bool operator==(const Key& o) const { return std::memcmp(v, o.v, sizeof v) == 0; }
};
struct KeyHash {
    size_t operator()(const Key& k) const noexcept {
        size_t h = 1469598103934665603ull;
        const unsigned char* p = reinterpret_cast<const unsigned char*>(k.v);
        for (size_t i = 0; i < sizeof k.v; ++i) h = (h ^ p[i]) * 1099511628211ull;
        return h;
    }
};
std::unordered_map<Key, Clearance, KeyHash> g_cache;

Clearance compute(double lat0, double lon0, double amsl0, double lat1, double lon1, double amsl1)
{
    Clearance c;
    const int n = std::clamp((int)std::ceil(rough_metres(lat0, lon0, lat1, lon1) / STEP_M), 1, MAX_SAMPLES);
    c.complete = true;
    for (int i = 0; i <= n; ++i) {
        const double t   = (double)i / n;
        const double lat = lat0 + (lat1 - lat0) * t, lon = lon0 + (lon1 - lon0) * t;
        const double h   = amsl0 + (amsl1 - amsl0) * t;
        const auto   g   = terrain_amsl(lat, lon);
        if (!g) { c.complete = false; continue; }
        const float clear = (float)(h - *g);
        if (!c.has_min || clear < c.min_m) {
            c.has_min   = true;
            c.min_m     = clear;
            c.lat       = lat;
            c.lon       = lon;
            c.path_amsl = (float)h;
        }
    }
    return c;
}

} // namespace

float terrain_clearance_m()          { return g_clearance_m; }
void  set_terrain_clearance_m(float m) { g_clearance_m = std::max(0.0f, m); }

Clearance path_clearance(double lat0, double lon0, double amsl0,
                         double lat1, double lon1, double amsl1)
{
    // A new terrain source makes every answer so far about the old one.
    static uint64_t s_gen = terrain_generation();
    if (s_gen != terrain_generation()) { s_gen = terrain_generation(); g_cache.clear(); }

    const Key k{ { lat0, lon0, amsl0, lat1, lon1, amsl1 } };
    if (auto it = g_cache.find(k); it != g_cache.end() && it->second.complete)
        return it->second;
    if (g_cache.size() > 1024) g_cache.clear();
    return g_cache[k] = compute(lat0, lon0, amsl0, lat1, lon1, amsl1);
}

std::optional<float> plan_home_amsl(std::optional<float> reported,
                                    const double* home_lat, const double* home_lon,
                                    const std::vector<MissionItem>* mission)
{
    if (reported) return reported;
    if (home_lat && home_lon) return terrain_amsl(*home_lat, *home_lon);
    if (mission)
        for (const MissionItem& it : *mission)
            if (it.lat != 0.0 || it.lon != 0.0) return terrain_amsl(it.lat, it.lon);
    return std::nullopt;
}

std::vector<MissionLeg> mission_legs(const std::vector<MissionItem>& mission,
                                     std::optional<float> home)
{
    std::vector<MissionLeg> legs;
    int prev = -1;
    for (int i = 0; i < (int)mission.size(); ++i) {
        const MissionItem& it = mission[i];
        if (it.lat == 0.0 && it.lon == 0.0) continue;
        if (!mission_frame_is_global(it.frame)) continue;
        if (prev >= 0) {
            const MissionItem& a = mission[prev];
            const auto ga = terrain_amsl(a.lat, a.lon), gb = terrain_amsl(it.lat, it.lon);
            const auto ha = mission_item_amsl(a, home, ga), hb = mission_item_amsl(it, home, gb);
            const auto terrain_frame = [](uint8_t f) {
                return f == MAV_FRAME_GLOBAL_TERRAIN_ALT || f == MAV_FRAME_GLOBAL_TERRAIN_ALT_INT;
            };
            MissionLeg leg{ prev, i, {} };
            if (terrain_frame(a.frame) && terrain_frame(it.frame)) {
                // Flown at its height over the ground: as close as its lower end.
                const bool lower_a = a.alt <= it.alt;
                leg.c.complete  = true;
                leg.c.has_min   = true;
                leg.c.min_m     = std::min(a.alt, it.alt);
                leg.c.lat       = lower_a ? a.lat : it.lat;
                leg.c.lon       = lower_a ? a.lon : it.lon;
                leg.c.path_amsl = (lower_a ? (ga ? *ga : 0.0f) : (gb ? *gb : 0.0f)) + leg.c.min_m;
                legs.push_back(leg);
            } else if (ha && hb) {
                leg.c = path_clearance(a.lat, a.lon, *ha, it.lat, it.lon, *hb);
                legs.push_back(leg);
            }
        }
        prev = i;
    }
    return legs;
}
