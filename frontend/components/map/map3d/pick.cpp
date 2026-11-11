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

// frontend/components/map/map3d/pick.cpp
//
// Picking a point in 3D: a new mission item (LOC?) and GO HERE — where on
// the ground, then how high over it — and the standing GO HERE target.

#include "frontend/components/map/map3d/map3d_internal.hpp"
#include "frontend/source/theme.hpp"
#include "frontend/components/ui_kit.hpp"
#include "frontend/source/mission_alt.hpp"
#include "frontend/source/terrain.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <optional>
#include <string>
#include <vector>

namespace map3d_detail {

// ── Placing a mission item ────────────────────────────────────────────────────
//
// Two clicks, as the mission tab's LOC? asks for one. The first is on the
// ground: the new waypoint floats above the pointer at the height it would
// get, with its line dropped to where the pointer is, so what is being chosen
// is the spot under it. The second sets the height: the waypoint stays over
// that spot and rides its vertical line to wherever the pointer is nearest.
// A click is a press and release that barely moved — a drag still orbits, in
// either step, so the view can be turned to judge the height.

constexpr float DEFAULT_REL_ALT = 20.0f;   // as the mission tab's DEFAULT_ALT_M

// The height a point is being set to: above home, and above the terrain data. Then
// how far it is from home, if that is known.
void height_readout(char* out, size_t n, double rel, double agl, const std::string& home_dist)
{
    const char* sep = home_dist.empty() ? "" : "  \xc2\xb7  ";
    if (agl < 0.0) snprintf(out, n, "%.0f m  \xc2\xb7  ~%.0f m IN TERRAIN%s%s", rel, -agl, sep, home_dist.c_str());
    else           snprintf(out, n, "%.0f m  \xc2\xb7  ALT AGL TER ~%.0f%s%s", rel, agl, sep, home_dist.c_str());
}

// A point by position and height AMSL.
struct Geo { double lat, lon, amsl; };

// What differs between the pickers: the button that clicks, what the banner
// calls the point, how its marker looks, where it starts and what its faint
// leg comes from.
struct PickStyle {
    ImGuiMouseButton  button;
    const char*       what;         // "the new WAYPOINT"
    const char*       tag;          // drawn in the marker
    ImU32             fill, text;
    double            start_rel;    // height above home the preview starts at
    std::optional<Geo> from;        // a faint leg from here to the point, checked against the terrain
    bool              tagged = false;   // its readout in the instruction's green box
    const MapVehicle* active = nullptr; // whose home its distance is from
};

PointPick s_place;                  // the mission tab's LOC?, left button
uint16_t  s_place_cmd = 0;          // the command it is for
PointPick s_goto;                   // GO HERE, left button

// A right-click on the view, outside any pick, until map_view takes it and
// opens its CLEAR menu.
bool s_clear_ready = false;

// The GO HERE point once its height is clicked, until map_view takes it.
bool   s_goto_ready = false;
double s_goto_lat = 0, s_goto_lon = 0;
float  s_goto_rel = 0;

bool s_press_in_view[2] = {};       // the left / right press began over the view

// A click of `b` on the view: pressed over it and released barely moved — so
// a drag with the same button still orbits or pans.
bool view_click(ImGuiMouseButton b, bool hovered)
{
    return hovered && s_press_in_view[b] && ImGui::IsMouseReleased(b) &&
           ImGui::GetIO().MouseDragMaxDistanceSqr[b] < 16.0f;
}

// A ray from the camera through the pointer.
V3 pointer_ray(const Frame& f, V3 eye, V3 target, ImVec2 mouse)
{
    const V3     fwd = norm(target - eye);
    const V3     rt  = norm(cross(fwd, { 0.0, 0.0, 1.0 }));
    const V3     up  = cross(rt, fwd);
    const double th  = std::tan(FOV_Y * 0.5);
    const double nx  = (mouse.x - f.p0.x) / f.size.x * 2.0 - 1.0;
    const double ny  = 1.0 - (mouse.y - f.p0.y) / f.size.y * 2.0;
    return norm(fwd + rt * (nx * th * f.size.x / f.size.y) + up * (ny * th));
}

// Where the ray first meets the ground: marched out in steps that grow with
// distance, then narrowed by halving the last step.
static std::optional<V3> ray_ground(const Frame& f, V3 o, V3 d, double max_t)
{
    const auto under = [&](double t) {
        const V3   p  = o + d * t;
        const auto gz = ground_z(f, p.x, p.y);
        return gz && p.z <= *gz;
    };
    double prev = 0.0, t = 1.0;
    for (int i = 0; i < 800 && t < max_t; ++i) {
        if (under(t)) {
            double lo = prev, hi = t;
            for (int k = 0; k < 14; ++k) {
                const double mid = 0.5 * (lo + hi);
                (under(mid) ? hi : lo) = mid;
            }
            return o + d * hi;
        }
        prev = t;
        t   += std::max(2.0, t * 0.015);
    }
    return std::nullopt;
}

// The height on the vertical line through (x, y) nearest the ray, or nothing
// when the ray runs along the line (looking straight down) or meets it
// behind the camera. Closest points of two lines.
std::optional<double> vertical_z_nearest(V3 o, V3 d, double x, double y)
{
    const V3     w0  = { x - o.x, y - o.y, -o.z };
    const double b   = d.z, e = dot(d, w0);
    const double den = 1.0 - b * b;
    if (den < 0.02) return std::nullopt;
    if ((e - b * w0.z) / den <= 0.0) return std::nullopt;
    return (b * e - w0.z) / den;
}

ImVec4 ok_or_error(bool error) { return error ? g_theme.col_error : col_ok(); }

// The relative altitude the new item starts at: the last placed item's
// height above home, as the mission tab would carry it on; else the default.
static double starting_rel_alt(const std::vector<MissionItem>* mission, std::optional<float> home)
{
    if (mission && home)
        for (auto it = mission->rbegin(); it != mission->rend(); ++it) {
            if (it->lat == 0.0 && it->lon == 0.0) continue;
            if (!mission_frame_is_global(it->frame)) continue;
            if (auto a = mission_item_amsl(*it, home, terrain_amsl(it->lat, it->lon)))
                return *a - *home;
        }
    return DEFAULT_REL_ALT;
}

// The instruction for the step being picked: the same box, in the same
// place, as the 2D map's.
void draw_banner(const Frame& f, const char* text)
{
    map_draw_instruction(f.p0, f.size, text);
}

// Advance a pick by this frame's pointer and draw it: the point, its line to
// the ground, the readout, the banner. True on the click that sets the
// height; the point is then in `pp` for the caller to take.
static bool run_point_pick(PointPick& pp, ImDrawList* dl, const Frame& f, V3 eye, V3 target, double range,
                    bool hovered, std::optional<float> home, const PickStyle& st)
{
    const V3   ray   = pointer_ray(f, eye, target, ImGui::GetIO().MousePos);
    const bool click = view_click(st.button, hovered);
    bool steep = false;

    if (!pp.alt_phase) {
        if (!hovered) return false;
        const auto hit = ray_ground(f, eye, ray, range);
        if (!hit) return false;
        unmerc(f.u0 + hit->x / f.m, f.v0 - hit->y / f.m, pp.lat, pp.lon);
        pp.ground = f.amsl_at(*hit);
        // Where it would start, but never in the hill: over rising ground the
        // line has to reach down to the spot chosen.
        pp.amsl   = std::max((home ? *home : pp.ground) + st.start_rel, pp.ground + DEFAULT_REL_ALT);
        if (click) pp.alt_phase = true;
    } else {
        const V3 foot = f.world(pp.lat, pp.lon, pp.ground);
        if (hovered) {
            if (auto z = vertical_z_nearest(eye, ray, foot.x, foot.y))
                pp.amsl = std::clamp(f.amsl_at({ foot.x, foot.y, *z }), pp.ground - POINT_MAX_REACH,
                                     pp.ground + POINT_MAX_REACH);
            else
                steep = true;
        }
        if (click) return true;
    }

    const V3 top = f.world(pp.lat, pp.lon, pp.amsl);
    const bool in_terrain = pp.alt_phase && pp.amsl < pp.ground;
    std::optional<Clearance> leg;
    if (st.from) {
        const V3 fw = f.world(st.from->lat, st.from->lon, st.from->amsl);
        draw_path(dl, f, fw, top, (st.fill & 0x00FFFFFFu) | 0x78000000u, 1.5f, under_col(0.7f));
        leg = path_clearance(st.from->lat, st.from->lon, st.from->amsl, pp.lat, pp.lon, pp.amsl);
        draw_clearance(dl, f, fw, top, *leg);
    }
    drop_line(dl, f, eye, top, pp.ground);
    ImVec2 sp;
    if (f.screen(top, sp)) {
        const ImVec2 ts = ImGui::CalcTextSize(st.tag);
        if (st.tagged) {
            // The marker as the readout beside it: dark, tinted green, a green
            // ring, green letters.
            const ImVec4 ok = in_terrain ? g_theme.col_error : col_ok();
            const float  r  = std::max(9.0f, ts.x * 0.5f + 3.0f);
            dl->AddCircleFilled(sp, r, ui_col(g_theme.bg_topbar, 0.92f));
            dl->AddCircleFilled(sp, r, ImGui::GetColorU32({ ok.x, ok.y, ok.z, 0.14f }));
            dl->AddCircle(sp, r, ImGui::GetColorU32(ok), 0, 1.5f);
            dl->AddText({ sp.x - ts.x * 0.5f, sp.y - ts.y * 0.5f }, ImGui::GetColorU32(ok), st.tag);
        } else {
            dl->AddCircleFilled(sp, 9.0f, in_terrain ? ui_col(g_theme.col_error, 0.95f) : st.fill);
            dl->AddCircle(sp, 9.0f, IM_COL32(255, 255, 255, 220), 0, 1.5f);
            dl->AddText({ sp.x - ts.x * 0.5f, sp.y - ts.y * 0.5f },
                        in_terrain ? IM_COL32(255, 255, 255, 255) : st.text, st.tag);
        }

        char ro[96];
        const double base = home ? *home : pp.ground;
        height_readout(ro, sizeof(ro), pp.amsl - base, pp.amsl - pp.ground,
                       map_home_distance(st.active, pp.lat, pp.lon));
        if (st.tagged) {
            map_draw_tag(dl, { sp.x + 14.0f, sp.y }, ro, ok_or_error(in_terrain));
        } else {
            const ImVec2 rs = ImGui::CalcTextSize(ro);
            const ImVec2 rp = { sp.x + 14.0f, sp.y - rs.y * 0.5f };
            dl->AddRectFilled({ rp.x - 3, rp.y - 1 }, { rp.x + rs.x + 3, rp.y + rs.y + 1 }, IM_COL32(0, 0, 0, 180), 3.0f);
            dl->AddText(rp, in_terrain ? ui_col(g_theme.col_error) : IM_COL32(255, 230, 60, 255), ro);
        }
    }

    const char* btn = st.button == ImGuiMouseButton_Right ? "Right-click" : "Click";
    char hint[128];
    if (!pp.alt_phase)
        snprintf(hint, sizeof(hint), "  %s the ground under %s  \xe2\x80\x94  Esc to cancel  ", btn, st.what);
    else if (steep)
        snprintf(hint, sizeof(hint), "  Tilt the view to set the height  \xe2\x80\x94  Esc to cancel  ");
    else if (in_terrain)
        snprintf(hint, sizeof(hint), "  ~%.0f m inside the terrain data here  \xe2\x80\x94  %s to set anyway, Esc to cancel  ",
                 pp.ground - pp.amsl, st.button == ImGuiMouseButton_Right ? "right-click" : "click");
    else
        snprintf(hint, sizeof(hint), "  Move up or down for its height, %s to set  \xe2\x80\x94  Esc to cancel  ",
                 st.button == ImGuiMouseButton_Right ? "right-click" : "click");
    draw_banner(f, hint);
    return false;
}

// The mission tab's LOC?, in 3D: the new item's place and height.
void place_item(ImDrawList* dl, const Frame& f, V3 eye, V3 target, double range, bool hovered,
                MissionPickState& pick, const std::vector<MissionItem>* mission,
                const MapVehicle* active)
{
    if (s_place_cmd != pick.add_command) {
        s_place     = {};
        s_place_cmd = pick.add_command;
    }
    const std::optional<float> home = plan_home_amsl(active, mission);

    // The faint leg comes from the last placed item.
    std::optional<Geo> from;
    if (mission && home)
        for (auto it = mission->rbegin(); it != mission->rend(); ++it) {
            if (it->lat == 0.0 && it->lon == 0.0) continue;
            if (!mission_frame_is_global(it->frame)) continue;
            if (const auto g = terrain_amsl(it->lat, it->lon))
                from = Geo{ it->lat, it->lon, mission_item_amsl(*it, home, g).value_or(*g) };
            break;
        }

    char what[48], tag[8];
    snprintf(what, sizeof(what), "the new %s", pick.add_label ? pick.add_label : "item");
    snprintf(tag, sizeof(tag), "%d", mission ? (int)mission->size() : 0);
    const PickStyle st{ ImGuiMouseButton_Left, what, tag, IM_COL32(255, 240, 0, 240),
                        IM_COL32(0, 0, 0, 255), starting_rel_alt(mission, home), from, false, active };

    if (run_point_pick(s_place, dl, f, eye, target, range, hovered, home, st)) {
        const double base = home ? *home : s_place.ground;
        pick.picked_lat     = s_place.lat;
        pick.picked_lon     = s_place.lon;
        pick.picked_alt     = (float)std::round(s_place.amsl - base);
        pick.has_picked_alt = true;
        pick.pick_done      = true;
        s_place = {};
    }
}

// GO HERE, in 3D: a click starts it, a click on the ground fixes where, a
// click sets the height — and the map opens its GO HERE menu there, the
// height filled in above home. Starts at the vehicle's own height, as the 2D
// menu does: "go there, stay at this altitude". Esc or a right-click gives it
// up; outside it, a right-click is for the map's CLEAR menu.
void pick_goto(ImDrawList* dl, const Frame& f, V3 eye, V3 target, double range, bool hovered,
               const GotoTargetState& go, const std::vector<MissionItem>* mission,
               const MapVehicle* active)
{
    if (!s_goto.active) {
        if (view_click(ImGuiMouseButton_Left, hovered)) {
            s_goto = {};
            s_goto.active = true;   // the click that starts it places nothing
        } else if (view_click(ImGuiMouseButton_Right, hovered)) {
            s_clear_ready = true;
        }
        return;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Escape, false) || view_click(ImGuiMouseButton_Right, hovered)) {
        s_goto = {};
        return;
    }

