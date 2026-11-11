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

// frontend/components/map/map3d/bands.cpp
//
// The mission's flight path and the geofence walls, sampled over the terrain
// as bands for the scene to draw.

#include "frontend/components/map/map3d/map3d_internal.hpp"
#include "frontend/source/mission_alt.hpp"
#include "frontend/source/terrain.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace map3d_detail {

// ── Mission ground projection ─────────────────────────────────────────────────
//
// The planned path cast straight down onto the terrain: a line along the
// ground under every leg, and the upright band between it and the path tinted
// a faint blue, so how high each leg runs over the ground — and where the
// ground rises to meet it — reads from any angle. Sampled along each leg at
// the terrain's own spacing, the way the clearance check reads it.

// The bands are sampled once, as ground and top heights AMSL at each point,
// and kept: reading the terrain takes its cache's lock, and a long mission or
// a wide fence is thousands of reads. Each frame only places the samples. A
// band is sampled again when what it is made from changes — the mission, the
// zones, home's height, the terrain source — and, while some of its terrain
// was not yet loaded, every REBAND_FRAMES until it all is.
constexpr int REBAND_FRAMES = 15;

struct BandSample { double lat, lon, gnd, top; };   // gnd and top AMSL
struct Band {
    std::vector<std::vector<BandSample>> runs;      // unbroken stretches of samples
    TintLayer tint;                                  // its colour; the geometry is made per frame
    bool top_edge = false;
};
struct BandSet {
    std::vector<Band> bands;
    bool     complete = false;   // every sample had terrain
    uint64_t gen      = 0;       // terrain_generation() when sampled
    uint64_t frame    = 0;       // when sampled
    bool     built    = false;

    // Whether it must be sampled again, given that its inputs are unchanged
    // or not (`same`).
    bool stale(bool same) const
    {
        return !built || !same || gen != terrain_generation() ||
               (!complete && s_frame - frame >= REBAND_FRAMES);
    }
    void begin()
    {
        bands.clear();
        complete = true;
        gen      = terrain_generation();
        frame    = s_frame;
        built    = true;
    }
};

// The band's quads and edges for this frame's view. The ground line is lifted
// a touch off the surface, so the surface does not swallow it; more with
// distance, where depth is coarser.
void place_bands(const Frame& f, V3 eye, const BandSet& set, Projection& out)
{
    for (const Band& b : set.bands) {
        TintLayer& L = out.layers.emplace_back();
        L.r = b.tint.r; L.g = b.tint.g; L.b = b.tint.b;
        for (const auto& run : b.runs) {
            V3 pt{}, pg{};
            for (size_t k = 0; k < run.size(); ++k) {
                const BandSample& s = run[k];
                const V3 top = f.world(s.lat, s.lon, s.top);
                V3 bot = f.world(s.lat, s.lon, s.gnd);
                const V3 d = bot - eye;
                bot.z += std::max(1.0, 0.002 * std::sqrt(dot(d, d)));
                if (k) band_step(L, pg, pt, bot, top, b.top_edge);
                pt = top;
                pg = bot;
            }
        }
    }
}

// A run of samples along a straight edge from a to b, about STEP_M apart and
// at most `max_n` steps; `top` gives the top AMSL at fraction t and the
// ground there. A sample without terrain ends the run and marks the set
// incomplete.
template <class Top>
static void sample_edge(double alat, double alon, double blat, double blon, int max_n, Top top,
                 Band& band, BandSet& set)
{
    constexpr double STEP_M = 25.0;
    const double m_lat = 111320.0, m_lon = 111320.0 * std::cos(alat * DEG);
    const double len = std::hypot((blat - alat) * m_lat, (blon - alon) * m_lon);
    const int    n   = std::clamp((int)std::ceil(len / STEP_M), 1, max_n);
    std::vector<BandSample>* run = nullptr;
    for (int k = 0; k <= n; ++k) {
        const double t   = (double)k / n;
        const double lat = alat + (blat - alat) * t, lon = alon + (blon - alon) * t;
        const auto   gnd = terrain_amsl(lat, lon);
        if (!gnd) { set.complete = false; run = nullptr; continue; }
        if (!run) run = &band.runs.emplace_back();
        run->push_back({ lat, lon, (double)*gnd, top(t, (double)*gnd) });
    }
    if (run && run->size() < 2) band.runs.pop_back();
}

// What the mission's band is made from, to tell when it changes.
struct LegPoint {
    double lat, lon; float alt; uint8_t frame;
    bool operator==(const LegPoint& o) const
    { return lat == o.lat && lon == o.lon && alt == o.alt && frame == o.frame; }
};

