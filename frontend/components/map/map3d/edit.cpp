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

// frontend/components/map/map3d/edit.cpp
//
// EDIT in 3D: grabbing a waypoint's marker to move it over the ground, or its
// line to change its height.

#include "frontend/components/map/map3d/map3d_internal.hpp"
#include "frontend/source/theme.hpp"
#include "frontend/components/others/ui_kit.hpp"
#include "frontend/source/mission_alt.hpp"
#include "frontend/source/terrain.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <optional>
#include <vector>

namespace map3d_detail {

// ── EDIT: moving waypoints ────────────────────────────────────────────────────
//
// A waypoint in the air has two things to grab. The marker moves it over the
// ground, holding its height: it follows the pointer across the flat plane it
// sits in, which keeps it under the pointer wherever the camera is. Its vertical
// line changes only its height, sliding along that line. Both write straight into
// the editor's vector, as the flat map's drag does, so the list and the path
// follow as it happens.
//
// Grabbing is judged in the frame's own projection, so what is grabbed is what
// was on screen. A left-drag anywhere else still orbits.

constexpr float  WP_MARKER_REACH = 12.0f;   // marker radius plus a little slack, as in 2D
constexpr float  WP_LINE_REACH   = 6.0f;

WaypointDrag s_wp_drag;

struct WaypointGrab {
    int  index  = -1;
    bool height = false;
};

static float point_segment_dist_sq(ImVec2 p, ImVec2 a, ImVec2 b)
{
    const float dx = b.x - a.x, dy = b.y - a.y;
    const float len = dx * dx + dy * dy;
    const float t = len > 0.0f ? std::clamp(((p.x - a.x) * dx + (p.y - a.y) * dy) / len, 0.0f, 1.0f) : 0.0f;
    const float qx = a.x + t * dx - p.x, qy = a.y + t * dy - p.y;
    return qx * qx + qy * qy;
}

// What is under the pointer. Only what draw_mission draws can be grabbed, and any
// marker wins over any line: lines cross markers in most views, and the marker is
// the smaller target.
static WaypointGrab waypoint_under(const Frame& f, const std::vector<MissionItem>& mission,
                            std::optional<float> home, ImVec2 mouse)
{
    WaypointGrab marker, line;
    float best_marker = WP_MARKER_REACH * WP_MARKER_REACH;
    float best_line   = WP_LINE_REACH * WP_LINE_REACH;

    for (int i = 0; i < (int)mission.size(); ++i) {
        const MissionItem& it = mission[i];
        if ((it.lat == 0.0 && it.lon == 0.0) || !mission_frame_is_global(it.frame)) continue;
        const std::optional<float> ground = terrain_amsl(it.lat, it.lon);
        if (!ground) continue;
        const float amsl = mission_item_amsl(it, home, ground).value_or(*ground);
        const V3    top  = f.world(it.lat, it.lon, amsl);

        ImVec2 sp;
        if (f.screen(top, sp)) {
            const float dx = sp.x - mouse.x, dy = sp.y - mouse.y;
            const float d  = dx * dx + dy * dy;
            if (d <= best_marker) { best_marker = d; marker.index = i; }
        }
        ImVec2 a, b;
        if (f.segment(top, f.world(it.lat, it.lon, *ground), a, b)) {
            const float d = point_segment_dist_sq(mouse, a, b);
            if (d <= best_line) { best_line = d; line = { i, true }; }
        }
    }
    return marker.index >= 0 ? marker : line;
}

// Where the pointer's ray crosses the flat plane at height z, within what the view
// can see. Nothing once the ray runs level or points away, where a marker would
// otherwise fly off towards the horizon.
static std::optional<V3> ray_plane(V3 o, V3 d, double z, double max_t)
{
    if (std::fabs(d.z) < 1e-4) return std::nullopt;
    const double t = (z - o.z) / d.z;
    if (t <= 0.0 || t > max_t) return std::nullopt;
    return o + d * t;
}

void edit_waypoints(ImDrawList* dl, const Frame& f, V3 eye, V3 target, double range, bool hovered,
                    std::vector<MissionItem>& mission, const MapVehicle* active, Map3DCenter center)
{
    const ImVec2 mouse = ImGui::GetIO().MousePos;
    const V3     ray   = pointer_ray(f, eye, target, mouse);
    const std::optional<float> plan_home = plan_home_amsl(active, &mission);

    if (s_wp_drag.index >= (int)mission.size()) s_wp_drag = {};
    if (s_wp_drag.index >= 0 && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) s_wp_drag = {};

    WaypointGrab hover;
    if (s_wp_drag.index < 0 && hovered) hover = waypoint_under(f, mission, plan_home, mouse);

    if (hover.index >= 0 && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        const MissionItem& it = mission[hover.index];
        const float ground = terrain_amsl(it.lat, it.lon).value_or(0.0f);
        const float amsl   = mission_item_amsl(it, plan_home, ground).value_or(ground);
        const V3    top    = f.world(it.lat, it.lon, amsl);

        WaypointDrag d;
        d.index   = hover.index;
        d.height  = hover.height;
        d.plane_z = top.z;
        // With no vehicle, home is the ground under the first item, and moving that
        // item would shift the reference while it is being measured against.
        d.home = plan_home;
        d.low  = std::min(ground - POINT_MAX_REACH, (double)amsl);
        if (auto hit = ray_plane(eye, ray, top.z, range * 4.0)) {
            d.off_x = top.x - hit->x;
            d.off_y = top.y - hit->y;
        }
        if (auto z = vertical_z_nearest(eye, ray, top.x, top.y)) d.off_z = top.z - *z;
        s_wp_drag = d;

        s_drag        = -1;      // this press is the waypoint's, not an orbit
        center.follow = false;   // and the view must hold still under it
    }

    bool   steep = false;
    double into_terrain = 0.0;   // how far the one held is inside it, metres
    if (s_wp_drag.index >= 0) {
        MissionItem& it = mission[s_wp_drag.index];
        if (!s_wp_drag.height) {
            if (auto hit = ray_plane(eye, ray, s_wp_drag.plane_z, range * 4.0))
                geo_unmercator(f.u0 + (hit->x + s_wp_drag.off_x) / f.m,
                       f.v0 - (hit->y + s_wp_drag.off_y) / f.m, it.lat, it.lon);
        } else if (const std::optional<float> ground = terrain_amsl(it.lat, it.lon)) {
            const V3 foot = f.world(it.lat, it.lon, *ground);
            if (auto z = vertical_z_nearest(eye, ray, foot.x, foot.y)) {
                const double amsl = std::clamp(f.amsl_at({ foot.x, foot.y, *z + s_wp_drag.off_z }), s_wp_drag.low,
                                               (double)*ground + POINT_MAX_REACH);
                if (auto alt = mission_alt_for_amsl(it.frame, (float)amsl, s_wp_drag.home, ground))
                    it.alt = std::round(*alt);
            } else {
                steep = true;
            }
        }
    }

    const WaypointGrab shown = s_wp_drag.index >= 0 ? WaypointGrab{ s_wp_drag.index, s_wp_drag.height } : hover;
    if (shown.index >= 0) {
        ImGui::SetMouseCursor(shown.height ? ImGuiMouseCursor_ResizeNS : ImGuiMouseCursor_ResizeAll);

        // What it is about to do: the line lit for a height, the marker ringed for
        // a move, and while held, where it stands.
        const MissionItem& it = mission[shown.index];
        const std::optional<float> ground = terrain_amsl(it.lat, it.lon);
        if (ground) {
            const std::optional<float> home = s_wp_drag.index >= 0 ? s_wp_drag.home : plan_home;
            const float amsl = mission_item_amsl(it, home, ground).value_or(*ground);
            const V3    top  = f.world(it.lat, it.lon, amsl);
            ImVec2 a, b, sp;
            if (shown.height && f.segment(top, f.world(it.lat, it.lon, *ground), a, b)) {
                if (amsl < *ground) map_dotted_line(dl, a, b, under_col(), 3.5f);
                else                dl->AddLine(a, b, drop_col(1.0f), 3.5f);
            }
            if (f.screen(top, sp)) {
                dl->AddCircle(sp, shown.height ? 10.0f : 12.0f, ui_col(g_theme.accent), 0, 2.0f);
                if (s_wp_drag.index >= 0) {
                    char ro[96];
                    const double base = home ? *home : *ground;
                    height_readout(ro, sizeof(ro), amsl - base, amsl - *ground,
                                   map_home_distance(active, it.lat, it.lon));
                    map_draw_tag(dl, { sp.x + 16.0f, sp.y }, ro, ok_or_error(amsl < *ground));
                    into_terrain = (double)*ground - amsl;
                }
            }
        }
    }

    char hint[96];
    if (into_terrain > 0.0)
        snprintf(hint, sizeof(hint), "  ~%.0f m inside the terrain data here  \xe2\x80\x94  release to set anyway  ", into_terrain);
    draw_banner(f, into_terrain > 0.0 ? hint
                 : steep ? "  Tilt the view to set the height  "
                         : "  Drag a waypoint to move it  \xe2\x80\x94  drag its line to set its height  ");
}

} // namespace map3d_detail
