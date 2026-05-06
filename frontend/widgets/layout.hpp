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
#include "theme.hpp"

// ── GCS screen layout ─────────────────────────────────────────────────────────
//
//  ┌──────────────────────── TOPBAR (60px) ───────────────────────────────────┐
//  ├──────────────────────┬──────────────────────┬────────────────────────────┤
//  │ LEFT SIDEBAR         │  VIDEO 16:9 (top 50%)│ RIGHT SIDEBAR              │
//  │ (computed for 16:9)  ├──────────────────────┤  HUD / Sys msgs / MAVLink  │
//  │                      │  MAP   (bot 50%)     │                            │
//  └──────────────────────┴──────────────────────┴────────────────────────────┘
//
// LEFT_FRAC is fixed; center_w = remainder; video_h is derived for 16:9.
// Adjust LEFT_FRAC / RIGHT_FRAC to reflow all panels simultaneously.

static constexpr float TOPBAR_H    = 60.0f;
static constexpr float LEFT_FRAC  = 0.20f;
static constexpr float RIGHT_FRAC = 0.22f;

struct GcsLayout {
    float top;       // y where panels start (= TOPBAR_H)
    float total_h;   // usable panel height

    float left_x,   left_w;
    float center_x, center_w;
    float right_x,  right_w;

    float video_h;   // top half of center
    float map_h;     // bottom half of center

    static GcsLayout compute()
    {
        const ImGuiIO& io = ImGui::GetIO();
        const float sw = io.DisplaySize.x;
        const float sh = io.DisplaySize.y;

        GcsLayout l;
        l.top     = TOPBAR_H;
        l.total_h = sh - TOPBAR_H;

        l.left_w   = sw * LEFT_FRAC;
        l.left_x   = 0.0f;
        l.right_w  = sw * RIGHT_FRAC;
        l.right_x  = sw - l.right_w;
        l.center_w = sw - l.left_w - l.right_w;
        l.center_x = l.left_w;

        // Derive video_h for the aspect ratio; map gets whatever height remains.
        l.video_h = l.center_w * (9.0f / 20.0f);
        l.map_h   = l.total_h - l.video_h;
        if (l.map_h < 80.0f) {        // clamp on very wide / short windows
            l.map_h   = 80.0f;
            l.video_h = l.total_h - l.map_h;
        }

        return l;
    }
};

// accent_col() and panel_bg() are defined in theme.hpp (included above)
