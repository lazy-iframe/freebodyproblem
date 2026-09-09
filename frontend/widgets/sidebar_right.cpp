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


#include "sidebar_right.hpp"
#include "center_view.hpp"
#include "vehicle_ui_state.hpp"
#include "layout.hpp"
#include "theme.hpp"
#include "ui_kit.hpp"
#include "imgui.h"
#include "sidebar_left/mavlink_field_catalog_generated.hpp"
#include "../settings.hpp"
#include <vector>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <limits>
#include <algorithm>
#include <unordered_set>
#include <chrono>
#include <deque>
#include <cctype>
#include <ctime>
#include <string>

// Height of one telemetry tile in the 3×3 grid.
static constexpr float GRID_CELL_H = 66.0f;

#define _USE_MATH_DEFINES
#include <cmath>

// ── Artificial horizon (local to this TU) ────────────────────────────────────

// `text_size` overrides the size the labels are drawn at; 0 keeps the current
// font's own. The ball scales with `size` but its lettering does not, so at the
// diameter the fullscreen map's overlay asks for, sidebar-sized text fills the
// instrument instead of labelling it. Everything is drawn through the draw
// list anyway, so an arbitrary size costs nothing but the argument.
static void draw_artificial_horizon(float roll_deg, float pitch_deg, float yaw_deg,
                                     float airspeed_ms, int throttle_pct, bool has_vfr,
                                     float size, float text_size = 0.0f)
{
    ImDrawList* dl   = ImGui::GetWindowDrawList();
    const ImVec2 p0  = ImGui::GetCursorScreenPos();
    const float  cx  = p0.x + size * 0.5f;
    const float  cy  = p0.y + size * 0.5f;
    const float  r   = size * 0.45f;

    // The mono micro font when a size is asked for: digits of one width are
    // what the label-width estimates below assume, and it stays crisp small.
    ImFont* const fnt = (text_size > 0.0f && g_font_micro) ? g_font_micro
                                                           : ImGui::GetFont();
    const float  ts  = (text_size > 0.0f) ? text_size : ImGui::GetFontSize();
    const float  fh  = ts;

    // Per-character advance. The constants below were measured against the
    // default font at its own size, so they scale with the ratio — at
    // text_size 0 this is exactly 1 and nothing about the old sizing moves.
    const float  k   = ts / ImGui::GetFontSize();

    const float roll_rad = roll_deg  * (float)M_PI / 180.0f;
    const float pitch_px = pitch_deg * (r / 45.0f);

    const float ux  = -sinf(roll_rad);
    const float uy  = -cosf(roll_rad);
    const float ldx = -uy, ldy = ux;

    const float hx = cx - ux * pitch_px;
    const float hy = cy - uy * pitch_px;

    dl->PushClipRect({ cx - r, cy - r }, { cx + r, cy + r }, true);

    dl->AddCircleFilled({ cx, cy }, r, ah_sky());

    {
        const float dx_c = hx - cx, dy_c = hy - cy;
        const float b    = dx_c * ldx + dy_c * ldy;
        const float disc = b * b - (dx_c * dx_c + dy_c * dy_c - r * r);

        if (disc > 0.0f) {
            const float sq  = sqrtf(disc);
            const float px1 = hx + (-b - sq) * ldx, py1 = hy + (-b - sq) * ldy;
            const float px2 = hx + (-b + sq) * ldx, py2 = hy + (-b + sq) * ldy;

            const float a1 = atan2f(py1 - cy, px1 - cx);
            const float a2 = atan2f(py2 - cy, px2 - cx);
            const float a2_ccw = (a2 < a1) ? (a2 + 2.0f * (float)M_PI) : a2;

            const float amid  = (a1 + a2_ccw) * 0.5f;
            const float mid_wx = cx + cosf(amid) * r;
            const float mid_wy = cy + sinf(amid) * r;
            const bool ccw_is_ground =
                (mid_wx - hx) * (-ux) + (mid_wy - hy) * (-uy) > 0.0f;

            if (ccw_is_ground) {
                dl->PathLineTo({ px2, py2 });
                dl->PathLineTo({ px1, py1 });
                dl->PathArcTo({ cx, cy }, r, a1, a2_ccw, 40);
            } else {
                dl->PathLineTo({ px1, py1 });
                dl->PathLineTo({ px2, py2 });
                dl->PathArcTo({ cx, cy }, r, a2_ccw, a1 + 2.0f * (float)M_PI, 40);
            }
            dl->PathFillConvex(ah_ground());
            dl->AddLine({ px1, py1 }, { px2, py2 }, ah_horizon_line(), AH_THICK_HORIZON);

        } else {
            const float center_dot = (cx - hx) * (-ux) + (cy - hy) * (-uy);
            if (center_dot > 0.0f)
                dl->AddCircleFilled({ cx, cy }, r, ah_ground());
        }
    }

    static const int marks[] = { -30, -20, -10, -5, 5, 10, 20, 30 };
    for (int deg : marks) {
        const float disp = ((float)deg - pitch_deg) * (r / 45.0f);
        if (fabsf(disp) > r * 0.92f) continue;
        const float mx   = cx + ux * disp;
        const float my   = cy + uy * disp;
        const float hl   = (deg == 5 || deg == -5) ? r * 0.12f : r * 0.22f;
        const float thick = (deg % 10 == 0) ? AH_THICK_PITCH_BOLD : AH_THICK_PITCH_THIN;
        dl->AddLine({ mx + ldx * hl, my + ldy * hl },
                    { mx - ldx * hl, my - ldy * hl },
                    ah_pitch_ladder(), thick);

        if (deg != 5 && deg != -5) {
            char buf[8];
            snprintf(buf, sizeof(buf), "%d", deg);
            const float tw  = (float)strlen(buf) * 6.0f * k;
            const float lx  = mx + ldx * (hl + r * 0.04f);
            const float ly  = my + ldy * (hl + r * 0.04f);
            dl->AddRectFilled({ lx - 1, ly - 1 }, { lx + tw + 1, ly + fh + 1 },
                              ah_pitch_label_bg());
            dl->AddText(fnt, ts, { lx, ly - fh * 0.5f + 1 }, ah_pitch_label(), buf);
        }
    }

    const float ch  = r * 0.16f, gap = r * 0.06f;
    dl->AddLine({ cx - ch - gap, cy }, { cx - gap, cy }, ah_crosshair(), AH_THICK_CROSSHAIR);
    dl->AddLine({ cx + gap, cy }, { cx + ch + gap, cy }, ah_crosshair(), AH_THICK_CROSSHAIR);
    dl->AddCircleFilled({ cx, cy }, 3.0f, ah_crosshair());

    if (has_vfr) {
        char as_buf[16];
        snprintf(as_buf, sizeof(as_buf), "%.1f m/s", (double)airspeed_ms);
        const float tw = (float)strlen(as_buf) * 6.0f * k + 6.0f * k;
        const float bx = cx - r * 0.60f - tw * 0.5f;
        const float by = cy - r * 0.62f;
        dl->AddRectFilled({ bx - 2, by - 2 }, { bx + tw, by + fh + 2 },
                          ah_overlay_bg(), 2.0f);
        dl->AddText(fnt, ts, { bx, by }, ah_airspeed_text(), as_buf);

        char thr_buf[8];
        snprintf(thr_buf, sizeof(thr_buf), "%d%%", throttle_pct);
        const float tw2 = (float)strlen(thr_buf) * 6.0f * k + 6.0f * k;
        const float bx2 = cx + r * 0.35f;
        const float by2 = cy - r * 0.62f;
        dl->AddRectFilled({ bx2 - 2, by2 - 2 }, { bx2 + tw2, by2 + fh + 2 },
                          ah_overlay_bg(), 2.0f);
        dl->AddText(fnt, ts, { bx2, by2 }, ah_throttle_text(), thr_buf);
    }

    {
        char hdg_buf[12];
        snprintf(hdg_buf, sizeof(hdg_buf), "%.0f\xc2\xb0", (double)yaw_deg);
        const float tw  = (float)strlen(hdg_buf) * 6.0f * k + 6.0f * k;
        const float bx  = cx - tw * 0.5f;
        const float by  = cy + r * 0.65f;
        dl->AddRectFilled({ bx - 2, by - 2 }, { bx + tw, by + fh + 2 },
                          ah_overlay_bg(), 2.0f);
        dl->AddText(fnt, ts, { bx, by }, ah_heading_text(), hdg_buf);
    }

    dl->PopClipRect();

    {
        const float arc_r = r * 1.08f;
        const float base  = -(float)M_PI / 2.0f;

        // A full ring rather than the ±60° span this used to draw. Past 60° of
        // roll the pointer used to run off the end of its own scale and sit
        // against nothing, which is exactly the attitude where the number is
        // worth reading. A closed ring has graduation under the pointer at any
        // angle the vehicle can reach.
        dl->AddCircle({ cx, cy }, arc_r, ah_roll_arc(), 64, AH_THICK_ROLL_ARC);

        // Graduated every 15° the whole way round, so a tick is never more than
        // 7.5° from the pointer. Lengths carry the reading: the cardinals
        // (wings level, and inverted) longest, every 30° next, the 15°
        // subdivisions shortest.
        for (int deg = -180; deg < 180; deg += 15) {
            const bool cardinal = (deg == 0 || deg == 180 || deg == -180);
            const bool major    = (deg % 30 == 0);

            const float a   = base + (float)deg * (float)M_PI / 180.0f;
            const float len = cardinal ? r * 0.13f
                            : major    ? r * 0.09f
                                       : r * 0.05f;
            dl->AddLine({ cx + cosf(a) * arc_r,           cy + sinf(a) * arc_r },
                        { cx + cosf(a) * (arc_r - len),   cy + sinf(a) * (arc_r - len) },
                        ah_roll_tick(), AH_THICK_ROLL_TICK);

            // Every 30° gets its number, bar wings-level and inverted: those
            // two are what the long ticks say on their own.
            //
            // The numbers ride closer to the ring than they used to. ±90 is
            // where the scale is widest — the label goes straight out sideways
            // with none of the angle to shorten its reach — and at the old
            // radius it landed past the edge of the panel the instrument sits
            // in. Pulling the label ring in clears it at both HUD sizes and
            // leaves the instrument itself the size it was.
            if (deg != 0 && major && deg > -180 && deg < 180) {
                const float label_r = arc_r + r * 0.07f;
                char buf[8];
                snprintf(buf, sizeof(buf), "%d", (int)fabsf((float)deg));
                const float tw = (float)strlen(buf) * 5.5f * k;
                const float tx = cx + cosf(a) * label_r - tw * 0.5f;
                const float ty = cy + sinf(a) * label_r - fh * 0.5f;
                dl->AddText(fnt, ts, { tx, ty }, ah_roll_label(), buf);
            }
        }

        const float a   = base + roll_deg * (float)M_PI / 180.0f;
        const float ca  = cosf(a), sa = sinf(a);
        const float cp  = cosf(a + (float)M_PI / 2.0f);
        const float sp  = sinf(a + (float)M_PI / 2.0f);
        const float tip = arc_r + r * 0.04f, br = arc_r - r * 0.09f, hw = r * 0.04f;
        dl->AddTriangleFilled(
            { cx + ca * tip,          cy + sa * tip          },
            { cx + ca * br + cp * hw, cy + sa * br + sp * hw },
            { cx + ca * br - cp * hw, cy + sa * br - sp * hw },
            ah_roll_pointer());
    }

    dl->AddCircle({ cx, cy }, r, ah_border(), 64, AH_THICK_BORDER);

    ImGui::Dummy({ size, size });
}

