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
#include "ui_kit.hpp"
#include "layout_rules.hpp"
#include <algorithm>
#include <cfloat>

// ── GCS screen layout ─────────────────────────────────────────────────────────
//
//  ┌──────────────────────── TOPBAR (60px) ───────────────────────────────────┐
//  ├────────────────────┬───── CENTRE HEADER ───────┬────┬────────────────────┤
//  │ LEFT SIDEBAR       │ VIDEO 16:9 (top)          │ PL │ RIGHT SIDEBAR      │
//  │ (sized by its text)├───────────────────────────┤ UG │  HUD / Sys msgs /  │
//  │                    │ MAP   (bot)               │ IN │  MAVLink           │
//  └────────────────────┴───────────────────────────┴────┴────────────────────┘
//
// On a wide screen the centre turns on its side: MAP | VIDEO, the feed against
// the rail. On a narrow one the right sidebar and the rail go and the centre is
// the feed or the map, fullscreen. The rules that decide all of this are in
// layout_rules.hpp; this file feeds them the live window and font.
//
// The rail is a column of its own here, paid for out of the centre, so it never
// covers the picture. Fullscreen is the exception: with the sidebars gone there
// is nothing left to take the width from, so there the rail floats over the
// feed's right edge instead.

static constexpr float TOPBAR_H    = 60.0f;

// The centre view's header strip — the title and the VIDEO / MAP / SWARM
// buttons. Here rather than in center_view.cpp because the swarm layout sits
// under it too.
static constexpr float CENTER_HEADER_H = 30.0f;

// Plugin rail — the column of square user-function buttons down the right edge
// of the centre band. Fixed pixels, not a fraction: the buttons are square and
// a fixed size, so a proportional rail would only pad the margins.
static constexpr float PLUGIN_RAIL_W = 92.0f;

// The width of one glyph of the body font, which is what the sidebars are
// measured in. Read off the font rather than assumed, so a different font or
// size moves the whole layout with it.
inline float layout_glyph_w()
{
    ImFont* f = ImGui::GetIO().FontDefault ? ImGui::GetIO().FontDefault
                                           : ImGui::GetFont();
    const float w = f ? f->CalcTextSizeA(UI_SZ_BODY, FLT_MAX, 0.0f, "0").x : 0.0f;
    return w > 0.0f ? w : 7.0f;
}

struct GcsLayout {
    float top;       // y where panels start (= TOPBAR_H)
    float total_h;   // usable panel height

    // Too narrow for both sidebars beside a centre: no right sidebar, no rail,
    // and the centre is only ever the feed or the map, fullscreen.
    bool  narrow;

    float left_x,   left_w;
    float center_x, center_w;   // centre column: video and map
    float plugin_x, plugin_w;   // rail column, right of the centre column
    float right_x,  right_w;
    float band_w;    // = center_w + plugin_w: the whole centre band

    // MAP + VIDEO, below the centre header.
    CenterSplit split;

    static GcsLayout compute(float sw, float sh, float ch)
    {
        const LayoutColumns c = layout_columns(sw, ch, PLUGIN_RAIL_W);

        GcsLayout l;
        l.top     = TOPBAR_H;
        l.total_h = sh - TOPBAR_H;
        l.narrow  = c.narrow;

        l.left_x   = 0.0f;
        l.left_w   = c.left_w;
        l.center_x = c.left_w;
        l.center_w = c.center_w;
        l.plugin_w = c.narrow ? 0.0f : PLUGIN_RAIL_W;
        l.plugin_x = l.center_x + l.center_w;
        l.band_w   = l.center_w + l.plugin_w;
        l.right_w  = c.right_w;
        l.right_x  = c.narrow ? sw : sw - c.right_w;

        const float content_top = l.top + CENTER_HEADER_H;
        l.split = split_center({ l.center_x, content_top,
                                 l.center_w, sh - content_top });
        return l;
    }

    static GcsLayout compute()
    {
        const ImGuiIO& io = ImGui::GetIO();
        return compute(io.DisplaySize.x, io.DisplaySize.y, layout_glyph_w());
    }
};

// ── SWARM screen layout ───────────────────────────────────────────────────────
//
//  ┌──────────────────────── TOPBAR ──────────────────────────────────────────┐
//  ├──────────────────────── CENTRE HEADER (VIDEO / MAP / … / SWARM) ─────────┤
//  │ VEHICLE  │ FLIGHT        │ TELEMETRY   │ EVENT LOG                       │
//  │ CARDS    ├───────────────┴─────────────┴─────────────────────────────────┤
//  │ (width   │ MAP                                                           │
//  │  by fleet│                                                               │
//  └──────────┴───────────────────────────────────────────────────────────────┘
//
// No sidebars, no video, no plugin rail: the screen belongs to the fleet.
//
// Nothing here is a fixed fraction. The card column is as wide as the fleet
// needs (up to half the screen, then it scrolls); the band is as tall as the
// FLIGHT section's mode grid needs; the map takes whatever is left. Both
// inputs are worked out by the swarm view each frame — see
// swarm_view_layout().
static constexpr float SWARM_CARDS_MAX_FRAC = 0.5f;

// The band never gets shorter than the telemetry grid, three 66 px rows under
// a header, nor taller than two thirds of the screen: the map must survive.
static constexpr float SWARM_BAND_MIN_H    = 250.0f;
static constexpr float SWARM_BAND_MAX_FRAC = 0.67f;

// FLIGHT's column: three mode buttons across, each wide enough for most names.
static constexpr float SWARM_FLIGHT_W      = 390.0f;

// The telemetry tiles stop getting wider past this; the log takes the rest.
static constexpr float SWARM_GRID_MAX_W    = 480.0f;

struct SwarmLayout {
    float top, total_h;            // below the centre header

    float cards_x, cards_w;
    float right_x, right_w;

    float band_h;                  // the FLIGHT | TELEMETRY | LOG band
    float flight_x, flight_w;
    float grid_x,   grid_w;
    float log_x,    log_w;

    float map_y, map_h;

    // `cards_w`: the card column's width. `flight_h`: the height the FLIGHT
    // section wants, padding included.
    static SwarmLayout compute(float cards_w, float flight_h)
    {
        const ImGuiIO& io = ImGui::GetIO();
        const float sw = io.DisplaySize.x;
        const float sh = io.DisplaySize.y;

        SwarmLayout l;
        l.top     = TOPBAR_H + CENTER_HEADER_H;
        l.total_h = sh - l.top;

        l.cards_x = 0.0f;
        l.cards_w = std::min(cards_w, sw * SWARM_CARDS_MAX_FRAC);
        l.right_x = l.cards_w;
        l.right_w = sw - l.cards_w;

        l.band_h = std::max(SWARM_BAND_MIN_H, flight_h);
        l.band_h = std::min(l.band_h, l.total_h * SWARM_BAND_MAX_FRAC);

        l.flight_x = l.right_x;
        l.flight_w = std::min(SWARM_FLIGHT_W, l.right_w * 0.45f);
        const float rest = l.right_w - l.flight_w;
        l.grid_x   = l.flight_x + l.flight_w;
        l.grid_w   = std::min(rest * 0.5f, SWARM_GRID_MAX_W);
        l.log_x    = l.grid_x + l.grid_w;
        l.log_w    = rest - l.grid_w;

        l.map_y = l.top + l.band_h;
        l.map_h = l.total_h - l.band_h;
        return l;
    }
};

// accent_col() and panel_bg() are defined in theme.hpp (included above)
