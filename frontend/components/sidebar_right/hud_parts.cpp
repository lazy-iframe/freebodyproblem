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

// Right sidebar: the HUD's drawing parts.

#define _USE_MATH_DEFINES
#include <cmath>
#include "frontend/elements/sidebar_right.hpp"
#include "frontend/components/sidebar_right/sidebar_internal.hpp"
#include "frontend/components/sidebar_right/hud_parts.hpp"
#include "frontend/elements/center_view.hpp"
#include "frontend/source/vehicle_ui_state.hpp"
#include "frontend/source/layout.hpp"
#include "frontend/source/theme.hpp"
#include "frontend/components/others/ui_kit.hpp"
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

// ── Head-up display (local to this TU) ───────────────────────────────────────
//
// Laid out the way a flight-deck PFD is. The horizon fills the whole square,
// with three tapes over it: heading across the top, ground speed down the
// left and altitude down the right. The roll scale, pitch ladder and aircraft
// symbol sit in the middle, between the tapes.
//
// `text_size` overrides the size the labels are drawn at; 0 keeps the current
// font's own. The instrument scales with `size` but its lettering does not, so
// at the size the fullscreen map's overlay asks for, sidebar-sized text fills
// the instrument instead of labelling it. Everything is drawn through the draw
// list anyway, so an arbitrary size costs nothing but the argument.

// Fills a convex polygon with a colour per vertex, interpolated across it —
// AddConvexPolyFilled's one colour, made a gradient. No anti-aliased fringe:
// the edges here are the clip rect's and the horizon's, which has its line.
void hud_fill_gradient(ImDrawList* dl, const ImVec2* pts, const ImU32* cols, int n)
{
    if (n < 3) return;
    const ImVec2 uv = ImGui::GetFontTexUvWhitePixel();
    dl->PrimReserve((n - 2) * 3, n);
    const ImDrawIdx base = (ImDrawIdx)dl->_VtxCurrentIdx;
    for (int i = 0; i < n; ++i) dl->PrimWriteVtx(pts[i], uv, cols[i]);
    for (int i = 2; i < n; ++i) {
        dl->PrimWriteIdx(base);
        dl->PrimWriteIdx((ImDrawIdx)(base + i - 1));
        dl->PrimWriteIdx((ImDrawIdx)(base + i));
    }
}

// Keeps the part of a convex polygon on the ground side of the horizon: the
// side opposite `up` from the point `h`. Order (and so winding) is preserved.
int hud_clip_ground(const ImVec2* in, int n, ImVec2 h, float ux, float uy,
                    ImVec2* out)
{
    int m = 0;
    for (int i = 0; i < n; ++i) {
        const ImVec2 a  = in[i], b = in[(i + 1) % n];
        const float  da = -((a.x - h.x) * ux + (a.y - h.y) * uy);
        const float  db = -((b.x - h.x) * ux + (b.y - h.y) * uy);
        if (da >= 0.0f) out[m++] = a;
        if ((da >= 0.0f) != (db >= 0.0f)) {
            const float t = da / (da - db);
            out[m++] = { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t };
        }
    }
    return m;
}

// The value box a tape reads off, with an arrow pointing at the scale.
// `dir` is where the arrow points: -1/+1 left/right for a vertical tape,
// +2 down for the heading tape, 0 for none (a standalone readout).
//
// White digits on the HUD's dark wash, no frame; the arrow is the roll
// pointer's yellow, so every "you are here" marker reads as one family.
// `valid` false greys the text for a reading not yet received.
void hud_value_box(ImDrawList* dl, const HudText& t, ImVec2 a, ImVec2 b, int dir,
                   const char* text, bool valid)
{
    const ImU32 marker   = ah_roll_pointer();
    const ImU32 text_col = valid ? ah_horizon_line() : ui_col(g_theme.col_no_link_muted);
    const float mx = (a.x + b.x) * 0.5f, my = (a.y + b.y) * 0.5f;
    const float n  = hud_arrow(t);

    dl->AddRectFilled(a, b, hud_box_bg());
    if (dir == -1)
        dl->AddTriangleFilled({ a.x - n, my }, { a.x, my - n }, { a.x, my + n }, marker);
    else if (dir == 1)
        dl->AddTriangleFilled({ b.x + n, my }, { b.x, my + n }, { b.x, my - n }, marker);
    else if (dir == 2)
        dl->AddTriangleFilled({ mx, b.y + n }, { mx - n, b.y }, { mx + n, b.y }, marker);

    const float tw = t.width(text);
    dl->AddText(t.font, t.size, { mx - tw * 0.5f, my - t.size * 0.5f }, text_col, text);
}

