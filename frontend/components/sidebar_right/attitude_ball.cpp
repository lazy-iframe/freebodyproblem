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

// Right sidebar: the classic attitude ball, and the attitude instrument the SWARM view borrows.

#define _USE_MATH_DEFINES
#include <cmath>
#include "frontend/elements/sidebar_right.hpp"
#include "frontend/components/sidebar_right/sidebar_internal.hpp"
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

// ── Classic attitude ball (local to this TU) ─────────────────────────────────
//
// The round instrument the HUD below replaced, kept as the alternative the
// SETTINGS tab offers: a ball reads at a glance where a full display is too
// much, which is why the swarm cards default to it.

// `text_size` overrides the size the labels are drawn at; 0 keeps the current
// font's own. The ball scales with `size` but its lettering does not, so at the
// diameter the fullscreen map's overlay asks for, sidebar-sized text fills the
// instrument instead of labelling it. Everything is drawn through the draw
// list anyway, so an arbitrary size costs nothing but the argument.
void draw_classic_horizon(float roll_deg, float pitch_deg, float yaw_deg,
                           float airspeed_ms, int throttle_pct, bool has_vfr,
                           float size, float text_size)
{
    ImDrawList* dl   = ImGui::GetWindowDrawList();
    const ImVec2 p0  = ImGui::GetCursorScreenPos();
    const float  cx  = p0.x + size * 0.5f;
    const float  cy  = p0.y + size * 0.5f;
    const float  r   = size * 0.45f;

    // The mono micro font when a size is asked for: digits of one width are
    // what the label-width estimates below assume, and it stays crisp small.
    ImFont* const fnt = (text_size > 0.0f && g_font_micro) ? g_font_micro
                                                           : ImGui::GetFont();
    const float  ts  = (text_size > 0.0f) ? text_size : ImGui::GetFontSize();
    const float  fh  = ts;

    // Per-character advance. The constants below were measured against the
    // default font at its own size, so they scale with the ratio — at
    // text_size 0 this is exactly 1 and nothing about the old sizing moves.
    const float  k   = ts / ImGui::GetFontSize();

    const float roll_rad = roll_deg  * (float)M_PI / 180.0f;
    const float pitch_px = pitch_deg * (r / 45.0f);

    const float ux  = -sinf(roll_rad);
    const float uy  = -cosf(roll_rad);
    const float ldx = -uy, ldy = ux;

    const float hx = cx - ux * pitch_px;
    const float hy = cy - uy * pitch_px;

    dl->PushClipRect({ cx - r, cy - r }, { cx + r, cy + r }, true);

    dl->AddCircleFilled({ cx, cy }, r, ah_sky());

    {
        const float dx_c = hx - cx, dy_c = hy - cy;
        const float b    = dx_c * ldx + dy_c * ldy;
        const float disc = b * b - (dx_c * dx_c + dy_c * dy_c - r * r);

        if (disc > 0.0f) {
            const float sq  = sqrtf(disc);
            const float px1 = hx + (-b - sq) * ldx, py1 = hy + (-b - sq) * ldy;
            const float px2 = hx + (-b + sq) * ldx, py2 = hy + (-b + sq) * ldy;

            const float a1 = atan2f(py1 - cy, px1 - cx);
            const float a2 = atan2f(py2 - cy, px2 - cx);
            const float a2_ccw = (a2 < a1) ? (a2 + 2.0f * (float)M_PI) : a2;

            const float amid  = (a1 + a2_ccw) * 0.5f;
            const float mid_wx = cx + cosf(amid) * r;
            const float mid_wy = cy + sinf(amid) * r;
            const bool ccw_is_ground =
                (mid_wx - hx) * (-ux) + (mid_wy - hy) * (-uy) > 0.0f;

            if (ccw_is_ground) {
                dl->PathLineTo({ px2, py2 });
                dl->PathLineTo({ px1, py1 });
                dl->PathArcTo({ cx, cy }, r, a1, a2_ccw, 40);
            } else {
                dl->PathLineTo({ px1, py1 });
                dl->PathLineTo({ px2, py2 });
                dl->PathArcTo({ cx, cy }, r, a2_ccw, a1 + 2.0f * (float)M_PI, 40);
            }
            dl->PathFillConvex(ah_ground());
            dl->AddLine({ px1, py1 }, { px2, py2 }, ah_horizon_line(), AH_THICK_HORIZON);

        } else {
            const float center_dot = (cx - hx) * (-ux) + (cy - hy) * (-uy);
            if (center_dot > 0.0f)
                dl->AddCircleFilled({ cx, cy }, r, ah_ground());
        }
    }

    static const int marks[] = { -30, -20, -10, -5, 5, 10, 20, 30 };
    for (int deg : marks) {
        const float disp = ((float)deg - pitch_deg) * (r / 45.0f);
        if (fabsf(disp) > r * 0.92f) continue;
        const float mx   = cx + ux * disp;
        const float my   = cy + uy * disp;
        const float hl   = (deg == 5 || deg == -5) ? r * 0.12f : r * 0.22f;
        const float thick = (deg % 10 == 0) ? AH_THICK_PITCH_BOLD : AH_THICK_PITCH_THIN;
        dl->AddLine({ mx + ldx * hl, my + ldy * hl },
                    { mx - ldx * hl, my - ldy * hl },
                    ah_pitch_ladder(), thick);

        if (deg != 5 && deg != -5) {
            char buf[8];
            snprintf(buf, sizeof(buf), "%d", deg);
            const float tw  = (float)strlen(buf) * 6.0f * k;
            const float lx  = mx + ldx * (hl + r * 0.04f);
            const float ly  = my + ldy * (hl + r * 0.04f);
            dl->AddRectFilled({ lx - 1, ly - 1 }, { lx + tw + 1, ly + fh + 1 },
                              ah_pitch_label_bg());
            dl->AddText(fnt, ts, { lx, ly - fh * 0.5f + 1 }, ah_pitch_label(), buf);
        }
    }

    const float ch  = r * 0.16f, gap = r * 0.06f;
    dl->AddLine({ cx - ch - gap, cy }, { cx - gap, cy }, ah_crosshair(), AH_THICK_CROSSHAIR);
    dl->AddLine({ cx + gap, cy }, { cx + ch + gap, cy }, ah_crosshair(), AH_THICK_CROSSHAIR);
    dl->AddCircleFilled({ cx, cy }, 3.0f, ah_crosshair());

    if (has_vfr) {
        char as_buf[16];
        // NaN from PX4 without an airspeed sensor.
        if (std::isfinite(airspeed_ms))
            snprintf(as_buf, sizeof(as_buf), "%.1f m/s", (double)airspeed_ms);
        else
            snprintf(as_buf, sizeof(as_buf), "-- m/s");
        const float tw = (float)strlen(as_buf) * 6.0f * k + 6.0f * k;
        const float bx = cx - r * 0.60f - tw * 0.5f;
        const float by = cy - r * 0.62f;
        dl->AddRectFilled({ bx - 2, by - 2 }, { bx + tw, by + fh + 2 },
                          ah_overlay_bg(), 2.0f);
        dl->AddText(fnt, ts, { bx, by }, ah_airspeed_text(), as_buf);

        char thr_buf[8];
        snprintf(thr_buf, sizeof(thr_buf), "%d%%", throttle_pct);
        const float tw2 = (float)strlen(thr_buf) * 6.0f * k + 6.0f * k;
        const float bx2 = cx + r * 0.35f;
        const float by2 = cy - r * 0.62f;
        dl->AddRectFilled({ bx2 - 2, by2 - 2 }, { bx2 + tw2, by2 + fh + 2 },
                          ah_overlay_bg(), 2.0f);
        dl->AddText(fnt, ts, { bx2, by2 }, ah_throttle_text(), thr_buf);
    }

    {
        char hdg_buf[12];
        snprintf(hdg_buf, sizeof(hdg_buf), "%.0f\xc2\xb0", (double)yaw_deg);
        const float tw  = (float)strlen(hdg_buf) * 6.0f * k + 6.0f * k;
        const float bx  = cx - tw * 0.5f;
        const float by  = cy + r * 0.65f;
        dl->AddRectFilled({ bx - 2, by - 2 }, { bx + tw, by + fh + 2 },
                          ah_overlay_bg(), 2.0f);
        dl->AddText(fnt, ts, { bx, by }, ah_heading_text(), hdg_buf);
    }

    dl->PopClipRect();

    {
        const float arc_r = r * 1.08f;
        const float base  = -(float)M_PI / 2.0f;

        // A full ring rather than the ±60° span this used to draw. Past 60° of
        // roll the pointer used to run off the end of its own scale and sit
        // against nothing, which is exactly the attitude where the number is
        // worth reading. A closed ring has graduation under the pointer at any
        // angle the vehicle can reach.
        dl->AddCircle({ cx, cy }, arc_r, ah_roll_arc(), 64, AH_THICK_ROLL_ARC);

        // Graduated every 15° the whole way round, so a tick is never more than
        // 7.5° from the pointer. Lengths carry the reading: the cardinals
        // (wings level, and inverted) longest, every 30° next, the 15°
        // subdivisions shortest.
        for (int deg = -180; deg < 180; deg += 15) {
            const bool cardinal = (deg == 0 || deg == 180 || deg == -180);
            const bool major    = (deg % 30 == 0);

            const float a   = base + (float)deg * (float)M_PI / 180.0f;
            const float len = cardinal ? r * 0.13f
                            : major    ? r * 0.09f
                                       : r * 0.05f;
            dl->AddLine({ cx + cosf(a) * arc_r,           cy + sinf(a) * arc_r },
                        { cx + cosf(a) * (arc_r - len),   cy + sinf(a) * (arc_r - len) },
                        ah_roll_tick(), AH_THICK_ROLL_TICK);

            // Every 30° gets its number, bar wings-level and inverted: those
            // two are what the long ticks say on their own.
            //
            // The numbers ride closer to the ring than they used to. ±90 is
            // where the scale is widest — the label goes straight out sideways
            // with none of the angle to shorten its reach — and at the old
            // radius it landed past the edge of the panel the instrument sits
            // in. Pulling the label ring in clears it at both HUD sizes and
            // leaves the instrument itself the size it was.
            if (deg != 0 && major && deg > -180 && deg < 180) {
                const float label_r = arc_r + r * 0.07f;
                char buf[8];
                snprintf(buf, sizeof(buf), "%d", (int)fabsf((float)deg));
                const float tw = (float)strlen(buf) * 5.5f * k;
                const float tx = cx + cosf(a) * label_r - tw * 0.5f;
                const float ty = cy + sinf(a) * label_r - fh * 0.5f;
                dl->AddText(fnt, ts, { tx, ty }, ah_roll_label(), buf);
            }
        }

        const float a   = base + roll_deg * (float)M_PI / 180.0f;
        const float ca  = cosf(a), sa = sinf(a);
        const float cp  = cosf(a + (float)M_PI / 2.0f);
        const float sp  = sinf(a + (float)M_PI / 2.0f);
        const float tip = arc_r + r * 0.04f, br = arc_r - r * 0.09f, hw = r * 0.04f;
        dl->AddTriangleFilled(
            { cx + ca * tip,          cy + sa * tip          },
            { cx + ca * br + cp * hw, cy + sa * br + sp * hw },
            { cx + ca * br - cp * hw, cy + sa * br - sp * hw },
            ah_roll_pointer());
    }

    dl->AddCircle({ cx, cy }, r, ah_border(), 64, AH_THICK_BORDER);

    ImGui::Dummy({ size, size });
}

} // namespace right_sidebar

