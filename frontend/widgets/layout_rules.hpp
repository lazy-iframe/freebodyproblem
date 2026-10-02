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
#include <algorithm>
#include <cmath>

// ── Layout rules ──────────────────────────────────────────────────────────────
//
// The arithmetic behind the GCS screen, kept free of ImGui so it is a plain
// function of the screen and the font: layout.hpp feeds it the live window and
// the measured glyph width, and anything else — a table of monitors, a test —
// can feed it whatever it likes.
//
// The rules, in the order they are applied:
//
//  1. The sidebars are measured in characters, not in fractions of the screen.
//     The UI font is monospaced, so a column of text is exactly N glyphs wide;
//     the sidebars get what their text needs and stop there. Every pixel past
//     their maximum goes to the centre.
//
//  2. Space is handed out centre-minimum first, then sidebars up to their
//     maximum, then the rest to the centre. A screen that cannot give both
//     sidebars their minimum beside a minimum centre is NARROW: the right
//     sidebar and the plugin rail go, and the centre can only be the feed or
//     the map fullscreen — both of which already do without them.
//
//  3. MAP + VIDEO is stacked (video over map, equal areas) until the centre
//     gets too wide for it, and then split down the middle (map | video). The
//     feed is 16:9 and the map is not, so whatever the feed's box does not fill
//     is letterbox; the map is never cut to fit the feed.

// Widths in glyphs of the body font. The minimums are the widest row that can
// not shrink: the PARAMS row (16-character ID + value + WRITE) on the left, the
// three telemetry tiles and their header on the right. The maximums are what
// the columns were on a 1080p screen, where they were sized by eye and look
// right; past that, more width only pads the text.
static constexpr float LEFT_MIN_CH   = 54.0f;
static constexpr float LEFT_MAX_CH   = 62.0f;
static constexpr float RIGHT_MIN_CH  = 52.0f;
static constexpr float RIGHT_MAX_CH  = 65.0f;

// The narrowest centre column worth showing two panes in: a feed about 300 px
// tall at the default font, and room in the header for the mode buttons.
static constexpr float CENTER_MIN_CH = 72.0f;

static constexpr float VIDEO_ASPECT  = 16.0f / 9.0f;

// Centre width over height past which MAP + VIDEO goes side by side. Stacking
// a wide centre gives the feed most of the height and leaves the map a strip:
// at 1.55 (2560×1080) the map is 400 px tall under a 1530 px wide feed, which
// is not a map worth having. At 1.36 (a 4K screen at 100%) stacked is still a
// good map under a big feed. Halving gives the feed less than stacking would;
// that is the price of a map that can be used.
static constexpr float SIDE_BY_SIDE_ASPECT = 1.45f;

struct LayoutRect { float x, y, w, h; };

// The two panes of MAP + VIDEO inside the content rect below the centre header.
// `video` is the feed's window, which may be larger than the 16:9 picture the
// feed draws inside it; `picture` is that picture, centred in the window.
struct CenterSplit {
    bool       side_by_side;
    LayoutRect video, picture, map;
};

// The largest 16:9 box that fits in `r`, centred in it.
inline LayoutRect fit_video(LayoutRect r)
{
    float w = r.w, h = r.w / VIDEO_ASPECT;
    if (h > r.h) { h = r.h; w = r.h * VIDEO_ASPECT; }
    return { r.x + (r.w - w) * 0.5f, r.y + (r.h - h) * 0.5f, w, h };
}

// Rule 3. `r` is the content rect below the centre header.
inline CenterSplit split_center(LayoutRect r)
{
    const float W = std::max(r.w, 1.0f), H = std::max(r.h, 1.0f);

    const bool side = W / H > SIDE_BY_SIDE_ASPECT;

    // Stacked: the video row of height h holds a 16:9 picture (16/9·h)·h; the
    // map below is W·(H − h). Equal when (16/9)h² + W·h − W·H = 0. The picture
    // can not be wider than the column, so h stops at the full-width height.
    const float a  = VIDEO_ASPECT;
    float       vh = (-W + std::sqrt(W * W + 4.0f * a * W * H)) / (2.0f * a);
    vh = std::min(vh, W / VIDEO_ASPECT);

    // Side by side: halves. Only once a full-height feed is narrower than half
    // the centre (past about 32:9) does the map get more than half — the feed
    // has nothing to fill it with.
    const float vw = std::min(W * 0.5f, H * VIDEO_ASPECT);

    CenterSplit s;
    s.side_by_side = side;
    if (side) {
        // Map on the left, against the left sidebar the mission is planned
        // from; video on the right, against the plugin rail that drives it.
        s.map   = { r.x,          r.y, W - vw, H };
        s.video = { r.x + W - vw, r.y, vw,     H };
    } else {
        s.video = { r.x, r.y,      W, vh     };
        s.map   = { r.x, r.y + vh, W, H - vh };
    }
    s.picture = fit_video(s.video);
    return s;
}

// SWARM's map corner — the space under its band, beside the card column —
// takes the feed by the same split, but only while the map left over keeps at
// least this many square glyphs. The cards grow a column at a time and the
// map pays for each one; past this the map alone is worth more than both.
//
// About a 1080p MAP + VIDEO map. On a 1080p screen two card columns leave a
// map of some 8,800 and three some 7,400, so two columns keep the feed and
// three do not; on a wider screen the cards stop at half its width long
// before the map gets this small.
static constexpr float SWARM_MAP_MIN_CH2 = 7700.0f;

inline bool swarm_video_fits(LayoutRect r, float ch)
{
    const CenterSplit s = split_center(r);
    return s.map.w * s.map.h >= SWARM_MAP_MIN_CH2 * ch * ch;
}

struct LayoutColumns {
    bool  narrow;
    float left_w, center_w, right_w;
};

// Rules 1 and 2. `sw` is the screen width, `ch` the body font's glyph advance,
// `rail_w` the plugin rail's column.
inline LayoutColumns layout_columns(float sw, float ch, float rail_w)
{
    const float lmin = LEFT_MIN_CH  * ch, lmax = LEFT_MAX_CH  * ch;
    const float rmin = RIGHT_MIN_CH * ch, rmax = RIGHT_MAX_CH * ch;
    const float cmin = CENTER_MIN_CH * ch;

    LayoutColumns c;
    const float avail = sw - rail_w;
    c.narrow = avail < lmin + rmin + cmin;

    if (c.narrow) {
        // Only MAP FULL keeps a sidebar: the left one, beside the map. It gets
        // what it can without taking the map below a centre's minimum, but
        // never less than its own minimum — the tabs do not work narrower.
        c.left_w   = std::clamp(sw - cmin, lmin, lmax);
        c.right_w  = rmin;          // for the map overlay's expanded width only
        c.center_w = sw - c.left_w;
        return c;
    }

    // Centre minimum is already paid for. What is over goes to the sidebars,
    // each in proportion to how far it is from its maximum, so both reach it
    // together; anything past that goes to the centre.
    const float spare = avail - cmin - lmin - rmin;
    const float slack = (lmax - lmin) + (rmax - rmin);
    const float give  = std::min(spare, slack);
    c.left_w   = lmin + give * (lmax - lmin) / slack;
    c.right_w  = rmin + give * (rmax - rmin) / slack;
    c.center_w = avail - c.left_w - c.right_w;
    return c;
}
