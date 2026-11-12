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

#pragma once

// The HUD's drawing parts: tapes, dials, value boxes, chips. Used by hud.cpp.

#include <algorithm>
#include <cfloat>
#include "frontend/components/others/ui_kit.hpp"
#include "frontend/source/theme.hpp"
#include "imgui.h"

namespace right_sidebar {

// Every number box and chip on the HUD is a plain dark wash at this opacity,
// unframed: enough to lift white digits off a bright sky, light enough that
// the horizon still reads through.
constexpr float HUD_BOX_ALPHA = 0.55f;
inline ImU32 hud_box_bg() { return ui_col(g_theme.bg_video, HUD_BOX_ALPHA); }

struct HudText {
    ImFont* font;
    float   size;
    float width(const char* s) const {
        return font->CalcTextSizeA(size, FLT_MAX, 0.0f, s).x;
    }
};

// How far a value box's arrow reaches past the box.
inline float hud_arrow(const HudText& t) { return std::max(4.0f, t.size * 0.5f); }

// A vertical moving tape: the scale slides past a fixed box at its middle that
// holds the value. `ticks_right` puts the graduations on the right edge (the
// left tape, facing the centre) or the left edge (the right tape).
struct VTape {
    float minor;          // units between ticks
    int   major_every;    // ticks between labelled ones
    float span;           // units from the middle to either end
    bool  non_negative;   // no scale below zero (speed)
    const char* box_fmt;
};

void hud_fill_gradient(ImDrawList* dl, const ImVec2* pts, const ImU32* cols, int n);
int hud_clip_ground(const ImVec2* in, int n, ImVec2 h, float ux, float uy,
                    ImVec2* out);
void hud_value_box(ImDrawList* dl, const HudText& t, ImVec2 a, ImVec2 b, int dir,
                   const char* text, bool valid);
void hud_chip(ImDrawList* dl, const HudText& t, ImVec2 pos, const char* text,
              ImU32 col);
void hud_vtape(ImDrawList* dl, const HudText& t, ImVec2 a, ImVec2 b,
               bool ticks_right, const VTape& spec, bool valid, float value);
void hud_heading_tape(ImDrawList* dl, const HudText& t, ImVec2 a, ImVec2 b,
                      float hdg);
void hud_dial(ImDrawList* dl, const HudText& t, ImVec2 c, float rad,
              const char* label, bool bipolar, float base_full, float step,
              const char* fmt, bool valid, float value);

} // namespace right_sidebar
