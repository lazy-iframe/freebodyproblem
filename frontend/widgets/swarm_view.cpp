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

#include "swarm_view.hpp"
#include "layout.hpp"
#include "ui_kit.hpp"
#include "sidebar_left.hpp"    // FlightTarget, draw_flight_section, draw_ekf_bars
#include "sidebar_right.hpp"   // attitude ball, VFR strip, telemetry grid, event log
#include "topbar.hpp"          // mode_display_name, interlock_latched
#include "../../backend/firmware_profile.hpp"
#include "imgui.h"

#include <algorithm>
#include <cfloat>
#include <cstdio>
#include <string>
#include <unordered_set>
#include <vector>

// ── State ─────────────────────────────────────────────────────────────────────
//
// Keyed by VehicleId and pruned every frame against the fleet, so a vehicle
// that drops and is heard again comes back unchecked — a batch command must
// never reach an aircraft the operator did not pick since it reappeared.

static std::unordered_set<VehicleId, VehicleIdHash> s_checked;

// The one card drawn large, floating over its neighbours; invalid for none.
// One at a time: two expanded cards would cover each other.
static VehicleId s_expanded{};

bool swarm_is_checked(VehicleId id)
{
    return s_checked.count(id) != 0;
}

// ── Card geometry ─────────────────────────────────────────────────────────────
//
// A compact card is one grid cell; the column is as many cells wide as the
// fleet needs. The width is fixed — what a ball, a column of five readouts and
// six EKF bars with legible labels need side by side. The height is not: as
// many rows of at least CARD_MIN_H as the column holds, then stretched so they
// fill it exactly, rather than leaving a strip of dead space under the last
// row. An expanded card is two cells by two, drawn over the grid rather than
// in it, so expanding one moves nothing.

static constexpr float CARD_W      = 228.0f;
static constexpr float CARD_MIN_H  = 174.0f;
static constexpr float CARD_GAP    = 6.0f;
static constexpr float CARD_PAD    = 6.0f;
static constexpr float CARD_HEAD_H = 22.0f;
static constexpr float CHECK_SZ    = 14.0f;

static constexpr float PANEL_PAD   = 8.0f;
static constexpr float PANEL_SPC_Y = 4.0f;

// Above the cards in their column: padding, the FLEET header, the selection
// buttons and the spacing between them.
static constexpr float CARDS_CHROME_H = PANEL_PAD * 2.0f + UI_HEADER_H + 22.0f
                                      + PANEL_SPC_Y * 3.0f;

// This frame's layout and card columns, from swarm_view_layout().
// Zeroed until the first swarm_view_layout() — computing it here would read
// the ImGui IO before there is a context to read it from.
static SwarmLayout s_layout{};
static int         s_cols   = 1;
static int         s_rows   = 1;   // rows the column holds without scrolling

// This frame's card height, from the space the grid actually got.
static float       s_card_h = CARD_MIN_H;

static ImVec2 expanded_size()
{
    return { CARD_W * 2.0f + CARD_GAP, s_card_h * 2.0f + CARD_GAP };
}

// ── Helpers ───────────────────────────────────────────────────────────────────

static int flash_rank(CmdFlashState s)
{
    switch (s) {
    case CmdFlashState::Rejected: return 3;
    case CmdFlashState::Pending:  return 2;
    case CmdFlashState::Accepted: return 1;
    default:                      return 0;
    }
}

// What this vehicle made of the last flight command: the FLIGHT section's
// buttons show the fleet's loudest answer, and this is where the one vehicle
// that said no is found. DO_SET_MODE, DO_SET_STANDARD_MODE and NAV_TAKEOFF from
// that section; COMPONENT_ARM_DISARM and DO_AUX_FUNCTION (interlock) from the
// topbar.
static CmdFlashState card_flash(const MavlinkSender& s)
{
    CmdFlashState out = CmdFlashState::Normal;
    for (uint16_t cmd : { (uint16_t)176, (uint16_t)262, (uint16_t)22,
                          (uint16_t)400, (uint16_t)218 }) {
        const CmdFlashState f = s.query_flash(cmd);
        if (flash_rank(f) > flash_rank(out)) out = f;
    }
    return out;
}

