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

// Right sidebar: the fleet event log — drawing it, and selecting and copying lines.

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

static void draw_log_vehicle_tag(uint32_t number)
{
    char tag[16];
    snprintf(tag, sizeof(tag), "[%u]", (unsigned)number);

    const ImVec2 p   = ImGui::GetCursorScreenPos();
    const ImU32  col = ui_col_accent();
    ImDrawList*  dl  = ImGui::GetWindowDrawList();
    dl->AddText(p, col, tag);
    dl->AddText({ p.x + 0.7f, p.y }, col, tag);

    ImGui::Dummy(ImGui::CalcTextSize(tag));
}

// A line across the log where one vehicle's story broke: a band the width of
// the log, tinted and edged in the error red, with the stamp and what happened.
// Loud on purpose — its whole job is to be seen while scrolling, so nothing
// above it is read as being about the vehicle as it is now. A reboot the
// operator asked for is amber instead: it is intent, and the red line follows
// when the vehicle actually restarts.
//
// The band is drawn by the caller, which alone knows how tall the line came out
// once wrapped; this draws the text and says which colour the band should be.
static ImVec4 log_marker_col(const FleetLogEntry& e)
{
    return (e.kind == FleetLogKind::RebootRequested) ? col_warning() : col_error();
}

// Indented clear of the band's solid left edge, which the caller draws at the
// window's edge, inside the padding the other lines start at.
static constexpr float LOG_MARKER_INDENT = 8.0f;

static void draw_log_marker(const FleetLogEntry& e)
{
    ImGui::Indent(LOG_MARKER_INDENT);
    ImGui::PushStyleColor(ImGuiCol_Text, log_marker_col(e));
    ImGui::TextUnformatted(e.stamp.c_str());
    ImGui::SameLine(0, 8);
    ImGui::Text("\xe2\x80\x94 %s \xe2\x80\x94", e.text.c_str());
    ImGui::PopStyleColor();
    ImGui::Unindent(LOG_MARKER_INDENT);
}

// ── Selection ─────────────────────────────────────────────────────────────────
//
// The log is drawn as coloured text, and ImGui text cannot be selected — so the
// log keeps a selection of its own, by whole line, which is what gets pasted
// into a report or a bug anyway. Click a line, drag or shift-click to extend,
// Ctrl+A for everything, Ctrl+C or the right-click menu to copy.
//
// One selection for both places the log is drawn: the sidebar and the
// fullscreen map's overlay are the same log, and never on screen together.
static uint64_t g_log_sel_anchor   = 0;   // seq of the line the selection began on; 0 = none
static uint64_t g_log_sel_end      = 0;   // seq of the line it extends to
bool     g_log_sel_dragging = false;

// A line as it reads on screen, for the clipboard.
static void log_line_append(std::string& out, const FleetLogEntry& e, bool tag_vehicles)
{
    out += e.stamp;
    out += "  ";
    if (e.kind == FleetLogKind::Assistant) {
        out += "[MCP] " + e.text;
    } else if (e.kind != FleetLogKind::Message) {
        out += "\xe2\x80\x94 " + e.text + " \xe2\x80\x94";
    } else {
        if (tag_vehicles) out += "[" + std::to_string(e.number) + "] ";
        out += e.text;
    }
    out += '\n';
}

// Positions of the selection's two ends, in order. False when there is no
// selection, or when either end has since dropped off the front of the log.
static bool log_selection_range(size_t& lo, size_t& hi)
{
    if (g_log_sel_anchor == 0) return false;
    bool have_a = false, have_b = false;
    size_t a = 0, b = 0;
    for (size_t i = 0; i < g_fleet_log.size(); ++i) {
        if (g_fleet_log[i].seq == g_log_sel_anchor) { a = i; have_a = true; }
        if (g_fleet_log[i].seq == g_log_sel_end)    { b = i; have_b = true; }
    }
    if (!have_a || !have_b) {
        g_log_sel_anchor = g_log_sel_end = 0;
        return false;
    }
    lo = std::min(a, b);
    hi = std::max(a, b);
    return true;
}

