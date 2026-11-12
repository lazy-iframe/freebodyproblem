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

// frontend/components/map/map2d/layers.cpp
//
// What the flat map draws over itself, each a layer of its own, called bottom
// to top by draw_map_view(): the tile grid, the vehicles' trails, homes, line
// of sight and symbols, the mission, the GO HERE target and the hint banner.
// The geofences are in fences.cpp.

#include "frontend/components/map/map2d/map2d_internal.hpp"
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <vector>

using namespace map_detail;

namespace map2d_detail {

// ── Tile grid ─────────────────────────────────────────────────────────────────
void draw_tiles(const MapFrame& f)
{
    ImDrawList* dl = f.dl;
    const ImVec2 wp = f.wp;
    const double tl_px = f.tl_px, tl_py = f.tl_py;

    int tx0 = (int)std::floor(tl_px / TILE_PX);
    int ty0 = (int)std::floor(tl_py / TILE_PX);
    int tx1 = (int)std::ceil ((tl_px + f.win_w) / TILE_PX);
    int ty1 = (int)std::ceil ((tl_py + f.win_h) / TILE_PX);
    int max_tile = (1 << g_map.zoom) - 1;

    for (int ty = ty0; ty <= ty1; ++ty) {
        for (int tx = tx0; tx <= tx1; ++tx) {
            if (tx < 0 || ty < 0 || tx > max_tile || ty > max_tile) continue;

            float sx = wp.x + (float)(tx * TILE_PX - tl_px);
            float sy = wp.y + (float)(ty * TILE_PX - tl_py);

            TileKey key{tx, ty, g_map.zoom};
            auto it = g_textures.find(key);
            if (it != g_textures.end()) {
                dl->AddImage((ImTextureID)(intptr_t)it->second,
                             {sx,           sy          },
                             {sx + TILE_PX, sy + TILE_PX});
            } else {
                // Show a zoomed-out ancestor while the real tile loads
                if (!draw_fallback_tile(dl, tx, ty, g_map.zoom, sx, sy)) {
                    dl->AddRectFilled({sx, sy},
                                      {sx + TILE_PX, sy + TILE_PX},
                                      map_tile_placeholder());
                }
                queue_tile(key);
            }
        }
    }
}

// ── Position trails ───────────────────────────────────────────────────────────
// Under the aircraft and under the mission, so neither is obscured by
// where something has already been.
void draw_trails(const MapFrame& f)
{
    ImDrawList* dl = f.dl;
    in_draw_order(f, [&](const MapVehicle& mv) {
        const auto it = g_tracks.by_vehicle.find(mv.id);
        if (it == g_tracks.by_vehicle.end()) return;
        const std::deque<TrackPoint>& pts = it->second.pts;
        if (pts.size() < 2) return;

        // Projected and thinned again, this time in pixels: at low zoom a
        // kilometre of track collapses into a few pixels, and handing ImGui
        // six thousand points to draw inside one of them costs the whole
        // frame for nothing. Points outside the panel are kept — dropping
        // them would break the segments that cross it.
        static std::vector<ImVec2> scr;   // reused; UI thread only
        scr.clear();
        scr.reserve(pts.size());
        for (const TrackPoint& tp : pts) {
            const ImVec2 p = f.to_screen(tp.lat, tp.lon);
            if (!scr.empty()) {
                const ImVec2& q = scr.back();
                if (std::fabs(p.x - q.x) < 2.0f &&
                    std::fabs(p.y - q.y) < 2.0f) continue;
            }
            scr.push_back(p);
        }
        // The aircraft's own position closes its trail, so the line reaches
        // the symbol instead of stopping a sample short of it.
        if (mv.has_pos) scr.push_back(f.to_screen(mv.lat, mv.lon));
        if (scr.size() < 2) return;

        dl->AddPolyline(scr.data(), (int)scr.size(),
                        ui_col(g_theme.map_track, mv.active ? 1.0f : 0.40f),
                        ImDrawFlags_None, mv.active ? 3.0f : 2.0f);
    });
}

// ── Home ──────────────────────────────────────────────────────────────────────
// Under the aircraft: a vehicle sitting on its own home must not be
// hidden by the pad it is standing on.
void draw_homes(const MapFrame& f)
{
    in_draw_order(f, [&](const MapVehicle& mv) {
        if (!mv.has_home) return;
        draw_home(f.dl, f.to_screen(mv.home_lat, mv.home_lon), mv.active);
    });
}

// ── Line of sight ─────────────────────────────────────────────────────────────
// For the vehicle every other panel is showing; one per aircraft would
// lay a web over the imagery.
void draw_line_of_sight(const MapFrame& f)
{
    in_draw_order(f, [&](const MapVehicle& mv) {
        if (mv.active && mv.has_pos && mv.has_home)
            map_draw_los(f.dl, f.to_screen(mv.home_lat, mv.home_lon), f.to_screen(mv.lat, mv.lon), mv);
    });
}

// ── Aircraft ──────────────────────────────────────────────────────────────────
void draw_fleet(const MapFrame& f)
{
    in_draw_order(f, [&](const MapVehicle& mv) {
        if (!mv.has_pos) return;
        draw_aircraft(f.dl, f.to_screen(mv.lat, mv.lon), mv,
                      f.wp, f.win_w, f.win_h);
    });
}

// ── Mission waypoint overlay ──────────────────────────────────────────────────
void draw_mission_overlay(const MapFrame& f)
{
    ImDrawList* dl = f.dl;
    const std::vector<MissionItem>* display_mission = f.shown_mission();

    if (display_mission && !display_mission->empty()) {
        // Connecting polyline
        for (size_t i = 1; i < display_mission->size(); ++i) {
            const auto& a = (*display_mission)[i - 1];
            const auto& b = (*display_mission)[i];
            if (a.lat == 0.0 && a.lon == 0.0) continue;
            if (b.lat == 0.0 && b.lon == 0.0) continue;
            dl->AddLine(f.to_screen(a.lat, a.lon),
                        f.to_screen(b.lat, b.lon),
                        IM_COL32(255, 200, 50, 180), 1.5f);
        }

        // Waypoint circles with sequence number labels
        constexpr float R = 8.0f;
        for (size_t wi = 0; wi < display_mission->size(); ++wi) {
            const MissionItem& wpi = (*display_mission)[wi];
            if (wpi.lat == 0.0 && wpi.lon == 0.0) continue;
            const ImVec2 sp = f.to_screen(wpi.lat, wpi.lon);
            // Yellow ring on the waypoint being dragged
            const bool being_picked = f.drag_mission && (int)wi == s_drag_wp;
            const ImU32 fill = being_picked
                ? IM_COL32(255, 240,  0, 240)    // yellow  — awaiting map click
                : wpi.is_current
                ? IM_COL32(255, 100, 50, 220)    // orange  — active (current) WP
                : IM_COL32( 50, 150, 255, 200);  // blue    — normal WP
            const float radius = being_picked ? R + 2.0f : R;
            dl->AddCircleFilled(sp, radius, fill);
            dl->AddCircle(sp, radius, IM_COL32(255, 255, 255, 220), 0, 1.5f);
            char seq_str[8];
            snprintf(seq_str, sizeof(seq_str), "%u", (unsigned)wpi.seq);
            const ImVec2 tsz = ImGui::CalcTextSize(seq_str);
            dl->AddText({ sp.x - tsz.x * 0.5f, sp.y - tsz.y * 0.5f },
                        IM_COL32(255, 255, 255, 255), seq_str);

            // Where it is going, while it is being dragged.
            if (f.drag_mission && (int)wi == s_drag_wp) {
                char ll[48];
                snprintf(ll, sizeof(ll), "%.6f, %.6f", wpi.lat, wpi.lon);
                const ImVec2 lsz = ImGui::CalcTextSize(ll);
                const ImVec2 lp  = { sp.x + R + 6.0f, sp.y - lsz.y * 0.5f };
                dl->AddRectFilled({ lp.x - 3.0f, lp.y - 1.0f },
                                  { lp.x + lsz.x + 3.0f, lp.y + lsz.y + 1.0f },
                                  IM_COL32(0, 0, 0, 180), 3.0f);
                dl->AddText(lp, IM_COL32(255, 230, 60, 255), ll);
            }
        }
    }
}

// ── Guided target marker ──────────────────────────────────────────────────────
// Where the vehicle was last told to go, with the altitude it was told
// to hold. A ringed cross rather than a filled disc, so it reads as a
// destination and never as one more waypoint.
void draw_goto_marker(const MapFrame& f)
{
    ImDrawList* dl = f.dl;
    const GotoTargetState* go = f.go;

    if (go && go->has_target) {
        const ImVec2 tp = f.to_screen(go->target_lat, go->target_lon);
        constexpr float R = 10.0f;
        const ImU32 col = map_amber_u32();
        dl->AddCircle(tp, R,        col, 0, 2.0f);
        dl->AddCircle(tp, R * 0.35f, col, 0, 2.0f);
        dl->AddLine({ tp.x - R - 5.0f, tp.y }, { tp.x - R + 2.0f, tp.y }, col, 2.0f);
        dl->AddLine({ tp.x + R - 2.0f, tp.y }, { tp.x + R + 5.0f, tp.y }, col, 2.0f);
        dl->AddLine({ tp.x, tp.y - R - 5.0f }, { tp.x, tp.y - R + 2.0f }, col, 2.0f);
        dl->AddLine({ tp.x, tp.y + R - 2.0f }, { tp.x, tp.y + R + 5.0f }, col, 2.0f);

        char tlbl[32];
        snprintf(tlbl, sizeof(tlbl), "GOTO  %.0f M", (double)go->target_alt_m);
        map_draw_tag(dl, { tp.x + R + 8.0f, tp.y }, tlbl);

        // A line from the aircraft to the target: at a glance it says both
        // which vehicle the target belongs to and how far it has to run.
        if (f.has_pos)
            dl->AddLine(f.to_screen(f.lat, f.lon), tp, map_amber_u32(0.45f), 1.5f);
    }

    // The point the open GO HERE menu is for, so dragging the menu shows where
    // it is being moved to: a ring with a dot, lighter than a target sent.
    if (ImGui::IsPopupOpen("##map_go")) {
        const ImVec2 pp = f.to_screen(s_ctx_lat, s_ctx_lon);
        constexpr float R = 7.0f;
        dl->AddCircle(pp, R, map_amber_u32(0.9f), 0, 1.5f);
        dl->AddCircleFilled(pp, 2.0f, map_amber_u32(0.9f));
    }
}

// ── Pick-mode / EDIT hint banner ──────────────────────────────────────────────
void draw_hint_banner(const MapFrame& f)
{
    const MissionPickState* pick = f.pick;
    const bool pick_active = f.pick_active;

    if (pick_active || f.drag_mission || f.drag_fence) {
        char hint[112];
        using FD = MissionPickState::FenceDraw;
        if (pick_active && pick->fence_draw == FD::Polygon)
            snprintf(hint, sizeof(hint), pick->fence_pts.size() < 3
                     ? "  Click to add vertex %d  \xe2\x80\x94  Esc to cancel  "
                     : "  Click to add vertex %d, or the first one to close  \xe2\x80\x94  Esc to cancel  ",
                     (int)pick->fence_pts.size() + 1);
        else if (pick_active && pick->fence_draw == FD::Circle)
            snprintf(hint, sizeof(hint), pick->fence_pts.empty()
                     ? "  Click to place the circle's centre  \xe2\x80\x94  Esc to cancel  "
                     : "  Click to set the radius  \xe2\x80\x94  Esc to cancel  ");
        else if (pick_active)
            snprintf(hint, sizeof(hint),
                     "  Click to place %s  \xe2\x80\x94  Esc to cancel  ",
                     pick->add_label ? pick->add_label : "item");
        else if (f.drag_fence)
            snprintf(hint, sizeof(hint),
                     "  EDIT  \xe2\x80\x94  drag a fence vertex or circle centre  ");
        else
            snprintf(hint, sizeof(hint),
                     "  EDIT  \xe2\x80\x94  drag a waypoint to move it  ");
        map_draw_instruction(f.wp, { f.win_w, f.win_h }, hint);
    }
}

} // namespace map2d_detail