// ── Shared HUD / log pieces ───────────────────────────────────────────────────
//
// The sidebar and the fullscreen-map overlay show the same two things in
// different sizes. These are the parts they share, so the pair cannot drift.

// Ground speed, heading and climb rate as three readouts on one row, drawn at
// the cursor and consuming `strip_h` of vertical space.
//
// `value_size` is the number's size, not the label's. The fullscreen map's
// overlay asks for a smaller one: at the sidebar's size three of these numbers
// nearly meet across a block a third the sidebar's width, and a readout that
// touches its neighbour reads as one long number.
static void draw_vfr_strip(const VehicleState& vs, float strip_h,
                           float value_size = UI_SZ_BODY + 2.0f)
{
    ImDrawList*  dl = ImGui::GetWindowDrawList();
    const ImVec2 s0 = ImGui::GetCursorScreenPos();
    const float  w  = ImGui::GetContentRegionAvail().x;
    const float  cw = w / 3.0f;

    struct Item { const char* label; char value[16]; };
    Item items[3] = { { "GND m/s", "--" }, { "HDG", "--" }, { "CLB m/s", "--" } };
    if (vs.has_vfr) {
        snprintf(items[0].value, sizeof(items[0].value), "%.1f", (double)vs.groundspeed);
        snprintf(items[1].value, sizeof(items[1].value), "%03d\xc2\xb0", (int)vs.heading);
        snprintf(items[2].value, sizeof(items[2].value), "%+.1f", (double)vs.climb);
    }

    for (int i = 0; i < 3; ++i) {
        const ImVec2 p0 = { s0.x + i * cw, s0.y };
        const ImVec2 p1 = { p0.x + cw,     s0.y + strip_h };
        ui_readout(dl, p0, p1, items[i].label, items[i].value,
                   vs.has_vfr ? ui_col_value() : ui_col(g_theme.col_no_link_muted),
                   value_size);
        if (i > 0)
            dl->AddLine({ p0.x, p0.y + 4.0f }, { p0.x, p1.y - 4.0f },
                        ui_col(g_theme.separator, 0.6f), 1.0f);
    }
    ImGui::Dummy({ w, strip_h });
}