// A micro label on the left and a value on the right of one row, the value
// clipped to the row rather than allowed to run into the next card.
static void kv_row(ImDrawList* dl, float x, float y, float w, float row_h,
                   const char* label, const char* value, ImU32 vcol, float vsize)
{
    ImFont* const fm = g_font_micro ? g_font_micro : ImGui::GetFont();
    ImFont* const fv = (vsize > UI_SZ_MICRO && g_font_ui) ? g_font_ui : fm;

    const float lw = ui_tracked_width(fm, UI_SZ_MICRO, label);
    ui_tracked_text(dl, fm, UI_SZ_MICRO, { x, y + (row_h - UI_SZ_MICRO) * 0.5f },
                    ui_col_label(), label);

    const ImVec2 vsz = fv->CalcTextSizeA(vsize, FLT_MAX, 0.0f, value);
    const float  vx  = std::max(x + lw + 6.0f, x + w - vsz.x);
    const ImVec4 clip = { x, y, x + w, y + row_h };
    dl->AddText(fv, vsize, { vx, y + (row_h - vsize) * 0.5f }, vcol, value,
                nullptr, 0.0f, &clip);
}

// The rows a card shows beside its ball. Compact cards take the first five.
struct Readout { const char* label; char value[32]; ImU32 col; };

static int card_readouts(const SwarmVehicle& v, Readout* out, bool expanded)
{
    const VehicleState& vs   = v.state;
    const ImU32         val  = ui_col_value();
    const ImU32         none = ui_col(g_theme.col_no_link_muted);
    int n = 0;

    auto row = [&](const char* label, ImU32 col) -> Readout& {
        Readout& r = out[n++];
        r.label    = label;
        r.col      = col;
        snprintf(r.value, sizeof(r.value), "--");
        return r;
    };

    {
        Readout& r = row("MODE", vs.has_heartbeat ? ui_col_accent() : none);
        if (vs.has_heartbeat)
            snprintf(r.value, sizeof(r.value), "%s", mode_display_name(vs).c_str());
    }
    {
        Readout& r = row("BATT", none);
        if (vs.has_sys_status) {
            if (vs.battery_remaining >= 0) {
                snprintf(r.value, sizeof(r.value), "%.1fV %d%%",
                         (double)vs.voltage_V, (int)vs.battery_remaining);
                r.col = ui_col(col_battery(vs.battery_remaining / 100.0f));
            } else {
                snprintf(r.value, sizeof(r.value), "%.1fV", (double)vs.voltage_V);
                r.col = val;
            }
        }
    }
    {
        Readout& r = row("GPS", none);
        if (vs.has_gps_raw) {
            if (vs.gps_fix_type >= 2)
                snprintf(r.value, sizeof(r.value), "FIX %d \xc2\xb7 %d",
                         (int)vs.gps_fix_type, (int)vs.satellites_vis);
            else
                snprintf(r.value, sizeof(r.value), "NO FIX");
            r.col = vs.gps_fix_type >= 3 ? val : ui_col(g_theme.col_warning);
        }
    }
    {
        Readout& r = row("ALT", vs.has_global_pos ? val : none);
        if (vs.has_global_pos)
            snprintf(r.value, sizeof(r.value), "%.1f m", (double)vs.alt_rel);
    }
    {
        Readout& r = row("GS", vs.has_vfr ? val : none);
        if (vs.has_vfr)
            snprintf(r.value, sizeof(r.value), "%.1f m/s", (double)vs.groundspeed);
    }
    if (!expanded) return n;

    // Expanded: the rest of what tells one aircraft's state from another's.
    {
        Readout& r = row("ASL", vs.has_global_pos ? val : none);
        if (vs.has_global_pos)
            snprintf(r.value, sizeof(r.value), "%.1f m", (double)vs.alt_asl);
    }
    {
        Readout& r = row("AIRSPD", vs.has_vfr ? val : none);
        if (vs.has_vfr)
            snprintf(r.value, sizeof(r.value), "%.1f m/s", (double)vs.airspeed);
    }
    {
        Readout& r = row("THR", vs.has_vfr ? val : none);
        if (vs.has_vfr)
            snprintf(r.value, sizeof(r.value), "%d%%", (int)vs.throttle);
    }
    {
        Readout& r = row("STACK", vs.has_heartbeat ? val : none);
        if (vs.has_heartbeat)
            snprintf(r.value, sizeof(r.value), "%s", firmware_profile(vs).name());
    }
    {
        Readout& r = row("ID", val);
        snprintf(r.value, sizeof(r.value), "SYS %u \xc2\xb7 COMP %u",
                 (unsigned)v.vehicle->sysid(), (unsigned)v.vehicle->compid());
    }
    return n;
}

