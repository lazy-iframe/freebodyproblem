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

// frontend/components/map/map2d/input.cpp
//
// The pointer on the flat map: pan and zoom, a pick's click, dragging a
// waypoint or a fence point in EDIT, and the clicks that open GO HERE and
// CLEAR. No step hands the press to another; each stands aside on the state
// the ones before it left (a pick under way, a point picked up to drag), so
// draw_map_view() calls them in a fixed order, and that order is what
// decides which gesture a press belongs to.

#include "frontend/components/map/map2d/map2d_internal.hpp"
#include <algorithm>
#include <cmath>

using namespace map_detail;

namespace map2d_detail {

int       s_drag_wp = -1;
FenceDrag s_drag_fence;

static bool s_go_press = false;     // the left press may be a GO HERE click

static void pan_by_pixels(float dx, float dy)
{
    constexpr double TILE_PX = 256.0;
    double tx, ty;
    geo_to_tile(g_map.center_lat, g_map.center_lon, g_map.zoom, tx, ty);
    tx -= (double)dx / TILE_PX;
    ty -= (double)dy / TILE_PX;
    geo_from_tile(tx, ty, g_map.zoom, g_map.center_lat, g_map.center_lon);
    g_map.center_lat = std::max(-85.0, std::min(85.0, g_map.center_lat));
    while (g_map.center_lon >  180.0) g_map.center_lon -= 360.0;
    while (g_map.center_lon < -180.0) g_map.center_lon += 360.0;
}

// Whether a pick is under way, and which EDIT, if any, owns a press on a
// point.
void settle_edit_state(MapFrame& f)
{
    MissionPickState* pick = f.pick;

    f.pick_active = pick && pick->picking();

    // EDIT ends with the mode, or with the mission it was editing.
    f.drag_mission =
        (pick && pick->drag_edit && !f.pick_active) ? pick->edit_mission : nullptr;
    if (!f.drag_mission || s_drag_wp >= (int)f.drag_mission->size())
        s_drag_wp = -1;

    // The FENCE view's EDIT, likewise; never while a shape is being drawn.
    f.drag_fence =
        (pick && pick->fence_edit && !f.pick_active) ? pick->edit_fence : nullptr;
    if (!f.drag_fence || s_drag_fence.shape >= (int)f.drag_fence->shapes.size())
        s_drag_fence = {};
}

void pan_and_zoom(const MapFrame& f)
{
    if (f.map_hovered) {
        if (f.pick_active) {
            // Show a hand cursor so the user knows the map is clickable
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        } else if (s_drag_wp < 0 && s_drag_fence.shape < 0) {
            // Pan: left-drag (suppressed during pick mode, and while a
            // waypoint is being dragged — the gesture is the waypoint's)
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                ImVec2 d = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left, 0.0f);
                ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
                if (d.x != 0.0f || d.y != 0.0f) {
                    pan_by_pixels(d.x, d.y);
                    g_map.follow = false;
                }
            }
        }
        // Zoom: scroll wheel (always available)
        float wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0.0f) {
            int new_zoom = std::max(1, std::min(19, g_map.zoom + (int)wheel));
            if (new_zoom != g_map.zoom) {
                g_map.zoom = new_zoom;
                flush_stale_requests(new_zoom);
            }
        }
    }
}

// ── Pick-mode click → lat/lon ─────────────────────────────────────────────────
void pick_click(const MapFrame& f)
{
    MissionPickState* pick = f.pick;

    if (f.pick_active && f.map_hovered &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        const ImVec2 mouse = ImGui::GetMousePos();
        f.to_lat_lon(mouse, pick->picked_lat, pick->picked_lon);
        pick->pick_done = true;

        // A polygon closes on a click at its first vertex.
        pick->fence_close = false;
        if (pick->fence_draw == MissionPickState::FenceDraw::Polygon &&
            pick->fence_pts.size() >= 3) {
            const ImVec2 first = f.to_screen(pick->fence_pts[0].lat, pick->fence_pts[0].lon);
            const float dx = first.x - mouse.x;
            const float dy = first.y - mouse.y;
            pick->fence_close = dx * dx + dy * dy <= 12.0f * 12.0f;
        }
        // add_command intentionally left set; tab_mission clears it
        // after consuming
    }
}