// ── Fleet event log ───────────────────────────────────────────────────────────
//
// One log for the whole fleet rather than one per vehicle. A STATUSTEXT is the
// vehicle saying something went wrong, and the aircraft that most needs saying
// so is rarely the one already on screen — a per-vehicle log hides exactly the
// message the operator needed to see. So this does not follow the callsign
// chip: switching vehicles leaves it alone.

struct FleetLogEntry {
    uint32_t    number;     // Fleet's display number for the vehicle that spoke
    uint8_t     severity;
    std::string stamp;      // wall clock at the moment it was taken in
    std::string text;
};

// Bounded for the same reason the per-vehicle deque behind it is: a talkative
// airframe must not grow this without limit over a long flight. Deeper than one
// vehicle's 200, since several vehicles now share it.
static constexpr size_t FLEET_LOG_MAX = 800;

static std::deque<FleetLogEntry> g_fleet_log;

// Which vehicles have actually said something. The "[2]" prefix is only drawn
// once this holds more than one — with a single aircraft talking there is
// nothing to tell apart, and the prefix would be noise on every line.
static std::unordered_set<uint32_t> g_fleet_log_numbers;

void event_log_pump(const VehicleState& vs,
                    const std::vector<StatusText>& status_texts,
                    uint32_t number)
{
    // How many of this vehicle's messages have already been taken in. Per
    // vehicle, and dropped with it — see widgets/vehicle_ui_state.hpp.
    static VehicleUiState<uint32_t> s_ingested;
    uint32_t& ingested = *s_ingested;

    // Counted rather than diffed against the list. The vehicle keeps only its
    // last 200 messages and drops the oldest to make room, so the list's length
    // stops being a count of anything the moment it fills up; statustext_total
    // is the number the vehicle has ever sent and only goes up.
    const uint32_t total = vs.statustext_total;
    if (total == ingested) return;
    if (total < ingested) ingested = 0;   // vehicle replaced under the same id

    // Anything older than what the vehicle still holds is gone for good — this
    // takes what is left of the backlog and accepts the gap.
    const size_t n_new = std::min<size_t>(total - ingested, status_texts.size());

    char stamp[16];
    {
        const std::time_t t = std::time(nullptr);
        std::tm            lt{};
#ifdef _WIN32
        localtime_s(&lt, &t);
#else
        localtime_r(&t, &lt);
#endif
        snprintf(stamp, sizeof(stamp), "%02d:%02d:%02d",
                 lt.tm_hour, lt.tm_min, lt.tm_sec);
    }

    for (size_t i = status_texts.size() - n_new; i < status_texts.size(); ++i) {
        g_fleet_log.push_back(FleetLogEntry{ number, status_texts[i].severity,
                                             stamp, status_texts[i].text });
        if (g_fleet_log.size() > FLEET_LOG_MAX) g_fleet_log.pop_front();
    }
    if (n_new > 0) g_fleet_log_numbers.insert(number);

    ingested = total;
}