// The mission's band, sampled again when the mission or home's height has
// changed.
const BandSet& mission_bands(const std::vector<MissionItem>& mission, const MapVehicle* active)
{
    static BandSet               s_set;
    static std::vector<LegPoint> s_pts;
    static std::optional<float>  s_home;

    const std::optional<float> home = plan_home_amsl(active, &mission);
    std::vector<LegPoint> pts;
    for (const MissionItem& it : mission) {
        if (it.lat == 0.0 && it.lon == 0.0) continue;
        if (!mission_frame_is_global(it.frame)) continue;
        pts.push_back({ it.lat, it.lon, it.alt, it.frame });
    }
    if (!s_set.stale(pts == s_pts && home == s_home)) return s_set;
    s_pts  = std::move(pts);
    s_home = home;
    s_set.begin();

    const auto terrain_frame = [](uint8_t fr) {
        return fr == MAV_FRAME_GLOBAL_TERRAIN_ALT || fr == MAV_FRAME_GLOBAL_TERRAIN_ALT_INT;
    };
    const auto item = [](const LegPoint& p) {
        MissionItem it{};
        it.lat = p.lat; it.lon = p.lon; it.alt = p.alt; it.frame = p.frame;
        return it;
    };
    Band& band = s_set.bands.emplace_back();
    band.tint.r = 0.40f; band.tint.g = 0.65f; band.tint.b = 1.00f;   // blue
    for (size_t i = 1; i < s_pts.size(); ++i) {
        const LegPoint& a = s_pts[i - 1];
        const LegPoint& b = s_pts[i];
        const auto ga = terrain_amsl(a.lat, a.lon), gb = terrain_amsl(b.lat, b.lon);
        if (!ga || !gb) s_set.complete = false;
        const auto ha = mission_item_amsl(item(a), home, ga), hb = mission_item_amsl(item(b), home, gb);
        const bool follow = terrain_frame(a.frame) && terrain_frame(b.frame);
        if (!follow && !(ha && hb)) continue;
        sample_edge(a.lat, a.lon, b.lat, b.lon, 120, [&](double t, double gnd) {
            return follow ? gnd + a.alt + (b.alt - a.alt) * t : *ha + (*hb - *ha) * t;
        }, band, s_set);
    }
    return s_set;
}

// The geofence as walls: every uploaded zone, and the parameter cylinder round
// the active vehicle's home, standing on the terrain up to the ceiling —
// FENCE_ALT_MAX above home. Inclusion zones and the cylinder green, exclusion
// zones red. Without a ceiling the walls stand FENCE_DEFAULT_H above the
// ground, so the zone still shows; with one, their tops are drawn too.
// Sampled again when the zones, the cylinder or the ceiling have changed.
constexpr double FENCE_DEFAULT_H = 60.0;

const BandSet& fence_bands(const FencePlan* zones, const MapVehicle* active,
                           const std::vector<MissionItem>* mission)
{
    using Ring = std::vector<FenceVertex>;
    static BandSet               s_set;
    static std::vector<Ring>     s_rings[2];   // [0] exclusion, [1] inclusion
    static std::optional<double> s_ceiling;

    const std::optional<float> home = plan_home_amsl(active, mission);
    const bool fence_on = active && !active->fence.disabled();
    std::optional<double> ceiling;   // AMSL
    if (fence_on && home && active->fence.ceiling_configured()) ceiling = *home + active->fence.alt_max_m;

    std::vector<Ring> rings[2];
    const auto circle = [](FenceVertex c, double r_m) {
        // Inside the lambda: it captures nothing, and MSVC will not read an
        // enclosing constexpr local without a capture (C3493).
        constexpr int CIRCLE_SEG = 72;
        Ring ring;
        const double m_lat = 111320.0, m_lon = 111320.0 * std::cos(c.lat * DEG);
        for (int i = 0; i < CIRCLE_SEG; ++i) {
            const double a = 2 * M_PI * i / CIRCLE_SEG;
            ring.push_back({ c.lat + r_m * std::cos(a) / m_lat, c.lon + r_m * std::sin(a) / m_lon });
        }
        return ring;
    };
    if (zones)
        for (const FenceShape& sh : zones->shapes) {
            if (sh.kind == FenceShape::Kind::Circle) {
                if (sh.radius_m > 0.0f) rings[sh.inclusion].push_back(circle(sh.center, sh.radius_m));
            } else if (sh.pts.size() >= 3) {
                rings[sh.inclusion].push_back(sh.pts);
            }
        }
    if (fence_on && active->has_home && active->fence.circle_configured())
        rings[1].push_back(circle({ active->home_lat, active->home_lon }, active->fence.radius_m));

    const auto same_rings = [](const std::vector<Ring>& x, const std::vector<Ring>& y) {
        if (x.size() != y.size()) return false;
        for (size_t i = 0; i < x.size(); ++i) {
            if (x[i].size() != y[i].size()) return false;
            for (size_t k = 0; k < x[i].size(); ++k)
                if (x[i][k].lat != y[i][k].lat || x[i][k].lon != y[i][k].lon) return false;
        }
        return true;
    };
    const bool same = same_rings(rings[0], s_rings[0]) && same_rings(rings[1], s_rings[1]) &&
                      ceiling == s_ceiling;
    if (!s_set.stale(same)) return s_set;
    s_rings[0] = std::move(rings[0]);
    s_rings[1] = std::move(rings[1]);
    s_ceiling  = ceiling;
    s_set.begin();

    for (int kind = 0; kind < 2; ++kind) {
        if (s_rings[kind].empty()) continue;
        Band& band = s_set.bands.emplace_back();
        band.tint     = kind ? TintLayer{ {}, {}, 0.35f, 0.90f, 0.40f }
                             : TintLayer{ {}, {}, 1.00f, 0.32f, 0.30f };
        band.top_edge = ceiling.has_value();
        for (const Ring& ring : s_rings[kind])
            for (size_t e = 0; e < ring.size(); ++e) {
                const FenceVertex& a = ring[e];
                const FenceVertex& b = ring[(e + 1) % ring.size()];
                // A ceiling below the ground there leaves no wall to draw.
                sample_edge(a.lat, a.lon, b.lat, b.lon, 200, [&](double, double gnd) {
                    return ceiling ? std::max(*ceiling, gnd) : gnd + FENCE_DEFAULT_H;
                }, band, s_set);
            }
    }
    return s_set;
}

} // namespace map3d_detail
