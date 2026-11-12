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

#define _USE_MATH_DEFINES
#include <cmath>
#include "frontend/elements/sidebar_right.hpp"
#include "frontend/components/sidebar_right/sidebar_internal.hpp"
#include "frontend/elements/center_view.hpp"
#include "frontend/source/layout.hpp"
#include "frontend/source/theme.hpp"
#include "frontend/components/others/ui_kit.hpp"
#include "imgui.h"
#include "frontend/source/settings.hpp"
#include <cstdio>
#include <algorithm>

using namespace right_sidebar;

// ── Right sidebar ─────────────────────────────────────────────────────────────

void draw_sidebar_right(const VehicleState& vs,
                        MavlinkSender* sender,
                        AppSettings* settings)
{
    // Load cell config from settings on the first frame.
    if (!s_cells_loaded) {
        if (settings) cells_load(*settings);
        s_cells_loaded = true;
    }
    const GcsLayout l = GcsLayout::compute();

    ImGui::SetNextWindowPos ({ l.right_x, l.top    }, ImGuiCond_Always);
    ImGui::SetNextWindowSize({ l.right_w, l.total_h }, ImGuiCond_Always);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, panel_bg());
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,  { 8.0f, 8.0f });
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,    { 6.0f, 4.0f });
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar  |
        ImGuiWindowFlags_NoResize    |
        ImGuiWindowFlags_NoMove      |
        ImGuiWindowFlags_NoScrollbar;

    if (ImGui::Begin("##sidebar_right", nullptr, flags)) {
        {
            ImDrawList*  dl = ImGui::GetWindowDrawList();
            const ImVec2 wp = ImGui::GetWindowPos();
            dl->AddLine({ wp.x, wp.y }, { wp.x, wp.y + l.total_h },
                        col32_panel_border(), PANEL_BORDER_THICKNESS);
        }

        const float avail_h = ImGui::GetContentRegionAvail().y;
        const float avail_w = ImGui::GetContentRegionAvail().x;

        // HUD: the instrument + its header
        const float hud_h = std::min(avail_w + 46.0f, avail_h * 0.40f);

        // Data grid: 3 rows of tiles + header + spacing
        const float grid_section_h = 3.0f * GRID_CELL_H + UI_HEADER_H + 14.0f;

        // ── Attitude ─────────────────────────────────────────────────────
        ImGui::PushStyleColor(ImGuiCol_ChildBg, panel_bg());
        if (ImGui::BeginChild("##hud", { 0.0f, hud_h }, false,
                              ImGuiWindowFlags_NoScrollbar)) {
            char hud_meta[32];
            if (vs.has_attitude)
                snprintf(hud_meta, sizeof(hud_meta), "R %+.0f\xc2\xb0  P %+.0f\xc2\xb0",
                         (double)vs.roll, (double)vs.pitch);
            else
                snprintf(hud_meta, sizeof(hud_meta), "NO DATA");
            ui_panel_header("ATTITUDE \xc2\xb7 ROLL / PITCH", hud_meta);

            const bool   tapes = !settings || settings->hud_tapes_main;
            const ImVec2 avail = ImGui::GetContentRegionAvail();
            if (vs.has_attitude || !tapes) {
                draw_hud_body(vs, avail, tapes, 0.0f, UI_SZ_BODY + 2.0f);
            } else {
                const char*  msg = "NO ATTITUDE DATA";
                const ImVec2 ts  = ImGui::CalcTextSize(msg);
                ImGui::SetCursorPos({ ImGui::GetCursorPosX() + (avail.x - ts.x) * 0.5f,
                                      ImGui::GetCursorPosY() + (avail.y - ts.y) * 0.5f });
                ImGui::TextDisabled("%s", msg);
            }
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();

        // ── Telemetry tiles ──────────────────────────────────────────────
        ImGui::PushStyleColor(ImGuiCol_ChildBg, panel_bg());
        if (ImGui::BeginChild("##datagrid", { 0.0f, grid_section_h },
                              false, ImGuiWindowFlags_NoScrollbar)) {
            ui_panel_header("TELEMETRY", "CLICK A TILE TO ASSIGN");
            draw_data_grid(&vs, sender, settings);
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();

        // ── Event log ────────────────────────────────────────────────────
        ImGui::PushStyleColor(ImGuiCol_ChildBg, panel_bg());
        if (ImGui::BeginChild("##sysmsg", { 0.0f, 0.0f },
                              false, ImGuiWindowFlags_NoScrollbar)) {
            char log_meta[24];
            snprintf(log_meta, sizeof(log_meta), "%d ENTRIES", (int)g_fleet_log.size());
            draw_event_log_header(log_meta);
            draw_event_log_body(1.0f, /*small_text=*/false, /*wrap=*/true);
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();
    }
    ImGui::End();

    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor();
}

// Which half of the fullscreen-map overlay is expanded to its sidebar size.

// ── Fullscreen-map overlay ────────────────────────────────────────────────────
//
// What the right sidebar was showing, condensed into the map's top-right
// corner and made semi-transparent, for when the map has taken the sidebar's
// width. Attitude and the event log only: the telemetry tiles are a
// configuration surface as much as a readout, and clicking through them over a
// map the operator is flying on is not what the corner is for.
//
// Submitted after the centre view by the render loop, which is what puts it
// over the map. The map window is NoBringToFrontOnFocus, so clicking the map
// cannot raise it above this.

void draw_map_overlay(const VehicleState& vs, bool hud_tapes)
{
    float mx, my, mw, mh;
    center_view_map_rect(mx, my, mw, mh);
    if (mw < 240.0f || mh < 200.0f) return;   // no room to float anything

    constexpr float MARGIN = 10.0f;
    constexpr float PAD    = 8.0f;

    // How big each half is standing. Small is the resting state — the corner is
    // a glance, not a panel — and a double click on a half toggles that half between
    // it and the size it had in the sidebar.
    //
    // Each is its own switch: a click on one says nothing about the other, and
    // nothing outside the block moves either. An operator who opened the log to
    // read it and then went back to flying the map should not find it shut when
    // they look again.
    static bool s_hud_big = false;
    static bool s_log_big = false;

    // Both come back small. This function only runs while the map is
    // fullscreen, so a break in the frames it drew on is exactly "the operator
    // left and came back" — and coming back to an expanded block over a map
    // they have not looked at yet is not what the corner is for.
    static int s_last_frame = -2;
    const int  frame = ImGui::GetFrameCount();
    if (frame != s_last_frame + 1) { s_hud_big = false; s_log_big = false; }
    s_last_frame = frame;

    const bool hud_big = s_hud_big;
    const bool log_big = s_log_big;

    // "Normal size" is the size these had before the map went fullscreen, which
    // is the right sidebar's width — the width the map took in the first place.
    const GcsLayout l      = GcsLayout::compute();
    float           full_w = std::min(l.right_w, mw * 0.5f);

    // Resting width: the attitude ball sets it, rather than being fitted into a
    // box sized for something else — it is the widest thing in here, and a
    // wider box would only put dead space either side of it.
    //
    // 0.605 of the width a full-width ball would have had — half, then a tenth
    // back on top of that twice over. Over a map, roll and pitch are a glance
    // rather than something to fly on, and the room it gives up is map the
    // operator gets back. The clamp against the map's own width is what keeps
    // the block sane on a small window.
    const float ball_small = std::min(184.0f, (mw * 0.34f - PAD * 2.0f) * 0.605f);

    // But never narrower than its text: the ATTITUDE header with the widest
    // R / P readout beside it, in glyphs of the
    // body font like the sidebars. On a small map the ball shrinks and sits in
    // the middle of the wider block; the words do not shrink with it.
    constexpr float OVERLAY_MIN_CH = 30.0f;
    const float small_w = std::min(std::max(ball_small + PAD * 2.0f,
                                            OVERLAY_MIN_CH * layout_glyph_w()),
                                   mw - MARGIN * 2.0f);
    full_w = std::max(full_w, small_w);

    const float w      = (hud_big || log_big) ? full_w : small_w;
    const float ball_h = hud_big ? std::min(w - PAD * 2.0f, mh * 0.45f)
                                 : ball_small;

    // Lettering for the ball. The instrument is a bit over half the sidebar's
    // diameter but its labels do not shrink with it, so at the sidebar's size
    // the pitch ladder, the roll arc and the three readouts run into each
    // other. 10 px against the body font's 16 is what pulls them apart; the
    // expanded ball is the sidebar's size again and wants the sidebar's text,
    // which is what 0 asks for.
    const float hud_text_sz = hud_big ? 0.0f : 10.0f;

    // The GND / HDG / CLB numbers under the classic ball. Three of them share
    // a row a third of the sidebar's width, so the sidebar's 18 px leaves them
    // nearly touching; 13 px puts air between the columns. Expanded, the row
    // is the sidebar's width again and takes the sidebar's size back.
    const float strip_value_sz = hud_big ? UI_SZ_BODY + 2.0f : UI_SZ_MICRO;

    // Attitude is that square plus its header; the tapes HUD fills all of it,
    // the classic ball leaves room for the readout strip beneath.
    const float hud_h  = ball_h + UI_HEADER_H + PAD + (hud_tapes ? 0.0f : 40.0f);

    // The log comes down with the ball, so the corner stays a corner block
    // rather than becoming a log with an instrument on top.
    //
    // Half again as tall as it first was: wrapped, one message can take two or
    // three lines, and at the old height that left only a handful on screen.
    const float log_h   = log_big ? std::min(540.0f, mh * 0.75f)
                                  : std::min(225.0f, mh * 0.375f);
    const float h       = std::min(mh - MARGIN * 2.0f, hud_h + log_h + PAD * 2.0f);

    const ImVec2 ov_p0 = { mx + mw - w - MARGIN, my + MARGIN };
    ImGui::SetNextWindowPos (ov_p0, ImGuiCond_Always);
    ImGui::SetNextWindowSize({ w, h }, ImGuiCond_Always);

    // Semi-transparent throughout: the point of the fullscreen map is the map,
    // and a solid panel in the corner would just be the sidebar again in a
    // different place. The seam is kept at full strength so the block still has
    // an edge against whatever imagery is under it.
    const auto fade = [](ImVec4 c, float a) { c.w *= a; return c; };
    ImGui::PushStyleColor(ImGuiCol_WindowBg, fade(g_theme.bg_panel, 0.62f));
    ImGui::PushStyleColor(ImGuiCol_Border,   fade(g_theme.separator, 0.9f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,   { PAD, PAD });
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,     { 6.0f, 4.0f });
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,  0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar   | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove       | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoNavFocus;

    if (ImGui::Begin("##map_overlay", nullptr, flags)) {
        // ── Attitude ─────────────────────────────────────────────────────────
        ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32_BLACK_TRANS);
        if (ImGui::BeginChild("##ov_hud", { 0.0f, hud_h }, false,
                              ImGuiWindowFlags_NoScrollbar)) {
            char meta[32];
            if (vs.has_attitude)
                snprintf(meta, sizeof(meta), "R %+.0f\xc2\xb0  P %+.0f\xc2\xb0",
                         (double)vs.roll, (double)vs.pitch);
            else
                snprintf(meta, sizeof(meta), "NO DATA");
            ui_panel_header("ATTITUDE", meta);

            draw_hud_body(vs, ImGui::GetContentRegionAvail(), hud_tapes,
                          hud_text_sz, strip_value_sz);
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();

        // ── Event log ────────────────────────────────────────────────────────
        ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32_BLACK_TRANS);
        if (ImGui::BeginChild("##ov_log", { 0.0f, 0.0f }, false,
                              ImGuiWindowFlags_NoScrollbar)) {
            char log_meta[24];
            snprintf(log_meta, sizeof(log_meta), "%d", (int)g_fleet_log.size());
            draw_event_log_header(log_meta);
            draw_event_log_body(0.45f, /*small_text=*/!log_big, /*wrap=*/true);
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();
    }
    ImGui::End();

    ImGui::PopStyleVar(4);
    ImGui::PopStyleColor(2);

    // ── Sizing the halves ────────────────────────────────────────────────────
    //
    // A double click on the block resizes it, and only the half that was
    // clicked. A click anywhere else — the map, a sidebar, another panel —
    // leaves both exactly as they stand. Double rather than single, as on the
    // SWARM cards: a single click in the log is how a line is selected, and
    // it must not also throw the log to another size.
    //
    // Tested against the rectangle just drawn rather than through an ImGui
    // item: the log is a scrolling child with a scrollbar of its own, and an
    // invisible button over it would have to win the click that scrolls it.
    // Reading the mouse position instead leaves both halves fully usable while
    // expanded, and costs a frame of latency nobody can see.
    //
    // Skipped whenever a widget took the press — the log's scrollbar, the one
    // other thing in here that answers the mouse.
    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemActive()) {
        const ImVec2 m = ImGui::GetMousePos();
        const bool inside = m.x >= ov_p0.x && m.x <= ov_p0.x + w &&
                            m.y >= ov_p0.y && m.y <= ov_p0.y + h;
        if (inside) {
            if (m.y < ov_p0.y + PAD + hud_h) s_hud_big = !s_hud_big;
            else                             s_log_big = !s_log_big;

            // The press that resized the block also started a selection drag
            // in the log. With the button still down, the lines move under the
            // pointer as the block changes size, and the drag would sweep them
            // all into the selection. The click has picked its line; the drag
            // ends here.
            g_log_sel_dragging = false;
        }
    }
}
