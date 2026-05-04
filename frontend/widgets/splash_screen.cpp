// Free Body Problem: A modern, simple and minimalist Ground Control Station
// Copyright (C) 2026  Azhar Tanweer

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.


#include "splash_screen.hpp"
#include "theme.hpp"

#define _USE_MATH_DEFINES
#include <cmath>

static constexpr float SPLASH_DURATION = 5.0f;
static constexpr float FADE_IN_SECS    = 0.35f;
static constexpr float FADE_OUT_START  = 4.2f;
static constexpr float SPLASH_W        = 520.0f;
static constexpr float SPLASH_H        = 262.0f;
static constexpr float S               = 0.62f;   // diagram scale

// Semantic physics colors — fixed regardless of theme (these represent forces)
static constexpr ImU32 C_LIFT   = IM_COL32( 80, 200,  90, 255);
static constexpr ImU32 C_WEIGHT = IM_COL32(230, 170,  40, 255);
static constexpr ImU32 C_THRUST = IM_COL32( 70, 150, 225, 255);
static constexpr ImU32 C_DRAG   = IM_COL32(210,  65,  65, 255);
static constexpr ImU32 C_VEL    = IM_COL32(110, 205, 205, 255);
static constexpr ImU32 C_MOM    = IM_COL32(170, 170, 170, 255);
static constexpr ImU32 C_AXES   = IM_COL32(200, 200, 200, 160);
static constexpr ImU32 C_BODY   = IM_COL32(200, 200, 200, 255);
static constexpr ImU32 C_BODY_F = IM_COL32( 22,  16,   6, 255);

// Scaled diagram dimensions
static constexpr float BW = 140.f * S;  // fuselage body width
static constexpr float BH =  46.f * S;  // fuselage body height
static constexpr float AL =  78.f * S;  // force arrow length

static ImU32 ca(ImU32 col, float alpha)
{
    float a = ((col >> 24) & 0xFF) / 255.f * alpha;
    int ai = (int)(a < 0.f ? 0.f : a > 1.f ? 255.f : a * 255.f);
    return (col & 0x00FFFFFF) | ((ImU32)ai << 24);
}

// ImVec4 theme color → ImU32 with alpha multiplier
static ImU32 tv(ImVec4 v, float alpha = 1.f)
{
    float a = v.w * alpha;
    if (a < 0.f) a = 0.f; else if (a > 1.f) a = 1.f;
    return IM_COL32((int)(v.x*255.f+.5f), (int)(v.y*255.f+.5f),
                    (int)(v.z*255.f+.5f), (int)(a*255.f+.5f));
}

static void arrow(ImDrawList* dl, ImVec2 from, ImVec2 to, ImU32 col,
                  float thick = 1.8f, float hlen = 9.f*S, float hwid = 5.5f*S)
{
    float dx = to.x - from.x, dy = to.y - from.y;
    float len = sqrtf(dx*dx + dy*dy);
    if (len < 1.f) return;
    float ux = dx/len, uy = dy/len, px = -uy, py = ux;
    ImVec2 base = { to.x - ux*hlen, to.y - uy*hlen };
    dl->AddLine(from, base, col, thick);
    dl->AddTriangleFilled(to,
        { base.x + px*hwid*.5f, base.y + py*hwid*.5f },
        { base.x - px*hwid*.5f, base.y - py*hwid*.5f }, col);
}