// A text chip on the same dark wash as the value boxes, for labels and the
// scale numbers.
void hud_chip(ImDrawList* dl, const HudText& t, ImVec2 pos, const char* text,
              ImU32 col)
{
    const float tw = t.width(text);
    dl->AddRectFilled({ pos.x - 2.0f, pos.y - 1.0f },
                      { pos.x + tw + 2.0f, pos.y + t.size + 1.0f }, hud_box_bg());
    dl->AddText(t.font, t.size, pos, col, text);
}

void hud_vtape(ImDrawList* dl, const HudText& t, ImVec2 a, ImVec2 b,
               bool ticks_right, const VTape& spec, bool valid, float value)
{
    const float w   = b.x - a.x;
    const float h   = b.y - a.y;
    const float mid = (a.y + b.y) * 0.5f;
    const float ppu = (h * 0.5f) / spec.span;          // pixels per unit

    // No fill and no backing for the numbers: the horizon runs on under the
    // tape. The value box stops short of the graduations, its arrow tip on
    // the major ticks' inner ends.
    const float edge  = ticks_right ? b.x : a.x;
    const float sgn   = ticks_right ? -1.0f : 1.0f;
    const float t_maj = std::max(4.0f, w * 0.22f);
    const float t_min = t_maj * 0.55f;
    const float bh    = t.size + 6.0f;

    if (valid) {
        dl->PushClipRect(a, b, true);

        // Thin the scale out where it is too small to read: ticks under 4 px
        // apart blur into a bar, and labels closer than a line run together.
        const bool  draw_minor = spec.minor * ppu >= 4.0f;
        int label_every = spec.major_every;
        while (label_every * spec.minor * ppu < t.size * 1.3f) label_every *= 2;

        const int k0 = (int)floorf((value - spec.span) / spec.minor) - 1;
        const int k1 = (int)ceilf ((value + spec.span) / spec.minor) + 1;
        for (int k = k0; k <= k1; ++k) {
            const float v = (float)k * spec.minor;
            if (spec.non_negative && v < 0.0f) continue;
            const bool major = ((k % spec.major_every) + spec.major_every)
                               % spec.major_every == 0;
            if (!major && !draw_minor) continue;

            const float y   = mid - (v - value) * ppu;
            const float len = major ? t_maj : t_min;
            dl->AddLine({ edge, y }, { edge + sgn * len, y }, ah_roll_tick(),
                        major ? AH_THICK_PITCH_BOLD * 0.75f : AH_THICK_PITCH_THIN * 0.75f);

            // Numbers that would show through the see-through value box
            // are left out while they pass behind it.
            if (((k % label_every) + label_every) % label_every == 0 &&
                fabsf(y - mid) > (bh + t.size) * 0.5f) {
                char buf[12];
                snprintf(buf, sizeof(buf), "%.0f", (double)v);
                const float tw = t.width(buf);
                const float tx = ticks_right ? edge - t_maj - 3.0f - tw
                                             : edge + t_maj + 3.0f;
                dl->AddText(t.font, t.size, { tx, y - t.size * 0.5f },
                            ah_horizon_line(), buf);
            }
        }
        dl->PopClipRect();
    }

    char vbuf[16];
    if (valid) snprintf(vbuf, sizeof(vbuf), spec.box_fmt, (double)value);
    else       snprintf(vbuf, sizeof(vbuf), "--");
    const float inner = edge + sgn * (t_maj + hud_arrow(t));   // box's tick-side edge
    const float outer = ticks_right ? a.x + 1.0f : b.x - 1.0f;
    hud_value_box(dl, t, { std::min(inner, outer), mid - bh * 0.5f },
                  { std::max(inner, outer), mid + bh * 0.5f },
                  ticks_right ? 1 : -1, vbuf, valid);

}

