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

// frontend/components/map/map3d/overlays.cpp
//
// What is drawn over the 3D frame with ImGui: trails, the mission, the
// vehicles' drop lines and gizmos.

#include "frontend/components/map/map3d/map3d_internal.hpp"
#include "frontend/source/theme.hpp"
#include "frontend/components/others/ui_kit.hpp"
#include "frontend/source/mission_alt.hpp"
#include "frontend/source/terrain.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <optional>
#include <vector>

namespace map3d_detail {

// ── Overlays ──────────────────────────────────────────────────────────────────

// Lines dropped to the terrain are pink, every one of them — vehicle,
// waypoint, or the one being placed: a colour of their own, apart from the
// amber of the labels and marks and from anything in the imagery.
ImU32 drop_col(float alpha) { return IM_COL32(255, 105, 180, (int)(255.0f * alpha)); }

// What is inside the terrain data, through the terrain: the amber of an aircraft's
// cage, and dotted. The data is coarse and the operator may know better, so nothing
// is kept out of it, only shown for what it is.
ImU32 under_col(float alpha) { return IM_COL32(255, 194, 56, (int)(255.0f * alpha)); }

// A path flown from a to b: solid over the terrain, dotted in `under` where it is
// inside. It holds its height above sea level, so it follows the earth's curve; a
// straight line through the world would dip into the ground on a long leg flown level.
void draw_path(ImDrawList* dl, const Frame& f, V3 a, V3 b, ImU32 col, float w, ImU32 under)
{
    ImVec2 sa, sb;
    if (!f.segment(a, b, sa, sb)) return;
    const int n = std::clamp((int)(std::hypot(sb.x - sa.x, sb.y - sa.y) / 12.0f), 2, 64);
    const double da = f.drop(a.x, a.y), db = f.drop(b.x, b.y);
    const auto at = [&](double t) {
        V3 p = a + (b - a) * t;
        p.z += da + (db - da) * t - f.drop(p.x, p.y);
        return p;
    };
    const auto inside = [&](double t) {
        const V3   p  = at(t);
        const auto gz = ground_z(f, p.x, p.y);
        return gz && p.z < *gz;
    };
    int  start = 0;
    bool in    = inside(0.5 / n);
    for (int i = 1; i <= n; ++i) {
        const bool next = i < n && inside((i + 0.5) / n);
        if (i < n && next == in) continue;
        for (int k = start; k < i; ++k) {
            ImVec2 p, q;
            if (!f.segment(at((double)k / n), at((double)(k + 1) / n), p, q)) continue;
            if (in) map_dotted_line(dl, p, q, under, w);
            else    dl->AddLine(p, q, col, w);
        }
        start = i;
        in    = next;
    }
}

// A line from a point down to the ground under it, or up to it from inside, and a small dotted ring
// lying on the ground where it lands — drawn in the ground's plane, so in
// perspective it is an ellipse, and kept a few pixels across at any distance.
void drop_line(ImDrawList* dl, const Frame& f, V3 eye, V3 top, double ground_amsl, float alpha)
{
    const ImU32 col  = drop_col(0.92f * alpha);
    const V3    foot = { top.x, top.y, (ground_amsl - f.href) * f.exag - f.drop(top.x, top.y) };
    ImVec2 a, b;
    if (f.segment(top, foot, a, b)) {
        if (top.z < foot.z) map_dotted_line(dl, a, b, under_col(0.92f * alpha), 1.5f);
        else                dl->AddLine(a, b, col, 1.5f);
    }

    const V3     d = foot - eye;
    const double r = 7.0 * 2.0 * std::sqrt(dot(d, d)) * std::tan(FOV_Y * 0.5) / std::max(1.0f, f.size.y);
    constexpr int SEG = 16;   // drawn every other one: dotted
    for (int i = 0; i < SEG; i += 2) {
        const double a0 = 2 * M_PI * i / SEG, a1 = 2 * M_PI * (i + 1) / SEG;
        ImVec2 p, q;
        if (f.segment({ foot.x + r * std::cos(a0), foot.y + r * std::sin(a0), foot.z },
                      { foot.x + r * std::cos(a1), foot.y + r * std::sin(a1), foot.z }, p, q))
            dl->AddLine(p, q, col, 1.5f);
    }
}

void draw_trails(ImDrawList* dl, const Frame& f, const std::vector<MapVehicle>& fleet)
{
    static std::vector<ImVec2> run;
    for (const MapVehicle& mv : fleet) {
        const auto* pts = map_track_of(mv.id);
        if (!pts || pts->size() < 2) continue;
        const ImU32 col = ui_col(g_theme.map_track, mv.active ? 1.0f : 0.40f);
        const float w   = mv.active ? 2.5f : 1.5f;
        run.clear();
        const auto flush = [&] {
            if (run.size() >= 2) dl->AddPolyline(run.data(), (int)run.size(), col, ImDrawFlags_None, w);
            run.clear();
        };
        for (const TrackPoint& tp : *pts) {
            ImVec2 sp;
            if (!f.screen(f.world(tp.lat, tp.lon, tp.alt_amsl), sp)) { flush(); continue; }
            if (!run.empty() && std::fabs(sp.x - run.back().x) < 2.0f &&
                std::fabs(sp.y - run.back().y) < 2.0f) continue;
            run.push_back(sp);
        }
        flush();
    }
}

// A path checked against the terrain: drawn over in amber where it comes
// nearer the ground than the clearance set in settings, red where it goes
// into it, with a mark at its closest point saying by how much. Nothing for
// a path that clears.
void draw_clearance(ImDrawList* dl, const Frame& f, V3 a, V3 b, const Clearance& c)
{
    if (!c.too_close()) return;
    const ImU32 col = c.collides() ? ui_col(g_theme.col_error) : ui_col(g_theme.col_warning);
    draw_path(dl, f, a, b, col, 3.5f, col);
    ImVec2 sp;
    if (!f.screen(f.world(c.lat, c.lon, c.path_amsl), sp)) return;
    dl->AddCircleFilled(sp, 5.0f, col);
    dl->AddCircle(sp, 5.0f, IM_COL32(0, 0, 0, 200), 0, 1.5f);
    char t[40];
    if (c.collides()) snprintf(t, sizeof(t), "HITS TERRAIN ~%.0f m", (double)-c.min_m);
    else              snprintf(t, sizeof(t), "~%.0f m TO TERRAIN", (double)c.min_m);
    const ImVec2 ts = ImGui::CalcTextSize(t);
    // High enough to clear the readout of a point being set, when it is the closest.
    const ImVec2 tp = { sp.x + 8.0f, sp.y - ts.y - 16.0f };
    dl->AddRectFilled({ tp.x - 3, tp.y - 1 }, { tp.x + ts.x + 3, tp.y + ts.y + 1 }, IM_COL32(0, 0, 0, 180), 3.0f);
    dl->AddText(tp, col, t);
}

void draw_mission(ImDrawList* dl, const Frame& f, V3 eye, const std::vector<MissionItem>& mission,
                  const MapVehicle* active)
{
    const std::optional<float> home = plan_home_amsl(active, &mission);

    struct Placed { V3 p; ImVec2 sp; bool on_screen; float ground; bool below; uint16_t seq; bool current; };
    std::vector<Placed> placed;
    for (const MissionItem& it : mission) {
        if (it.lat == 0.0 && it.lon == 0.0) continue;
        if (!mission_frame_is_global(it.frame)) continue;
        const std::optional<float> ground = terrain_amsl(it.lat, it.lon);
        if (!ground) continue;
        const float amsl = mission_item_amsl(it, home, ground).value_or(*ground);
        Placed pl{ f.world(it.lat, it.lon, amsl), {}, false, *ground, amsl < *ground, it.seq, it.is_current };
        pl.on_screen = f.screen(pl.p, pl.sp);
        placed.push_back(pl);
    }

    const ImU32 path = IM_COL32(255, 200, 50, 200);
    for (size_t i = 1; i < placed.size(); ++i)
        draw_path(dl, f, placed[i - 1].p, placed[i].p, path, 2.0f, under_col());
    for (const Placed& pl : placed)
        drop_line(dl, f, eye, pl.p, pl.ground, 0.8f);

    // Legs too near the ground, over the plain path.
    const auto world_of = [&](const MissionItem& it) -> std::optional<V3> {
        const auto g = terrain_amsl(it.lat, it.lon);
        const auto h = mission_item_amsl(it, home, g);
        if (!h) return std::nullopt;
        return f.world(it.lat, it.lon, *h);
    };
    for (const MissionLeg& lg : mission_legs(mission, home)) {
        const auto a = world_of(mission[lg.from]), b = world_of(mission[lg.to]);
        if (a && b) draw_clearance(dl, f, *a, *b, lg.c);
    }

    constexpr float R = 8.0f;
    for (const Placed& pl : placed) {
        if (!pl.on_screen) continue;
        const ImU32 fill = pl.below   ? ui_col(g_theme.col_error, 0.95f)
                         : pl.current ? IM_COL32(255, 100, 50, 220)
                                      : IM_COL32( 50, 150, 255, 200);
        dl->AddCircleFilled(pl.sp, R, fill);
        dl->AddCircle(pl.sp, R, IM_COL32(255, 255, 255, 220), 0, 1.5f);
        char s[8];
        snprintf(s, sizeof(s), "%u", (unsigned)pl.seq);
        const ImVec2 ts = ImGui::CalcTextSize(s);
        dl->AddText({ pl.sp.x - ts.x * 0.5f, pl.sp.y - ts.y * 0.5f }, IM_COL32(255, 255, 255, 255), s);
    }
}

// The body axes as a gizmo, each reaching past the airframe and lettered at
// its end: X forward in the map's amber — the heading, the one most read —
// Y to the right in green, Z down in blue. Attitude reads off them from any angle,
// and they show through terrain, which the mesh does not.
static void draw_gizmo(ImDrawList* dl, const Frame& f, const Pose& ps, float alpha)
{
    struct Axis { V3 dir; double len; ImU32 col; const char* name; };
    const int   a8 = (int)(255.0f * alpha);
    const Axis  axes[3] = {
        { ps.b.x, 1.9, map_amber_u32(alpha),        "X" },
        { ps.b.y, 1.6, IM_COL32( 80, 220, 100, a8), "Y" },
        { ps.b.z, 1.2, IM_COL32( 80, 150, 255, a8), "Z" },
    };
    for (const Axis& ax : axes) {
        ImVec2 a, b;
        if (!f.segment(ps.p, ps.p + ax.dir * (ax.len * ps.s), a, b)) continue;
        dl->AddLine(a, b, ax.col, 2.0f);
        dl->AddCircleFilled(b, 2.5f, ax.col);
        dl->AddText({ b.x + 3.0f, b.y - 7.0f }, ax.col, ax.name);
    }
}

void draw_vehicles(ImDrawList* dl, const Frame& f, V3 eye, const std::vector<MapVehicle>& fleet)
{
    const auto one = [&](const MapVehicle& mv) {
        const float A = mv.active ? 1.0f : 0.55f;
        if (mv.has_home) {
            const std::optional<float> g = terrain_amsl(mv.home_lat, mv.home_lon);
            const double h = g ? *g : mv.has_home_alt ? mv.home_alt_amsl : f.href;
            ImVec2 sp;
            if (f.screen(f.world(mv.home_lat, mv.home_lon, h), sp))
                map_draw_home_symbol(dl, sp, mv.active);
        }
        if (!mv.has_pos) return;
        const Pose ps = vehicle_pose(f, eye, mv);
        if (mv.active && mv.has_home) {
            const std::optional<float> g = terrain_amsl(mv.home_lat, mv.home_lon);
            const double h = g ? *g : mv.has_home_alt ? mv.home_alt_amsl : f.href;
            ImVec2 a, b;
            if (f.segment(f.world(mv.home_lat, mv.home_lon, h), ps.p, a, b))
                map_draw_los(dl, a, b, mv);
        }
        if (ps.ground) drop_line(dl, f, eye, ps.p, *ps.ground, A);
        if (mv.has_att) draw_gizmo(dl, f, ps, A);
        ImVec2 sp;
        if (!f.screen(ps.p, sp)) return;
        if (mv.checked)   // in the SWARM view's command set, as the 2D ring says
            dl->AddCircle(sp, 30.0f, map_amber_u32(), 0, 2.0f);
        // Its name and height above the terrain, as the 2D map labels it —
        // the height in red and saying so when the aircraft is inside the
        // terrain, where it is otherwise only its gizmo and this, the mesh
        // hidden in the hill.
        map_draw_vehicle_tags(dl, sp, 38.0f, mv);
    };
    for (const MapVehicle& mv : fleet) if (!mv.active) one(mv);
    for (const MapVehicle& mv : fleet) if ( mv.active) one(mv);
}

} // namespace map3d_detail
