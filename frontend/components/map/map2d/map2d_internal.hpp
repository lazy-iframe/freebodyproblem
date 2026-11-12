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

// Shared by the flat map's files (map_view.cpp and map2d/). Not part of the
// public API, which is map_view.hpp.

#include "frontend/components/map/map_view.hpp"
#include "frontend/components/map/map_internal.hpp"
#include "imgui.h"

#include <optional>
#include <string>
#include <vector>

namespace map2d_detail {

// ── map_view.cpp ──────────────────────────────────────────────────────────────

struct MapState {
    double center_lat  = 0.0;
    double center_lon  = 0.0;
    int    zoom        = 2;
    bool   initialized = false;
    bool   follow      = true;  // keep re-centering on vehicle until user pans
};

extern MapState g_map;

// The 3D view in place of the flat map. Off while placing or dragging
// anything, which only the flat map does.
extern bool s_3d;

inline constexpr int TILE_PX = 256;

// One frame of the map window: what draw_map_view() was handed, and what it
// worked out from that, for each of its steps to read. Built once a frame by
// draw_map_view(); not copied, since draw_list may point at its own `solo`.
struct MapFrame {
    // The caller's arguments.
    double lat = 0.0, lon = 0.0;
    bool   has_pos = false;
    float  heading = 0.0f;
    bool   has_hdg = false;
    float  win_w = 0.0f, win_h = 0.0f;
    const std::vector<MissionItem>* mission = nullptr;
    MissionPickState*               pick    = nullptr;
    float  alt_rel = 0.0f, gs = 0.0f;
    GotoTargetState*                go      = nullptr;
    const std::vector<MapVehicle>*  fleet   = nullptr;
    const std::vector<MissionItem>* fence   = nullptr;

    // The window.
    ImDrawList* dl = nullptr;
    ImVec2      wp;
    bool        map_hovered = false;   // over the map and not its control column

    // What the pointer is doing to the plan: settle_edit_state().
    bool                      pick_active  = false;
    std::vector<MissionItem>* drag_mission = nullptr;   // EDIT's mission, or null
    FencePlan*                drag_fence   = nullptr;   // FENCE EDIT's plan, or null

    // The tile grid's top-left corner, in pixels at g_map.zoom: place_grid().
    double tl_px = 0.0, tl_py = 0.0;

    // The vehicles to draw: the caller's fleet, or `solo`: fleet_or_solo().
    const std::vector<MapVehicle>* draw_list = nullptr;
    std::vector<MapVehicle>        solo;

    MapFrame() = default;
    MapFrame(const MapFrame&) = delete;
    MapFrame& operator=(const MapFrame&) = delete;

    // A point on the window, from a position.
    ImVec2 to_screen(double plat, double plon) const
    {
        double px, py;
        geo_to_tile(plat, plon, g_map.zoom, px, py);
        return { wp.x + (float)(px * TILE_PX - tl_px),
                 wp.y + (float)(py * TILE_PX - tl_py) };
    }

    // The position under a point on the window.
    void to_lat_lon(ImVec2 p, double& plat, double& plon) const
    {
        const double tx = ((double)(p.x - wp.x) + tl_px) / TILE_PX;
        const double ty = ((double)(p.y - wp.y) + tl_py) / TILE_PX;
        geo_from_tile(tx, ty, g_map.zoom, plat, plon);
    }

    // The mission to draw: the live edit vector while there is one, so the map
    // reflects unsaved edits, else the caller's.
    const std::vector<MissionItem>* shown_mission() const
    {
        return (pick && pick->edit_mission) ? pick->edit_mission : mission;
    }