using namespace right_sidebar;

// ── Pieces the SWARM view borrows ─────────────────────────────────────────────
//
// The swarm screen shows the same instruments and the same log as the sidebar,
// in its own arrangement. Wrappers rather than copies, so the two cannot drift.

void draw_attitude_ball(const VehicleState& vs, float size, float text_size,
                        bool tapes)
{
    if (vs.has_attitude) {
        if (tapes)
            draw_artificial_horizon(vs, { size, size }, text_size);
        else
            draw_classic_horizon(vs.roll, vs.pitch, vs.yaw, vs.airspeed,
                                 vs.has_vfr ? (int)vs.throttle : -1,
                                 vs.has_vfr, size, text_size);
    } else {
        // An outline where the HUD would be, so a vehicle not yet sending
        // attitude leaves its card the same shape as the rest.
        ImDrawList*  dl = ImGui::GetWindowDrawList();
        const ImVec2 p0 = ImGui::GetCursorScreenPos();
        const ImVec2 c  = { p0.x + size * 0.5f, p0.y + size * 0.5f };
        if (tapes)
            dl->AddRect(p0, { p0.x + size, p0.y + size }, ui_col(g_theme.separator), 0.0f, 0, 1.0f);
        else
            dl->AddCircle(c, size * 0.45f, ui_col(g_theme.separator), 0, 1.0f);
        ImFont*     fm = g_font_micro ? g_font_micro : ImGui::GetFont();
        const char* t  = "NO ATT";
        const float tw = ui_tracked_width(fm, UI_SZ_MICRO, t);
        ui_tracked_text(dl, fm, UI_SZ_MICRO, { c.x - tw * 0.5f, c.y - UI_SZ_MICRO * 0.5f },
                        ui_col(g_theme.col_no_link_muted), t);
    }
    ImGui::Dummy({ size, size });
}