// The vehicle tag, "[2]", in the accent colour and thickened.
//
// Drawn twice a fraction of a pixel apart rather than in a bold face: the app
// ships Regular weights only, and at 13 px this reads as bold without a second
// font in the atlas. Submitted as an ImGui item so it sits in the line's normal
// left-to-right flow with the stamp and the message.
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

// The scrolling body of the event log: a wall-clock stamp per vehicle message,
// severity-coloured, pinned to the bottom while the view is already there.
// `bg_alpha` scales the well behind it, so the overlay can let the map through.
static void draw_event_log_body(float bg_alpha, bool small_text = false)
{
    // Only worth tagging lines once more than one vehicle has spoken.
    const bool tag_vehicles = g_fleet_log_numbers.size() > 1;

    ImGui::PushStyleColor(ImGuiCol_ChildBg,
                          ui_col(g_theme.bg_child_darker, bg_alpha));
    const ImVec2 inner_sz = { 0.0f, ImGui::GetContentRegionAvail().y };
    if (ImGui::BeginChild("##sysmsg_scroll", inner_sz, false,
                          ImGuiWindowFlags_HorizontalScrollbar)) {
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
            for (const FleetLogEntry& e : g_fleet_log) {
                ImGui::PushStyleColor(ImGuiCol_Text, col_log());
                ImGui::TextUnformatted(e.stamp.c_str());
                ImGui::PopStyleColor();
                ImGui::SameLine(0, 8);
                if (tag_vehicles) {
                    draw_log_vehicle_tag(e.number);
                    ImGui::SameLine(0, 6);
                }
                ImGui::TextColored(col_status_severity(e.severity),
                                   "%s", e.text.c_str());
            }
            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                ImGui::SetScrollHereY(1.0f);
        }
        ImGui::PopStyleVar();
        if (pushed_font) ImGui::PopFont();
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
}

// ── Data grid state ───────────────────────────────────────────────────────────

struct GridCellConfig {
    uint32_t msg_id         = UINT32_MAX;  // UINT32_MAX → unconfigured
    char     field_name[64] = {};
    char     title[48]      = {};          // short label shown at bottom of cell
};

static GridCellConfig s_cells[9];
static bool           s_cells_loaded = false;  // have we pulled from AppSettings yet?

// Copy persistent settings → runtime cells (called once on first draw).
static void cells_load(const AppSettings& settings)
{
    for (int i = 0; i < 9; ++i) {
        const GridCellPersist& src = settings.grid_cells[i];
        s_cells[i].msg_id = src.msg_id;
        strncpy(s_cells[i].field_name, src.field_name.c_str(),
                sizeof(s_cells[i].field_name) - 1);
        s_cells[i].field_name[sizeof(s_cells[i].field_name) - 1] = '\0';
        strncpy(s_cells[i].title, src.title.c_str(),
                sizeof(s_cells[i].title) - 1);
        s_cells[i].title[sizeof(s_cells[i].title) - 1] = '\0';
    }
}

// Copy runtime cells → persistent settings and flush to disk.
static void cells_save(AppSettings& settings)
{
    for (int i = 0; i < 9; ++i) {
        settings.grid_cells[i].msg_id     = s_cells[i].msg_id;
        settings.grid_cells[i].field_name = s_cells[i].field_name;
        settings.grid_cells[i].title      = s_cells[i].title;
    }
    settings_save(settings);
}

// Message IDs already asked for with SET_MESSAGE_INTERVAL, so the panel does
// not re-spam a vehicle every frame while data is absent.
//
// Per vehicle, and it has to be: the record is of what *this* vehicle was
// asked, and a shared set would tell the panel it had already requested a
// stream from an aircraft it has never spoken to — leaving the cell blank for
// good. See widgets/vehicle_ui_state.hpp.
static VehicleUiState<std::unordered_set<uint32_t>> s_requested_msgs_state;
static std::unordered_set<uint32_t>& requested_msgs() { return *s_requested_msgs_state; }

// Picker state ─────────────────────────────────────────────────────────────────
static int  s_picker_cell        = -1;
static bool s_picker_needs_open  = false;
static bool s_picker_scroll_msgs = false;  // scroll msg list to selection once
static bool s_picker_scroll_flds = false;  // scroll field list to selection once

static char     s_picker_filter[128]     = {};
static uint32_t s_picker_sel_msg         = UINT32_MAX;
static char     s_picker_sel_field[64]   = {};
static char     s_picker_sel_label[48]   = {};

// ── Case-insensitive substring helper ────────────────────────────────────────

static bool ci_contains(const char* haystack, const char* needle_lower)
{
    if (!needle_lower[0]) return true;
    char h[128]; h[0] = '\0';
    size_t hl = strlen(haystack);
    for (size_t k = 0; k < hl && k < 127; ++k)
        h[k] = (char)tolower((unsigned char)haystack[k]);
    h[hl < 127 ? hl : 127] = '\0';
    return strstr(h, needle_lower) != nullptr;
}

