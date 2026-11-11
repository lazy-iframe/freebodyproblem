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

// Right sidebar: the head-up display, and the attitude panel's body in either style.

#define _USE_MATH_DEFINES
#include <cmath>
#include "frontend/elements/sidebar_right.hpp"
#include "frontend/components/sidebar_right/sidebar_internal.hpp"
#include "frontend/components/sidebar_right/hud_parts.hpp"
#include "frontend/elements/center_view.hpp"
#include "frontend/source/vehicle_ui_state.hpp"
#include "frontend/source/layout.hpp"
#include "frontend/source/theme.hpp"
#include "frontend/components/ui_kit.hpp"
#include "imgui.h"
#include "frontend/components/sidebar_left/mavlink_field_catalog_generated.hpp"
#include "frontend/source/settings.hpp"
#include <vector>
#include <cstdio>
#include <cstring>
#include <limits>
#include <cfloat>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <deque>
#include <cctype>
#include <ctime>
#include <string>

namespace right_sidebar {

void draw_artificial_horizon(const VehicleState& vs, ImVec2 sz,
                             float text_size)
{
    ImDrawList*  dl = ImGui::GetWindowDrawList();
    const ImVec2 p0 = ImGui::GetCursorScreenPos();
    const ImVec2 p1 = { p0.x + sz.x, p0.y + sz.y };
    const float  size = std::min(sz.x, sz.y);

    // Bold mono throughout: digits of one width keep the tape numbers from
    // shuffling as they change, and the heavier stroke holds up over a busy
    // sky. Falls back to the micro font, then the current one.
    const HudText t = {
        g_font_mono_bold ? g_font_mono_bold
                         : (text_size > 0.0f && g_font_micro) ? g_font_micro
                                                              : ImGui::GetFont(),
        (text_size > 0.0f) ? text_size : ImGui::GetFontSize(),
    };
    const float fh = t.size;

    // ── Layout ───────────────────────────────────────────────────────────────
    // Room in each tape for its value box and arrow beside the ticks.
    const float head_h = fh + 4.0f + hud_arrow(t) + 1.0f + std::max(6.0f, size * 0.045f);
    const float tape_w = std::max(size * 0.15f,
                                  (t.width("+00.0") + 8.0f + hud_arrow(t)) / 0.78f);

    // The middle, between the tapes, where the attitude itself reads.
    const ImVec2 c0 = { p0.x + tape_w, p0.y + head_h };
    const ImVec2 c1 = { p1.x - tape_w, p1.y };
    const float  cx = (c0.x + c1.x) * 0.5f;
    const float  cy = (c0.y + c1.y) * 0.5f;

    // The roll arc spans ±60° over the top of the middle, its labels clear of
    // the heading tape above and the speed and altitude tapes either side. The
    // pitch scale and the aircraft symbol are proportioned off it.
    const float arc_r = std::max(8.0f, std::min(cy - c0.y - fh - 6.0f,
                                                ((c1.x - c0.x) * 0.5f - fh * 1.5f)
                                                / 0.866f));
    const float r     = arc_r / 1.08f;

    const float roll_rad = vs.roll  * (float)M_PI / 180.0f;
    const float pitch_px = vs.pitch * (r / 45.0f);

    const float ux  = -sinf(roll_rad);
    const float uy  = -cosf(roll_rad);
    const float ldx = -uy, ldy = ux;

    const float hx = cx - ux * pitch_px;
    const float hy = cy - uy * pitch_px;

    dl->PushClipRect(p0, p1, true);

    // ── Sky and ground, edge to edge ─────────────────────────────────────────
    // Each a slight gradient, brightest at the horizon and dimming away from
    // it, as a real sky and ground do. Measured from the horizon rather than
    // the screen, so the shading banks and pitches with it.
    {
        const ImVec2 sq[4] = { p0, { p1.x, p0.y }, p1, { p0.x, p1.y } };
        const float  span  = std::max(sz.x, sz.y);

        auto shade = [](ImVec4 c, float k) {
            return ui_col({ std::min(1.0f, c.x * k), std::min(1.0f, c.y * k),
                            std::min(1.0f, c.z * k), c.w });
        };
        // `side` +1 for the sky, -1 for the ground: which way is away.
        auto fill = [&](const ImVec2* poly, int n, ImVec4 c, float side) {
            ImU32 cols[8];
            for (int i = 0; i < n; ++i) {
                const float d = side * ((poly[i].x - hx) * ux + (poly[i].y - hy) * uy);
                // Darker with every pixel away from the horizon: 1.25× the
                // theme colour on the line, 0.50× a display-and-a-bit away.
                const float f = std::clamp(d / (span * 1.2f), 0.0f, 1.0f);
                cols[i] = shade(c, 1.25f - 0.75f * f);
            }
            hud_fill_gradient(dl, poly, cols, n);
        };

        ImVec2 sky[8], ground[8];
        const int ns = hud_clip_ground(sq, 4, { hx, hy }, -ux, -uy, sky);
        const int n  = hud_clip_ground(sq, 4, { hx, hy },  ux,  uy, ground);
        fill(sky,    ns, g_theme.ah_sky,     1.0f);
        fill(ground, n,  g_theme.ah_ground, -1.0f);

        // As thin as a 10° rung, and broken where it passes behind the
        // aircraft symbol: a gap out past its wing tips, cut wherever the line
        // crosses it, so the symbol reads clean at any pitch and roll.
        const float L   = std::max(sz.x, sz.y) * 1.5f;
        const float rg  = std::max(0.9f, r * 0.22f / 26.0f) * 30.0f;  // symbol units
        const float tc  = (cx - hx) * ldx + (cy - hy) * ldy;  // nearest point to centre
        const float d2  = pitch_px * pitch_px;                // its distance, squared
        const ImU32 hc  = ah_horizon_line();
        if (d2 < rg * rg) {
            const float half = sqrtf(rg * rg - d2);
            dl->AddLine({ hx - ldx * L, hy - ldy * L },
                        { hx + ldx * (tc - half), hy + ldy * (tc - half) },
                        hc, AH_THICK_PITCH_BOLD);
            dl->AddLine({ hx + ldx * (tc + half), hy + ldy * (tc + half) },
                        { hx + ldx * L, hy + ldy * L },
                        hc, AH_THICK_PITCH_BOLD);
        } else {
            dl->AddLine({ hx - ldx * L, hy - ldy * L }, { hx + ldx * L, hy + ldy * L },
                        hc, AH_THICK_PITCH_BOLD);
        }
    }

    // ── Pitch ladder ─────────────────────────────────────────────────────────
    dl->PushClipRect(c0, c1, true);
    // The ladder moves with the vehicle: rungs every 5° in a window either side
    // of the current pitch, rather than a fixed ±30°. The window stops short
    // of the roll arc, so a steep attitude brings up the rungs around it
    // (0 to -60 nose down at -30) instead of pushing one into the arc.
    {
    const float ppd    = r / 45.0f;
    const float reach  = std::min(r * 0.70f, (c1.y - c0.y) * 0.45f);
    const int   k_lo   = (int)ceilf ((vs.pitch - reach / ppd) / 5.0f);
    const int   k_hi   = (int)floorf((vs.pitch + reach / ppd) / 5.0f);
    for (int k = k_lo; k <= k_hi; ++k) {
        const int deg = k * 5;
        if (deg == 0 || deg < -90 || deg > 90) continue;   // 0 is the horizon line
        const float disp  = ((float)deg - vs.pitch) * ppd;
        const float mx    = cx + ux * disp;
        const float my    = cy + uy * disp;
        const bool  major = (deg % 10 == 0);
        const bool  climb = deg > 0;
        const float hl    = major ? r * 0.29f : r * 0.158f;
        const float gap   = r * 0.07f;      // the gap the aircraft symbol reads through
        const float thick = major ? AH_THICK_PITCH_BOLD : AH_THICK_PITCH_THIN;
        const ImU32 col   = ah_roll_tick();

        // Climb / dive notation, as a flight-deck ladder draws it: climb rungs
        // solid, dive rungs dashed, and each end turned toward the horizon,
        // so a single rung says which side of level the nose is on without
        // its number.
        for (int side = -1; side <= 1; side += 2) {
            const ImVec2 in  = { mx + ldx * gap * side, my + ldy * gap * side };
            const ImVec2 out = { mx + ldx * hl  * side, my + ldy * hl  * side };
            if (climb) {
                dl->AddLine(in, out, col, thick);
            } else {
                const float len  = hl - gap;
                const float dash = std::max(3.0f, r * 0.045f);
                for (float d = 0.0f; d < len; d += dash * 2.0f) {
                    const float e = std::min(d + dash, len);
                    dl->AddLine({ in.x + ldx * d * side, in.y + ldy * d * side },
                                { in.x + ldx * e * side, in.y + ldy * e * side },
                                col, thick);
                }
            }
            if (major) {
                // Toward the horizon: down the screen's "up" for a climb rung,
                // up it for a dive rung.
                const float tl = r * 0.05f * (climb ? -1.0f : 1.0f);
                dl->AddLine(out, { out.x + ux * tl, out.y + uy * tl }, col, thick);

                char buf[12];
                snprintf(buf, sizeof(buf), "%d", deg);
                const float tw  = t.width(buf);
                const float off = hl + r * 0.04f + tw * 0.5f;
                const float lx  = mx + ldx * off * side - tw * 0.5f;
                const float ly  = my + ldy * off * side;
                dl->AddRectFilled({ lx - 1, ly - fh * 0.5f }, { lx + tw + 1, ly + fh * 0.5f + 1 },
                                  ah_pitch_label_bg());
                dl->AddText(t.font, fh, { lx, ly - fh * 0.5f + 1 }, ah_roll_label(), buf);
            }
        }
    }
    }
    dl->PopClipRect();

    // ── Roll scale ───────────────────────────────────────────────────────────
    // A fixed arc over the top, ±60°, ticked at 10, 20, 30, 45 and 60 as a
    // flight-deck one is, with a pointer that turns with the vehicle. Past the
    // end of the scale the pointer stops at it and turns to the warning colour,
    // so a steep bank still reads as one rather than as a pointer gone missing.
    {
        const float base   = -(float)M_PI / 2.0f;
        const float deg2r  = (float)M_PI / 180.0f;

        dl->PathArcTo({ cx, cy }, arc_r, base - 60.0f * deg2r, base + 60.0f * deg2r, 48);
        dl->PathStroke(ah_roll_arc(), 0, AH_THICK_ROLL_ARC);

        static const int ticks[] = { -60, -45, -30, -20, -10, 0, 10, 20, 30, 45, 60 };
        for (int deg : ticks) {
            const bool  major = (deg % 30 == 0);
            const float a     = base + (float)deg * deg2r;
            const float len   = deg == 0 ? r * 0.13f : major ? r * 0.10f : r * 0.06f;
            dl->AddLine({ cx + cosf(a) * arc_r,         cy + sinf(a) * arc_r },
                        { cx + cosf(a) * (arc_r + len), cy + sinf(a) * (arc_r + len) },
                        ah_roll_tick(), AH_THICK_ROLL_TICK);

            if (major && deg != 0) {
                const float label_r = arc_r + r * 0.10f + fh * 0.6f + 1.0f;
                char buf[12];
                snprintf(buf, sizeof(buf), "%d", abs(deg));
                const float tw = t.width(buf);
                dl->AddText(t.font, fh,
                            { cx + cosf(a) * label_r - tw * 0.5f,
                              cy + sinf(a) * label_r - fh * 0.5f },
                            ah_roll_label(), buf);
            }
        }

        const bool  over = fabsf(vs.roll) > 60.0f;
        const float roll = std::clamp(vs.roll, -60.0f, 60.0f);
        const float a  = base + roll * deg2r;
        const float ca = cosf(a), sa = sinf(a);
        const float cp = cosf(a + (float)M_PI / 2.0f);
        const float sp = sinf(a + (float)M_PI / 2.0f);
        const float tip = arc_r - 1.0f, br = arc_r - r * 0.11f, hw = r * 0.05f;
        dl->AddTriangleFilled(
            { cx + ca * tip,          cy + sa * tip          },
            { cx + ca * br + cp * hw, cy + sa * br + sp * hw },
            { cx + ca * br - cp * hw, cy + sa * br - sp * hw },
            over ? ui_col(g_theme.col_warning) : ah_roll_pointer());
    }

    // ── Aircraft symbol ──────────────────────────────────────────────────────
    {
        // A single ring with a fin up from its top and a wing out from
        // either side, in the horizon line's colour, which it is read
        // against. The wings are the heavier strokes. Measured in units of
        // which 26 reach the wing tips, the span the old wing symbol had.
        const ImU32 col = ah_horizon_line();
        const float u   = std::max(0.9f, r * 0.22f / 26.0f);
        const float lw  = std::max(1.5f, u * 1.1f);
        const float rr  = u * 8.0f;
        dl->AddCircle({ cx, cy }, rr, col, 32, lw);
        dl->AddLine({ cx, cy - rr },  { cx, cy - rr - u * 7.0f },   col, lw);
        dl->AddLine({ cx - rr, cy },  { cx - u * 26.0f, cy },       col, lw * 2.0f);
        dl->AddLine({ cx + rr, cy },  { cx + u * 26.0f, cy },       col, lw * 2.0f);
    }

    // ── Airspeed and vertical speed dials, either side of the ladder ─────────
    // Sized to the room between a side tape and that side's rung labels, a
    // well below the middle so the wings and the level rungs stay clear,
    // and left out when that room is too small to read a needle in.
    {
        const float room = (cx - c0.x) - r * 0.33f - fh * 1.2f - 8.0f;
        const float dr   = std::min(r * 0.24f, room * 0.5f);
        if (dr >= 8.0f) {
            const float dy = cy + r * 0.50f;
            // NaN airspeed from PX4 without an airspeed sensor.
            const bool as_ok = vs.has_vfr && std::isfinite(vs.airspeed);
            const bool vs_ok = vs.has_vfr && std::isfinite(vs.climb);
            hud_dial(dl, t, { c0.x + 4.0f + dr, dy }, dr, "AS", false, 20.0f, 10.0f,
                     "%.1f", as_ok, as_ok ? vs.airspeed : 0.0f);
            // Set in from the altitude tape a little more than the AS dial is
            // from the speed tape: right against it, the dial's ticks run
            // into the tape's own and read as one scale.
            hud_dial(dl, t, { c1.x - 4.0f - dr * 1.4f, dy }, dr, "VS", true, 5.0f, 5.0f,
                     "%+.1f", vs_ok, vs_ok ? vs.climb : 0.0f);
        }
    }

    // ── Throttle, in the bottom-right corner of the middle ───────────────────
    if (vs.has_vfr) {
        char thr_buf[12];
        snprintf(thr_buf, sizeof(thr_buf), "THR %u%%", (unsigned)vs.throttle);
        hud_chip(dl, t, { c1.x - 4.0f - t.width(thr_buf), c1.y - fh - 5.0f },
                 thr_buf, ah_horizon_line());
    }

    // ── Tapes ────────────────────────────────────────────────────────────────
    static const VTape SPEED_TAPE = { 1.0f, 5, 10.0f, true,  "%.1f" };
    static const VTape ALT_TAPE   = { 2.0f, 5, 20.0f, false, "%.0f" };

    hud_vtape(dl, t, { p0.x, c0.y }, { c0.x, p1.y }, true, SPEED_TAPE,
              vs.has_vfr, vs.groundspeed);
    hud_vtape(dl, t, { c1.x, c0.y }, { p1.x, p1.y }, false, ALT_TAPE,
              vs.has_global_pos, vs.alt_rel);

    float hdg = fmodf(vs.yaw, 360.0f);
    if (hdg < 0.0f) hdg += 360.0f;
    hud_heading_tape(dl, t, p0, { p1.x, c0.y }, hdg);

    // Units, in the corners the tapes leave free beneath the heading tape.
    {
        const ImU32 uc = ah_roll_label();
        hud_chip(dl, t, { p0.x + 3.0f, c0.y + 2.0f }, "GS", uc);
        hud_chip(dl, t, { p1.x - 3.0f - t.width("ALT"), c0.y + 2.0f }, "ALT", uc);
    }

    dl->PopClipRect();
    dl->AddRect(p0, p1, ah_border(), 0.0f, 0, AH_THICK_BORDER);

    ImGui::Dummy(sz);
}

// The attitude panel's body, below its header, in whichever style is set.
// The tapes HUD fills `avail` and carries speed, heading and altitude itself;
// the classic ball sits square in the middle with the GND / HDG / CLB strip
// beneath it to carry them instead.
static void draw_vfr_strip(const VehicleState& vs, float strip_h, float value_size);

void draw_hud_body(const VehicleState& vs, ImVec2 avail, bool tapes,
                   float text_size, float strip_value_size)
{
    constexpr float STRIP_H = 40.0f;
    if (tapes) {
        if (vs.has_attitude && avail.x > 30.0f && avail.y > 30.0f)
            draw_artificial_horizon(vs, avail, text_size);
        else
            ImGui::Dummy(avail);
        return;
    }
    const float hs = std::min(avail.x, avail.y - STRIP_H);
    if (vs.has_attitude && hs > 30.0f) {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail.x - hs) * 0.5f);
        draw_classic_horizon(vs.roll, vs.pitch, vs.yaw, vs.airspeed,
                             vs.has_vfr ? (int)vs.throttle : -1,
                             vs.has_vfr, hs, text_size);
    } else {
        ImGui::Dummy({ avail.x, std::max(0.0f, avail.y - STRIP_H) });
    }
    draw_vfr_strip(vs, STRIP_H, strip_value_size);
}

