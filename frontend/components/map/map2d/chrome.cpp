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

// frontend/components/map/map2d/chrome.cpp
//
// The map window's chrome, over the flat map and the 3D view alike: the
// control column, the A/C block, the north pointer and the attribution; and
// on the flat map, the scale bar.

#include "frontend/components/map/map2d/map2d_internal.hpp"
#include "frontend/source/coord_format.hpp"
#include "frontend/components/map/map_view_3d.hpp"
#include "frontend/source/terrain.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <optional>
#include <string>
#include <vector>

using namespace map_detail;

namespace map2d_detail {

// ── Overlay chrome shared by the flat map and the 3D view ─────────────────────

constexpr float AC_H    = 55.0f;   // the A/C block's height
constexpr float BTN_W   = 72.0f;   // the control column, under the A/C block
constexpr float BTN_H   = 30.0f;
constexpr float BTN_GAP = 6.0f;

// The control column, top left under the A/C block, one button a row:
// CENTER, 3D, and in 3D the exaggeration and the imagery switch. Fixed in
// place — turning 3D on adds rows below and moves nothing. r[2i..2i+1] is
// row i's rectangle.
static void overlay_button_rects(ImVec2 wp, ImVec2 r[8])
{
    for (int i = 0; i < 4; ++i) {
        r[2 * i]     = { wp.x + OVL_M, wp.y + OVL_M + AC_H + BTN_GAP + i * (BTN_H + BTN_GAP) };
        r[2 * i + 1] = { r[2 * i].x + BTN_W, r[2 * i].y + BTN_H };
    }
}

bool over_overlay_buttons(ImVec2 wp)
{
    ImVec2 r[8];
    overlay_button_rects(wp, r);
    const int rows = s_3d ? 4 : 2;
    for (int i = 0; i < rows; ++i)
        if (ImGui::IsMouseHoveringRect(r[2 * i], r[2 * i + 1], false)) return true;
    return false;
}

// CENTER, lit while the map is following, and 3D under it; in 3D, the
// vertical exaggeration under that, pressed to step through its values,
// and TEX, lit while imagery covers the terrain.
void draw_overlay_buttons(ImVec2 wp, bool has_pos, double lat, double lon)
{
    ImVec2 r[8];
    overlay_button_rects(wp, r);

    // Without a position there is nothing to centre on.
    ImGui::SetCursorScreenPos(r[0]);
    ImGui::BeginDisabled(!has_pos);
    if (ui_tab_button("CENTER##map_center", { BTN_W, BTN_H }, g_map.follow) &&
        has_pos) {
        g_map.follow     = true;
        g_map.center_lat = lat;
        g_map.center_lon = lon;
    }
    ImGui::EndDisabled();

    const bool avail = map3d_available();
    ImGui::SetCursorScreenPos(r[2]);
    ImGui::BeginDisabled(!avail);
    if (ui_tab_button("3D##map_3d", { BTN_W, BTN_H }, s_3d)) s_3d = !s_3d;
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_AllowWhenDisabled))
        ImGui::SetTooltip(avail ? "Terrain in 3D: drag to orbit, right-drag to pan, wheel to zoom"
                                : "3D needs OpenGL 3.3, which this driver does not offer");
    if (s_3d) {
        char ex[32];
        snprintf(ex, sizeof(ex), "\xc3\x97%g##map_exag", (double)map3d_exaggeration());
        ImGui::SetCursorScreenPos(r[4]);
        if (ui_tab_button(ex, { BTN_W, BTN_H }, false)) map3d_cycle_exaggeration();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
            ImGui::SetTooltip("Vertical exaggeration: heights drawn this many times their size");

        ImGui::SetCursorScreenPos(r[6]);
        if (ui_tab_button("TEX##map_tex", { BTN_W, BTN_H }, map3d_textures())) map3d_toggle_textures();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
            ImGui::SetTooltip("Imagery on the terrain; off shows the bare elevation mesh");
    }
}

// The active vehicle's height above the terrain under it: its own AMSL
// altitude less the terrain data's ground there — so it is the GCS's word on
// the ground, not the vehicle's, and disagrees with the vehicle's own
// "AGL" (above home) wherever home and the ground there differ. nullopt
// without a position and altitude, or while the terrain tile loads.
std::optional<float> active_terrain_agl(const std::vector<MapVehicle>* fleet)
{
    if (!fleet) return std::nullopt;
    for (const MapVehicle& mv : *fleet) {
        if (!mv.active || !mv.has_pos || !mv.has_alt) continue;
        if (auto g = terrain_amsl(mv.lat, mv.lon)) return mv.alt_amsl - *g;
    }
    return std::nullopt;
}