// Build lowercase filter once per call ────────────────────────────────────────
static void make_lower(const char* src, char* dst, size_t dstsz)
{
    size_t n = strlen(src);
    if (n >= dstsz) n = dstsz - 1;
    for (size_t k = 0; k < n; ++k)
        dst[k] = (char)tolower((unsigned char)src[k]);
    dst[n] = '\0';
}

// ── Picker helpers ────────────────────────────────────────────────────────────

// Returns unique message IDs whose fields match the filter.
// When filter is empty, returns all messages.
// When filter is set, searches field labels (not message names).
static std::vector<uint32_t> picker_unique_msgs(const char* filter)
{
    char flt[128] = {};
    make_lower(filter, flt, sizeof(flt));

    std::vector<uint32_t> result;
    uint32_t prev_id = UINT32_MAX;

    for (size_t i = 0; i < kMavlinkFieldCatalogCount; ++i) {
        const MavlinkFieldDesc& fd = kMavlinkFieldCatalog[i];

        if (fd.msg_id == prev_id) {
            // We already added this message — still need to check if this entry
            // is the reason it should appear (for filter matching we just continue,
            // the message is already in the list once any field matched).
            continue;
        }

        bool match;
        if (!flt[0]) {
            match = true;
        } else {
            // Scan all fields of this message for a label match
            match = false;
            for (size_t j = i; j < kMavlinkFieldCatalogCount; ++j) {
                if (kMavlinkFieldCatalog[j].msg_id != fd.msg_id) break;
                if (ci_contains(kMavlinkFieldCatalog[j].field_name, flt) ||
                    ci_contains(kMavlinkFieldCatalog[j].label,      flt)) {
                    match = true;
                    break;
                }
            }
        }

        if (match) {
            result.push_back(fd.msg_id);
            prev_id = fd.msg_id;
        }
    }
    return result;
}

// Fields for a given msg_id, optionally filtered by label.
static std::vector<const MavlinkFieldDesc*> picker_fields_for_msg(uint32_t msg_id,
                                                                    const char* filter)
{
    char flt[128] = {};
    make_lower(filter, flt, sizeof(flt));

    std::vector<const MavlinkFieldDesc*> result;
    for (size_t i = 0; i < kMavlinkFieldCatalogCount; ++i) {
        const MavlinkFieldDesc& fd = kMavlinkFieldCatalog[i];
        if (fd.msg_id != msg_id) continue;
        if (flt[0]) {
            if (!ci_contains(fd.field_name, flt) && !ci_contains(fd.label, flt))
                continue;
        }
        result.push_back(&fd);
    }
    return result;
}

// ── Draw picker popup ─────────────────────────────────────────────────────────