    // The uploaded fence zones to draw, by both views: the fence editor's plan
    // while it has one, else the active vehicle's fence as last downloaded,
    // converted into `scratch`. nullptr for neither.
    const FencePlan* shown_fence(FencePlan& scratch) const
    {
        if (pick && pick->edit_fence) return pick->edit_fence;
        if (!fence || fence->empty()) return nullptr;
        scratch = fence_from_items(*fence);
        return &scratch;
    }
};

// Every vehicle the caller handed over. A caller with no fleet becomes a
// fleet of one built from the scalar arguments, so there is one drawing
// path rather than two that have to be kept in agreement. Sets f.draw_list.
void fleet_or_solo(MapFrame& f);

// Inactive vehicles first, the active one last, so the aircraft every
// other panel is showing is never buried under another's trail or
// symbol. Every pass runs over the same list, so a vehicle's trail and
// its symbol keep the same relative order.
template <class Fn>
void in_draw_order(const MapFrame& f, Fn&& fn)
{
    for (const MapVehicle& mv : *f.draw_list) if (!mv.active) fn(mv);
    for (const MapVehicle& mv : *f.draw_list) if ( mv.active) fn(mv);
}

// ── input.cpp ─────────────────────────────────────────────────────────────────

// The waypoint being dragged in the mission tab's EDIT, as an index into
// MissionPickState::edit_mission; -1 when none. Set on the press, followed
// while the button is held, dropped on release.
extern int s_drag_wp;

// The fence point being dragged in the FENCE view's EDIT: a shape in
// MissionPickState::edit_fence and a vertex of it, -1 for a circle's centre.
// shape < 0 when none.
struct FenceDrag { int shape = -1; int vertex = -1; };
extern FenceDrag s_drag_fence;

void settle_edit_state(MapFrame& f);
void pan_and_zoom(const MapFrame& f);
void pick_click(const MapFrame& f);
void drag_waypoint(const MapFrame& f);
void drag_fence_point(const MapFrame& f);
void goto_and_clear_clicks(const MapFrame& f);

// ── menus.cpp ─────────────────────────────────────────────────────────────────

extern double s_ctx_lat;            // the point the GO HERE menu is for
extern double s_ctx_lon;
extern float  s_goto_alt_m;         // its altitude, metres above home
extern bool   s_goto_alt_seeded;

inline constexpr float GOTO_ALT_MIN_M = -10000.0f;
inline constexpr float GOTO_ALT_MAX_M = 10000.0f;

void draw_goto_menu(GotoTargetState& go, const MapVehicle* active,
                    const std::vector<MissionItem>* mission, const MapFrame* flat = nullptr);
void draw_clear_menu(GotoTargetState& go);

// ── chrome.cpp ────────────────────────────────────────────────────────────────

bool over_overlay_buttons(ImVec2 wp);
void draw_overlay_buttons(ImVec2 wp, bool has_pos, double lat, double lon);
std::optional<float> active_terrain_agl(const std::vector<MapVehicle>* fleet);
void draw_ac_block(ImDrawList* dl, ImVec2 wp, bool has_pos,
                   double lat, double lon, float heading, bool has_hdg,
                   float alt_rel, float gs, std::optional<float> ter_agl,
                   const std::string& home_dist);
void draw_north_pointer(ImDrawList* dl, ImVec2 wp, float win_w, float win_h,
                        float rot_deg, float lift = 0.0f);
void draw_attribution(ImDrawList* dl, ImVec2 wp, float win_w, float win_h, const char* extra);
void draw_tactical_overlay(const MapFrame& f);

// ── fences.cpp ────────────────────────────────────────────────────────────────

void draw_fences(const MapFrame& f);
void draw_fence_in_progress(const MapFrame& f);

// ── layers.cpp ────────────────────────────────────────────────────────────────

void draw_tiles(const MapFrame& f);
void draw_trails(const MapFrame& f);
void draw_homes(const MapFrame& f);
void draw_line_of_sight(const MapFrame& f);
void draw_fleet(const MapFrame& f);
void draw_mission_overlay(const MapFrame& f);
void draw_goto_marker(const MapFrame& f);
void draw_hint_banner(const MapFrame& f);

} // namespace map2d_detail
