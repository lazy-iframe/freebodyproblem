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


#include "sidebar_internal.hpp"
#include "../../../backend/firmware_profile.hpp"
#include "../vehicle_ui_state.hpp"
#include "../sidebar_themes.hpp"
#include "../../app_log.hpp"
#include "../../../backend/rc_binding.hpp"
#include "imgui.h"
#include <algorithm>
#include <cctype>
#include <cfloat>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>

// ── EKF variance bars ─────────────────────────────────────────────────────────

void draw_ekf_bars(const VehicleState& vs, float bar_h, bool small_labels)
{
    if (!vs.has_ekf_status) {
        ImGui::TextDisabled("No data");
        return;
    }

    struct Bar { const char* label; float value; };
    const Bar bars[] = {
        { "Vel",  vs.ekf_velocity_variance    },
        { "PosH", vs.ekf_pos_horiz_variance   },
        { "PosV", vs.ekf_pos_vert_variance    },
        { "Comp", vs.ekf_compass_variance     },
        { "TerH", vs.ekf_terrain_alt_variance },
        { "AS",   vs.ekf_airspeed_variance    },
    };
    constexpr int N = 6;

    ImDrawList*  dl      = ImGui::GetWindowDrawList();
    const float  avail_w = ImGui::GetContentRegionAvail().x;
    const ImVec2 p0      = ImGui::GetCursorScreenPos();

    // The swarm cards draw a row of these a fraction of the sidebar's width,
    // where body-sized labels would run into each other; the micro face is
    // what they get there.
    ImFont* const fnt = (small_labels && g_font_micro) ? g_font_micro : ImGui::GetFont();
    const float   fsz = small_labels ? UI_SZ_MICRO : ImGui::GetFontSize();
    const auto text_size = [&](const char* t) {
        return fnt->CalcTextSizeA(fsz, FLT_MAX, 0.0f, t);
    };
    const float fh = fsz;

    const float BAR_H = bar_h;
    constexpr float GAP   = 3.0f;
    const float bar_w = (avail_w - GAP * (N - 1)) / N;

    for (int i = 0; i < N; ++i) {
        const float x   = p0.x + i * (bar_w + GAP);

        // A quantity the vehicle is not estimating at all arrives as NaN — PX4
        // sends it for terrain height with no rangefinder, and for airspeed
        // with no airspeed sensor. An empty bar would read as a perfect score,
        // so those get a dash instead of a fill.
        const bool  estimated = std::isfinite(bars[i].value);
        const float val = estimated
            ? std::max(0.0f, std::min(bars[i].value, 1.0f)) : 0.0f;

        dl->AddRectFilled({x, p0.y}, {x + bar_w, p0.y + BAR_H}, ekf_bg());

        if (!estimated) {
            dl->AddRect({x, p0.y}, {x + bar_w, p0.y + BAR_H}, ekf_outline());
            const ImVec2 dsz = text_size("-");
            dl->AddText(fnt, fsz, { x + (bar_w - dsz.x) * 0.5f,
                                    p0.y + (BAR_H - dsz.y) * 0.5f }, ekf_label(), "-");
            const char* nlbl = bars[i].label;
            const ImVec2 ntsz = text_size(nlbl);
            dl->AddText(fnt, fsz, {x + (bar_w - ntsz.x) * 0.5f, p0.y + BAR_H + 2.0f},
                        ekf_label(), nlbl);
            continue;
        }

        // Color: green (0) → yellow (0.5) → red (1)
        ImU32 fill_col;
        if (val < 0.5f) {
            const uint8_t r = (uint8_t)(val * 2.0f * 220);
            fill_col = IM_COL32(r, 200, 40, 255);
        } else {
            const uint8_t g = (uint8_t)((1.0f - (val - 0.5f) * 2.0f) * 200);
            fill_col = IM_COL32(220, g, 40, 255);
        }

        // Filled portion grows from the bottom
        const float fill_h = BAR_H * val;
        dl->AddRectFilled(
            {x,         p0.y + BAR_H - fill_h},
            {x + bar_w, p0.y + BAR_H},
            fill_col);

        dl->AddRect({x, p0.y}, {x + bar_w, p0.y + BAR_H}, ekf_outline());

        // Centred label below the bar
        const char* lbl = bars[i].label;
        const ImVec2 tsz = text_size(lbl);
        dl->AddText(fnt, fsz,
            {x + (bar_w - tsz.x) * 0.5f, p0.y + BAR_H + 2.0f},
            ekf_label(), lbl);
    }

    ImGui::Dummy({avail_w, BAR_H + fh + 4.0f});
}

