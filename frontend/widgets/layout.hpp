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
//  ├────────────────────┬────────────────────┬────┬───────────────────────────┤
//  │ LEFT SIDEBAR       │ VIDEO 16:9 (top)   │ PL │ RIGHT SIDEBAR             │
//  │ (computed for 16:9)├────────────────────┤ UG │  HUD / Sys msgs / MAVLink │
//  │                    │ MAP   (bot)        │ IN │                           │
//  └────────────────────┴────────────────────┴────┴───────────────────────────┘
//
// The rail is a column of its own here, paid for out of the sidebars, so it
// never covers the picture. Fullscreen is the exception: with the sidebars gone
// there is nothing left to take the width from, so there the rail floats over
// the feed's right edge instead.
//
// LEFT_FRAC is fixed; center_w = remainder; video_h is derived for 16:9.
// Adjust LEFT_FRAC / RIGHT_FRAC to reflow all panels simultaneously.

static constexpr float TOPBAR_H    = 60.0f;

// The centre column is sized so its 16:9 video box leaves roughly half the
// column height for the map — widening the sidebars is what buys map height.
//
// Both fractions are trimmed by 2 points to pay for the plugin rail. The
// sidebars are the right place to take it from: they are wide, and the video is
// the one panel where covered pixels are information lost.
static constexpr float LEFT_FRAC  = 0.24f;
static constexpr float RIGHT_FRAC = 0.25f;

// Plugin rail — the column of square user-function buttons down the right edge
// of the centre band. Fixed pixels, not a fraction: the buttons are square and
// a fixed size, so a proportional rail would only pad the margins.
static constexpr float PLUGIN_RAIL_W = 92.0f;

struct GcsLayout {
    float top;       // y where panels start (= TOPBAR_H)
    float total_h;   // usable panel height

    float left_x,   left_w;
    float center_x, center_w;   // centre column: video over map
    float plugin_x, plugin_w;   // rail column, right of the centre column
    float right_x,  right_w;

    float video_h;   // top half of center
    float map_h;     // bottom half of center
    float band_w;    // = center_w + plugin_w: the whole centre band

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

        // The rail is carved out of the middle band and runs its full height,
        // so video and map are both that much narrower than the band.
        l.band_w   = sw - l.left_w - l.right_w;
        l.plugin_w = PLUGIN_RAIL_W;
        l.center_w = l.band_w - l.plugin_w;
        l.center_x = l.left_w;
        l.plugin_x = l.center_x + l.center_w;

        // Derive video_h for the 16:9 aspect ratio; map gets the height left over.
        l.video_h = l.center_w * (9.0f / 16.0f);
        l.map_h   = l.total_h - l.video_h;
        if (l.map_h < 80.0f) {        // clamp on very wide / short windows
            l.map_h   = 80.0f;
            l.video_h = l.total_h - l.map_h;
        }

        return l;
    }
};

// accent_col() and panel_bg() are defined in theme.hpp (included above)
