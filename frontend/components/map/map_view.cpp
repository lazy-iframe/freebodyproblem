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

// frontend/components/map/map_view.cpp
//
// The map window: the flat map and everything drawn and clicked on it — the
// fleet, trails, home, geofences, the mission, GO HERE and CLEAR, and the
// overlay chrome — or the 3D view in its place. draw_map_view() is the frame
// in outline, a step per call: the input steps are in map2d/input.cpp, the
// layers in map2d/layers.cpp and map2d/fences.cpp, the chrome in
// map2d/chrome.cpp and the menus in map2d/menus.cpp. The imagery, trails,
// symbols and labels they draw come from tiles.cpp, tracks.cpp, symbols.cpp
// and overlay.cpp, which the 3D view shares.

#include "frontend/components/map/map_view.hpp"
#include "frontend/components/map/map_view_3d.hpp"
#include "frontend/components/map/map2d/map2d_internal.hpp"
#include "frontend/source/terrain.hpp"
#include <algorithm>
#include <string>
#include <vector>

using namespace map_detail;

namespace map2d_detail {

// ── Map state ─────────────────────────────────────────────────────────────────

MapState g_map;
bool     s_3d = false;

const MapVehicle* active_vehicle(const std::vector<MapVehicle>* fleet)
{
    if (fleet)
        for (const MapVehicle& mv : *fleet)
            if (mv.active) return &mv;
    return nullptr;
}

void fleet_or_solo(MapFrame& f)
{
    f.draw_list = f.fleet;
    if (!f.draw_list || f.draw_list->empty()) {
        MapVehicle one;
        one.id      = ui_bound_vehicle();   // so it finds its own trail
        one.lat     = f.lat;
        one.lon     = f.lon;
        one.has_pos = f.has_pos;
        one.heading = f.heading;
        one.has_hdg = f.has_hdg;
        one.active  = true;
        f.solo.push_back(one);
        f.draw_list = &f.solo;
    }
}

// ── Window ────────────────────────────────────────────────────────────────────

// The map's window, filling win_x..win_x+win_w etc. False when it is not to be
// drawn, its Begin already undone.
static bool begin_map_window(float win_x, float win_y, float win_w, float win_h)
{
    ImGui::SetNextWindowPos ({win_x, win_y}, ImGuiCond_Always);
    ImGui::SetNextWindowSize({win_w, win_h}, ImGuiCond_Always);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, bg_map());
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,  {0.0f, 0.0f});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

    constexpr ImGuiWindowFlags FLAGS =
        ImGuiWindowFlags_NoTitleBar       | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove           | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        // Panning the map must not raise it above the plugin rail floating over
        // its right edge — see the same flag on the video window.
        ImGuiWindowFlags_NoBringToFrontOnFocus;

    if (!ImGui::Begin("##map", nullptr, FLAGS)) {
        ImGui::End();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();
        return false;
    }
    return true;
}

static void end_map_window()
{
    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
}

// ── Keep centered on vehicle until user pans ──────────────────────────────────
static void follow_vehicle(const MapFrame& f)
{
    if (f.has_pos && (!g_map.initialized || g_map.follow)) {
        g_map.center_lat = f.lat;
        g_map.center_lon = f.lon;
        if (!g_map.initialized) {
            g_map.zoom        = 15;
            g_map.initialized = true;
        }
    }
}

// ── 3D ────────────────────────────────────────────────────────────────────────
// The same window, inputs and overlay chrome, with the 3D view where the
// tiles would be. False if it could not draw, for the flat map to take over.
static bool draw_3d(MapFrame& f)
{
    ImDrawList* dl = f.dl;
    const ImVec2 wp = f.wp;
    const MissionPickState* pick = f.pick;
    GotoTargetState* go = f.go;

    fleet_or_solo(f);
    Map3DScene scene;
    scene.fleet   = f.draw_list;
    scene.mission = f.shown_mission();
    scene.pick    = f.pick;
    scene.go      = go;
    FencePlan downloaded;
    scene.fence = (pick && pick->edit_fence) ? pick->edit_fence : nullptr;
    if (!scene.fence && f.fence && !f.fence->empty()) {
        downloaded  = fence_from_items(*f.fence);
        scene.fence = &downloaded;
    }
    if (!map3d_draw(dl, wp, { f.win_w, f.win_h }, f.map_hovered,
                    { g_map.center_lat, g_map.center_lon, g_map.follow }, scene))
        return false;

    draw_overlay_buttons(wp, f.has_pos, f.lat, f.lon);
    draw_ac_block(dl, wp, f.has_pos, f.lat, f.lon, f.heading, f.has_hdg, f.alt_rel, f.gs,
                  active_terrain_agl(f.fleet),
                  f.has_pos ? map_home_distance(active_vehicle(f.fleet), f.lat, f.lon) : std::string());
    draw_north_pointer(dl, wp, f.win_w, f.win_h, -map3d_yaw_deg(),
                       ImGui::GetTextLineHeight() + 6.0f);
    draw_attribution(dl, wp, f.win_w, f.win_h, terrain_attribution().c_str());

    // GO HERE, picked in 3D place and height: the same menu as a 2D
    // click, at that point, its altitude filled in. A right-click
    // outside the pick: the same CLEAR menu.
    double glat, glon;
    float  galt;
    if (go && map3d_take_goto(glat, glon, galt)) {
        s_ctx_lat         = glat;
        s_ctx_lon         = glon;
        s_goto_alt_m      = std::clamp(galt, GOTO_ALT_MIN_M, GOTO_ALT_MAX_M);
        s_goto_alt_seeded = true;
        ImGui::OpenPopup("##map_go");
    }
    if (go && map3d_take_clear()) ImGui::OpenPopup("##map_clear");
    if (go) {
        draw_goto_menu(*go, active_vehicle(f.draw_list), scene.mission);
        draw_clear_menu(*go);
    }
    return true;
}