static void draw_picker_popup(const VehicleState* vs, MavlinkSender* sender,
                               AppSettings* settings)
{
    ui_push_dialog_style();
    if (!ImGui::BeginPopupModal("##field_picker", nullptr,
                                ImGuiWindowFlags_AlwaysAutoResize |
                                ImGuiWindowFlags_NoTitleBar)) {
        ui_pop_dialog_style();
        return;
    }

    const float panel_h = 300.0f;
    const float left_w  = 190.0f;
    const float right_w = 230.0f;
    const float body_w  = left_w + right_w + 6.0f;

    char title_buf[48];
    snprintf(title_buf, sizeof(title_buf), "ASSIGN TELEMETRY \xe2\x80\x94 TILE %d",
             s_picker_cell + 1);
    ui_dialog_title(title_buf, body_w);

    // Filter — searches field names / labels.
    // Auto-focus on the first frame the popup appears so typing works immediately.
    if (ImGui::IsWindowAppearing())
        ImGui::SetKeyboardFocusHere();
    ImGui::SetNextItemWidth(body_w);
    bool filter_changed = ImGui::InputTextWithHint("##filter", "SEARCH FIELDS\xe2\x80\xa6",
                                                   s_picker_filter,
                                                   sizeof(s_picker_filter));
    ImGui::Spacing();

    // If the filter changed and the currently selected message no longer has
    // matching fields, clear the message selection so the user sees a fresh list.
    if (filter_changed && s_picker_sel_msg != UINT32_MAX) {
        const auto fields = picker_fields_for_msg(s_picker_sel_msg, s_picker_filter);
        if (fields.empty()) {
            s_picker_sel_msg      = UINT32_MAX;
            s_picker_sel_field[0] = '\0';
            s_picker_sel_label[0] = '\0';
        }
    }

    // ── Left: message list ────────────────────────────────────────────────────
    ImGui::PushStyleColor(ImGuiCol_ChildBg, bg_child_darker());
    ImGui::BeginChild("##msg_list", { left_w, panel_h }, true);

    const std::vector<uint32_t> msg_ids = picker_unique_msgs(s_picker_filter);

    for (uint32_t mid : msg_ids) {
        const char* msg_name = nullptr;
        for (size_t i = 0; i < kMavlinkFieldCatalogCount; ++i) {
            if (kMavlinkFieldCatalog[i].msg_id == mid) {
                msg_name = kMavlinkFieldCatalog[i].msg_name;
                break;
            }
        }
        if (!msg_name) continue;

        const bool has_data = vs && vs->last_messages.count(mid) > 0;
        const bool selected = (mid == s_picker_sel_msg);

        // Scroll to the pre-selected item only once (on open / selection change).
        if (selected && s_picker_scroll_msgs) {
            ImGui::SetScrollHereY(0.5f);
            s_picker_scroll_msgs = false;
        }

        if (!has_data)
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.33f, 0.0f, 1.0f));

        if (ImGui::Selectable(msg_name, selected)) {
            if (s_picker_sel_msg != mid) {
                s_picker_sel_msg      = mid;
                s_picker_sel_field[0] = '\0';
                s_picker_sel_label[0] = '\0';
                s_picker_scroll_flds  = true;  // scroll field list to new selection
            }
        }

        if (!has_data) {
            ImGui::PopStyleColor();
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
                ImGui::BeginTooltip();
                ImGui::TextDisabled("Not receiving — will be requested on OK");
                ImGui::EndTooltip();
            }
        }
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::SameLine(0, 6);

    // ── Right: field list ─────────────────────────────────────────────────────
    ImGui::PushStyleColor(ImGuiCol_ChildBg, bg_child_darker());
    ImGui::BeginChild("##field_list", { right_w, panel_h }, true);

    if (s_picker_sel_msg == UINT32_MAX) {
        ImGui::TextDisabled("Select a message.");
    } else {
        const auto fields = picker_fields_for_msg(s_picker_sel_msg, s_picker_filter);
        if (fields.empty()) {
            ImGui::TextDisabled("No matches.");
        } else {
            for (const MavlinkFieldDesc* fd : fields) {
                const bool selected = (strcmp(fd->field_name, s_picker_sel_field) == 0);

                // Scroll to the pre-selected field only once.
                if (selected && s_picker_scroll_flds) {
                    ImGui::SetScrollHereY(0.5f);
                    s_picker_scroll_flds = false;
                }

                if (ImGui::Selectable(fd->label, selected)) {
                    strncpy(s_picker_sel_field, fd->field_name,
                            sizeof(s_picker_sel_field) - 1);
                    s_picker_sel_field[sizeof(s_picker_sel_field) - 1] = '\0';
                    strncpy(s_picker_sel_label, fd->field_name,
                            sizeof(s_picker_sel_label) - 1);
                    s_picker_sel_label[sizeof(s_picker_sel_label) - 1] = '\0';
                }
            }
        }
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Separator, col_separator());
    ImGui::Separator();
    ImGui::PopStyleColor();
    ImGui::Spacing();

    const bool can_ok = (s_picker_sel_msg != UINT32_MAX &&
                         s_picker_sel_field[0] != '\0');

    // Centred [ASSIGN][CLEAR][CANCEL] row
    constexpr float BW = 110.0f, BGAP = 8.0f;
    ui_dialog_row(BW * 3.0f + BGAP * 2.0f);

    if (!can_ok) ImGui::PushStyleVar(ImGuiStyleVar_Alpha,
                                      ImGui::GetStyle().Alpha * 0.4f);

    const bool hit_assign = ui_solid_button("ASSIGN", { BW, UI_DIALOG_BH },
                                            btn_write_base(), btn_write_hov());

    if (hit_assign && can_ok) {
        GridCellConfig& cell = s_cells[s_picker_cell];
        cell.msg_id = s_picker_sel_msg;
        strncpy(cell.field_name, s_picker_sel_field, sizeof(cell.field_name) - 1);
        cell.field_name[sizeof(cell.field_name) - 1] = '\0';
        strncpy(cell.title, s_picker_sel_label, sizeof(cell.title) - 1);
        cell.title[sizeof(cell.title) - 1] = '\0';

        // Request the message from the FC if it's not already streaming.
        if (vs && vs->last_messages.count(s_picker_sel_msg) == 0 &&
            sender && vs->has_heartbeat &&
            requested_msgs().find(s_picker_sel_msg) == requested_msgs().end())
        {
            sender->request_message_interval(vs->sysid, vs->compid,
                                              s_picker_sel_msg,
                                              200000 /* 5 Hz */);
            requested_msgs().insert(s_picker_sel_msg);
        }

        if (settings) cells_save(*settings);
        ImGui::CloseCurrentPopup();
    }

    if (!can_ok) ImGui::PopStyleVar();

    ImGui::SameLine(0, BGAP);
    const bool hit_clear = ui_grid_button("CLEAR", { BW, UI_DIALOG_BH });
    if (hit_clear) {
        s_cells[s_picker_cell].msg_id         = UINT32_MAX;
        s_cells[s_picker_cell].field_name[0]  = '\0';
        s_cells[s_picker_cell].title[0]       = '\0';
        if (settings) cells_save(*settings);
        ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine(0, BGAP);
    const bool hit_cancel = ui_grid_button("CANCEL", { BW, UI_DIALOG_BH });
    if (hit_cancel || ImGui::IsKeyPressed(ImGuiKey_Escape, false))
        ImGui::CloseCurrentPopup();

    ImGui::EndPopup();
    ui_pop_dialog_style();
}

// ── Draw 9-cell data grid ─────────────────────────────────────────────────────