// The heading tape: a compass card read off a fixed box at its middle.
// Graduated every 5°, labelled every 30° the way a compass card is: the
// cardinals by letter, the rest in tens of degrees.
void hud_heading_tape(ImDrawList* dl, const HudText& t, ImVec2 a, ImVec2 b,
                      float hdg)
{
    const float w   = b.x - a.x;
    const float mid = (a.x + b.x) * 0.5f;
    const float ppd = w / 90.0f;                         // ±45° across the tape

    dl->PushClipRect(a, b, true);

    // The ticks rise from the foot; the value box sits above them, its arrow
    // tip on the major ticks' tops, and the labels share its row.
    const bool  draw_minor = 5.0f * ppd >= 4.0f;
    const float box_h = t.size + 4.0f;
    const float t_maj = std::max(4.0f, (b.y - a.y) - box_h - hud_arrow(t) - 1.0f);
    const float t_min = t_maj * 0.55f;
    const float bw    = t.width("000") + 8.0f;

    const int k0 = (int)floorf((hdg - 50.0f) / 5.0f);
    const int k1 = (int)ceilf ((hdg + 50.0f) / 5.0f);
    for (int k = k0; k <= k1; ++k) {
        const float d   = (float)k * 5.0f;
        const int   deg = (((int)d % 360) + 360) % 360;
        const bool  major = deg % 10 == 0;
        if (!major && !draw_minor) continue;

        const float x   = mid + (d - hdg) * ppd;
        const float len = major ? t_maj : t_min;
        dl->AddLine({ x, b.y }, { x, b.y - len }, ah_roll_tick(),
                    major ? AH_THICK_PITCH_BOLD * 0.75f : AH_THICK_PITCH_THIN * 0.75f);

        if (deg % 30 == 0 && fabsf(x - mid) > bw * 0.5f + t.size) {
            char buf[4];
            switch (deg) {
            case 0:   snprintf(buf, sizeof(buf), "N"); break;
            case 90:  snprintf(buf, sizeof(buf), "E"); break;
            case 180: snprintf(buf, sizeof(buf), "S"); break;
            case 270: snprintf(buf, sizeof(buf), "W"); break;
            default:  snprintf(buf, sizeof(buf), "%d", deg / 10); break;
            }
            const ImU32 col = (deg % 90 == 0) ? ah_heading_text() : ah_horizon_line();
            const float tw  = t.width(buf);
            dl->AddText(t.font, t.size, { x - tw * 0.5f, b.y - t_maj - t.size - 1.0f },
                        col, buf);
        }
    }
    dl->PopClipRect();

    char vbuf[8];
    snprintf(vbuf, sizeof(vbuf), "%03d", ((int)lroundf(hdg)) % 360);
    hud_value_box(dl, t, { mid - bw * 0.5f, b.y - t_maj - hud_arrow(t) - box_h },
                  { mid + bw * 0.5f, b.y - t_maj - hud_arrow(t) }, 2, vbuf, true);

}

// A round gauge over the horizon: graduations and a needle only, no face, so
// the attitude behind it stays readable. 270° of scale, open at the bottom.
//
// `bipolar` false: 0 at the lower left, clockwise to full scale at the lower
// right (airspeed). true: 0 at the top, -full at the lower left and +full at
// the lower right, so the needle leans the way the vehicle is going (vertical
// speed). Full scale starts at `base_full` and steps up by `step` as the
// reading nears it, so a fast vehicle never pins the needle. The label sits
// above, the value in a chip below; `valid` false leaves the needle out and
// the chip reads "--".
void hud_dial(ImDrawList* dl, const HudText& t, ImVec2 c, float rad,
              const char* label, bool bipolar, float base_full, float step,
              const char* fmt, bool valid, float value)
{
    const float mag  = valid ? fabsf(value) : 0.0f;
    const float full = std::max(base_full, ceilf((mag + step * 0.5f) / step) * step);

    const float a0    = 0.75f * (float)M_PI;          // lower left
    const float sweep = 1.5f  * (float)M_PI;          // 270° clockwise

    // A tick every tenth of the sweep, longer at both ends and the middle.
    for (int i = 0; i <= 10; ++i) {
        const float a   = a0 + sweep * (float)i / 10.0f;
        const float len = (i % 5 == 0) ? rad * 0.30f : rad * 0.16f;
        dl->AddLine({ c.x + cosf(a) * rad,         c.y + sinf(a) * rad },
                    { c.x + cosf(a) * (rad - len), c.y + sinf(a) * (rad - len) },
                    ah_roll_tick(), (i % 5 == 0) ? 1.8f : 1.2f);
    }

    if (valid) {
        const float f = bipolar ? std::clamp(0.5f + 0.5f * value / full, 0.0f, 1.0f)
                                : std::clamp(value / full, 0.0f, 1.0f);
        const float a = a0 + sweep * f;
        dl->AddLine(c, { c.x + cosf(a) * rad * 0.85f, c.y + sinf(a) * rad * 0.85f },
                    ah_roll_pointer(), 2.0f);
        dl->AddCircleFilled(c, std::max(2.0f, rad * 0.08f), ah_roll_pointer());
    }

    hud_chip(dl, t, { c.x - t.width(label) * 0.5f, c.y - rad - t.size - 3.0f },
             label, ah_roll_label());
    char buf[16];
    if (valid) snprintf(buf, sizeof(buf), fmt, (double)value);
    else       snprintf(buf, sizeof(buf), "--");
    hud_chip(dl, t, { c.x - t.width(buf) * 0.5f, c.y + rad + 3.0f }, buf,
             valid ? ah_horizon_line() : ui_col(g_theme.col_no_link_muted));
}

} // namespace right_sidebar