static void log_copy(size_t lo, size_t hi, bool tag_vehicles)
{
    std::string out;
    for (size_t i = lo; i <= hi && i < g_fleet_log.size(); ++i)
        log_line_append(out, g_fleet_log[i], tag_vehicles);
    ImGui::SetClipboardText(out.c_str());
}

// Empties the log as it stands. The cursors stay: they are what the log has
// already taken in, and the vehicles still hold those messages, so without them
// everything cleared would come straight back. New lines arrive as usual.
static void fleet_log_clear()
{
    g_fleet_log.clear();
    g_fleet_log_numbers.clear();
    g_log_sel_anchor = g_log_sel_end = 0;
    g_log_sel_dragging = false;
}

// The log's header strip, with its CLEAR button at the right end. One clear for
// every place the log is drawn, since they all show the same log.
void draw_event_log_header(const char* meta)
{
    constexpr float BTN_W = 56.0f, BTN_H = 19.0f, MARGIN = 3.0f;
    const ImVec2 p0    = ImGui::GetCursorScreenPos();
    const float  width = ImGui::GetContentRegionAvail().x;

    ui_panel_header("EVENT LOG", meta, BTN_W + MARGIN);
    const ImVec2 below = ImGui::GetCursorScreenPos();

    ImGui::SetCursorScreenPos({ p0.x + width - BTN_W - MARGIN,
                                p0.y + (UI_HEADER_H - BTN_H) * 0.5f });
    ImGui::BeginDisabled(g_fleet_log.empty());
    if (ui_solid_button("CLEAR##event_log", { BTN_W, BTN_H }, btn_disconnect_base(),
                        btn_disconnect_hov(), CmdFlashState::Normal, UI_SZ_MICRO))
        fleet_log_clear();
    ImGui::EndDisabled();
    ImGui::SetCursorScreenPos(below);
}