// ── Card ──────────────────────────────────────────────────────────────────────

struct CardInput {
    bool toggle_check   = false;
    bool clicked        = false;
    bool double_clicked = false;
};

static CardInput draw_card(const SwarmVehicle& v, ImVec2 p0, ImVec2 size,
                           bool expanded, bool focused, bool checked)
{
    CardInput in;
    const VehicleState& vs = v.state;

    ImGui::SetCursorScreenPos(p0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { CARD_PAD, CARD_PAD });
    ImGui::PushStyleColor(ImGuiCol_ChildBg, g_theme.bg_child_darker);
    ImGui::BeginChild("##card", size, ImGuiChildFlags_None,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    ImDrawList*  dl = ImGui::GetWindowDrawList();
    const ImVec2 wp = ImGui::GetWindowPos();

    // The whole card is the select / expand target; the checkbox, submitted
    // after it, wins the overlap.
    ImGui::SetCursorScreenPos(wp);
    ImGui::SetNextItemAllowOverlap();
    in.clicked        = ImGui::InvisibleButton("##body", size);
    in.double_clicked = ImGui::IsItemHovered() &&
                        ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);

    // ── Header strip ──────────────────────────────────────────────────────────
    dl->AddRectFilled(wp, { wp.x + size.x, wp.y + CARD_HEAD_H }, ui_col_strip());

    {
        // Checkbox. The hit area is the whole left end of the strip, larger
        // than the box, because a missed checkbox lands on the body and
        // changes focus instead — harmless, but not what was meant.
        const ImVec2 hit0 = { wp.x, wp.y };
        ImGui::SetCursorScreenPos(hit0);
        if (ImGui::InvisibleButton("##chk", { CARD_PAD * 2.0f + CHECK_SZ, CARD_HEAD_H }))
            in.toggle_check = true;

        const ImVec2 b0 = { wp.x + CARD_PAD, wp.y + (CARD_HEAD_H - CHECK_SZ) * 0.5f };
        const ImVec2 b1 = { b0.x + CHECK_SZ, b0.y + CHECK_SZ };
        dl->AddRectFilled(b0, b1, ui_col_well());
        if (checked)
            dl->AddRectFilled({ b0.x + 3.0f, b0.y + 3.0f }, { b1.x - 3.0f, b1.y - 3.0f },
                              ui_col_accent());
        ui_frame(dl, b0, b1, checked ? ui_col_accent() : ui_col(g_theme.separator));
    }

    ImFont* const fm = g_font_micro ? g_font_micro : ImGui::GetFont();
    {
        char title[32];
        snprintf(title, sizeof(title), "#%u  SYS %u",
                 (unsigned)v.number, (unsigned)v.vehicle->sysid());
        ui_tracked_text(dl, fm, UI_SZ_MICRO,
                        { wp.x + CARD_PAD * 2.0f + CHECK_SZ + 2.0f,
                          wp.y + (CARD_HEAD_H - UI_SZ_MICRO) * 0.5f - 1.0f },
                        ui_col_accent(), title);

        // Link before arming: an aircraft that has gone quiet is LOST whatever
        // it last said about its motors.
        const char* tag;
        ImU32       tag_col;
        if (!vs.has_heartbeat)  { tag = "WAIT";     tag_col = ui_col(g_theme.col_no_link_muted); }
        else if (!v.online)     { tag = "LOST";     tag_col = ui_col(g_theme.col_error); }
        else if (vs.armed)      { tag = "ARMED";    tag_col = ui_col(g_theme.col_armed); }
        else                    { tag = "DISARMED"; tag_col = ui_col(g_theme.col_disarmed); }
        const float tw = ui_tracked_width(fm, UI_SZ_MICRO, tag);
        const float ty = wp.y + (CARD_HEAD_H - UI_SZ_MICRO) * 0.5f - 1.0f;
        ui_tracked_text(dl, fm, UI_SZ_MICRO, { wp.x + size.x - CARD_PAD - tw, ty },
                        tag_col, tag);

        // Interlock, which the vehicle never reports: this is the latch the
        // topbar's buttons set, shown only while it is HIGH.
        if (interlock_latched(v.vehicle->id())) {
            const char* ilk = "ILK";
            const float iw  = ui_tracked_width(fm, UI_SZ_MICRO, ilk);
            ui_tracked_text(dl, fm, UI_SZ_MICRO,
                            { wp.x + size.x - CARD_PAD - tw - 8.0f - iw, ty },
                            ui_col(g_theme.col_warning), ilk);
        }
    }

    // ── Body ──────────────────────────────────────────────────────────────────
    const float body_y = wp.y + CARD_HEAD_H + CARD_PAD;
    const float inner_w = size.x - CARD_PAD * 2.0f;

    // 40, as the sidebar and the map overlay use: ui_readout stacks its label
    // over its number, and any shorter the two run into each other.
    const float strip_h  = expanded ? 40.0f : 0.0f;
    const float ekf_bar  = expanded ? 40.0f : 22.0f;
    const float ekf_font = expanded ? ImGui::GetFontSize() : UI_SZ_MICRO;
    const float ekf_h    = ekf_bar + ekf_font + 4.0f;

    const float room_h = size.y - CARD_HEAD_H - CARD_PAD * 3.0f - ekf_h
                         - (expanded ? strip_h + CARD_PAD : 0.0f);
    const float ball   = std::max(40.0f, std::min(room_h,
                                                  inner_w * (expanded ? 0.52f : 0.46f)));

    // Attitude. Lettering shrunk with the ball, as the map overlay's is: at
    // sidebar size the ladder and the readouts run into each other.
    ImGui::SetCursorScreenPos({ wp.x + CARD_PAD, body_y });
    draw_attitude_ball(vs, ball, expanded ? 11.0f : 8.0f);

    // Readouts beside it.
    {
        Readout rows[12];
        const int   n     = card_readouts(v, rows, expanded);
        const float col_x = wp.x + CARD_PAD + ball + 8.0f;
        const float col_w = wp.x + size.x - CARD_PAD - col_x;
        const float row_h = ball / (float)n;
        const float vsize = expanded ? UI_SZ_BODY : 14.0f;
        for (int i = 0; i < n; ++i)
            kv_row(dl, col_x, body_y + i * row_h, col_w, row_h,
                   rows[i].label, rows[i].value, rows[i].col, vsize);
    }

    if (expanded) {
        ImGui::SetCursorScreenPos({ wp.x + CARD_PAD, body_y + ball + CARD_PAD });
        draw_vfr_readouts(vs, strip_h, UI_SZ_BODY);
    }

    // Estimator bars along the bottom edge.
    ImGui::SetCursorScreenPos({ wp.x + CARD_PAD, wp.y + size.y - CARD_PAD - ekf_h });
    draw_ekf_bars(vs, ekf_bar, !expanded);

    // ── Frame ─────────────────────────────────────────────────────────────────
    // A command the vehicle just answered, then which vehicle is selected —
    // the one the topbar, the telemetry grid and GO HERE are about — then a
    // link that has gone. Hovering, checking and expanding leave it alone:
    // across a screen of cards, frames lighting up under the mouse draw the
    // eye away from the ones that matter.
    ImU32 border;
    float thick = 1.0f;
    switch (card_flash(v.vehicle->sender())) {
    case CmdFlashState::Rejected: border = ui_col(g_theme.col_error);  thick = 2.0f; break;
    case CmdFlashState::Accepted: border = ui_col(g_theme.col_ok);     thick = 2.0f; break;
    case CmdFlashState::Pending:  border = ui_col_accent();            thick = 2.0f; break;
    default:
        if (focused)                       { border = ui_col(g_theme.col_ok);          thick = 2.0f; }
        else if (vs.has_heartbeat && !v.online) { border = ui_col(g_theme.col_error, 0.8f); thick = 1.5f; }
        else                               { border = ui_col(g_theme.separator); }
        break;
    }
    const float h = thick * 0.5f;
    dl->AddRect({ wp.x + h, wp.y + h }, { wp.x + size.x - h, wp.y + size.y - h },
                border, 0.0f, 0, thick);

    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    return in;
}