// Where the tile grid sits in the window this frame.
static void place_grid(MapFrame& f)
{
    double cx, cy;
    lat_lon_to_tile_frac(g_map.center_lat, g_map.center_lon, g_map.zoom, cx, cy);

    f.tl_px = cx * TILE_PX - f.win_w * 0.5;
    f.tl_py = cy * TILE_PX - f.win_h * 0.5;
}

static void waiting_for_gps(const MapFrame& f)
{
    const char* msg = "Waiting for GPS\xe2\x80\xa6";
    ImVec2 tsz = ImGui::CalcTextSize(msg);
    ImGui::SetCursorPos({(f.win_w - tsz.x) * 0.5f, (f.win_h - tsz.y) * 0.5f});
    ImGui::TextDisabled("%s", msg);
}

} // namespace map2d_detail

using namespace map2d_detail;

void draw_map_view(double lat, double lon, bool has_pos,
                   float heading, bool has_hdg,
                   float win_x, float win_y, float win_w, float win_h,
                   const std::vector<MissionItem>* mission,
                   MissionPickState* pick,
                   float alt_rel, float gs,
                   GotoTargetState* go,
                   const std::vector<MapVehicle>* fleet,
                   const std::vector<MissionItem>* fence)
{
    ensure_started();
    drain_upload_queue();

    if (!begin_map_window(win_x, win_y, win_w, win_h)) return;

    MapFrame f;
    f.lat = lat;  f.lon = lon;  f.has_pos = has_pos;
    f.heading = heading;  f.has_hdg = has_hdg;
    f.win_w = win_w;  f.win_h = win_h;
    f.mission = mission;  f.pick = pick;
    f.alt_rel = alt_rel;  f.gs = gs;
    f.go = go;  f.fleet = fleet;  f.fence = fence;
    f.dl = ImGui::GetWindowDrawList();
    f.wp = ImGui::GetWindowPos();

    // The control column, top left — CENTER puts the map back on the active
    // vehicle and keeps it there, which a drag gives up. Its rects are fixed
    // so the input below can leave them alone: a press on a button must not
    // also pan, pick or open GO HERE.
    f.map_hovered = ImGui::IsWindowHovered() && !over_overlay_buttons(f.wp);

    // A mission item can be placed and moved in 3D, ground then height. Fences are
    // flat-map work; drawing or editing one leaves 3D.
    if (pick && (pick->fence_drawing() || pick->fence_edit))
        s_3d = false;

    follow_vehicle(f);

    // Should the 3D view fail to draw, the flat map takes over.
    if (s_3d && g_map.initialized) {
        if (draw_3d(f)) { end_map_window(); return; }
        s_3d = false;
        f.solo.clear();
    }

    // ── Input ─────────────────────────────────────────────────────────────────
    settle_edit_state(f);
    pan_and_zoom(f);

    if (!g_map.initialized) {
        waiting_for_gps(f);
        end_map_window();
        return;
    }
    place_grid(f);

    // In this order: each stands aside on what the ones before it took.
    pick_click(f);
    drag_waypoint(f);
    drag_fence_point(f);
    goto_and_clear_clicks(f);

    // ── Layers, bottom to top ─────────────────────────────────────────────────
    draw_tiles(f);
    fleet_or_solo(f);
    draw_fences(f);
    draw_trails(f);
    draw_homes(f);
    draw_line_of_sight(f);
    draw_fleet(f);
    draw_mission_overlay(f);
    draw_fence_in_progress(f);
    draw_goto_marker(f);
    draw_hint_banner(f);
    draw_tactical_overlay(f);
    draw_attribution(f.dl, f.wp, win_w, win_h, nullptr);

    end_map_window();
}