static void draw_data_grid(const VehicleState* vs, MavlinkSender* sender,
                            AppSettings* settings)
{
    const float avail_w  = ImGui::GetContentRegionAvail().x;
    const float cell_w   = avail_w / 3.0f;
    const float cell_h   = GRID_CELL_H;
    const float pad      = 1.0f;

    ImDrawList*  dl     = ImGui::GetWindowDrawList();
    const ImVec2 origin = ImGui::GetCursorScreenPos();

    // "--" when data absent: muted link colour
    const ImU32 col_no_data = ui_col(g_theme.col_no_link_muted);
    // Live value text: standard data colour
    const ImU32 col_val     = ui_col_value();

    for (int idx = 0; idx < 9; ++idx) {
        const int row = idx / 3;
        const int col = idx % 3;

        const ImVec2 p0 = { origin.x + col * cell_w + pad,
                            origin.y + row * cell_h + pad };
        const ImVec2 p1 = { p0.x + cell_w - pad * 2.0f,
                            p0.y + cell_h - pad * 2.0f };

        ImGui::SetCursorScreenPos(p0);
        char btn_id[16]; snprintf(btn_id, sizeof(btn_id), "##gc%d", idx);
        ImGui::InvisibleButton(btn_id, { cell_w - pad * 2.0f, cell_h - pad * 2.0f });
        const bool hovered = ImGui::IsItemHovered();
        const bool clicked = ImGui::IsItemClicked();

        const GridCellConfig& cell = s_cells[idx];

        if (cell.msg_id == UINT32_MAX) {
            // Empty slot — well with a dim "+ ASSIGN" hint
            dl->AddRectFilled(p0, p1, hovered ? ui_col_strip() : ui_col_well());
            ui_frame(dl, p0, p1, hovered ? ui_col_accent() : ui_col(g_theme.separator, 0.5f));
            const ImU32 col_hint = ui_col(g_theme.col_no_link_muted);
            const float tw = ui_tracked_width(g_font_micro, UI_SZ_MICRO, "+ ASSIGN");
            ui_tracked_text(dl, g_font_micro, UI_SZ_MICRO,
                            { p0.x + (p1.x - p0.x - tw) * 0.5f,
                              p0.y + (p1.y - p0.y - UI_SZ_MICRO) * 0.5f },
                            hovered ? ui_col_accent() : col_hint, "+ ASSIGN");
        } else {
            // Read current value
            double  value    = std::numeric_limits<double>::quiet_NaN();
            bool    has_data = false;
            if (vs) {
                auto it = vs->last_messages.find(cell.msg_id);
                if (it != vs->last_messages.end())
                    has_data = mavlink_field_read(cell.msg_id, cell.field_name,
                                                  it->second, &value);
            }

            // ── Value + label, drawn as a tactical readout tile ───────────
            char val_buf[32];
            if (has_data && !std::isnan(value) && !std::isinf(value))
                snprintf(val_buf, sizeof(val_buf), "%.4g", value);
            else
                snprintf(val_buf, sizeof(val_buf), "--");

            // Field labels are lowercase in the catalog — uppercase them so the
            // tile matches the rest of the chrome.
            char lbl_buf[32];
            size_t li = 0;
            for (const char* c = cell.title; *c && li < sizeof(lbl_buf) - 1; ++c)
                lbl_buf[li++] = (char)toupper((unsigned char)*c);
            lbl_buf[li] = '\0';

            ui_readout_tile(dl, p0, p1, lbl_buf, val_buf,
                            has_data ? col_val : col_no_data, hovered);

            // If this message is configured but not yet received, request it once.
            if (!has_data && sender && vs && vs->has_heartbeat &&
                requested_msgs().find(cell.msg_id) == requested_msgs().end())
            {
                sender->request_message_interval(vs->sysid, vs->compid,
                                                  cell.msg_id, 200000 /* 5 Hz */);
                requested_msgs().insert(cell.msg_id);
            }
        }

        if (clicked) {
            s_picker_cell        = idx;
            s_picker_needs_open  = true;
            s_picker_scroll_msgs = true;   // auto-scroll to pre-selection once on open
            s_picker_scroll_flds = true;
            s_picker_filter[0]   = '\0';

            // Pre-select current cell
            if (cell.msg_id != UINT32_MAX) {
                s_picker_sel_msg = cell.msg_id;
                strncpy(s_picker_sel_field, cell.field_name,
                        sizeof(s_picker_sel_field) - 1);
                s_picker_sel_field[sizeof(s_picker_sel_field) - 1] = '\0';
                strncpy(s_picker_sel_label, cell.title,
                        sizeof(s_picker_sel_label) - 1);
                s_picker_sel_label[sizeof(s_picker_sel_label) - 1] = '\0';
            } else {
                s_picker_sel_msg      = UINT32_MAX;
                s_picker_sel_field[0] = '\0';
                s_picker_sel_label[0] = '\0';
            }
        }
    }

    // Advance cursor below grid
    ImGui::SetCursorScreenPos({ origin.x, origin.y + 3.0f * cell_h });
    ImGui::Dummy({ avail_w, 0.0f });

    if (s_picker_needs_open) {
        ImGui::OpenPopup("##field_picker");
        s_picker_needs_open = false;
    }
    draw_picker_popup(vs, sender, settings);
}

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

        // HUD: ball + header + the readout strip beneath it
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

            const float strip_h = 40.0f;
            if (vs.has_attitude) {
                const ImVec2 avail = ImGui::GetContentRegionAvail();
                const float  hs    = std::min(avail.x, avail.y - strip_h);
                if (hs > 30.0f) {
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail.x - hs) * 0.5f);
                    draw_artificial_horizon(vs.roll, vs.pitch, vs.yaw,
                                            vs.airspeed,
                                            vs.has_vfr ? (int)vs.throttle : -1,
                                            vs.has_vfr, hs);
                }
            } else {
                const ImVec2 avail = ImGui::GetContentRegionAvail();
                ImGui::Dummy({ avail.x, std::max(0.0f, avail.y - strip_h) });
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() - avail.y * 0.5f);
                ImGui::TextDisabled("   NO ATTITUDE DATA");
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + avail.y * 0.5f
                                     - ImGui::GetTextLineHeight());
            }

            // ── GND / HDG / CLB strip ────────────────────────────────────
            draw_vfr_strip(vs, strip_h);
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
            ui_panel_header("EVENT LOG", log_meta);
            draw_event_log_body(1.0f);
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