// ── Panels ────────────────────────────────────────────────────────────────────

static void begin_panel(const char* id, ImVec2 pos, ImVec2 size, ImVec4 bg,
                        bool scroll)
{
    ImGui::SetNextWindowPos (pos,  ImGuiCond_Always);
    ImGui::SetNextWindowSize(size, ImGuiCond_Always);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, bg);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,    { PANEL_PAD, PANEL_PAD });
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,      { 4.0f, PANEL_SPC_Y });
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,   0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,    FRAME_ROUNDING_MD);

    // Never raised by a click, so the expanded card — submitted last — stays
    // on top of whichever panel was clicked.
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoCollapse |
                             ImGuiWindowFlags_NoBringToFrontOnFocus;
    if (!scroll)
        flags |= ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    ImGui::Begin(id, nullptr, flags);

    // Seams on the right and bottom edges, where this panel meets the next.
    // Inside the window's own clip, as the sidebars draw theirs, so a popup
    // opened from a neighbouring panel is never crossed by one.
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float x1 = pos.x + size.x - PANEL_BORDER_THICKNESS * 0.5f;
    const float y1 = pos.y + size.y - PANEL_BORDER_THICKNESS * 0.5f;
    dl->AddLine({ x1, pos.y }, { x1, pos.y + size.y },
                col32_panel_border(), PANEL_BORDER_THICKNESS);
    dl->AddLine({ pos.x, y1 }, { pos.x + size.x, y1 },
                col32_panel_border(), PANEL_BORDER_THICKNESS);
}

