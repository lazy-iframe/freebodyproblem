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

// The map's symbols: home, the vehicle shapes, and the aircraft with its labels.

#include "frontend/components/map/map_internal.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>

#ifndef M_PI
#  define M_PI 3.14159265358979323846
#endif

namespace map_detail {

// ── Home symbol ───────────────────────────────────────────────────────────────
//
// A helipad: the H inside its circle, which is what the marking on the ground
// looks like from the air. Drawn as strokes rather than as the letter in a
// font, so it keeps its weight and its proportions at any size and matches the
// rest of the vector chrome on this map.

void draw_home(ImDrawList* dl, ImVec2 c, bool active)
{
    const float A = active ? 1.0f : 0.55f;

    constexpr float R  = 11.0f;   // pad radius
    constexpr float HW = 4.5f;    // half-width of the H
    constexpr float HH = 5.5f;    // half-height of the H

    // Green rather than the aircraft's red or the target's amber: home is the
    // one mark on the map that is neither the vehicle nor an instruction to it.
    const ImU32 col = ui_col(g_theme.col_ok, A);

    dl->AddCircleFilled(c, R, ui_col(g_theme.bg_topbar, 0.60f * A));
    dl->AddCircle      (c, R, col, 0, 2.0f);

    dl->AddLine({ c.x - HW, c.y - HH }, { c.x - HW, c.y + HH }, col, 2.0f);
    dl->AddLine({ c.x + HW, c.y - HH }, { c.x + HW, c.y + HH }, col, 2.0f);
    dl->AddLine({ c.x - HW, c.y      }, { c.x + HW, c.y      }, col, 2.0f);
}

// ── Vehicle symbols ───────────────────────────────────────────────────────────
//
// Top-down silhouettes, one per airframe family, or the legacy arrow. All of
// them keep the arrow's idea of colour: the same fixed red pair, port side the
// darker, so which way up the airframe is reads at a glance whatever its
// shape, and an outline in the theme's ring colour.
//
// Shapes are written in a local frame — x to starboard, y forward, in pixels
// at scale 1 — and turned onto the heading by Frame. Every filled part is
// convex, which is what ImDrawList's polygon fill wants.

static MapIconStyle g_icon_style = MapIconStyle::ByType;

static SymbolKind symbol_kind(uint8_t mav_type, MapIconStyle style)
{
    switch (style) {
    case MapIconStyle::Legacy:      return SymbolKind::Arrow;
    case MapIconStyle::Plane:       return SymbolKind::Plane;
    case MapIconStyle::Multicopter: return SymbolKind::Multicopter;
    case MapIconStyle::Helicopter:  return SymbolKind::Helicopter;
    case MapIconStyle::ByType:      break;
    }
    switch (mav_type) {
    case MAV_TYPE_FIXED_WING:
    case MAV_TYPE_FLAPPING_WING:
    case MAV_TYPE_PARAFOIL:
    case MAV_TYPE_VTOL_TAILSITTER_DUOROTOR:
    case MAV_TYPE_VTOL_TAILSITTER_QUADROTOR:
    case MAV_TYPE_VTOL_TILTROTOR:
    case MAV_TYPE_VTOL_FIXEDROTOR:
    case MAV_TYPE_VTOL_TAILSITTER:
    case MAV_TYPE_VTOL_TILTWING:
    case MAV_TYPE_VTOL_RESERVED5:
        return SymbolKind::Plane;
    case MAV_TYPE_QUADROTOR:
    case MAV_TYPE_HEXAROTOR:
    case MAV_TYPE_OCTOROTOR:
    case MAV_TYPE_TRICOPTER:
    case MAV_TYPE_DODECAROTOR:
    case MAV_TYPE_DECAROTOR:
    case MAV_TYPE_GENERIC_MULTIROTOR:
        return SymbolKind::Multicopter;
    case MAV_TYPE_HELICOPTER:
    case MAV_TYPE_COAXIAL:
        return SymbolKind::Helicopter;
    default:
        // Rovers, boats, and an autopilot that has not said: the arrow, which
        // claims nothing about the airframe beyond which way it points.
        return SymbolKind::Arrow;
    }
}

// Local airframe coordinates → screen.
struct Frame {
    ImVec2 c;
    float  dx, dy;    // forward, as a screen-space unit vector
    float  s;         // scale
    ImVec2 operator()(float x, float y) const
    {
        // Starboard is forward turned 90° clockwise on screen: (-dy, dx).
        return { c.x + (-dy * x + dx * y) * s, c.y + (dx * x + dy * y) * s };
    }
};

// Fill and outline one convex part given in local coordinates.
static void part(ImDrawList* dl, const Frame& f, std::initializer_list<ImVec2> local,
                 ImU32 fill, ImU32 edge, float edge_w)
{
    ImVec2 pts[16];
    int n = 0;
    for (const ImVec2& p : local) pts[n++] = f(p.x, p.y);
    dl->AddConvexPolyFilled(pts, n, fill);
    dl->AddPolyline(pts, n, edge, ImDrawFlags_Closed, edge_w);
}

static float draw_plane(ImDrawList* dl, const Frame& f, ImU32 port, ImU32 stbd, ImU32 edge)
{
    // Wings and tailplane under the fuselage, so the fuselage seam is unbroken.
    part(dl, f, { { -3.0f, 9.0f }, { -25.0f, 1.0f }, { -25.0f, -4.0f }, { -3.0f, -2.0f } }, port, edge, 1.2f);
    part(dl, f, { {  3.0f, 9.0f }, {  25.0f, 1.0f }, {  25.0f, -4.0f }, {  3.0f, -2.0f } }, stbd, edge, 1.2f);
    part(dl, f, { { -2.0f, -13.0f }, { -10.0f, -17.5f }, { -10.0f, -20.5f }, { -2.0f, -19.0f } }, port, edge, 1.2f);
    part(dl, f, { {  2.0f, -13.0f }, {  10.0f, -17.5f }, {  10.0f, -20.5f }, {  2.0f, -19.0f } }, stbd, edge, 1.2f);
    // Fuselage in two halves, split down the centreline like the arrow.
    part(dl, f, { { 0.0f, 27.0f }, { 3.0f, 21.0f }, { 3.6f, 9.0f }, { 3.0f, -14.0f },
                  { 1.4f, -21.0f }, { 0.0f, -21.0f } }, stbd, edge, 1.3f);
    part(dl, f, { { 0.0f, 27.0f }, { 0.0f, -21.0f }, { -1.4f, -21.0f }, { -3.0f, -14.0f },
                  { -3.6f, 9.0f }, { -3.0f, 21.0f } }, port, edge, 1.3f);
    return 27.0f;
}

static float draw_multicopter(ImDrawList* dl, const Frame& f, ImU32 port, ImU32 stbd, ImU32 edge,
                              float A)
{
    constexpr float ARM = 15.0f, ROTOR = 8.5f;
    const ImVec2 arms[4] = { { -ARM, ARM }, { ARM, ARM }, { ARM, -ARM }, { -ARM, -ARM } };
    for (int i = 0; i < 4; ++i) {
        const bool   port_side = arms[i].x < 0.0f;
        const bool   front     = arms[i].y > 0.0f;
        const ImVec2 hub       = f(arms[i].x, arms[i].y);
        dl->AddLine(f(0.0f, 0.0f), hub, port_side ? port : stbd, 3.0f * f.s);
        // Rotor discs: washed, the front pair less so — the nose of a
        // symmetric airframe is otherwise only the outline's say-so.
        const float disc_a = (front ? 0.55f : 0.28f) * A;
        dl->AddCircleFilled(hub, ROTOR * f.s, (port_side ? port : stbd) & 0x00FFFFFF |
                                              ((ImU32)(disc_a * 255.0f) << IM_COL32_A_SHIFT));
        dl->AddCircle(hub, ROTOR * f.s, edge, 0, (front ? 1.6f : 1.0f));
    }
    // Body: a small rounded plate with a nose wedge.
    part(dl, f, { { 0.0f, 12.0f }, { 5.0f, 5.0f }, { 5.5f, -5.0f }, { 0.0f, -6.5f } }, stbd, edge, 1.2f);
    part(dl, f, { { 0.0f, 12.0f }, { 0.0f, -6.5f }, { -5.5f, -5.0f }, { -5.0f, 5.0f } }, port, edge, 1.2f);
    return ARM + ROTOR;
}

static float draw_helicopter(ImDrawList* dl, const Frame& f, ImU32 port, ImU32 stbd, ImU32 edge,
                             float A, float rotor)
{
    // Tail boom and tail rotor first, under the cabin.
    part(dl, f, { { 1.6f, -5.0f }, { 1.0f, -27.0f }, { -1.0f, -27.0f }, { -1.6f, -5.0f } }, port, edge, 1.0f);
    dl->AddLine(f(2.5f, -22.0f), f(2.5f, -29.0f), edge, 2.0f * f.s);
    part(dl, f, { { -1.0f, -24.0f }, { -6.0f, -26.0f }, { -6.0f, -27.5f }, { -1.0f, -27.0f } }, port, edge, 1.0f);
    // Cabin, split down the middle like the others.
    part(dl, f, { { 0.0f, 15.0f }, { 4.5f, 12.0f }, { 6.5f, 5.0f }, { 6.0f, -3.0f }, { 3.5f, -7.0f }, { 0.0f, -7.0f } },
         stbd, edge, 1.3f);
    part(dl, f, { { 0.0f, 15.0f }, { 0.0f, -7.0f }, { -3.5f, -7.0f }, { -6.0f, -3.0f }, { -6.5f, 5.0f }, { -4.5f, 12.0f } },
         port, edge, 1.3f);
    // Main rotor: the disc it sweeps, and two blades at `rotor` radians — the
    // caller turns them while the vehicle is armed, slowly enough to read as a
    // rotor rather than as flicker.
    constexpr float R = 23.0f;
    dl->AddCircle(f(0.0f, 0.0f), R * f.s, (edge & 0x00FFFFFF) | ((ImU32)(0.45f * A * 255.0f) << IM_COL32_A_SHIFT),
                  0, 1.0f);
    // Washed back, so the cabin under them still reads.
    const ImU32 blade = (edge & 0x00FFFFFF) | ((ImU32)(0.7f * A * 255.0f) << IM_COL32_A_SHIFT);
    const float bx = std::cos(rotor) * R, by = std::sin(rotor) * R;
    dl->AddLine(f(-bx, -by), f(bx, by), blade, 1.6f * f.s);
    dl->AddLine(f(by, -bx),  f(-by, bx), blade, 1.6f * f.s);
    dl->AddCircleFilled(f(0.0f, 0.0f), 2.2f * f.s, edge);
    return 15.0f;
}

static float draw_arrow(ImDrawList* dl, const Frame& f, ImU32 port, ImU32 stbd, ImU32 edge)
{
    // Arrow dimensions
    constexpr float BODY   = 27.0f; // distance from center to tip
    constexpr float TAIL   = 15.0f; // distance from center to tail
    constexpr float WING   = 10.5f; // half-width at wing base
    constexpr float NOTCH  = 6.0f;  // tail notch depth

    const ImVec2 tip    = f(0.0f, BODY);
    const ImVec2 tail_c = f(0.0f, -TAIL);
    const ImVec2 wL     = f(-WING, 0.0f);
    const ImVec2 wR     = f( WING, 0.0f);
    // Tail notch pulls tail center slightly toward tip
    const ImVec2 notch  = f(0.0f, -(TAIL - NOTCH));

    dl->AddTriangleFilled(tip, wL, notch, port);
    dl->AddTriangleFilled(tip, wR, notch, stbd);
    dl->AddQuad(tip, wL, notch, wR, edge, 1.5f);
    dl->AddLine(tail_c, notch, edge, 1.5f);
    return BODY;
}

// Draws the symbol and returns how far ahead of `c` its nose is, at scale 1 —
// where the heading line starts.
static float draw_symbol(ImDrawList* dl, ImVec2 c, SymbolKind kind, float heading_deg,
                         float A, float scale, float rotor)
{
    const float rad = heading_deg * (float)(M_PI / 180.0);
    const Frame f   = { c, std::sin(rad), -std::cos(rad), scale };

    // Two shades of the same red rather than two colours: the seam is what
    // says which way up the airframe is at a glance, and it has to read
    // without becoming a second marker. Fixed rather than themed, since the
    // pair only works as a pair.
    const ImU32 port = IM_COL32(150, 26, 26, (int)(235 * A));
    const ImU32 stbd = IM_COL32(206, 48, 48, (int)(235 * A));
    const ImU32 edge = ui_col(g_theme.map_vehicle_ring, A);

    switch (kind) {
    case SymbolKind::Plane:       return draw_plane(dl, f, port, stbd, edge);
    case SymbolKind::Multicopter: return draw_multicopter(dl, f, port, stbd, edge, A);
    case SymbolKind::Helicopter: {
        // Drawn 10% up: at the others' size its rotor disc read as the whole
        // symbol and the cabin inside it as too small to make out.
        constexpr float HELI_SCALE = 1.1f;
        Frame big = f;
        big.s *= HELI_SCALE;
        return draw_helicopter(dl, big, port, stbd, edge, A, rotor) * HELI_SCALE;
    }
    case SymbolKind::Arrow:       break;
    }
    return draw_arrow(dl, f, port, stbd, edge);
}

// ── Aircraft symbol ───────────────────────────────────────────────────────────
//
// One vehicle: the airframe, its heading line, and the label naming it. Drawn
// for every vehicle on the map, so everything that distinguishes the one the
// panels are bound to from the rest is the `active` flag rather than a second
// copy of this code.

void draw_aircraft(ImDrawList* dl, ImVec2 c, const MapVehicle& mv,
                   ImVec2 wp, float win_w, float win_h)
{
    // Inactive aircraft are drawn down, not out: they stay legible, but the
    // vehicle every other panel is showing is the one the eye lands on.
    const float A = mv.active ? 1.0f : 0.55f;

    // Checked in the SWARM view: a ring at full strength whatever the
    // vehicle's own dimming, so the set a batch command will reach can be read
    // off the map before the button is pressed.
    if (mv.checked)
        dl->AddCircle(c, 34.0f, map_amber_u32(), 0, 2.0f);

    if (mv.has_hdg) {
        // Heading (degrees CW from north) as a screen vector. Screen Y grows
        // downward, so north is -Y: (sin, -cos).
        const float rad = mv.heading * (float)(M_PI / 180.0);
        const float dx  = std::sin(rad);   // east component  (+x on screen)
        const float dy  = -std::cos(rad);  // north component (-y on screen)

        // Rotor angle per vehicle, advanced only while it is armed — so on
        // disarm the blades stop where they are rather than snapping to a
        // parked angle, and spin up again from there.
        static std::unordered_map<VehicleId, float, VehicleIdHash> s_rotor;
        float& rotor = s_rotor.try_emplace(mv.id, (float)(M_PI * 0.25)).first->second;
        if (mv.armed)
            rotor = std::fmod(rotor + 1.6f * ImGui::GetIO().DeltaTime, (float)(2.0 * M_PI));

        const float  nose = draw_symbol(dl, c, symbol_kind(mv.mav_type, g_icon_style),
                                        mv.heading, A, 1.0f, rotor);
        const ImVec2 tip  = { c.x + dx * nose, c.y + dy * nose };

        // Dotted heading line. For the active aircraft it runs the width of the
        // panel, which is what says where the vehicle is pointed against
        // everything else on the map. The others get a stub: three lines across
        // the whole map is not three times as much information, it is a grid
        // over the imagery.
        constexpr float DOT_LEN  = 5.0f;
        constexpr float DOT_GAP  = 5.0f;
        constexpr float STUB_LEN = 40.0f;
        constexpr float PERIOD   = DOT_LEN + DOT_GAP;

        // The span of the ray that is actually on the panel — where it comes
        // into view and where it leaves — rather than only how far it is to the
        // far edge.
        //
        // The difference matters when the aircraft is off screen, which happens
        // the moment there are two vehicles far apart: switch to one while the
        // map is still over the other, and the tip of this ray is however many
        // pixels apart those two places are. At zoom 19 that is tens of
        // millions, and asking only for the exit distance and stepping ten
        // pixels at a time meant millions of segments per frame for a line with
        // nothing of it visible. Clipped properly the span is at most the
        // panel's diagonal, whatever the aircraft is doing.
        float t_enter = 0.0f;
        float t_exit  = mv.active ? std::numeric_limits<float>::max() : STUB_LEN;
        {
            const float org[2] = { tip.x, tip.y };
            const float dir[2] = { dx, dy };
            const float lo [2] = { wp.x, wp.y };
            const float hi [2] = { wp.x + win_w, wp.y + win_h };
            for (int ax = 0; ax < 2; ++ax) {
                if (std::fabs(dir[ax]) < 1e-6f) {
                    // Parallel to this edge: either always within it or never.
                    if (org[ax] < lo[ax] || org[ax] > hi[ax]) {
                        t_exit = t_enter;   // nothing to draw
                        break;
                    }
                    continue;
                }
                float ta = (lo[ax] - org[ax]) / dir[ax];
                float tb = (hi[ax] - org[ax]) / dir[ax];
                if (ta > tb) { const float tmp = ta; ta = tb; tb = tmp; }
                if (ta > t_enter) t_enter = ta;
                if (tb < t_exit)  t_exit  = tb;
            }
        }

        // Phase pinned to the ray's own origin rather than to wherever it
        // enters the panel, so the dashes do not crawl along the line as the
        // map is panned.
        for (float t = std::floor(t_enter / PERIOD) * PERIOD;
             t < t_exit; t += PERIOD) {
            const float a0 = std::max(t, t_enter);
            const float a1 = std::min(t + DOT_LEN, t_exit);
            if (a1 <= a0) continue;
            dl->AddLine({ tip.x + dx * a0, tip.y + dy * a0 },
                        { tip.x + dx * a1, tip.y + dy * a1 },
                        ui_col(g_theme.map_vehicle_ring, A), INDICATOR_LINE_W);
        }
    } else {
        // No heading — plain dot marker
        dl->AddCircleFilled(c, 13.5f, ui_col(g_theme.map_vehicle_fill, A));
        dl->AddCircle      (c, 13.5f, ui_col(g_theme.map_vehicle_ring, A), 0, 2.0f);
    }

    // ── Label ────────────────────────────────────────────────────────────────
    // "(2) SYS 1" — Fleet's number, then what the aircraft calls itself. The
    // number is there because the sysid is not a name: two airframes fresh off
    // the bench both answer to 1, and the map would show two identical labels.
    // Under it, its height above the terrain data.
    //
    // Held above the symbol at a fixed offset rather than swung around with the
    // heading, so a turning aircraft does not drag its own label round with it.
    // Boxed, because text straight on satellite imagery is unreadable over
    // anything pale.
    map_draw_vehicle_tags(dl, c, 38.0f, mv);
}

} // namespace map_detail

using namespace map_detail;

void map_draw_home_symbol(ImDrawList* dl, ImVec2 c, bool active) { draw_home(dl, c, active); }

void map_view_set_icon_style(MapIconStyle s) { g_icon_style = s; }

SymbolKind map_symbol_kind(uint8_t mav_type) { return symbol_kind(mav_type, g_icon_style); }

void map_draw_vehicle_symbol(ImDrawList* dl, ImVec2 c, uint8_t mav_type, MapIconStyle style,
                             float heading_deg, float alpha, float scale)
{
    // Blades parked across the cabin: a preview is of no vehicle, armed or not.
    draw_symbol(dl, c, symbol_kind(mav_type, style), heading_deg, alpha, scale,
                (float)(M_PI * 0.25));
}
