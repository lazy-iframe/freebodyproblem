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

// frontend/components/map/map2d/fences.cpp
//
// Geofences on the flat map: the uploaded zones, each vehicle's parameter
// cylinder, and the shape being drawn.

#include "frontend/components/map/map2d/map2d_internal.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

using namespace map_detail;

namespace map2d_detail {

// ── Geofence drawing ──────────────────────────────────────────────────────────

static ImU32 fence_col(bool inclusion, float alpha)
{
    return ui_col(inclusion ? g_theme.col_ok : g_theme.col_error, alpha);
}

// Metres on the ground to pixels on the map, at a latitude: the screen
// distance of a point that far north. Mercator is conformal, so one direction
// serves for a circle's radius.
template <class Project>
static float metres_to_px(const Project& project, double lat, double lon, float m)
{
    const ImVec2 a = project(lat, lon);
    const ImVec2 b = project(lat + (double)m / GEO_M_PER_DEG, lon);
    return std::fabs(a.y - b.y);
}

// A circle on the screen as a ring of points, close enough that the dots and
// the outline lie on it at any size.
static void screen_ring(ImVec2 c, float r, std::vector<ImVec2>& out)
{
    const int n = std::clamp((int)(r * 0.5f), 24, 256);
    out.clear();
    for (int i = 0; i < n; ++i) {
        const float a = 2.0f * (float)GEO_PI * (float)i / (float)n;
        out.push_back({ c.x + r * std::cos(a), c.y + r * std::sin(a) });
    }
}

// One uploaded fence zone, with its number at the middle. Inclusion zones are
// green, exclusion zones red. While the vehicle has the fence on (`off`
// false): a translucent fill, so overlapping zones still read, and a solid
// outline. Off: the outline dotted and nothing filled — still there to be
// planned against, plainly not enforced.
template <class Project>
static void draw_fence_shape(ImDrawList* dl, const Project& project,
                             const FenceShape& s, int number, bool dragging_this, bool off)
{
    const ImU32 fill  = fence_col(s.inclusion, 0.16f);
    const ImU32 line  = fence_col(s.inclusion, 0.95f);
    const float width = dragging_this ? 3.0f : 2.0f;
    static std::vector<ImVec2> scr;   // UI thread only
    ImVec2 label_at;

    if (s.kind == FenceShape::Kind::Polygon) {
        if (s.pts.size() < 2) return;
        scr.clear();
        float cx = 0.0f, cy = 0.0f;
        for (const FenceVertex& v : s.pts) {
            scr.push_back(project(v.lat, v.lon));
            cx += scr.back().x; cy += scr.back().y;
        }
        if (off) {
            map_dotted_polyline(dl, scr.data(), (int)scr.size(), true, line, width);
        } else {
            if (scr.size() >= 3)
                dl->AddConcavePolyFilled(scr.data(), (int)scr.size(), fill);
            dl->AddPolyline(scr.data(), (int)scr.size(), line, ImDrawFlags_Closed, width);
        }
        for (const ImVec2& p : scr) dl->AddCircleFilled(p, 3.5f, line);
        label_at = { cx / scr.size(), cy / scr.size() };
    } else {
        const ImVec2 c = project(s.center.lat, s.center.lon);
        const float  r = metres_to_px(project, s.center.lat, s.center.lon, s.radius_m);
        if (off) {
            screen_ring(c, r, scr);
            map_dotted_polyline(dl, scr.data(), (int)scr.size(), true, line, width);
        } else {
            dl->AddCircleFilled(c, r, fill, 64);
            dl->AddCircle(c, r, line, 64, width);
        }
        dl->AddCircleFilled(c, 3.5f, line);
        label_at = c;
    }

    char lbl[24];
    snprintf(lbl, sizeof(lbl), "F%d %s", number, s.inclusion ? "IN" : "EX");
    ImFont*      fm = g_font_micro ? g_font_micro : ImGui::GetFont();
    const float  tw = ui_tracked_width(fm, UI_SZ_MICRO, lbl);
    const ImVec2 tp = { label_at.x - tw * 0.5f, label_at.y + 6.0f };
    dl->AddRectFilled({ tp.x - 3.0f, tp.y - 1.0f }, { tp.x + tw + 3.0f, tp.y + UI_SZ_MICRO + 1.0f },
                      IM_COL32(0, 0, 0, 150), 2.0f);
    ui_tracked_text(dl, fm, UI_SZ_MICRO, tp, line, lbl);
}

// The parameter-set fence: a ring around home of FENCE_RADIUS (PX4:
// GF_MAX_HOR_DIST), amber, labelled with its radius and ceiling. As the zones
// are: filled and solid while the fence is on, dotted and unfilled while off.
template <class Project>
static void draw_fence_cylinder(ImDrawList* dl, const Project& project,
                                const MapVehicle& mv)
{
    const FenceParams& f = mv.fence;
    if (!mv.has_home || !f.circle_configured()) return;

    const float  alpha = mv.active ? 1.0f : 0.5f;
    const ImU32  col   = ui_col(g_theme.col_warning, 0.9f * alpha);
    const ImVec2 c     = project(mv.home_lat, mv.home_lon);
    const float  r     = metres_to_px(project, mv.home_lat, mv.home_lon, f.radius_m);

    if (f.disabled()) {
        static std::vector<ImVec2> ring;   // UI thread only
        screen_ring(c, r, ring);
        map_dotted_polyline(dl, ring.data(), (int)ring.size(), true, col, 2.5f);
    } else {
        dl->AddCircleFilled(c, r, ui_col(g_theme.col_warning, 0.08f * alpha), 96);
        dl->AddCircle(c, r, col, 96, 2.5f);
    }

    char lbl[64];
    if (f.ceiling_configured())
        snprintf(lbl, sizeof(lbl), "FENCE %.0f M  \xe2\x89\xa4 %.0f M",
                 (double)f.radius_m, (double)f.alt_max_m);
    else
        snprintf(lbl, sizeof(lbl), "FENCE %.0f M", (double)f.radius_m);
    ImFont*      fm = g_font_micro ? g_font_micro : ImGui::GetFont();
    const float  tw = ui_tracked_width(fm, UI_SZ_MICRO, lbl);
    const ImVec2 tp = { c.x - tw * 0.5f, c.y - r - UI_SZ_MICRO - 6.0f };
    dl->AddRectFilled({ tp.x - 3.0f, tp.y - 1.0f }, { tp.x + tw + 3.0f, tp.y + UI_SZ_MICRO + 1.0f },
                      IM_COL32(0, 0, 0, 150), 2.0f);
    ui_tracked_text(dl, fm, UI_SZ_MICRO, tp, col, lbl);
}

// ── Geofences ─────────────────────────────────────────────────────────────────
// Lowest of everything drawn over the tiles: they are areas, and the
// trails, the aircraft and the mission are all read against them.
void draw_fences(const MapFrame& f)
{
    ImDrawList* dl = f.dl;
    const auto project = [&f](double plat, double plon) { return f.to_screen(plat, plon); };

    // Uploaded zones (MapFrame::shown_fence()), as the active vehicle has its
    // fence: on, or known to be off. Unknown — no vehicle, or its parameters not
    // read yet — is drawn as on.
    {
        FencePlan downloaded;
        const FencePlan*  zones  = f.shown_fence(downloaded);
        const MapVehicle* active = active_vehicle(f.fleet);
        const bool        off    = active && active->fence.disabled();
        if (zones)
            for (int si = 0; si < (int)zones->shapes.size(); ++si)
                draw_fence_shape(dl, project, zones->shapes[si], si + 1,
                                 f.drag_fence && si == s_drag_fence.shape, off);
    }
    // The parameter-set cylinder, every vehicle's around its own home.
    in_draw_order(f, [&](const MapVehicle& mv) { draw_fence_cylinder(dl, project, mv); });
}

// ── Fence shape being drawn ───────────────────────────────────────────────────
// The vertices so far, joined, with a rubber band to the pointer; a
// circle's centre with the ring it would have if the next click set
// its radius. The first vertex rings once a polygon can be closed on it.
void draw_fence_in_progress(const MapFrame& f)
{
    ImDrawList* dl = f.dl;
    const MissionPickState* pick = f.pick;
    const bool map_hovered = f.map_hovered;

    if (pick && pick->fence_drawing()) {
        const ImU32  col   = fence_col(pick->fence_inclusion, 0.95f);
        const ImVec2 mouse = ImGui::GetMousePos();
        const auto&  pts   = pick->fence_pts;
        if (pick->fence_draw == MissionPickState::FenceDraw::Polygon) {
            static std::vector<ImVec2> scr;
            scr.clear();
            for (const FenceVertex& v : pts) scr.push_back(f.to_screen(v.lat, v.lon));
            if (scr.size() >= 2)
                dl->AddPolyline(scr.data(), (int)scr.size(), col, ImDrawFlags_None, 2.0f);
            if (!scr.empty() && map_hovered)
                dl->AddLine(scr.back(), mouse, fence_col(pick->fence_inclusion, 0.6f), 1.5f);
            for (const ImVec2& p : scr) dl->AddCircleFilled(p, 4.0f, col);
            if (scr.size() >= 3) {
                const float dx = scr[0].x - mouse.x, dy = scr[0].y - mouse.y;
                const bool  on_first = dx * dx + dy * dy <= 12.0f * 12.0f;
                dl->AddCircle(scr[0], on_first ? 10.0f : 7.0f, col, 0, 2.0f);
            }
        } else if (!pts.empty()) {
            const ImVec2 c = f.to_screen(pts[0].lat, pts[0].lon);
            dl->AddCircleFilled(c, 4.0f, col);
            if (map_hovered) {
                const float r = std::sqrt((mouse.x - c.x) * (mouse.x - c.x) +
                                          (mouse.y - c.y) * (mouse.y - c.y));
                dl->AddCircleFilled(c, r, fence_col(pick->fence_inclusion, 0.12f), 64);
                dl->AddCircle(c, r, col, 64, 2.0f);
                dl->AddLine(c, mouse, fence_col(pick->fence_inclusion, 0.6f), 1.5f);
            }
        }
    }
}

} // namespace map2d_detail