static void end_panel()
{
    ImGui::End();
    ImGui::PopStyleVar(5);
    ImGui::PopStyleColor();
}

// ── Layout ────────────────────────────────────────────────────────────────────

void swarm_view_layout(const std::vector<SwarmVehicle>& fleet)
{
    // Forget vehicles that have gone. See the note on s_checked.
    {
        std::unordered_set<VehicleId, VehicleIdHash> live;
        for (const SwarmVehicle& v : fleet) live.insert(v.vehicle->id());
        for (auto it = s_checked.begin(); it != s_checked.end(); )
            it = live.count(*it) ? std::next(it) : s_checked.erase(it);
        if (s_expanded.valid() && !live.count(s_expanded)) s_expanded = VehicleId{};
    }

    const ImGuiIO& io = ImGui::GetIO();
    const float total_h = io.DisplaySize.y - TOPBAR_H - CENTER_HEADER_H;

    // Card column: as many rows as fit the height, then as many columns as the
    // fleet needs at that — so three vehicles are one narrow column and the
    // map gets the rest. Past half the screen the column stops widening and
    // scrolls instead.
    const int rows_fit = std::max(1, (int)((total_h - CARDS_CHROME_H + CARD_GAP)
                                           / (CARD_MIN_H + CARD_GAP)));
    s_rows = rows_fit;
    const int n        = std::max<int>(1, (int)fleet.size());
    const float scrollbar = ImGui::GetStyle().ScrollbarSize;
    const int max_cols = std::max(1, (int)((io.DisplaySize.x * SWARM_CARDS_MAX_FRAC
                                            - PANEL_PAD * 2.0f - scrollbar + CARD_GAP)
                                           / (CARD_W + CARD_GAP)));
    s_cols = std::min(max_cols, (n + rows_fit - 1) / rows_fit);
    const bool scrolls = n > s_cols * rows_fit;
    const float cards_w = s_cols * CARD_W + (s_cols - 1) * CARD_GAP
                        + PANEL_PAD * 2.0f + (scrolls ? scrollbar : 0.0f);

    // Band: as tall as the mode grid for the whole fleet. The whole fleet
    // rather than the checked vehicles, whose modes are a subset of it — so
    // ticking a box never moves the map under the cursor.
    std::vector<FlightTarget> all;
    for (const SwarmVehicle& v : fleet)
        all.push_back({ &v.vehicle->sender(), &v.state, v.number, v.vehicle->id() });
    const float flight_h = flight_section_height(all, PANEL_SPC_Y) + PANEL_PAD * 2.0f;

    s_layout = SwarmLayout::compute(cards_w, flight_h);
}