// ── Shared HUD / log pieces ───────────────────────────────────────────────────
//
// The sidebar and the fullscreen-map overlay show the same two things in
// different sizes. These are the parts they share, so the pair cannot drift.

// Ground speed, heading and climb rate as three readouts on one row, drawn at
// the cursor and consuming `strip_h` of vertical space.
//
// `value_size` is the number's size, not the label's. The fullscreen map's
// overlay asks for a smaller one: at the sidebar's size three of these numbers
// nearly meet across a block a third the sidebar's width, and a readout that
// touches its neighbour reads as one long number.
static void draw_vfr_strip(const VehicleState& vs, float strip_h,
                           float value_size)
{
    ImDrawList*  dl = ImGui::GetWindowDrawList();
    const ImVec2 s0 = ImGui::GetCursorScreenPos();
    const float  w  = ImGui::GetContentRegionAvail().x;
    const float  cw = w / 3.0f;

    struct Item { const char* label; char value[16]; };
    Item items[3] = { { "GND m/s", "--" }, { "HDG", "--" }, { "CLB m/s", "--" } };
    if (vs.has_vfr) {
        snprintf(items[0].value, sizeof(items[0].value), "%.1f", (double)vs.groundspeed);
        snprintf(items[1].value, sizeof(items[1].value), "%03d\xc2\xb0", (int)vs.heading);
        snprintf(items[2].value, sizeof(items[2].value), "%+.1f", (double)vs.climb);
    }

    for (int i = 0; i < 3; ++i) {
        const ImVec2 p0 = { s0.x + i * cw, s0.y };
        const ImVec2 p1 = { p0.x + cw,     s0.y + strip_h };
        ui_readout(dl, p0, p1, items[i].label, items[i].value,
                   vs.has_vfr ? ui_col_value() : ui_col(g_theme.col_no_link_muted),
                   value_size);
        if (i > 0)
            dl->AddLine({ p0.x, p0.y + 4.0f }, { p0.x, p1.y - 4.0f },
                        ui_col(g_theme.separator, 0.6f), 1.0f);
    }
    ImGui::Dummy({ w, strip_h });
}

} // namespace right_sidebar
