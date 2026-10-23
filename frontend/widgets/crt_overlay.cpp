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
#include <cstdint>

// Cheap integer hash → [0, 1). Deterministic per (line, tick), so the noise
// holds still between ticks instead of boiling at the frame rate.
static float hash01(uint32_t x)
{
    x ^= x >> 16; x *= 0x7feb352dU;
    x ^= x >> 15; x *= 0x846ca68bU;
    x ^= x >> 16;
    return (x & 0xFFFFFF) / float(0x1000000);
}

static ImU32 black(float a)
{
    return IM_COL32(0, 0, 0, (int)(std::clamp(a, 0.0f, 1.0f) * 255.0f));
}

static ImU32 rgba(const ImVec4& c, float a)
{
    return ImGui::ColorConvertFloat4ToU32({ c.x, c.y, c.z, std::clamp(a, 0.0f, 1.0f) });
}

// The same hue pushed up until its brightest channel is full — the glow of a
// phosphor whose dark line color is `c`.
static ImVec4 lit(const ImVec4& c)
{
    const float m = std::max({ c.x, c.y, c.z, 0.001f });
    return { c.x / m, c.y / m, c.z / m, 1.0f };
}

// ── Scanlines ────────────────────────────────────────────────────────────────
//
// A hairline in the theme's scanline color every SPACING px, each one's depth
// nudged by noise that is re-rolled NOISE_HZ times a second — enough to read
// as a live tube, slow enough not to shimmer over text. A faint band of the
// same phosphor, lit, drifts down the screen once every ROLL_S seconds, the
// way an unlocked vertical hold creeps.

static void draw_scanlines(ImDrawList* dl, ImVec2 size, float strength, double t)
{
    constexpr float  SPACING  = 3.0f;
    constexpr double NOISE_HZ = 24.0;
    constexpr double ROLL_S   = 9.0;

    const uint32_t tick  = (uint32_t)(t * NOISE_HZ);
    const float    base  = 0.55f * strength * g_theme.crt_scanline.w;
    const ImVec4&  col   = g_theme.crt_scanline;
    const ImVec4   glow  = lit(col);

    uint32_t line = 0;
    for (float y = 0.0f; y < size.y; y += SPACING, ++line) {
        const float n = hash01(line * 0x9E3779B1U ^ tick * 0x85EBCA77U);
        dl->AddRectFilled({ 0.0f, y }, { size.x, y + 1.0f }, rgba(col, base * (0.7f + 0.6f * n)));
    }

    // A rare brighter streak — a single line catching interference.
    if (hash01(tick * 0xC2B2AE3DU) < 0.06f) {
        const float y = std::floor(hash01(tick * 0x27D4EB2FU) * size.y);
        dl->AddRectFilled({ 0.0f, y }, { size.x, y + 1.0f }, rgba(glow, 0.16f * strength));
    }

    // Rolling band: soft phosphor wash, fading in and out across its height.
    const float band_h = size.y * 0.14f;
    const float phase  = (float)std::fmod(t, ROLL_S) / (float)ROLL_S;
    const float y0     = phase * (size.y + band_h) - band_h;
    const ImU32 clear  = rgba(glow, 0.0f);
    const ImU32 band   = rgba(glow, 0.065f * strength);
    const float mid    = y0 + band_h * 0.5f;
    dl->AddRectFilledMultiColor({ 0.0f, y0 },  { size.x, mid },           clear, clear, band, band);
    dl->AddRectFilledMultiColor({ 0.0f, mid }, { size.x, y0 + band_h },   band, band, clear, clear);
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

void draw_crt_overlay(const AppSettings& settings, double time_s)
{
    if (!settings.crt_scanlines && !settings.crt_vignette) return;

    ImDrawList*  dl   = ImGui::GetForegroundDrawList();
    const ImVec2 size = ImGui::GetIO().DisplaySize;

    if (settings.crt_scanlines)
        draw_scanlines(dl, size, settings.crt_scanline_strength, time_s);
    if (settings.crt_vignette)
        draw_vignette(dl, size, settings.crt_vignette_strength);
}