// Vehicle-reported mode names run from "Acro" to "Loiter to QLand"; the grid is
// three narrow columns. Uppercase to match the rest of the UI, then clamp to
// what actually fits — the button's tooltip carries the full name.
//
// The limit is pixels rather than a character count: at three columns a button
// is roughly a ninth of the window wide, so a fixed count both clips on small
// displays and wastes room on large ones. Measured with the same font and
// tracking ui_grid_button draws with, or the fit would be off.
static std::string mode_button_label(const std::string& name, float button_w)
{
    std::string out;
    out.reserve(name.size());
    for (char c : name) {
        out.push_back(static_cast<char>(
            std::toupper(static_cast<unsigned char>(c))));
    }
    if (out.empty()) return "?";

    const float budget = button_w - ImGui::GetStyle().FramePadding.x * 2.0f;
    if (budget <= 0.0f) return out;

    ImFont* fu = g_font_ui ? g_font_ui : ImGui::GetFont();
    while (out.size() > 1 &&
           ui_tracked_width(fu, UI_SZ_BODY, out.c_str(), UI_TRACK * 0.6f) > budget)
        out.pop_back();
    return out;
}

// Takeoff altitude, metres above home. Editable beside the button that uses it;
// 5 m is a conservative first hop that clears most obstacles without committing
// to a climb. Clamped to what MAV_CMD_NAV_TAKEOFF can sensibly ask for.
static float s_takeoff_alt_m = 5.0f;

static constexpr float TAKEOFF_ALT_MIN_M = 0.0f;
static constexpr float TAKEOFF_ALT_MAX_M = 1000.0f;

// ── Flight section ────────────────────────────────────────────────────────────
//
// TAKEOFF and the mode grid, for one vehicle or several. The FLIGHT tab hands
// it the vehicle on screen; the SWARM view hands it whichever vehicles are
// checked. One implementation for both, so a fix to how a mode is sent cannot
// reach one screen and miss the other.

// One target a mode button reaches, with that target's own number for it.
struct ModeHit {
    size_t   target;
    uint32_t mode;
    uint8_t  standard_mode;
};

// One button in the mode grid, and every target it reaches.
//
// Matched across vehicles by name, never by number: custom_mode numbering is
// per frame and per stack — GUIDED is 4 on a Copter, 15 on a Plane, a packed
// main/sub pair on PX4 — so each target keeps its own number and its own
// encoding, and a vehicle with no mode of that name is simply not sent one.
struct ModeBtn {
    std::string          name;             // uppercase, untruncated
    bool                 advanced = false;
    std::vector<ModeHit> hits;
};

// Modes grouped by flight stack. Two ArduPilot frames share most of their mode
// names and one list serves both; a PX4 HOLD and an ArduPilot LOITER are the
// same idea under different names, and merging the stacks would put both in
// one grid with nothing to say which vehicles each one reaches.
struct ModeGroup {
    const FirmwareProfile* prof;
    size_t                 n_targets = 0;
    std::vector<ModeBtn>   modes;
};

static std::string to_upper(std::string s)
{
    for (char& c : s)
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return s;
}

static bool target_connected(const FlightTarget& t)
{
    return t.vs && t.vs->has_heartbeat;
}