// ── EDIT: drag a waypoint ─────────────────────────────────────────────────────
// Picked up by a press within a marker's reach, the nearest if several,
// and moved with the pointer until the button comes up. Written into
// the editor's own vector, so the list and the line follow the drag.
void drag_waypoint(const MapFrame& f)
{
    std::vector<MissionItem>* drag_mission = f.drag_mission;
    if (!drag_mission) return;

    const ImVec2 mouse = ImGui::GetMousePos();

    int   under  = -1;
    float best_d = 12.0f * 12.0f;   // marker radius plus a little slack
    for (int i = 0; i < (int)drag_mission->size(); ++i) {
        const MissionItem& it = (*drag_mission)[i];
        if (it.lat == 0.0 && it.lon == 0.0) continue;
        const ImVec2 sp = f.to_screen(it.lat, it.lon);
        const float  dx = sp.x - mouse.x, dy = sp.y - mouse.y;
        if (dx * dx + dy * dy <= best_d) { best_d = dx * dx + dy * dy; under = i; }
    }

    if (s_drag_wp < 0 && f.map_hovered && under >= 0) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            s_drag_wp = under;
            g_map.follow = false;   // the map must hold still under the drag
        }
    }
    if (s_drag_wp >= 0) {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
            MissionItem& it = (*drag_mission)[s_drag_wp];
            f.to_lat_lon(mouse, it.lat, it.lon);
        } else {
            s_drag_wp = -1;
        }
    }
}

// ── FENCE EDIT: drag a vertex or a circle's centre ────────────────────────────
// As EDIT does for waypoints: the nearest point within reach is picked
// up on the press and follows the pointer until the button comes up.
void drag_fence_point(const MapFrame& f)
{
    FencePlan* drag_fence = f.drag_fence;
    if (!drag_fence) return;

    const ImVec2 mouse = ImGui::GetMousePos();

    FenceDrag under;
    float best_d = 10.0f * 10.0f;
    auto consider = [&](int si, int vi, double flat, double flon) {
        const ImVec2 sp = f.to_screen(flat, flon);
        const float  d  = (sp.x - mouse.x) * (sp.x - mouse.x) +
                          (sp.y - mouse.y) * (sp.y - mouse.y);
        if (d <= best_d) { best_d = d; under = { si, vi }; }
    };
    for (int si = 0; si < (int)drag_fence->shapes.size(); ++si) {
        const FenceShape& fs = drag_fence->shapes[si];
        if (fs.kind == FenceShape::Kind::Polygon)
            for (int vi = 0; vi < (int)fs.pts.size(); ++vi)
                consider(si, vi, fs.pts[vi].lat, fs.pts[vi].lon);
        else
            consider(si, -1, fs.center.lat, fs.center.lon);
    }

    if (s_drag_fence.shape < 0 && f.map_hovered && under.shape >= 0) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            s_drag_fence = under;
            g_map.follow = false;
        }
    }
    if (s_drag_fence.shape >= 0) {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
            FenceShape& fs = drag_fence->shapes[s_drag_fence.shape];
            FenceVertex* v = nullptr;
            if (fs.kind == FenceShape::Kind::Circle)            v = &fs.center;
            else if (s_drag_fence.vertex < (int)fs.pts.size()) v = &fs.pts[s_drag_fence.vertex];
            if (v) f.to_lat_lon(mouse, v->lat, v->lon);
        } else {
            s_drag_fence = {};
        }
    }
}

// ── Click → GO HERE, right-click → CLEAR ──────────────────────────────────────
// A click is a press and release that barely moved, so a drag still
// pans. Not while picking a waypoint or a fence point — the mouse
// belongs to that gesture, and a target offered mid-pick is one the
// operator did not ask for — nor on a press that picked up a waypoint
// or a fence point to drag.
void goto_and_clear_clicks(const MapFrame& f)
{
    GotoTargetState* go = f.go;
    const bool map_hovered = f.map_hovered;
    const bool pick_active = f.pick_active;

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        s_go_press = map_hovered && !pick_active && s_drag_wp < 0 && s_drag_fence.shape < 0;
    if (go && s_go_press && map_hovered && !pick_active &&
        ImGui::IsMouseReleased(ImGuiMouseButton_Left) &&
        ImGui::GetIO().MouseDragMaxDistanceSqr[ImGuiMouseButton_Left] < 16.0f) {
        s_go_press = false;
        f.to_lat_lon(ImGui::GetMousePos(), s_ctx_lat, s_ctx_lon);
        if (!s_goto_alt_seeded) {
            // Not at home's own height, where it may be standing on the ground.
            // Well below home is a real height, and is kept.
            const float here = std::round(go->current_alt_m);
            s_goto_alt_m = std::clamp(std::fabs(here) < 2.0f ? 2.0f : here,
                                      GOTO_ALT_MIN_M, GOTO_ALT_MAX_M);
            s_goto_alt_seeded = true;
        }
        ImGui::OpenPopup("##map_go");
    }
    if (go && !pick_active && map_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        ImGui::OpenPopup("##map_clear");
    if (go) {
        draw_goto_menu(*go, active_vehicle(f.fleet), f.shown_mission());
        draw_clear_menu(*go);
    }
}

} // namespace map2d_detail
