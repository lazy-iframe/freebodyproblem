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


#include "crt_overlay.hpp"
#include "../settings.hpp"
#include "theme.hpp"
#include "imgui.h"
#include <algorithm>
#include <cmath>

static ImU32 black(float a)
{
    return IM_COL32(0, 0, 0, (int)(std::clamp(a, 0.0f, 1.0f) * 255.0f));
}

static ImU32 rgba(const ImVec4& c, float a)
{
    return ImGui::ColorConvertFloat4ToU32({ c.x, c.y, c.z, std::clamp(a, 0.0f, 1.0f) });
}

// ── Scanlines ────────────────────────────────────────────────────────────────
//
// A phosphor terminal's raster, held still, inside the boxes: rows two pixels
// tall in a lighter tone of the box's own colour, the box's colour itself
// between them. Lighter, not darker, so the lines stand off the fill without
// taking contrast from the caption over it. Nothing in it moves, so it never
// draws the eye away from the data under it.

// A lighter tone of `c`: its hue pushed up until the brightest channel is
// full, then a third of the way on to white — so even a bright fill, already
// near its hue's full strength, gets a line lighter than itself.
static ImVec4 lighter(const ImVec4& c)
{
    const float m = std::max({ c.x, c.y, c.z, 0.001f });
    constexpr float W = 0.35f;
    return { c.x / m + (1.0f - c.x / m) * W, c.y / m + (1.0f - c.y / m) * W,
             c.z / m + (1.0f - c.z / m) * W, 1.0f };
}

static bool  s_lines    = false;   // this frame's settings, from crt_begin_frame()
static float s_strength = 0.0f;

void crt_begin_frame(const AppSettings& settings)
{
    s_lines    = settings.crt_scanlines;
    s_strength = settings.crt_scanline_strength;
}

void crt_box_scanlines(ImDrawList* dl, ImVec2 p0, ImVec2 p1, ImVec4 fill)
{
    if (!s_lines || !dl) return;
    constexpr int ROW = 2;   // px: a line, then a gap as tall
    const float x0 = std::floor(p0.x) + 1.0f, x1 = std::floor(p1.x) - 1.0f;
    const float y1 = std::floor(p1.y) - 1.0f;
    float       y  = std::floor(p0.y) + 1.0f;
    if (x1 <= x0 || y1 <= y) return;

    // Drawn straight to the list, so the disabled fade ImGui gives its own
    // widgets is folded in by hand, as the plates do.
    const float a    = s_strength * ImGui::GetStyle().Alpha;
    const ImU32 line = rgba(lighter(fill), 0.16f * a);
    while (y < y1) {
        // Each band runs to the next multiple of ROW on screen, so every box
        // shares one raster whatever row it starts on. The odd bands are the
        // gaps, left as the box drew them.
        const int   band = (int)y / ROW;
        const float end  = std::min((float)((band + 1) * ROW), y1);
        if (!(band & 1)) dl->AddRectFilled({ x0, y }, { x1, end }, line);
        y = end;
    }
}

// ── Edge darkening ───────────────────────────────────────────────────────────
//
// Four gradients falling off from each edge. Where two meet they stack, so the
// corners go darkest — which is what a curved tube does anyway.

static void draw_vignette(ImDrawList* dl, ImVec2 size, float strength)
{
    const float d     = std::min(size.x, size.y) * 0.24f;
    const ImU32 dark  = black(0.75f * strength);
    const ImU32 clear = black(0.0f);

    dl->AddRectFilledMultiColor({ 0.0f, 0.0f },          { size.x, d },      dark, dark, clear, clear);
    dl->AddRectFilledMultiColor({ 0.0f, size.y - d },    { size.x, size.y }, clear, clear, dark, dark);
    dl->AddRectFilledMultiColor({ 0.0f, 0.0f },          { d, size.y },      dark, clear, clear, dark);
    dl->AddRectFilledMultiColor({ size.x - d, 0.0f },    { size.x, size.y }, clear, dark, dark, clear);
}

void draw_crt_overlay(const AppSettings& settings)
{
    if (!settings.crt_vignette) return;
    draw_vignette(ImGui::GetForegroundDrawList(), ImGui::GetIO().DisplaySize,
                  settings.crt_vignette_strength);
}