// The A/C state block, top left. Its third line is height above the terrain
// data, `ter_agl`; red, and saying so, when the vehicle is below it. Then how far it
// is from home.
void draw_ac_block(ImDrawList* dl, ImVec2 wp, bool has_pos,
                   double lat, double lon, float heading, bool has_hdg,
                   float alt_rel, float gs, std::optional<float> ter_agl,
                   const std::string& home_dist)
{
    ImFont*     fm  = g_font_micro ? g_font_micro : ImGui::GetFont();
    const ImU32 lbl = ui_col_label();
    const ImU32 val = ui_col_value();

    char l1[64], l2[64];
    if (has_pos)
        snprintf(l1, sizeof(l1), "A/C  %s", format_lat_lon(lat, lon, 5).c_str());
    else
        snprintf(l1, sizeof(l1), "A/C  NO POSITION");
    snprintf(l2, sizeof(l2), "HDG %s  ALT %.0f AGL  GS %.0f",
             has_hdg ? "" : "---", (double)alt_rel, (double)gs);
    if (has_hdg)
        snprintf(l2, sizeof(l2), "HDG %03d  ALT %.0f AGL  GS %.0f",
                 (int)heading, (double)alt_rel, (double)gs);

    char l3[96];
    const bool below = ter_agl && *ter_agl < 0.0f;
    if (!ter_agl)    snprintf(l3, sizeof(l3), "ALT AGL TER ---");
    else if (below)  snprintf(l3, sizeof(l3), "BELOW TERRAIN ~%.0f m", (double)-*ter_agl);
    else             snprintf(l3, sizeof(l3), "ALT AGL TER ~%.0f", (double)*ter_agl);
    if (!home_dist.empty()) {
        const size_t len = strlen(l3);
        snprintf(l3 + len, sizeof(l3) - len, "  %s", home_dist.c_str());
    }

    const float w1 = ui_tracked_width(fm, UI_SZ_MICRO, l1);
    const float w2 = ui_tracked_width(fm, UI_SZ_MICRO, l2);
    const float w3 = ui_tracked_width(fm, UI_SZ_MICRO, l3);
    const float bw = std::max({ w1, w2, w3 }) + 16.0f;

    const ImVec2 b0 = { wp.x + OVL_M, wp.y + OVL_M };
    const ImVec2 b1 = { b0.x + std::max(bw, BTN_W), b0.y + AC_H };
    s_ac_right = b1.x;
    dl->AddRectFilled(b0, b1, ui_col(g_theme.bg_topbar, 0.78f));
    ui_frame(dl, b0, b1, ui_col(g_theme.separator, 0.7f));
    ui_tracked_text(dl, fm, UI_SZ_MICRO, { b0.x + 8.0f, b0.y + 8.0f },
                    has_pos ? val : lbl, l1);
    ui_tracked_text(dl, fm, UI_SZ_MICRO, { b0.x + 8.0f, b0.y + 23.0f },
                    lbl, l2);
    ui_tracked_text(dl, fm, UI_SZ_MICRO, { b0.x + 8.0f, b0.y + 38.0f },
                    below ? ui_col(g_theme.col_error) : ter_agl ? val : lbl, l3);
}

// North, bottom-right above the attribution, `lift` px higher when that runs
// to a second line; `rot_deg` turns it for a view that does not have north up.
void draw_north_pointer(ImDrawList* dl, ImVec2 wp, float win_w, float win_h,
                        float rot_deg, float lift)
{
    ImFont*     fm  = g_font_micro ? g_font_micro : ImGui::GetFont();
    const float nx  = wp.x + win_w - 22.0f;
    const float ny  = wp.y + win_h - 40.0f - lift;
    const float r   = rot_deg * (float)(M_PI / 180.0);
    const float cs  = std::cos(r), sn = std::sin(r);
    const auto  at  = [&](float x, float y) { return ImVec2{ nx + x * cs - y * sn, ny + x * sn + y * cs }; };
    dl->AddTriangleFilled(at(0.0f, -10.0f), at(-5.0f, 4.0f), at(5.0f, 4.0f), map_amber_u32());
    ui_tracked_text(dl, fm, UI_SZ_MICRO, { nx - 3.0f, ny + 6.0f }, ui_col_label(), "N");
}