    const std::optional<float> home = plan_home_amsl(active, mission);
    std::optional<Geo> from;
    if (active && active->has_pos && active->has_alt)
        from = Geo{ active->lat, active->lon, active->alt_amsl };
    const PickStyle st{ ImGuiMouseButton_Left, "the GO HERE target", "GO", map_amber_u32(),
                        IM_COL32(0, 0, 0, 255), go.current_alt_m, from, true, active };

    if (run_point_pick(s_goto, dl, f, eye, target, range, hovered, home, st)) {
        const double base = home ? *home : s_goto.ground;
        s_goto_lat   = s_goto.lat;
        s_goto_lon   = s_goto.lon;
        s_goto_rel   = (float)std::round(s_goto.amsl - base);
        s_goto_ready = true;
        s_goto = {};
    }
}

// The standing GO HERE target, as on the 2D map: a ringed cross at the height
// it was sent with, its line to the ground, and a line from the vehicle.
void draw_goto_target(ImDrawList* dl, const Frame& f, V3 eye, const GotoTargetState& go,
                      const std::vector<MissionItem>* mission, const MapVehicle* active)
{
    const std::optional<float> ground = terrain_amsl(go.target_lat, go.target_lon);
    const std::optional<float> home   = plan_home_amsl(active, mission);
    if (!ground) return;
    const V3 p = f.world(go.target_lat, go.target_lon, (home ? *home : *ground) + go.target_alt_m);
    const ImU32 col = map_amber_u32();
    if (active && active->has_pos && active->has_alt)
        draw_path(dl, f, f.world(active->lat, active->lon, active->alt_amsl), p,
                  map_amber_u32(0.5f), 1.5f, under_col(0.7f));
    drop_line(dl, f, eye, p, *ground);
    ImVec2 tp;
    if (!f.screen(p, tp)) return;
    constexpr float R = 10.0f;
    dl->AddCircle(tp, R,        col, 0, 2.0f);
    dl->AddCircle(tp, R * 0.35f, col, 0, 2.0f);
    dl->AddLine({ tp.x - R - 5.0f, tp.y }, { tp.x - R + 2.0f, tp.y }, col, 2.0f);
    dl->AddLine({ tp.x + R - 2.0f, tp.y }, { tp.x + R + 5.0f, tp.y }, col, 2.0f);
    dl->AddLine({ tp.x, tp.y - R - 5.0f }, { tp.x, tp.y - R + 2.0f }, col, 2.0f);
    dl->AddLine({ tp.x, tp.y + R - 2.0f }, { tp.x, tp.y + R + 5.0f }, col, 2.0f);
    char t[32];
    snprintf(t, sizeof(t), "GOTO  %.0f M", (double)go.target_alt_m);
    map_draw_tag(dl, { tp.x + R + 8.0f, tp.y }, t);
}

} // namespace map3d_detail