// The scrolling body of the event log: a wall-clock stamp per vehicle message,
// severity-coloured, pinned to the bottom while the view is already there.
// `bg_alpha` scales the well behind it, so the overlay can let the map through.
// `wrap` folds long lines onto the next under their own text rather than
// scrolling sideways. Every log on screen wraps: a sidebar or overlay is narrow
// enough that it would otherwise show the start of every message and hide the
// part that says what went wrong.
void draw_event_log_body(float bg_alpha, bool small_text,
                         bool wrap)
{
    // Only worth tagging lines once more than one vehicle has spoken.
    const bool tag_vehicles = g_fleet_log_numbers.size() > 1;

    ImGui::PushStyleColor(ImGuiCol_ChildBg,
                          ui_col(g_theme.bg_child_darker, bg_alpha));
    const ImVec2 inner_sz = { 0.0f, ImGui::GetContentRegionAvail().y };
    if (ImGui::BeginChild("##sysmsg_scroll", inner_sz, false,
                          wrap ? ImGuiWindowFlags_None
                               : ImGuiWindowFlags_HorizontalScrollbar)) {
        // The 13 px mono rather than a scaled body font: these lines are a
        // stamp and a message, and a mono face is what keeps the stamps in a
        // column when the block is small enough that they nearly touch.
        const bool pushed_font = small_text && g_font_micro;
        if (pushed_font) ImGui::PushFont(g_font_micro);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 6.0f, 3.0f });
        if (g_fleet_log.empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, col_no_link_muted());
            ImGui::TextUnformatted("AWAITING VEHICLE MESSAGES");
            ImGui::PopStyleColor();
        } else {
            size_t sel_lo = 0, sel_hi = 0;
            const bool has_sel = log_selection_range(sel_lo, sel_hi);

            const float  line_h  = ImGui::GetTextLineHeight();
            const float  gap     = ImGui::GetStyle().ItemSpacing.y;
            const float  mouse_y = ImGui::GetIO().MousePos.y;
            const ImVec2 win_pos = ImGui::GetWindowPos();
            const float  win_r   = win_pos.x + ImGui::GetWindowWidth();
            ImDrawList*  dl      = ImGui::GetWindowDrawList();

            // The line under the pointer, found while drawing since that is
            // where each line's position is known. The band a line owns takes
            // half the gap either side, so there is no dead strip between two.
            long hot = -1;

            // Wrapped, a line is as tall as it comes out, which is known only
            // once it is drawn — so the selection and marker bands behind it go
            // on a lower channel, drawn after the text but composited under it.
            // Wrap position 0 is the window's right edge; -1 turns it off. Plain
            // Text calls below rather than TextWrapped, which wraps regardless.
            ImGui::PushTextWrapPos(wrap ? 0.0f : -1.0f);
            dl->ChannelsSplit(2);

            for (size_t i = 0; i < g_fleet_log.size(); ++i) {
                const FleetLogEntry& e = g_fleet_log[i];
                const float y0 = ImGui::GetCursorScreenPos().y;

                dl->ChannelsSetCurrent(1);
                if (e.kind == FleetLogKind::Assistant) {
                    // The agent's lines: the stamp, an "MCP" tag in the plugin
                    // rail's blue, then the text in its severity colour.
                    ImGui::PushStyleColor(ImGuiCol_Text, col_log());
                    ImGui::TextUnformatted(e.stamp.c_str());
                    ImGui::PopStyleColor();
                    ImGui::SameLine(0, 8);
                    ImGui::PushStyleColor(ImGuiCol_Text, plugin_accent());
                    ImGui::TextUnformatted("MCP");
                    ImGui::PopStyleColor();
                    ImGui::SameLine(0, 6);
                    ImGui::PushStyleColor(ImGuiCol_Text, col_status_severity(e.severity));
                    ImGui::TextUnformatted(e.text.c_str());
                    ImGui::PopStyleColor();
                } else if (e.kind != FleetLogKind::Message) {
                    draw_log_marker(e);
                } else {
                    ImGui::PushStyleColor(ImGuiCol_Text, col_log());
                    ImGui::TextUnformatted(e.stamp.c_str());
                    ImGui::PopStyleColor();
                    ImGui::SameLine(0, 8);
                    if (tag_vehicles) {
                        draw_log_vehicle_tag(e.number);
                        ImGui::SameLine(0, 6);
                    }
                    ImGui::PushStyleColor(ImGuiCol_Text, col_status_severity(e.severity));
                    ImGui::TextUnformatted(e.text.c_str());
                    ImGui::PopStyleColor();
                }
                const float y1 = ImGui::GetItemRectMax().y;

                if (mouse_y >= y0 - gap * 0.5f && mouse_y < y1 + gap * 0.5f)
                    hot = (long)i;

                // Across the visible width however far the log is scrolled
                // sideways. A marker's band is loud on purpose — its whole job
                // is to be seen while scrolling — with a solid edge on the left.
                dl->ChannelsSetCurrent(0);
                if (e.kind != FleetLogKind::Message && e.kind != FleetLogKind::Assistant) {
                    const ImVec4 mc = log_marker_col(e);
                    dl->AddRectFilled({ win_pos.x, y0 - 2.0f }, { win_r, y1 + 2.0f },
                                      ui_col(mc, 0.16f));
                    dl->AddRectFilled({ win_pos.x, y0 - 2.0f }, { win_pos.x + 3.0f, y1 + 2.0f },
                                      ui_col(mc));
                }
                if (has_sel && i >= sel_lo && i <= sel_hi)
                    dl->AddRectFilled({ win_pos.x, y0 - gap * 0.5f },
                                      { win_r,     y1 + gap * 0.5f },
                                      ui_col(g_theme.accent, 0.22f));
            }

            dl->ChannelsMerge();
            ImGui::PopTextWrapPos();

            // ── Selecting ─────────────────────────────────────────────────
            const ImGuiIO& io      = ImGui::GetIO();
            const bool     hovered = ImGui::IsWindowHovered();

            if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                if (hot >= 0) {
                    const uint64_t seq = g_fleet_log[(size_t)hot].seq;
                    if (!(io.KeyShift && g_log_sel_anchor != 0)) g_log_sel_anchor = seq;
                    g_log_sel_end      = seq;
                    g_log_sel_dragging = true;
                } else {
                    g_log_sel_anchor = g_log_sel_end = 0;   // click past the last line
                }
            }
            if (g_log_sel_dragging) {
                if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                    g_log_sel_dragging = false;
                } else {
                    // Dragged past either edge: scroll that way, and extend to
                    // the line now at the edge.
                    const float top = win_pos.y, bot = win_pos.y + ImGui::GetWindowHeight();
                    if (mouse_y < top)
                        ImGui::SetScrollY(ImGui::GetScrollY() - line_h);
                    else if (mouse_y > bot)
                        ImGui::SetScrollY(ImGui::GetScrollY() + line_h);
                    if (hot >= 0) g_log_sel_end = g_fleet_log[(size_t)hot].seq;
                }
            }

            // Right-click on a line outside the selection selects that line
            // first, so the menu always acts on what the pointer is on.
            if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                if (hot >= 0 && !(has_sel && (size_t)hot >= sel_lo && (size_t)hot <= sel_hi))
                    g_log_sel_anchor = g_log_sel_end = g_fleet_log[(size_t)hot].seq;
                ImGui::OpenPopup("##log_ctx");
            }

            if (ImGui::IsWindowFocused()) {
                if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_A)) {
                    g_log_sel_anchor = g_fleet_log.front().seq;
                    g_log_sel_end    = g_fleet_log.back().seq;
                }
                if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_C) &&
                    log_selection_range(sel_lo, sel_hi))
                    log_copy(sel_lo, sel_hi, tag_vehicles);
                if (ImGui::IsKeyPressed(ImGuiKey_Escape))
                    g_log_sel_anchor = g_log_sel_end = 0;
            }

            if (ImGui::BeginPopup("##log_ctx")) {
                const bool sel_now = log_selection_range(sel_lo, sel_hi);
                if (ImGui::MenuItem("Copy", "Ctrl+C", false, sel_now))
                    log_copy(sel_lo, sel_hi, tag_vehicles);
                if (ImGui::MenuItem("Copy all"))
                    log_copy(0, g_fleet_log.size() - 1, tag_vehicles);
                ImGui::Separator();
                if (ImGui::MenuItem("Select all", "Ctrl+A")) {
                    g_log_sel_anchor = g_fleet_log.front().seq;
                    g_log_sel_end    = g_fleet_log.back().seq;
                }
                ImGui::EndPopup();
            }

            // Pinned to the newest line while the view is already there — but
            // not while a drag is scrolling it somewhere else.
            //
            // Whether it is there is remembered per window, across a resize or
            // a font change: the map overlay does both at once when it shrinks,
            // the wrapped lines reflow taller, and the old scroll offset is
            // suddenly far short of the bottom. Judged afresh that frame, the
            // log would let go of the newest line and sit somewhere in the
            // middle of the history.
            ImGuiStorage* st = ImGui::GetStateStorage();
            const ImGuiID k_pin = ImGui::GetID("##log_pinned");
            const ImGuiID k_w   = ImGui::GetID("##log_w");
            const ImGuiID k_h   = ImGui::GetID("##log_h");
            const ImGuiID k_fs  = ImGui::GetID("##log_fs");
            const ImVec2  ws    = ImGui::GetWindowSize();
            const float   fs    = ImGui::GetFontSize();
            const bool reflowed = st->GetFloat(k_w, -1.0f) != ws.x ||
                                  st->GetFloat(k_h, -1.0f) != ws.y ||
                                  st->GetFloat(k_fs, -1.0f) != fs;
            const bool pinned = reflowed ? st->GetBool(k_pin, true)
                                         : ImGui::GetScrollY() >= ImGui::GetScrollMaxY();
            if (!g_log_sel_dragging && pinned)
                ImGui::SetScrollHereY(1.0f);
            st->SetBool(k_pin, pinned);
            st->SetFloat(k_w, ws.x);
            st->SetFloat(k_h, ws.y);
            st->SetFloat(k_fs, fs);
        }
        ImGui::PopStyleVar();
        if (pushed_font) ImGui::PopFont();
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
}

} // namespace right_sidebar

using namespace right_sidebar;

void draw_event_log_panel(bool wrap)
{
    char log_meta[24];
    snprintf(log_meta, sizeof(log_meta), "%d ENTRIES", (int)g_fleet_log.size());
    draw_event_log_header(log_meta);
    draw_event_log_body(1.0f, /*small_text=*/false, wrap);
}