void draw_map_overlay(const VehicleState& vs)
{
    float mx, my, mw, mh;
    center_view_map_rect(mx, my, mw, mh);
    if (mw < 240.0f || mh < 200.0f) return;   // no room to float anything

    constexpr float MARGIN = 10.0f;
    constexpr float PAD    = 8.0f;

    // How big each half is standing. Small is the resting state — the corner is
    // a glance, not a panel — and a click on a half toggles that half between
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
    const float     full_w = std::min(l.right_w, mw * 0.5f);

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

    const float w      = (hud_big || log_big) ? full_w : ball_small + PAD * 2.0f;
    const float ball_h = hud_big ? std::min(w - PAD * 2.0f, mh * 0.45f)
                                 : ball_small;

    // Lettering for the ball. The instrument is a bit over half the sidebar's
    // diameter but its labels do not shrink with it, so at the sidebar's size
    // the pitch ladder, the roll arc and the three readouts run into each
    // other. 10 px against the body font's 16 is what pulls them apart; the
    // expanded ball is the sidebar's size again and wants the sidebar's text,
    // which is what 0 asks for.
    const float hud_text_sz = hud_big ? 0.0f : 10.0f;

    // The GND / HDG / CLB numbers under it. Three of them share a row a third
    // of the sidebar's width, so the sidebar's 18 px leaves them nearly
    // touching; 13 px puts air between the columns. Expanded, the row is the
    // sidebar's width again and takes the sidebar's size back.
    const float strip_value_sz = hud_big ? UI_SZ_BODY + 2.0f : UI_SZ_MICRO;

    // Attitude is that square plus its header and the readout strip.
    const float hud_h  = ball_h + UI_HEADER_H + 40.0f + PAD;

    // The log comes down with the ball, so the corner stays a corner block
    // rather than becoming a log with an instrument on top.
    const float log_h   = log_big ? std::min(360.0f, mh * 0.50f)
                                  : std::min(150.0f, mh * 0.25f);
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

            constexpr float STRIP_H = 40.0f;
            if (vs.has_attitude) {
                const ImVec2 avail = ImGui::GetContentRegionAvail();
                const float  hs    = std::min(avail.x, avail.y - STRIP_H);
                if (hs > 30.0f) {
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail.x - hs) * 0.5f);
                    draw_artificial_horizon(vs.roll, vs.pitch, vs.yaw, vs.airspeed,
                                            vs.has_vfr ? (int)vs.throttle : -1,
                                            vs.has_vfr, hs, hud_text_sz);
                }
            } else {
                const ImVec2 avail = ImGui::GetContentRegionAvail();
                ImGui::Dummy({ avail.x, std::max(0.0f, avail.y - STRIP_H) });
            }
            draw_vfr_strip(vs, STRIP_H, strip_value_sz);
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();

        // ── Event log ────────────────────────────────────────────────────────
        ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32_BLACK_TRANS);
        if (ImGui::BeginChild("##ov_log", { 0.0f, 0.0f }, false,
                              ImGuiWindowFlags_NoScrollbar)) {
            char log_meta[24];
            snprintf(log_meta, sizeof(log_meta), "%d", (int)g_fleet_log.size());
            ui_panel_header("EVENT LOG", log_meta);
            draw_event_log_body(0.45f, /*small_text=*/!log_big);
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();
    }
    ImGui::End();

    ImGui::PopStyleVar(4);
    ImGui::PopStyleColor(2);

    // ── Sizing the halves ────────────────────────────────────────────────────
    //
    // Only a click on the block resizes it, and only the half that was clicked.
    // A click anywhere else — the map, a sidebar, another panel — leaves both
    // exactly as they stand.
    //
    // Tested against the rectangle just drawn rather than through an ImGui
    // item: the log is a scrolling child with a scrollbar of its own, and an
    // invisible button over it would have to win the click that scrolls it.
    // Reading the mouse position instead leaves both halves fully usable while
    // expanded, and costs a frame of latency nobody can see.
    //
    // Skipped whenever a widget took the press. Nothing in here is interactive
    // except that scrollbar, so this is what keeps a drag on it from toggling
    // the very log being scrolled — the one gesture that lands inside a half
    // without meaning "resize it".
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemActive()) {
        const ImVec2 m = ImGui::GetMousePos();
        const bool inside = m.x >= ov_p0.x && m.x <= ov_p0.x + w &&
                            m.y >= ov_p0.y && m.y <= ov_p0.y + h;
        if (inside) {
            if (m.y < ov_p0.y + PAD + hud_h) s_hud_big = !s_hud_big;
            else                             s_log_big = !s_log_big;
        }
    }
}