// IC chip — colors pulled from g_theme at draw time
static void draw_chip(ImDrawList* dl, float cx, float cy, float alpha)
{
    constexpr float CW    = 46.f*S, CH = 38.f*S;
    constexpr float PIN_W = 8.f*S,  PIN_H = 3.5f*S;
    constexpr int   N_PINS = 3;
    const float     PITCH  = CH / (N_PINS + 1);

    dl->AddRectFilled({cx-CW*.5f, cy-CH*.5f}, {cx+CW*.5f, cy+CH*.5f},
                      tv(g_theme.bg_child_dark, alpha), 2.f);
    dl->AddRect      ({cx-CW*.5f, cy-CH*.5f}, {cx+CW*.5f, cy+CH*.5f},
                      tv(g_theme.col_ok, alpha), 2.f, 0, 1.2f);

    for (int i = 0; i < N_PINS; ++i) {
        float py = cy - CH*.5f + PITCH*(i+1);
        dl->AddRectFilled({cx-CW*.5f-PIN_W, py-PIN_H*.5f}, {cx-CW*.5f,        py+PIN_H*.5f},
                          tv(g_theme.col_data, alpha));
        dl->AddRectFilled({cx+CW*.5f,        py-PIN_H*.5f}, {cx+CW*.5f+PIN_W, py+PIN_H*.5f},
                          tv(g_theme.col_data, alpha));
    }

    // Pin-1 notch
    dl->AddCircleFilled({cx-CW*.5f+5.f*S, cy-CH*.5f+4.5f*S}, 1.8f*S,
                        tv(g_theme.col_ok, alpha*.45f));

    // Internal cross traces
    const float ix = CW*.3f, iy = CH*.3f;
    dl->AddLine({cx-ix, cy}, {cx+ix, cy}, tv(g_theme.col_ok, alpha*.50f), .8f);
    dl->AddLine({cx, cy-iy}, {cx, cy+iy}, tv(g_theme.col_ok, alpha*.50f), .8f);

    // Corner via pads
    for (int sx : {-1, 1})
        for (int sy : {-1, 1})
            dl->AddCircleFilled({cx+sx*CW*.22f, cy+sy*CH*.22f}, 1.5f*S,
                                tv(g_theme.col_ok, alpha*.45f));

    const char* lbl = "AP";
    ImVec2 lsz = ImGui::CalcTextSize(lbl);
    dl->AddText(nullptr, 0.f, {cx-lsz.x*.5f, cy-lsz.y*.5f},
                tv(g_theme.col_ok, alpha*.80f), lbl);
}

// Fuselage: rounded body, nose triangle, tail fin, CG marker
static void draw_fuselage(ImDrawList* dl, float cx, float cy, float alpha)
{
    const ImU32 body_col  = ca(C_BODY,   alpha);
    const ImU32 body_fill = ca(C_BODY_F, alpha);

    dl->AddRectFilled({cx-BW*.5f, cy-BH*.5f}, {cx+BW*.5f, cy+BH*.5f}, body_fill, 5.f);
    dl->AddRect      ({cx-BW*.5f, cy-BH*.5f}, {cx+BW*.5f, cy+BH*.5f}, body_col, 5.f, 0, 1.5f);

    const float nr = BH * .38f;
    dl->AddTriangleFilled(
        {cx+BW*.5f+nr*1.6f, cy}, {cx+BW*.5f, cy-nr}, {cx+BW*.5f, cy+nr}, body_fill);
    dl->AddTriangle(
        {cx+BW*.5f+nr*1.6f, cy}, {cx+BW*.5f, cy-nr}, {cx+BW*.5f, cy+nr}, body_col, 1.4f);

    const float fw = BH * .18f, fh = BH * .65f;
    dl->AddRectFilled({cx-BW*.5f-fw, cy-fh}, {cx-BW*.5f, cy}, body_fill);
    dl->AddRect      ({cx-BW*.5f-fw, cy-fh}, {cx-BW*.5f, cy}, body_col, 0.f, 0, 1.3f);

    // CG marker
    dl->AddCircle({cx, cy}, 5.f*S, body_col, 20, 1.3f);
    dl->AddLine({cx-5.f*S, cy}, {cx+5.f*S, cy}, ca(C_BODY, alpha*.8f), 1.f);
    dl->AddLine({cx, cy-5.f*S}, {cx, cy+5.f*S}, ca(C_BODY, alpha*.8f), 1.f);
}

// ── Main draw function ────────────────────────────────────────────────────────