const SwarmLayout& swarm_layout()
{
    return s_layout;
}

std::vector<FlightTarget> swarm_checked_targets(const std::vector<SwarmVehicle>& fleet)
{
    std::vector<FlightTarget> out;
    for (const SwarmVehicle& v : fleet)
        if (s_checked.count(v.vehicle->id()))
            out.push_back({ &v.vehicle->sender(), &v.state, v.number, v.vehicle->id() });
    return out;
}

// ── Card grid ─────────────────────────────────────────────────────────────────

// Where the expanded card's cell was drawn this frame, for the overlay.
static bool   s_expanded_seen = false;
static ImVec2 s_expanded_at   = {};

static void draw_cards_panel(const std::vector<SwarmVehicle>& fleet,
                             VehicleId focused_id, VehicleId* focus_out,
                             const SwarmLayout& sl)
{
    begin_panel("##swarm_cards", { sl.cards_x, sl.top }, { sl.cards_w, sl.total_h },
                bg_sidebar_left(), false);

    char meta[48];
    snprintf(meta, sizeof(meta), "%zu / %zu CHECKED", s_checked.size(), fleet.size());
    ui_panel_header("FLEET", meta);

    // Selection shortcuts. ALL, NONE and INVERT only ever touch the checkboxes:
    // focus and expansion stay as they were.
    {
        constexpr float BH = 22.0f;
        const float bw = std::min(72.0f, (ImGui::GetContentRegionAvail().x - 6.0f) / 3.0f);
        if (ui_tab_button("ALL##sw", { bw, BH }, false))
            for (const SwarmVehicle& v : fleet) s_checked.insert(v.vehicle->id());
        ImGui::SameLine(0, 3);
        if (ui_tab_button("NONE##sw", { bw, BH }, false))
            s_checked.clear();
        ImGui::SameLine(0, 3);
        if (ui_tab_button("INVERT##sw", { bw, BH }, false))
            for (const SwarmVehicle& v : fleet) {
                const VehicleId id = v.vehicle->id();
                if (!s_checked.erase(id)) s_checked.insert(id);
            }
    }
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32_BLACK_TRANS);
    ImGui::BeginChild("##cards_scroll", { 0.0f, 0.0f });

    s_expanded_seen = false;
    if (fleet.empty()) {
        ImGui::TextColored(col_no_link_muted(), "NO VEHICLES");
    } else {
        const ImVec2 origin = ImGui::GetCursorScreenPos();

        // Stretch the rows to the height the grid really has, measured here
        // rather than trusted from swarm_view_layout()'s estimate of the
        // chrome above it — a few pixels off either way is a gap or a
        // scrollbar.
        const float avail_h = ImGui::GetContentRegionAvail().y;
        s_card_h = std::max(CARD_MIN_H, (avail_h + CARD_GAP) / s_rows - CARD_GAP);

        for (size_t i = 0; i < fleet.size(); ++i) {
            const SwarmVehicle& v  = fleet[i];
            const VehicleId     id = v.vehicle->id();
            const int r = (int)i / s_cols, c = (int)i % s_cols;
            const ImVec2 p0 = { origin.x + c * (CARD_W + CARD_GAP),
                                origin.y + r * (s_card_h + CARD_GAP) };

            ImGui::PushID((int)((id.link_id << 8) | id.sysid));
            const CardInput in = draw_card(v, p0, { CARD_W, s_card_h }, false,
                                           id == focused_id, s_checked.count(id) != 0);
            ImGui::PopID();

            if (id == s_expanded) { s_expanded_seen = true; s_expanded_at = p0; }

            if (in.toggle_check) {
                if (!s_checked.erase(id)) s_checked.insert(id);
            } else if (in.double_clicked) {
                // The first click of the pair has already selected it.
                s_expanded = id;
            } else if (in.clicked) {
                if (focus_out) *focus_out = id;
            }
        }

        // The cards were placed by hand; this is what tells the scroll region
        // how tall they came to.
        const int rows = ((int)fleet.size() + s_cols - 1) / s_cols;
        ImGui::SetCursorScreenPos(origin);
        ImGui::Dummy({ s_cols * (CARD_W + CARD_GAP) - CARD_GAP,
                       rows * (s_card_h + CARD_GAP) - CARD_GAP });
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();

    end_panel();
}