static std::vector<ModeGroup> collect_modes(const std::vector<FlightTarget>& targets)
{
    std::vector<ModeGroup> groups;
    for (size_t t = 0; t < targets.size(); ++t) {
        const VehicleState* vs = targets[t].vs;

        // Which flight stack this is, and so what a mode number means and how
        // one is written on the wire. An unknown stack resolves to ArduPilot,
        // which is what this panel assumed before the profile existed.
        const FirmwareProfile& prof =
            firmware_profile(vs ? vs->autopilot : MAV_AUTOPILOT_GENERIC);

        ModeGroup* g = nullptr;
        for (ModeGroup& e : groups)
            if (e.prof == &prof) { g = &e; break; }
        if (!g) {
            groups.push_back({ &prof, 0, {} });
            g = &groups.back();
        }
        ++g->n_targets;

        const auto add = [&](const std::string& raw, uint32_t mode,
                             uint8_t standard_mode, bool advanced) {
            const std::string name = to_upper(raw);
            for (ModeBtn& b : g->modes) {
                if (b.name != name) continue;
                // A vehicle reporting two modes of one name gets the first:
                // one press must be one command to it, not two.
                for (const ModeHit& h : b.hits)
                    if (h.target == t) return;
                b.hits.push_back({ t, mode, standard_mode });
                return;
            }
            g->modes.push_back({ name, advanced, { { t, mode, standard_mode } } });
        };

        // The vehicle's own AVAILABLE_MODES list when it publishes one — a
        // hardcoded table is only correct by accident. standard_mode rides
        // along because it, not the custom mode number, is what actually
        // reaches some modes — see FirmwareProfile::encode_set_mode.
        if (target_connected(targets[t]) && !vs->available_modes.empty()) {
            for (const FlightModeInfo& m : vs->available_modes)
                if (m.user_selectable())   // MAV_MODE_PROPERTY_NOT_USER_SELECTABLE
                    add(m.name, m.custom_mode, m.standard_mode, m.advanced());
        } else {
            // The stack's own built-in table. Which one matters: offering the
            // Copter table to a PX4 vehicle would command it somewhere
            // arbitrary.
            for (const FallbackMode& m : prof.fallback_modes(vs ? vs->type
                                                                : MAV_TYPE_GENERIC))
                add(m.label, m.custom_mode, 0, false);
        }
    }

    // A stable order, whatever order the vehicles were checked in.
    std::sort(groups.begin(), groups.end(),
              [](const ModeGroup& a, const ModeGroup& b) {
                  return std::strcmp(a.prof->name(), b.prof->name()) < 0;
              });
    return groups;
}

// The loudest of several vehicles' flashes: a rejection anywhere is what the
// button has to show, then a command still in flight.
static int flash_rank(CmdFlashState s)
{
    switch (s) {
    case CmdFlashState::Rejected: return 3;
    case CmdFlashState::Pending:  return 2;
    case CmdFlashState::Accepted: return 1;
    default:                      return 0;
    }
}

static CmdFlashState merge_flash(CmdFlashState a, CmdFlashState b)
{
    return flash_rank(b) > flash_rank(a) ? b : a;
}

// " #1 #3 #4" — the vehicles a command went to, for the log and the tooltips.
static std::string target_numbers(const std::vector<FlightTarget>& targets,
                                  const std::vector<size_t>& which)
{
    std::string out;
    for (size_t i : which) {
        char b[16];
        snprintf(b, sizeof(b), " #%u", (unsigned)targets[i].number);
        out += b;
    }
    return out;
}

// Height of everything above the mode grid — heading, TAKEOFF row, spacing —
// as last drawn. Measured rather than added up, so a change to that part of
// the section cannot leave flight_section_height() quietly wrong.
static float s_flight_head_h = 90.0f;

static constexpr int   MODE_COLS  = 3;
static constexpr float MODE_BTN_H = 26.0f;

float flight_section_height(const std::vector<FlightTarget>& targets, float spacing_y)
{
    const std::vector<ModeGroup> groups = collect_modes(targets);
    float h = s_flight_head_h;
    for (size_t g = 0; g < groups.size(); ++g) {
        if (groups.size() > 1) {
            if (g > 0) h += spacing_y;                            // Spacing()
            h += ImGui::GetTextLineHeight() + spacing_y;          // stack label
        }
        const size_t rows = (groups[g].modes.size() + MODE_COLS - 1) / MODE_COLS;
        h += rows * (MODE_BTN_H + spacing_y);
    }
    return h;
}