bool draw_splash_screen(float elapsed, ImFont* title_font)
{
    if (elapsed >= SPLASH_DURATION) return true;

    float alpha = 1.0f;
    if (elapsed < FADE_IN_SECS)
        alpha = elapsed / FADE_IN_SECS;
    else if (elapsed > FADE_OUT_START)
        alpha = 1.f - (elapsed - FADE_OUT_START) / (SPLASH_DURATION - FADE_OUT_START);
    if (alpha < 0.f) alpha = 0.f;
    if (alpha > 1.f) alpha = 1.f;

    ImGuiIO& io = ImGui::GetIO();
    const float W = io.DisplaySize.x, H = io.DisplaySize.y;
    const float wpx = (W - SPLASH_W) * .5f;
    const float wpy = (H - SPLASH_H) * .5f;
    const float cx  = wpx + SPLASH_W * .5f;
    const float cy  = wpy + SPLASH_H * .60f;

    // Draw everything onto the foreground list — always on top, no z-order fights
    ImDrawList* dl = ImGui::GetForegroundDrawList();

    // Dim overlay
    dl->AddRectFilled({0.f, 0.f}, {W, H},
                      IM_COL32(0, 0, 0, (int)(160.f * alpha)));

    // Panel background + border
    dl->AddRectFilled({wpx, wpy}, {wpx+SPLASH_W, wpy+SPLASH_H},
                      tv(g_theme.bg_panel));
    dl->AddRect({wpx, wpy}, {wpx+SPLASH_W, wpy+SPLASH_H},
                tv(g_theme.panel_border, alpha), 0.f, 0, 1.f);

    // ── Title + subtitle ──────────────────────────────────────────────────────
    const char* TITLE = "FREE BODY PROBLEM";
    const char* SUBT  = "Ground Control Station";

    float title_sz = title_font ? title_font->LegacySize : 15.f;
    ImVec2 tsz = title_font
        ? title_font->CalcTextSizeA(title_sz, FLT_MAX, 0.f, TITLE)
        : ImGui::CalcTextSize(TITLE);
    ImVec2 ssz = ImGui::CalcTextSize(SUBT);

    const float title_y = wpy + 16.f;
    dl->AddText(title_font, title_sz,
                {cx - tsz.x*.5f, title_y},
                tv(g_theme.accent, alpha), TITLE);

    const float sub_y = title_y + title_sz + 5.f;
    dl->AddText(nullptr, 0.f,
                {cx - ssz.x*.5f, sub_y},
                tv(g_theme.col_log, alpha), SUBT);

    // ── Autopilot chip + PCB traces ───────────────────────────────────────────
    {
        constexpr float CW    = 46.f*S, CH = 38.f*S, PIN_W = 8.f*S;
        constexpr int   N_PINS = 3;
        const float     PITCH  = CH / (N_PINS + 1);

        const float chip_cx = cx - BW*.5f - 118.f*S;
        const float chip_cy = cy - 40.f*S;

        draw_chip(dl, chip_cx, chip_cy, alpha);

        const float px0 = chip_cx + CW*.5f + PIN_W;
        const float py[3] = {
            chip_cy - CH*.5f + PITCH,
            chip_cy,
            chip_cy + CH*.5f - PITCH,
        };
        const float via_x[3] = {
            cx - BW*.5f - 26.f*S,
            cx - BW*.5f - 17.f*S,
            cx - BW*.5f - 10.f*S,
        };
        const float land_y[3] = { cy - BH*.4f, cy, cy + BH*.4f };

        const ImU32 tcol = tv(g_theme.col_ok, alpha * .70f);
        constexpr float TW = 1.1f;

        for (int i = 0; i < 3; ++i) {
            dl->AddLine({px0,       py[i]    }, {via_x[i],  py[i]     }, tcol, TW);
            dl->AddLine({via_x[i],  py[i]    }, {via_x[i],  land_y[i] }, tcol, TW);
            dl->AddLine({via_x[i],  land_y[i]}, {cx-BW*.5f, land_y[i] }, tcol, TW);
            dl->AddCircleFilled({via_x[i], py[i]}, 2.f*S, tv(g_theme.col_ok, alpha*.60f));
        }
    }

    // ── Fuselage ──────────────────────────────────────────────────────────────
    draw_fuselage(dl, cx, cy, alpha);

    // ── Body axes (xb forward, zb up) ─────────────────────────────────────────
    constexpr float AXLEN = 30.f * S;
    arrow(dl, {cx, cy}, {cx+AXLEN, cy},
          ca(C_AXES, alpha), 1.f, 6.f*S, 3.5f*S);
    dl->AddText(nullptr, 0.f, {cx+AXLEN+2.f*S, cy+2.f*S},
                ca(C_AXES, alpha*.75f), "xb");
    arrow(dl, {cx, cy}, {cx, cy-AXLEN},
          ca(C_AXES, alpha), 1.f, 6.f*S, 3.5f*S);
    dl->AddText(nullptr, 0.f, {cx+3.f*S, cy-AXLEN-13.f*S},
                ca(C_AXES, alpha*.75f), "zb");

    // ── Lift ──────────────────────────────────────────────────────────────────
    ImVec2 lift_from = {cx, cy-BH*.5f};
    ImVec2 lift_to   = {cx, cy-BH*.5f-AL};
    arrow(dl, lift_from, lift_to, ca(C_LIFT, alpha));
    dl->AddText(nullptr, 0.f, {cx+4.f*S, lift_to.y-1.f},
                ca(C_LIFT, alpha), "L");

    // ── Weight ────────────────────────────────────────────────────────────────
    ImVec2 wgt_from = {cx, cy+BH*.5f};
    ImVec2 wgt_to   = {cx, cy+BH*.5f+AL};
    arrow(dl, wgt_from, wgt_to, ca(C_WEIGHT, alpha));
    dl->AddText(nullptr, 0.f, {cx+4.f*S, wgt_to.y-13.f*S},
                ca(C_WEIGHT, alpha), "W");

    // ── Thrust ────────────────────────────────────────────────────────────────
    float nose_x = cx + BW*.5f + BH*.38f*1.6f;
    ImVec2 thr_from = {nose_x, cy};
    ImVec2 thr_to   = {nose_x+AL, cy};
    arrow(dl, thr_from, thr_to, ca(C_THRUST, alpha));
    dl->AddText(nullptr, 0.f, {thr_to.x-3.f*S, thr_to.y-17.f*S},
                ca(C_THRUST, alpha), "T");

    // ── Drag ──────────────────────────────────────────────────────────────────
    float tail_x = cx - BW*.5f - BH*.18f;
    ImVec2 drg_from = {tail_x, cy};
    ImVec2 drg_to   = {tail_x-AL, cy};
    arrow(dl, drg_from, drg_to, ca(C_DRAG, alpha));
    dl->AddText(nullptr, 0.f, {drg_to.x+3.f*S, drg_to.y-17.f*S},
                ca(C_DRAG, alpha), "D");

    // ── Velocity vector ───────────────────────────────────────────────────────
    const float vel_angle = -(float)M_PI * .13f;
    const float vel_len   = 88.f * S;
    ImVec2 vel_from = {cx+BW*.3f, cy+BH*.5f+18.f*S};
    ImVec2 vel_to   = { vel_from.x + cosf(vel_angle)*vel_len,
                        vel_from.y + sinf(vel_angle)*vel_len };
    arrow(dl, vel_from, vel_to, ca(C_VEL, alpha), 1.5f, 8.f*S, 5.f*S);
    dl->AddText(nullptr, 0.f, {vel_to.x+3.f*S, vel_to.y-13.f*S},
                ca(C_VEL, alpha), "V");

    // ── Pitch moment arc ──────────────────────────────────────────────────────
    const float m_r     = BH*.5f + 20.f*S;
    const float a_start = (float)M_PI * (-150.f / 180.f);
    const float a_end   = (float)M_PI * ( -30.f / 180.f);
    dl->PathArcTo({cx, cy}, m_r, a_start, a_end, 24);
    dl->PathStroke(ca(C_MOM, alpha), false, 1.4f);
    {
        float tx = -sinf(a_end), ty = cosf(a_end);
        ImVec2 tip  = {cx + cosf(a_end)*m_r, cy + sinf(a_end)*m_r};
        ImVec2 back = {tip.x - tx*12.f*S,    tip.y - ty*12.f*S};
        arrow(dl, back, tip, ca(C_MOM, alpha), 0.f, 8.f*S, 4.5f*S);
    }
    dl->AddText(nullptr, 0.f, {cx+m_r+4.f*S, cy-9.f*S},
                ca(C_MOM, alpha), "M");

    // ── Progress bar + hint ───────────────────────────────────────────────────
    {
        const float prog  = elapsed / SPLASH_DURATION;
        const float bar_w = 200.f, bar_h = 2.f;
        const float bar_x = cx - bar_w * .5f;
        const float bar_y = wpy + SPLASH_H - 20.f;

        dl->AddRectFilled({bar_x, bar_y}, {bar_x+bar_w,        bar_y+bar_h},
                          tv(g_theme.bg_child_dark, alpha));
        dl->AddRectFilled({bar_x, bar_y}, {bar_x+bar_w*prog,   bar_y+bar_h},
                          tv(g_theme.accent, alpha*.85f));

        const char* hint = "Press any key or click to continue";
        ImVec2 hs = ImGui::CalcTextSize(hint);
        dl->AddText(nullptr, 0.f,
                    {cx - hs.x*.5f, bar_y + 5.f},
                    tv(g_theme.col_no_link_muted, alpha), hint);
    }

    return false;
}