// The expanded card, over the grid and — when the column is narrower than two
// cards — over whatever is beside it. Anchored at its own cell so it grows out
// of the card that was clicked, then pushed back inside the screen.
static void draw_expanded_card(const std::vector<SwarmVehicle>& fleet,
                               VehicleId focused_id, const SwarmLayout& sl)
{
    if (!s_expanded.valid() || !s_expanded_seen) return;
    const SwarmVehicle* v = nullptr;
    for (const SwarmVehicle& f : fleet)
        if (f.vehicle->id() == s_expanded) { v = &f; break; }
    if (!v) return;

    // Escape puts it away, unless a popup is what Escape is for.
    if (!ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel) &&
        ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
        s_expanded = VehicleId{};
        return;
    }

    const ImGuiIO& io = ImGui::GetIO();
    ImVec2 pos = s_expanded_at;
    const ImVec2 sz = expanded_size();
    pos.x = std::max(0.0f, std::min(pos.x, io.DisplaySize.x - sz.x));
    pos.y = std::max(sl.top, std::min(pos.y, io.DisplaySize.y - sz.y));

    ImGui::SetNextWindowPos (pos,         ImGuiCond_Always);
    ImGui::SetNextWindowSize(sz, ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,    { 0.0f, 0.0f });
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,   0.0f);
    ImGui::Begin("##swarm_expanded", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoScrollbar |
                 ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings |
                 ImGuiWindowFlags_NoCollapse);

    const VehicleId id = s_expanded;
    ImGui::PushID("expanded");
    const CardInput in = draw_card(*v, pos, sz, true, id == focused_id,
                                   s_checked.count(id) != 0);
    ImGui::PopID();
    if (in.toggle_check) {
        if (!s_checked.erase(id)) s_checked.insert(id);
    } else if (in.clicked) {
        s_expanded = VehicleId{};   // a click on it puts it back
    }

    ImGui::End();
    ImGui::PopStyleVar(3);
}

// ── Entry point ───────────────────────────────────────────────────────────────

void draw_swarm_view(const std::vector<SwarmVehicle>& fleet,
                     const VehicleState& focused_vs,
                     MavlinkSender*      focused_sender,
                     VehicleId           focused_id,
                     AppSettings*        settings,
                     VehicleId*          focus_out)
{
    const SwarmLayout& sl = s_layout;

    draw_cards_panel(fleet, focused_id, focus_out, sl);

    // ── FLIGHT, aimed at the checked vehicles ─────────────────────────────────
    begin_panel("##swarm_flight", { sl.flight_x, sl.top }, { sl.flight_w, sl.band_h },
                bg_sidebar_left(), true);
    draw_flight_section(swarm_checked_targets(fleet), true);
    end_panel();

    // ── Telemetry grid of the focused vehicle ─────────────────────────────────
    begin_panel("##swarm_grid", { sl.grid_x, sl.top }, { sl.grid_w, sl.band_h },
                panel_bg(), false);
    {
        char meta[32] = "NO VEHICLE";
        for (const SwarmVehicle& v : fleet)
            if (v.vehicle->id() == focused_id) {
                snprintf(meta, sizeof(meta), "#%u FOCUSED", (unsigned)v.number);
                break;
            }
        draw_telemetry_panel(focused_vs, focused_sender, settings, meta);
    }
    end_panel();

    // ── Fleet event log ───────────────────────────────────────────────────────
    begin_panel("##swarm_log", { sl.log_x, sl.top }, { sl.log_w, sl.band_h },
                panel_bg(), false);
    draw_event_log_panel(/*wrap=*/true);
    end_panel();

    // Last, so it is on top of everything it overlaps.
    draw_expanded_card(fleet, focused_id, sl);
}