void draw_flight_section(const std::vector<FlightTarget>& targets, bool name_targets)
{
    const float y0 = ImGui::GetCursorPosY();

    bool any_connected = false;
    std::vector<size_t> all;
    for (size_t i = 0; i < targets.size(); ++i) {
        all.push_back(i);
        if (target_connected(targets[i])) any_connected = true;
    }

    ImGui::Spacing();
    ImGui::TextColored(accent_col(), "FLIGHT");
    if (name_targets) {
        // Who the buttons below will reach, always in view. A batch command
        // aimed at the wrong set is the one mistake this section can make that
        // the single-vehicle tab cannot.
        ImGui::SameLine(0, 6);
        if (targets.empty())
            ImGui::TextColored(col_no_link_muted(), "\xe2\x86\x92 NO VEHICLES CHECKED");
        else
            ImGui::TextColored(col_ok(), "\xe2\x86\x92%s", target_numbers(targets, all).c_str());
    } else if (!any_connected) {
        ImGui::SameLine(0, 6);
        ImGui::TextColored(col_no_link_muted(), "(no link)");
    }
    themed_sep();
    ImGui::Spacing();

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,   0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, FRAME_BORDER_NORMAL);
    ImGui::PushStyleColor(ImGuiCol_Border, col_border_tab());
    ImGui::BeginDisabled(!any_connected);

    // TAKEOFF | altitude | unit, on one row: the number is only ever read by the
    // button next to it, so keeping them apart would just hide the argument.
    {
        constexpr float ROW_H   = 28.0f;
        constexpr float ALT_W   = 56.0f;
        constexpr float ROW_GAP = 4.0f;

        const float unit_w = ImGui::CalcTextSize("M").x;
        const float btn_w  = ImGui::GetContentRegionAvail().x
                             - ALT_W - unit_w - ROW_GAP * 2.0f;

        CmdFlashState fs = CmdFlashState::Normal;
        for (const FlightTarget& t : targets)
            fs = merge_flash(fs, t.sender->query_flash(22));

        if (ui_grid_button("TAKEOFF", { btn_w, ROW_H }, false, fs)) {
            std::vector<size_t> sent;
            for (size_t i = 0; i < targets.size(); ++i) {
                if (!target_connected(targets[i])) continue;
                targets[i].sender->takeoff(targets[i].vs->sysid,
                                           targets[i].vs->compid, s_takeoff_alt_m);
                sent.push_back(i);
            }
            if (name_targets)
                gcs_log("takeoff command sent (%.1f m) \xe2\x86\x92%s",
                        (double)s_takeoff_alt_m, target_numbers(targets, sent).c_str());
            else
                gcs_log("takeoff command sent (%.1f m)", (double)s_takeoff_alt_m);
        }

        // The input frame is padded up to the button's height — items on a
        // SameLine row hang from the line top, so a default-height field would
        // sit proud of the button beside it.
        const float pad_y = std::max(0.0f,
                                     (ROW_H - ImGui::GetFontSize()) * 0.5f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                            { ImGui::GetStyle().FramePadding.x, pad_y });

        ImGui::SameLine(0, ROW_GAP);
        ImGui::SetNextItemWidth(ALT_W);
        ImGui::InputFloat("##takeoff_alt", &s_takeoff_alt_m, 0.f, 0.f, "%.1f");
        s_takeoff_alt_m = std::max(TAKEOFF_ALT_MIN_M,
                                   std::min(TAKEOFF_ALT_MAX_M, s_takeoff_alt_m));
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Takeoff altitude, metres above home");

        ImGui::SameLine(0, ROW_GAP);
        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("M");

        ImGui::PopStyleVar(); // FramePadding
    }

    ImGui::Spacing();
    s_flight_head_h = ImGui::GetCursorPosY() - y0;

    // Three columns: a vehicle that publishes its full list reports ~25 modes,
    // which at two columns runs past the bottom of the sidebar.
    constexpr float MODE_GAP  = 4.0f;
    const float col_w = (ImGui::GetContentRegionAvail().x
                         - MODE_GAP * (MODE_COLS - 1)) / MODE_COLS;

    const std::vector<ModeGroup> groups = collect_modes(targets);

    for (size_t g = 0; g < groups.size(); ++g) {
        const ModeGroup& grp = groups[g];

        // A mixed fleet gets one grid per stack, each under its stack's name.
        if (groups.size() > 1) {
            if (g > 0) ImGui::Spacing();
            ImGui::TextColored(col_no_link_muted(), "%s",
                               to_upper(grp.prof->name()).c_str());
        }

        for (size_t i = 0; i < grp.modes.size(); ++i) {
            const ModeBtn& b = grp.modes[i];
            if (i % MODE_COLS != 0) ImGui::SameLine(0, MODE_GAP);

            // Lit when every vehicle it reaches is already in it.
            bool is_active_mode = true;
            bool reachable      = false;
            CmdFlashState fs    = CmdFlashState::Normal;
            for (const ModeHit& h : b.hits) {
                const FlightTarget& t = targets[h.target];
                if (target_connected(t)) reachable = true;
                if (!target_connected(t) || t.vs->custom_mode != h.mode)
                    is_active_mode = false;
                // Flash on whichever command this particular button sends: a
                // standard mode goes out as DO_SET_STANDARD_MODE, not
                // DO_SET_MODE, and keying every button to 176 would leave those
                // with no ACK feedback.
                fs = merge_flash(fs, t.sender->query_flash(
                    grp.prof->encode_set_mode(h.mode, h.standard_mode).command));
            }

            // ##group/index keeps the ImGui ID unique when two modes truncate
            // to the same label.
            const std::string label = mode_button_label(b.name, col_w);
            const std::string id    = label + "##mode" + std::to_string(g)
                                    + "_" + std::to_string(i);

            ImGui::BeginDisabled(!reachable);
            if (ui_grid_button(id.c_str(), { col_w, MODE_BTN_H }, is_active_mode, fs)) {
                std::vector<size_t> sent;
                for (const ModeHit& h : b.hits) {
                    const FlightTarget& t = targets[h.target];
                    if (!target_connected(t)) continue;
                    // Through the profile, not sender->set_mode(): ArduPilot
                    // takes the whole mode number in one command parameter, PX4
                    // splits it across two.
                    send_set_mode(*t.sender, t.vs->sysid, t.vs->compid, *grp.prof,
                                  h.mode, h.standard_mode);
                    sent.push_back(h.target);
                }
                if (name_targets)
                    gcs_log("mode \xe2\x86\x92 %s \xe2\x86\x92%s", b.name.c_str(),
                            target_numbers(targets, sent).c_str());
                else
                    gcs_log("mode \xe2\x86\x92 %s (%u)", b.name.c_str(),
                            (unsigned)b.hits.front().mode);
            }
            ImGui::EndDisabled();

            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                // Full name, since the button label is truncated to fit — and,
                // when not every checked vehicle of this stack has the mode,
                // which ones the press will actually reach.
                std::string tip = b.name;
                if (b.advanced) tip += "  (advanced)";
                if (name_targets && b.hits.size() < grp.n_targets) {
                    std::vector<size_t> which;
                    for (const ModeHit& h : b.hits) which.push_back(h.target);
                    tip += "\nonly";
                    tip += target_numbers(targets, which);
                }
                if (tip != label) ImGui::SetTooltip("%s", tip.c_str());
            }
        }
    }

    ImGui::EndDisabled();
    ImGui::PopStyleColor(); // Border
    ImGui::PopStyleVar(2);  // FrameRounding, FrameBorderSize
}