// The imagery's attribution, bottom-right, with `extra` on a line above it.
void draw_attribution(ImDrawList* dl, ImVec2 wp, float win_w, float win_h, const char* extra)
{
    float bottom = wp.y + win_h - 4.0f;

    // Wrapped to the window, never clipped: several providers require their credit
    // in full, and a narrow map or a hybrid's two credits would otherwise cut it.
    // Built bottom line first, as they are drawn from the bottom up.
    const float max_w = std::max(60.0f, win_w - 16.0f);
    std::vector<std::string> lines;
    const auto add_wrapped = [&](const std::string& text) {
        std::vector<std::string> wrapped;
        std::string cur;
        size_t start = 0;
        while (start <= text.size()) {
            const size_t sp   = text.find(' ', start);
            const std::string word = text.substr(start, sp == std::string::npos ? sp : sp - start);
            const std::string next = cur.empty() ? word : cur + " " + word;
            if (!cur.empty() && ImGui::CalcTextSize(next.c_str()).x > max_w) {
                wrapped.push_back(cur);
                cur = word;
            } else {
                cur = next;
            }
            if (sp == std::string::npos) break;
            start = sp + 1;
        }
        if (!cur.empty()) wrapped.push_back(cur);
        lines.insert(lines.end(), wrapped.rbegin(), wrapped.rend());
    };

    // A provider's separate credits go on lines of their own, the first on top.
    const std::string attr_str = map_tile_attribution();
    const std::string sep = " \xc2\xb7 ";
    std::vector<std::string> credits;
    for (size_t pos = 0;;) {
        const size_t hit = attr_str.find(sep, pos);
        credits.push_back(attr_str.substr(pos, hit == std::string::npos ? hit : hit - pos));
        if (hit == std::string::npos) break;
        pos = hit + sep.size();
    }
    for (auto it = credits.rbegin(); it != credits.rend(); ++it) add_wrapped(*it);
    if (extra) add_wrapped(extra);

    for (const std::string& text : lines) {
        const char* line = text.c_str();
        const ImVec2 asz = ImGui::CalcTextSize(line);
        const float  ax  = wp.x + win_w - asz.x - 6.0f;
        const float  ay  = bottom - asz.y;
        dl->AddRectFilled({ax - 4.0f, ay - 2.0f},
                           {ax + asz.x + 4.0f, ay + asz.y + 2.0f},
                           map_attr_bg());
        dl->AddText({ax, ay}, map_attr_text(), line);
        bottom = ay - 6.0f;
    }
}

// ── Tactical overlay: A/C block, scale bar, north pointer ─────────────────────
void draw_tactical_overlay(const MapFrame& f)
{
    ImDrawList* dl = f.dl;
    const ImVec2 wp = f.wp;
    const float win_h = f.win_h;

    ImFont*     fm  = g_font_micro ? g_font_micro : ImGui::GetFont();
    const ImU32 lbl = ui_col_label();

    draw_overlay_buttons(wp, f.has_pos, f.lat, f.lon);
    draw_ac_block(dl, wp, f.has_pos, f.lat, f.lon, f.heading, f.has_hdg, f.alt_rel, f.gs,
                  active_terrain_agl(f.fleet),
                  f.has_pos ? map_home_distance(active_vehicle(f.fleet), f.lat, f.lon) : std::string());

    // Scale bar, bottom-left: 120 px of ground truth at this latitude.
    {
        constexpr float BAR_PX = 120.0f;
        const double m_per_px = geo_mercator_world_m(g_map.center_lat) /
                                (256.0 * (double)(1 << g_map.zoom));
        const double span = m_per_px * BAR_PX;
        char scale_s[48];
        if (span >= 1000.0) snprintf(scale_s, sizeof(scale_s), "%.1f km  \xc2\xb7  Z%d", span / 1000.0, g_map.zoom);
        else                snprintf(scale_s, sizeof(scale_s), "%.0f m  \xc2\xb7  Z%d",  span,          g_map.zoom);

        const float sx = wp.x + 10.0f;
        const float sy = wp.y + win_h - 16.0f;
        dl->AddLine({ sx, sy }, { sx + BAR_PX, sy }, lbl, 1.5f);
        dl->AddLine({ sx, sy - 4.0f }, { sx, sy + 4.0f }, lbl, 1.5f);
        dl->AddLine({ sx + BAR_PX, sy - 4.0f }, { sx + BAR_PX, sy + 4.0f }, lbl, 1.5f);
        ui_tracked_text(dl, fm, UI_SZ_MICRO, { sx, sy - 17.0f }, lbl, scale_s);
    }

    draw_north_pointer(dl, wp, f.win_w, win_h, 0.0f);
}

} // namespace map2d_detail