// ─────────────────────────────────────────────────────────────────────────────

void draw_tab_flight(MavlinkSender* sender, const VehicleState* vs)
{
    draw_flight_section({ FlightTarget{ sender, vs, 0 } }, false);

    const bool    connected = (vs && vs->has_heartbeat);
    const uint8_t tsys      = connected ? vs->sysid  : 1;
    const uint8_t tcomp     = connected ? vs->compid : 1;

    // What the estimator bars are measuring depends on the stack.
    const FirmwareProfile& prof =
        firmware_profile(vs ? vs->autopilot : MAV_AUTOPILOT_GENERIC);

    // ── EKF status ────────────────────────────────────────────────────────────

    ImGui::Spacing();
    // The six bars are the same fields whichever stack filled them, but not
    // the same quantity — say which, rather than let a PX4 innovation ratio be
    // read as an ArduPilot variance.
    ImGui::TextColored(accent_col(), "EKF STATUS \xc2\xb7 %s", prof.estimator_units());
    themed_sep();
    ImGui::Spacing();
    draw_ekf_bars(vs ? *vs : VehicleState{});

    // ── SERVO / AUX sub-tabs ──────────────────────────────────────────────────

    ImGui::Spacing();
    themed_sep();
    ImGui::Spacing();

    static int s_flight_subtab = 0; // 0 = SERVO, 1 = AUX

    {
        const float tab_w = (ImGui::GetContentRegionAvail().x - 2.0f) * 0.5f;
        static const char* const sub_labels[] = { "SERVO", "AUX" };
        for (int i = 0; i < 2; ++i) {
            if (i > 0) ImGui::SameLine(0, 2);
            if (ui_tab_button(sub_labels[i], { tab_w, 24.0f }, s_flight_subtab == i))
                s_flight_subtab = i;
        }
    }

    ImGui::Spacing();
    const float avail_h = ImGui::GetContentRegionAvail().y;

    // ── SERVO panel ───────────────────────────────────────────────────────────
    if (s_flight_subtab == 0) {
        // Per vehicle — see widgets/vehicle_ui_state.hpp. These are outputs
        // about to be commanded, not readings, so carrying one aircraft's
        // slider positions over to another is the wrong kind of memory.
        struct ServoState { int pwm[16]; bool init = false; };
        static VehicleUiState<ServoState> s_servo;
        int* s_srv_pwm = s_servo->pwm;
        if (!s_servo->init) {
            for (int i = 0; i < 16; ++i) s_srv_pwm[i] = 1500;
            s_servo->init = true;
        }

        ImGui::PushStyleColor(ImGuiCol_ChildBg, bg_child_dark());
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,   FRAME_ROUNDING_SM);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, FRAME_BORDER_NORMAL);
        ImGui::PushStyleColor(ImGuiCol_Border, col_separator());
        if (ImGui::BeginChild("##srv_list", { -1.0f, avail_h }, false)) {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 4.0f, 3.0f });
            ImGui::BeginDisabled(!connected);

            const float row_w = ImGui::GetContentRegionAvail().x;
            const float pwm_w = 110.0f;
            const float btn_w = 76.0f;
            const float lbl_w = row_w - pwm_w - btn_w - 12.0f;

            for (int i = 0; i < 16; ++i) {
                char lbl[24]; snprintf(lbl, sizeof(lbl), "SERVO CHANNEL %d", i + 1);
                ImGui::PushID(i);

                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted(lbl);
                ImGui::SameLine(lbl_w);

                ImGui::SetNextItemWidth(pwm_w);
                ImGui::InputInt("##pwm", &s_srv_pwm[i], 0, 0);
                s_srv_pwm[i] = std::max(0, std::min(2200, s_srv_pwm[i]));
                ImGui::SameLine(0, 4);

                // Solid amber: writing a servo output commits a change to the
                // vehicle, same signal as WRITE in the params tab.
                if (ui_solid_button("SET", { btn_w, 0.0f },
                                    btn_write_base(), btn_write_hov())) {
                    sender->do_set_servo(tsys, tcomp,
                                         (uint8_t)(i + 1),
                                         (uint16_t)s_srv_pwm[i]);
                    gcs_log("servo %d → %d µs", i + 1, s_srv_pwm[i]);
                }
                ImGui::PopID();
            }

            ImGui::EndDisabled();
            ImGui::PopStyleVar(); // ItemSpacing
        }
        ImGui::EndChild();
        ImGui::PopStyleColor(); // Border
        ImGui::PopStyleVar(2);  // FrameRounding, FrameBorderSize
        ImGui::PopStyleColor(); // ChildBg
    }

    // ── AUX panel ─────────────────────────────────────────────────────────────
    if (s_flight_subtab == 1) {
        // The table lives in backend/rc_binding so the RC tab's RCn_OPTION
        // editor and this pad cannot drift apart: one triggers a function live,
        // the other binds it to a switch, but they must agree on what exists.
        const RcFirmware fw = rc_firmware_for(
            (vs && vs->has_heartbeat) ? vs->type : MAV_TYPE_QUADROTOR);
        const auto& aux_options = rc_aux_options();

        // The whole pad is ArduPilot's RCn_OPTION table fired through
        // MAV_CMD_DO_AUX_FUNCTION. PX4 has neither, so rather than offer a
        // grid of buttons it will reject, say so and grey the lot.
        const Capability aux_cap = prof.aux_functions();
        if (!aux_cap.supported && aux_cap.reason) {
            ImGui::TextColored(col_no_link_muted(), "%s", aux_cap.reason);
            ImGui::Spacing();
        }
        ImGui::BeginDisabled(!aux_cap.supported);

        // Firmware badge
        const char* fw_label = rc_firmware_label(fw);
        if (!connected) {
            ImGui::TextColored(col_no_link_muted(), "No link — COPTER defaults");
        } else {
            ImGui::TextColored(accent_col(), "Firmware:"); ImGui::SameLine(0, 4);
            ImGui::Text("%s", fw_label);
        }

        // Search bar
        static char aux_search[64] = {};
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputTextWithHint("##aux_search", "Search by name or ID...",
                                 aux_search, sizeof(aux_search));
        ImGui::Spacing();

        // Upper-case search string for case-insensitive matching
        char srch_up[64] = {};
        for (int k = 0; k < 63 && aux_search[k]; ++k)
            srch_up[k] = (char)toupper((unsigned char)aux_search[k]);

        const float aux_list_h = ImGui::GetContentRegionAvail().y;
        ImGui::PushStyleColor(ImGuiCol_ChildBg, bg_child_dark());
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,   FRAME_ROUNDING_SM);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, FRAME_BORDER_NORMAL);
        ImGui::PushStyleColor(ImGuiCol_Border, col_separator());
        if (ImGui::BeginChild("##aux_list", { -1.0f, aux_list_h }, false)) {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 4.0f, 3.0f });
            ImGui::BeginDisabled(!connected);

            const float row_w  = ImGui::GetContentRegionAvail().x;
            const float id_w   = 36.0f;
            const float btn3_w = (row_w * 0.40f) / 3.0f;
            const float lbl_w  = id_w + row_w - id_w - btn3_w * 3.0f - 3.0f * 2.0f - 6.0f;

            static const char* const pos_labels[] = { "LOW", "MID", "HI" };

            for (int i = 0; i < (int)aux_options.size(); ++i) {
                const RcAuxOption& fn = aux_options[i];

                if (!fn.supported(fw)) continue;

                // Search filter — match on name or numeric ID
                if (srch_up[0] != '\0') {
                    char name_up[64] = {};
                    for (int k = 0; k < 63 && fn.label[k]; ++k)
                        name_up[k] = (char)toupper((unsigned char)fn.label[k]);
                    char id_str[8];
                    snprintf(id_str, sizeof(id_str), "%u", (unsigned)fn.value);
                    if (strstr(name_up, srch_up) == nullptr &&
                        strstr(id_str, srch_up) == nullptr) continue;
                }

                ImGui::PushID(i);
                ImGui::AlignTextToFramePadding();
                ImGui::TextDisabled("%3u", (unsigned)fn.value);
                ImGui::SameLine(id_w);
                ImGui::TextUnformatted(fn.label);
                ImGui::SameLine(lbl_w);

                for (int p = 0; p < 3; ++p) {
                    if (p > 0) ImGui::SameLine(0, 2);
                    char btn_id[16];
                    snprintf(btn_id, sizeof(btn_id), "%s##%d", pos_labels[p], i);
                    if (ui_grid_button(btn_id, { btn3_w, 0.0f })) {
                        sender->do_aux_function(tsys, tcomp, fn.value, (uint8_t)p);
                        gcs_log("aux fn %u (%s) → %s",
                                (unsigned)fn.value, fn.label, pos_labels[p]);
                    }
                }
                ImGui::PopID();
            }

            ImGui::EndDisabled();
            ImGui::PopStyleVar(); // ItemSpacing
        }
        ImGui::EndChild();
        ImGui::PopStyleColor(); // Border
        ImGui::PopStyleVar(2);  // FrameRounding, FrameBorderSize
        ImGui::PopStyleColor(); // ChildBg
        ImGui::EndDisabled();   // aux_cap.supported
    }
}
