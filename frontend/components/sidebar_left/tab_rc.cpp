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


#include "frontend/components/sidebar_left/sidebar_internal.hpp"
#include "frontend/components/sidebar_themes.hpp"
#include "frontend/source/vehicle_ui_state.hpp"
#include "frontend/source/app_log.hpp"
#include "frontend/source/audio.hpp"
#include "frontend/source/joystick.hpp"
#include "backend/vehicle/firmware_profile.hpp"
#include "backend/input/rc_binding.hpp"
#include "backend/calibration/rc.hpp"
#include "imgui.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// Panel state.
//
// The calibration outlives any one frame and — via rc_tab_pump() below — any
// one tab: a sweep started here keeps recording while the operator is looking
// at PARAMS or the map, which is the difference between a calibration you can
// walk away from and one that silently stops the moment you glance elsewhere.
// ─────────────────────────────────────────────────────────────────────────────

// Per vehicle, not per panel.
//
// A calibration is a conversation with one airframe, and switching the callsign
// chip to look at another aircraft must not disturb it — the whole point of
// rc_tab_pump() is that a sweep survives the operator looking elsewhere, and
// with a fleet on screen "elsewhere" now includes another vehicle. Pending
// binding edits are keyed here for a sharper reason: committing edits staged
// against one aircraft to a different one would write the wrong RC map to
// flying hardware.
struct RcTabState {
    RcCalibration cal;
    uint32_t      seen_mask       = 0;
    bool          use_param_ext   = false;
    int           detect_target   = -1;
    RcChannels    detect_baseline;
    std::unordered_map<std::string, float> bind_edits;
    int           aux_channel     = 5;
};

static VehicleUiState<RcTabState> s_state;

// The vehicle whose state the panel and the pump are working on; the render
// loop binds it before either runs. See vehicle_ui_state.hpp.
static RcTabState& st() { return *s_state; }

// Channels seen carrying a pulse on this link, one bit each, 0-based.
//
// The monitor lists these and nothing else — eighteen rows for an eight-channel
// radio is ten rows of "--" between the operator and the numbers they came for.
// It is a running set rather than a test against the current frame because a
// receiver drops the odd frame: filtering live would make rows vanish and the
// list reflow under the cursor every time one did. A channel that has spoken
// once keeps its row, and shows "--" while it is quiet.
//
// Cleared when the link or the RC stream goes away, so a different airframe
// does not inherit the last one's channel set.

static bool channel_seen(int idx_0based)
{
    return (st().seen_mask & (1u << idx_0based)) != 0u;
}

// Plain loop rather than a popcount intrinsic: __builtin_popcount is not MSVC,
// std::popcount is C++20, and this project builds as C++17 on both.
static int seen_count()
{
    int n = 0;
    for (int i = 0; i < RcChannels::MAX_CHANNELS; ++i)
        if (channel_seen(i)) ++n;
    return n;
}

// The extended parameter protocol is opt-in. Neither ArduPilot nor PX4 answers
// PARAM_EXT_SET for RC parameters — both speak the classic protocol for these,
// and MAVLink advertises no capability bit that would let us detect otherwise —
// so the default has to be PARAM_SET or a commit would land nowhere on the
// hardware this GCS actually flies. The switch is here for MAVLink components
// that do implement it, and because a commit is exactly where you want to be
// able to choose.

// Channel-detection ("move the stick you want") state. -1 when nothing is armed;
// otherwise the index into rc_stick_bindings() waiting for a movement.

// Binding values the operator has typed or demonstrated but not yet written.
// Keyed by parameter name, so ArduPilot and PX4 runs cannot collide.

// Aux-function channel the numeric editor is pointed at, 1-based.

// ── Helpers ──────────────────────────────────────────────────────────────────

// The vehicle's own value for a parameter, when the PARAMS tab has fetched it.
static const ParamEntry* find_param(
    const std::unordered_map<std::string, ParamEntry>* params, const char* id)
{
    if (!params || !id) return nullptr;
    auto it = params->find(id);
    return (it == params->end()) ? nullptr : &it->second;
}

// True once a complete parameter set is in hand.
//
// main.cpp only publishes the map when a fetch actually finishes — a vehicle
// volunteers the odd PARAM_VALUE on its own, and those are counted but not
// published — so an empty map here is exactly "nobody has pressed FETCH ALL".
static bool params_ready(const std::unordered_map<std::string, ParamEntry>* params)
{
    return params && !params->empty();
}

// The type the vehicle reported for a parameter, falling back to REAL32.
// Writing a parameter under a type the vehicle does not agree with is how a
// perfectly good value gets rejected, so prefer what it told us over what we
// assumed.
static uint8_t param_type_for(
    const std::unordered_map<std::string, ParamEntry>* params, const char* id)
{
    const ParamEntry* p = find_param(params, id);
    return p ? p->type : (uint8_t)MAV_PARAM_TYPE_REAL32;
}

// One PARAM_SET or PARAM_EXT_SET, per the protocol switch above. The two
// messages have deliberately identical signatures on MavlinkSender, so the
// choice is this one line and nothing downstream has to care.
static void commit_write(MavlinkSender* sender, uint8_t tsys, uint8_t tcomp,
                         const char* id, float value, uint8_t type)
{
    if (st().use_param_ext) sender->set_param_ext(tsys, tcomp, id, value, type);
    else                 sender->set_param    (tsys, tcomp, id, value, type);
}

// The value a parameter row should show: the operator's staged edit if there is
// one, otherwise what the vehicle reported, otherwise the given default.
static float staged_value(const std::unordered_map<std::string, ParamEntry>* params,
                          const char* id, float fallback)
{
    auto ed = st().bind_edits.find(id);
    if (ed != st().bind_edits.end()) return ed->second;
    if (const ParamEntry* p = find_param(params, id)) return p->value;
    return fallback;
}

// Wrapped text with every line centred.
//
// ImGui's own wrapping is left-aligned only: it hands the whole string to the
// draw list with a wrap width and never reports where it broke, so there is no
// line to centre after the fact. Doing the breaking here gives us both.
//
// Returns the number of lines. Called once to measure and once to draw, so the
// box drawn around it is always exactly the height of what goes inside — a
// fixed guess would clip on a narrow sidebar and leave a gap on a wide one.
static int centered_wrapped(const char* text, float wrap_w, ImVec4 col, bool draw)
{
    int lines = 0;

    if (draw) ImGui::PushStyleColor(ImGuiCol_Text, col);
    const float x0 = ImGui::GetCursorPosX();

    for (const char* p = text; *p; ) {
        while (*p == ' ') ++p;              // a wrapped line does not open on a space
        if (!*p) break;

        // The longest run of whole words that still fits.
        const char* fit = nullptr;
        for (const char* q = p; ; ) {
            while (*q && *q != ' ') ++q;
            if (ImGui::CalcTextSize(p, q).x > wrap_w) break;
            fit = q;
            if (!*q) break;
            ++q;                            // step over the space, try one more word
        }
        // A single word wider than the box: break it by character rather than
        // let it run out past the border.
        if (!fit) {
            fit = p + 1;
            while (*fit && ImGui::CalcTextSize(p, fit + 1).x <= wrap_w) ++fit;
        }

        // Trailing spaces are part of the run but not part of what you see;
        // measuring them would push the visible text left of centre.
        const char* vis_end = fit;
        while (vis_end > p && vis_end[-1] == ' ') --vis_end;

        if (draw) {
            const float w = ImGui::CalcTextSize(p, vis_end).x;
            ImGui::SetCursorPosX(x0 + std::max(0.0f, (wrap_w - w) * 0.5f));
            ImGui::TextUnformatted(p, vis_end);
        }
        ++lines;
        p = fit;
    }

    if (draw) ImGui::PopStyleColor();
    return lines;
}

// ── Live channel bar ─────────────────────────────────────────────────────────

// The display window. Wider than the 1000-2000 a receiver nominally produces,
// so a channel travelling past its nominal ends stays on the bar instead of
// pinning to an edge and looking calibrated when it is not.
static constexpr float BAR_LO = 900.0f;
static constexpr float BAR_HI = 2100.0f;

static constexpr float ROW_H = 15.0f;

static float bar_frac(float pwm)
{
    return std::max(0.0f, std::min((pwm - BAR_LO) / (BAR_HI - BAR_LO), 1.0f));
}

// One channel: number, a bar, and the raw microseconds. During a sweep the
// measured travel is drawn behind the live position, so the operator can see
// which channels still have not been taken to their stops.
static void draw_channel_row(const char* lbl, uint16_t pwm,
                             const RcChannelCal* cal, const char* bind_label)
{
    constexpr float LBL_W = 30.0f;
    constexpr float VAL_W = 42.0f;
    constexpr float GAP   = 4.0f;

    ImDrawList*  dl    = ImGui::GetWindowDrawList();
    const ImVec2 p0    = ImGui::GetCursorScreenPos();
    const float  row_w = ImGui::GetContentRegionAvail().x;
    const float  bar_w = row_w - LBL_W - VAL_W - GAP * 2.0f;
    if (bar_w <= 0.0f) { ImGui::Dummy({ row_w, ROW_H }); return; }

    const float bar_x0 = p0.x + LBL_W + GAP;
    const float bar_x1 = bar_x0 + bar_w;
    const float text_y = p0.y + (ROW_H - ImGui::GetTextLineHeight()) * 0.5f;

    // A channel with no pulse is drawn as an empty well rather than skipped:
    // the panel shows every channel the protocol can carry, so the gap between
    // "channel 9 is quiet" and "this radio only has 8" stays visible.
    const bool live = (pwm > 0);

    dl->AddText({ p0.x, text_y },
                bind_label ? ui_col_accent()
                           : (live ? ui_col_label() : ui_col(g_theme.col_log, 0.45f)),
                lbl);

    dl->AddRectFilled({ bar_x0, p0.y }, { bar_x1, p0.y + ROW_H }, ui_col_well());

    // Measured travel, behind the live marker.
    if (cal && cal->seen && cal->max > cal->min) {
        const float mx0 = bar_x0 + bar_w * bar_frac((float)cal->min);
        const float mx1 = bar_x0 + bar_w * bar_frac((float)cal->max);
        dl->AddRectFilled({ mx0, p0.y }, { mx1, p0.y + ROW_H },
                          ui_col(g_theme.accent, 0.22f));
    }

    // Live position — a tick rather than a fill from zero. A receiver's centre
    // is 1500 µs, not 1000, so a left-anchored fill would read as "half output"
    // on a stick that is sitting still in the middle.
    if (live) {
        const float x = bar_x0 + bar_w * bar_frac((float)pwm);
        dl->AddRectFilled({ x - 1.5f, p0.y }, { x + 1.5f, p0.y + ROW_H },
                          ui_col_value());
    }

    dl->AddRect({ bar_x0, p0.y }, { bar_x1, p0.y + ROW_H }, ui_col_seam());

    // Trim marker, once a centre has been captured.
    if (cal && cal->seen && cal->trim > 0) {
        const float tx = bar_x0 + bar_w * bar_frac((float)cal->trim);
        dl->AddLine({ tx, p0.y }, { tx, p0.y + ROW_H }, ui_col_accent());
    }

    char val[12];
    if (live) snprintf(val, sizeof(val), "%u", (unsigned)pwm);
    else      snprintf(val, sizeof(val), "--");
    dl->AddText({ bar_x1 + GAP, text_y },
                live ? ui_col_value() : ui_col(g_theme.col_log, 0.45f), val);

    ImGui::Dummy({ row_w, ROW_H });

    if (bind_label && ImGui::IsItemHovered())
        ImGui::SetTooltip("Channel %s \xe2\x80\x94 %s", lbl, bind_label);
}

// ── Sections ─────────────────────────────────────────────────────────────────

static void draw_monitor(const RcChannels& rc,
                         const char* const bind_of_channel[RcChannels::MAX_CHANNELS])
{
    const int active_n = seen_count();

    ImGui::TextColored(accent_col(), "CHANNELS");
    if (rc.valid && active_n > 0) {
        // The count is the whole story about what is not on screen: the rows
        // stop at the last channel that has ever spoken, and this says how many
        // that is out of what the protocol could carry.
        ImGui::SameLine(0, 6);
        ImGui::TextColored(col_data(), "%d", active_n);
        ImGui::SameLine(0, 0);
        ImGui::TextDisabled("/%d", (int)RcChannels::MAX_CHANNELS);
    }
    {
        // Which message is feeding this, said plainly. It changes what the panel
        // can show — RC_CHANNELS_RAW tops out at 8 channels per port and carries
        // no channel count — so it is not an implementation detail to bury.
        const char* src = !rc.valid   ? "no rc"
                        : rc.from_raw ? "#35 RAW"
                                      : "#65";
        // Right-aligned against the window, not against whatever the cursor has
        // already consumed: GetContentRegionAvail() measures from the cursor, so
        // taking it after a SameLine would push the badge off the edge.
        const float x = ImGui::GetWindowContentRegionMax().x
                      - ImGui::CalcTextSize(src).x;
        ImGui::SameLine(x);
        ImGui::TextColored(rc.valid ? col_data() : col_no_link_muted(), "%s", src);
    }

    if (!rc.valid) {
        ImGui::Spacing();
        ImGui::TextDisabled("Waiting for RC_CHANNELS.");
        ImGui::TextDisabled("Check the receiver is bound and powered.");
        return;
    }

    if (rc.usable_count() == 0 || active_n == 0) {
        // chancount == 0 is the vehicle saying it has a link to us but no link
        // to a transmitter — a distinct failure from never having heard an RC
        // message at all, and the one an operator hits far more often.
        ImGui::TextColored(col_warning(), "No RC input \xe2\x80\x94 0 channels");
        return;
    }

    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_ChildBg, bg_param_list());
    ImGui::PushStyleColor(ImGuiCol_Border,  col_separator());
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, FRAME_BORDER_NORMAL);

    // Sized to what is actually listed, so an eight-channel radio does not sit
    // in a box built for eighteen with half of it empty.
    const float list_h = active_n * (ROW_H + 2.0f) + ImGui::GetStyle().WindowPadding.y * 2.0f;
    if (ImGui::BeginChild("##rc_channels", { -1.0f, list_h }, true,
                          ImGuiWindowFlags_NoScrollbar)) {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 2.0f, 2.0f });
        for (int i = 0; i < RcChannels::MAX_CHANNELS; ++i) {
            if (!channel_seen(i)) continue;
            const RcChannelCal& cal = st().cal.channel(i);
            // The channel keeps its own number, not its position in the list:
            // a gap where a dead channel would be is the point, since RCn_*
            // parameters and every binding are named by that number.
            char ch_lbl[8];
            snprintf(ch_lbl, sizeof(ch_lbl), "%d", i + 1);
            draw_channel_row(ch_lbl, rc.chan[i],
                             st().cal.phase() == RcCalibration::Phase::Idle
                                 ? nullptr : &cal,
                             bind_of_channel[i]);
        }
        ImGui::PopStyleVar();
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);

    if (rc.has_rssi()) {
        // 0-254 in device-dependent units, so a percentage would be a fiction.
        // Shown as the raw figure with the scale named.
        ImGui::TextDisabled("RSSI %u/254", (unsigned)rc.rssi);
    }
}

static void draw_calibration(MavlinkSender* sender, const VehicleState* vs,
                             const std::unordered_map<std::string, ParamEntry>* params,
                             const RcChannels& rc, RcParamDialect dialect,
                             bool connected, uint8_t tsys, uint8_t tcomp)
{
    ImGui::TextColored(accent_col(), "CALIBRATION");
    ImGui::Spacing();

    // The vehicle reports a joystick's commands as if they were receiver pulses,
    // so a sweep taken now would measure the joystick and write the result over
    // the radio's calibration.
    const bool joy_overriding = joystick_live() &&
                                joystick_live_target() == ui_bound_vehicle();
    const bool can_calibrate = connected && rc.valid && rc.usable_count() > 0 &&
                               !joy_overriding;

    switch (st().cal.phase()) {

    case RcCalibration::Phase::Idle: {
        ImGui::BeginDisabled(!can_calibrate);
        if (ui_grid_button("START CALIBRATION", { -1.0f, 26.0f }) && can_calibrate) {
            // Every channel the protocol carries, not the count the vehicle
            // reports. Which of them are really there is then decided by what
            // arrives, so an under-reported chancount cannot quietly drop a
            // channel out of the calibration.
            st().cal.begin(RcChannels::MAX_CHANNELS);

            // Seed reversal from the vehicle rather than from zero. A sweep
            // cannot measure direction — the two endpoints are the same pair of
            // numbers either way — so starting every channel un-reversed would
            // quietly un-reverse an already-correct radio on commit.
            const char* rev_name = (dialect == RcParamDialect::PX4)
                                 ? "REV" : "REVERSED";
            for (int i = 0; i < RcChannels::MAX_CHANNELS; ++i) {
                char id[24];
                snprintf(id, sizeof(id), "RC%d_%s", i + 1, rev_name);
                if (const ParamEntry* p = find_param(params, id)) {
                    const bool rev = (dialect == RcParamDialect::PX4)
                                   ? (p->value < 0.0f)
                                   : (p->value != 0.0f);
                    st().cal.set_reversed(i, rev);
                }
            }

            // The throttle's trim belongs at the bottom of its travel, so the
            // calibration needs to know which channel that is. Taken from the
            // live binding, not assumed to be channel 3.
            const auto& bindings = rc_stick_bindings();
            const int   ti       = rc_stick_throttle_index();
            if (ti < (int)bindings.size()) {
                const char* id = rc_stick_param(bindings[ti], dialect);
                const float ch = id ? staged_value(params, id,
                                                   (float)bindings[ti].default_ch)
                                    : (float)bindings[ti].default_ch;
                st().cal.set_throttle_channel((int)ch);
            }

            gcs_log("rc calibration started");
        }
        ImGui::EndDisabled();

        // The missing-parameters case is covered by the banner at the top of
        // the panel; repeating it here would say the same thing twice.
        if (!connected)
            ImGui::TextDisabled("Connect to a vehicle first.");
        else if (joy_overriding)
            ImGui::TextDisabled("Stop the joystick first.");
        else if (!can_calibrate)
            ImGui::TextDisabled("No RC input to calibrate.");
        break;
    }

    case RcCalibration::Phase::Center: {
        ImGui::TextColored(col_warning(), "1 / 3  CENTRE");
        ImGui::TextWrapped("Centre the sticks and pull the throttle fully down, "
                           "then capture.");
        ImGui::Spacing();

        const float half = (ImGui::GetContentRegionAvail().x - 4.0f) * 0.5f;
        ImGui::BeginDisabled(!rc.valid);
        if (ui_solid_button("CAPTURE", { half, 26.0f },
                            btn_write_base(), btn_write_hov())) {
            if (st().cal.capture_center(rc)) {
                gcs_log("rc calibration: centre captured on %d channels \xe2\x80\x94 "
                        "now move every stick and switch to both stops",
                        st().cal.active_count());
            } else {
                // capture_center() refuses a frame with nothing plausible in it.
                gcs_log("rc calibration: no usable RC input to capture");
                gcs_tone(GcsTone::Failure);
            }
        }
        ImGui::EndDisabled();

        ImGui::SameLine(0, 4);
        if (ui_grid_button("CANCEL##cal", { -1.0f, 26.0f })) {
            st().cal.cancel();
            gcs_log("rc calibration cancelled");
        }
        break;
    }

    case RcCalibration::Phase::Sweep: {
        ImGui::TextColored(col_warning(), "2 / 3  SWEEP");
        ImGui::TextWrapped("Move every stick and switch to both stops. The amber "
                           "band on each bar is the travel recorded so far.");
        ImGui::Spacing();

        // Progress is measured against the channels that are actually carrying
        // a signal, not against the eighteen the protocol allows or the count
        // the vehicle claims. An eight-channel radio finishing its sweep should
        // read 8 / 8, not 8 / 18 — a bar that can never fill is a bar that
        // tells the operator nothing about when they are done.
        const int swept = st().cal.usable_count();
        const int total = st().cal.active_count();
        char prog[48];
        snprintf(prog, sizeof(prog), "%d / %d active channel%s",
                 swept, total, total == 1 ? "" : "s");
        ui_progress_bar(prog, total > 0 ? (float)swept / total : 0.0f, 12.0f);

        ImGui::Spacing();
        const float half = (ImGui::GetContentRegionAvail().x - 4.0f) * 0.5f;

        ImGui::BeginDisabled(swept == 0);
        if (ui_solid_button("FINISH", { half, 26.0f },
                            btn_write_base(), btn_write_hov())) {
            st().cal.finish();
            gcs_log("rc calibration: %d of %d active channels swept", swept, total);
        }
        ImGui::EndDisabled();
        if (swept == 0 && ImGui::IsItemHovered())
            ImGui::SetTooltip("Nothing has moved far enough to record yet");

        ImGui::SameLine(0, 4);
        if (ui_grid_button("CANCEL##cal", { -1.0f, 26.0f })) {
            st().cal.cancel();
            gcs_log("rc calibration cancelled");
        }
        break;
    }

    case RcCalibration::Phase::Review: {
        const std::vector<RcParamWrite> writes = st().cal.params(dialect);

        ImGui::TextColored(col_warning(), "3 / 3  REVIEW");
        ImGui::TextWrapped("Check each channel's travel, then commit. Nothing has "
                           "been written to the vehicle yet.");
        ImGui::Spacing();

        // Per-channel table: measured endpoints, trim, and the reversal flag —
        // the one value a sweep cannot determine and the operator must set.
        ImGui::PushStyleColor(ImGuiCol_ChildBg, bg_param_list());
        ImGui::PushStyleColor(ImGuiCol_Border,  col_separator());
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, FRAME_BORDER_NORMAL);
        // Sized to the channels it lists, like the monitor above, so the panel's
        // own scroll is the only one in play.
        const float review_h = st().cal.active_count() * (ImGui::GetFrameHeight() + 3.0f)
                             + ImGui::GetStyle().WindowPadding.y * 2.0f;
        if (ImGui::BeginChild("##cal_review", { -1.0f, review_h }, true,
                              ImGuiWindowFlags_NoScrollbar)) {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 4.0f, 3.0f });

            const float rev_x = ImGui::GetContentRegionAvail().x - 46.0f;

            for (int i = 0; i < st().cal.channel_count(); ++i) {
                // Channels that never carried a pulse are not a finding — they
                // are the empty half of an eighteen-slot protocol. Only a
                // channel that is live but was not moved is worth a line.
                if (!st().cal.channel_active(i)) continue;

                const RcChannelCal& c = st().cal.channel(i);
                ImGui::PushID(i);

                if (!st().cal.channel_usable(i)) {
                    ImGui::AlignTextToFramePadding();
                    ImGui::TextDisabled("%2d  not moved", i + 1);
                    ImGui::PopID();
                    continue;
                }

                const uint16_t trim = (i + 1 == st().cal.throttle_channel())
                                    ? c.min : c.trim;

                ImGui::AlignTextToFramePadding();
                ImGui::TextColored(accent_col(), "%2d", i + 1);
                ImGui::SameLine(34.0f);
                ImGui::Text("%u \xe2\x80\x93 %u", (unsigned)c.min, (unsigned)c.max);
                ImGui::SameLine(140.0f);
                ImGui::TextDisabled("T%u", (unsigned)trim);

                ImGui::SameLine(rev_x);
                bool rev = c.reversed;
                if (ImGui::Checkbox("REV", &rev))
                    st().cal.set_reversed(i, rev);

                ImGui::PopID();
            }
            ImGui::PopStyleVar();
        }
        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);

        ImGui::Spacing();

        // Protocol switch — see the note on RcTabState::use_param_ext.
        ImGui::Checkbox("Commit over PARAM_EXT_SET", &st().use_param_ext);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("PARAM_SET (#23) is what ArduPilot and PX4 answer.\n"
                              "PARAM_EXT_SET (#323) is for components that\n"
                              "implement the extended parameter protocol.");

        ImGui::Spacing();

        const float half = (ImGui::GetContentRegionAvail().x - 4.0f) * 0.5f;

        // Held to the same rule as every WRITE below: a commit is a burst of
        // parameter writes, and it needs the fetch for more than consistency —
        // param_type_for() falls back to REAL32 without it, and the reversal
        // flags were seeded from a table that was never read.
        const bool can_commit = connected && !writes.empty() && params_ready(params);
        ImGui::BeginDisabled(!can_commit);
        {
            char lbl[48];
            snprintf(lbl, sizeof(lbl), "COMMIT (%d)###cal_commit", (int)writes.size());
            const bool hit = can_commit
                ? ui_solid_button(lbl, { half, 26.0f },
                                  btn_write_base(), btn_write_hov())
                : ui_grid_button(lbl, { half, 26.0f });
            if (hit) ImGui::OpenPopup("##confirm_cal_commit");
        }
        ImGui::EndDisabled();

        ImGui::SameLine(0, 4);
        if (ui_solid_button("DISCARD##cal", { -1.0f, 26.0f },
                            btn_disconnect_base(), btn_disconnect_hov())) {
            st().cal.cancel();
            gcs_log("rc calibration discarded \xe2\x80\x94 vehicle untouched");
        }

        {
            char q[128];
            snprintf(q, sizeof(q), "WRITE %d PARAMETERS FOR %d CHANNELS?",
                     (int)writes.size(), st().cal.usable_count());
            if (ui_confirm_popup("##confirm_cal_commit", "COMMIT CALIBRATION", q,
                                 "COMMIT", btn_write_base()) == UiConfirm::Confirmed) {
                for (const RcParamWrite& w : writes) {
                    commit_write(sender, tsys, tcomp, w.id.c_str(), w.value,
                                 param_type_for(params, w.id.c_str()));
                }
                // No retry behind this, for the same reason the PARAMS tab has
                // none: each write comes back as a PARAM_VALUE, so anything the
                // vehicle missed stays visible in the parameter table.
                gcs_log("rc calibration committed: %d parameters over %s",
                        (int)writes.size(),
                        st().use_param_ext ? "PARAM_EXT_SET" : "PARAM_SET");
                gcs_tone(GcsTone::Success);
                st().cal.cancel();
            }
        }
        break;
    }
    }

    (void)vs;
}

// One editable parameter row: name on the left, a value, and a WRITE that only
// lights once the value differs from the vehicle's. Shared by the stick
// bindings, the aux function and the flight-mode slots so all three behave the
// same way under the operator's hands.
static bool draw_write_row(MavlinkSender* sender,
                           const std::unordered_map<std::string, ParamEntry>* params,
                           const char* id, float val, bool connected,
                           uint8_t tsys, uint8_t tcomp, float btn_w)
{
    // Nothing is writable until the vehicle's parameters have been read.
    // Every row here is "change this from what it is now", and without the
    // fetch there is no "now": the row would be offering to overwrite a setting
    // nobody has looked at, with a value taken from a hardcoded default.
    //
    // A parameter missing from a completed fetch is one the vehicle does not
    // have — a PX4 airframe has no FLTMODE3 — so that stays unwritable too.
    const ParamEntry* p     = find_param(params, id);
    const bool        ready = params_ready(params);
    const bool        dirty = ready && p && (val != p->value);

    ImGui::BeginDisabled(!connected || !dirty);
    const bool wrote = dirty
        ? ui_solid_button("WRITE", { btn_w, 0.0f },
                          btn_write_base(), btn_write_hov())
        : ui_grid_button("WRITE", { btn_w, 0.0f });
    if (wrote) {
        commit_write(sender, tsys, tcomp, id, val, param_type_for(params, id));
        st().bind_edits.erase(id);
    }
    ImGui::EndDisabled();
    return wrote;
}

static void draw_binding(MavlinkSender* sender, const VehicleState* vs,
                         const std::unordered_map<std::string, ParamEntry>* params,
                         const RcChannels& rc, RcParamDialect dialect,
                         bool connected, uint8_t tsys, uint8_t tcomp)
{
    ImGui::TextColored(accent_col(), "BINDING");
    ImGui::Spacing();

    // ── Stick / axis bindings ────────────────────────────────────────────────
    //
    // Each section pushes its own ID scope. Without it the rows here (keyed by
    // their index in the axis table) and the flight-mode slots below (keyed by
    // slot number) hash to the same ImGui IDs for 1..4, and since both draw a
    // button captioned WRITE, the two sections fight over every click in that
    // range — only the slots outside the overlap ever responded.
    ImGui::PushID("sticks");
    {
        const auto& bindings = rc_stick_bindings();
        const float row_w    = ImGui::GetContentRegionAvail().x;
        const float lbl_w    = 74.0f;
        const float in_w     = 46.0f;
        const float det_w    = 52.0f;
        const float wr_w     = std::max(46.0f, row_w - lbl_w - in_w - det_w - 12.0f);

        for (size_t i = 0; i < bindings.size(); ++i) {
            const RcStickBinding& b  = bindings[i];
            const char*           id = rc_stick_param(b, dialect);

            ImGui::PushID((int)i);

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(b.label);

            if (!id) {
                // PX4 has no parameter for this axis. Say so rather than
                // silently writing the ArduPilot name at a PX4 vehicle.
                ImGui::SameLine(lbl_w);
                ImGui::TextDisabled("n/a on this stack");
                ImGui::PopID();
                continue;
            }

            float val = staged_value(params, id, (float)b.default_ch);

            ImGui::SameLine(lbl_w);
            ImGui::SetNextItemWidth(in_w);
            int ch = (int)val;
            if (ImGui::InputInt("##ch", &ch, 0, 0)) {
                ch = std::max(1, std::min(ch, (int)RcChannels::MAX_CHANNELS));
                st().bind_edits[id] = (float)ch;
                val = (float)ch;
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", id);

            // Arm detection: take a baseline now, then watch for the first
            // channel to move by more than the noise threshold.
            ImGui::SameLine(0, 4);
            const bool armed = (st().detect_target == (int)i);
            if (ui_grid_button(armed ? "MOVE" : "DETECT", { det_w, 0.0f }, armed)) {
                if (armed) {
                    st().detect_target = -1;
                } else if (rc.valid) {
                    st().detect_baseline = rc;
                    st().detect_target   = (int)i;
                    gcs_log("rc: move the %s stick to bind it", b.label);
                }
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip(armed ? "Waiting \xe2\x80\x94 move the stick"
                                        : "Bind by moving the stick");

            ImGui::SameLine(0, 4);
            if (draw_write_row(sender, params, id, val, connected, tsys, tcomp, wr_w))
                gcs_log("rc: binding %s = channel %d", id, (int)val);

            ImGui::PopID();
        }
    }
    ImGui::PopID();

    if (dialect != RcParamDialect::ArduPilot) return;

    const RcFirmware fw = rc_firmware_for(vs && vs->has_heartbeat
                                          ? vs->type : MAV_TYPE_QUADROTOR);

    // ── Flight-mode switch slots ─────────────────────────────────────────────
    //
    // ArduPilot bands the mode channel's travel into six positions and keeps a
    // mode number for each. Editing them here, beside the live channel bar that
    // shows which band the switch is in, is the only place the two halves of
    // "what does this switch position do" are visible together.
    ImGui::PushID("fltmode");
    {
        ImGui::Spacing();
        ImGui::TextColored(accent_col(), "FLIGHT MODES");

        // Where the mode switch is sitting right now, so the row it selects can
        // be marked. Read through the live binding, not assumed to be channel 5.
        const auto& bindings = rc_stick_bindings();
        int mode_ch = 5;
        for (const RcStickBinding& b : bindings) {
            if (std::string(b.label) != "MODE") continue;
            if (const char* id = rc_stick_param(b, dialect))
                mode_ch = (int)staged_value(params, id, (float)b.default_ch);
            break;
        }
        const uint16_t mode_pwm  = rc.at(mode_ch);
        const int      live_slot = rc_mode_slot_for_pwm(mode_pwm);

        if (mode_pwm > 0)
            ImGui::TextDisabled("channel %d at %u \xc2\xb5s \xe2\x86\x92 position %d",
                                mode_ch, (unsigned)mode_pwm, live_slot);
        else
            ImGui::TextDisabled("channel %d has no signal", mode_ch);

        // The vehicle's own mode list where it publishes one — custom_mode
        // numbering is per-frame, so a Plane's 3 is not a Copter's 3 and a
        // hardcoded table is right only by luck. The Copter table is the
        // fallback for firmware that predates AVAILABLE_MODES (#435).
        struct ModeChoice { uint32_t mode; std::string label; };
        std::vector<ModeChoice> choices;
        if (vs && !vs->available_modes.empty()) {
            for (const FlightModeInfo& m : vs->available_modes) {
                if (!m.user_selectable()) continue;
                choices.push_back({ m.custom_mode, m.name });
            }
        } else {
            for (const RcModeOption& m : rc_copter_modes())
                choices.push_back({ m.custom_mode, m.label });
        }

        const float row_w = ImGui::GetContentRegionAvail().x;
        const float lbl_w = 74.0f;
        const float num_w = 46.0f;
        const float wr_w  = 60.0f;
        const float cmb_w = std::max(60.0f, row_w - lbl_w - num_w - wr_w - 12.0f);

        for (const RcModeSlot& slot : rc_mode_slots()) {
            ImGui::PushID(slot.index);

            float val = staged_value(params, slot.param, 0.0f);

            // The position the switch is actually in, called out so the row you
            // are editing is the one you can feel under your thumb.
            const bool is_live = (slot.index == live_slot);
            ImGui::AlignTextToFramePadding();
            if (is_live) ImGui::TextColored(accent_col(), "%d \xe2\x97\x8f", slot.index);
            else         ImGui::TextDisabled("%d", slot.index);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s \xe2\x80\x94 PWM %u to %u%s",
                                  slot.param, (unsigned)slot.pwm_lo,
                                  (unsigned)slot.pwm_hi,
                                  is_live ? "\n(switch is here now)" : "");

            const char* preview = "Unknown";
            for (const ModeChoice& c : choices)
                if (c.mode == (uint32_t)val) { preview = c.label.c_str(); break; }

            ImGui::SameLine(lbl_w - 40.0f);
            ImGui::SetNextItemWidth(cmb_w);
            if (ImGui::BeginCombo("##mode", preview, ImGuiComboFlags_HeightLargest)) {
                for (size_t c = 0; c < choices.size(); ++c) {
                    const bool sel = (choices[c].mode == (uint32_t)val);
                    // ##index keeps the ID unique when a vehicle reports two
                    // modes under the same name.
                    const std::string item =
                        choices[c].label + "##" + std::to_string(c);
                    if (ImGui::Selectable(item.c_str(), sel)) {
                        st().bind_edits[slot.param] = (float)choices[c].mode;
                        val = (float)choices[c].mode;
                    }
                    if (sel) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            // The number stays editable: the list above is only as complete as
            // what the vehicle published, and a mode it did not name is still a
            // mode this parameter accepts.
            ImGui::SameLine(0, 4);
            ImGui::SetNextItemWidth(num_w);
            int mode_num = (int)val;
            if (ImGui::InputInt("##mode_num", &mode_num, 0, 0)) {
                mode_num = std::max(0, mode_num);
                st().bind_edits[slot.param] = (float)mode_num;
                val = (float)mode_num;
            }

            ImGui::SameLine(0, 4);
            if (draw_write_row(sender, params, slot.param, val, connected,
                               tsys, tcomp, wr_w))
                gcs_log("rc: %s = mode %d", slot.param, (int)val);

            ImGui::PopID();
        }
    }
    ImGui::PopID();

    // ── Auxiliary function binding ───────────────────────────────────────────
    ImGui::PushID("aux");
    {
        ImGui::Spacing();
        ImGui::TextColored(accent_col(), "AUX FUNCTION");
        ImGui::SameLine();
        ImGui::TextDisabled("(%s)", rc_firmware_label(fw));

        // Channel picker. A list rather than a stepper: this number chooses
        // which parameter is being edited, not a value being tuned, and walking
        // eighteen channels one click at a time to reach the far end is a
        // stepper doing a list's job. Every channel is offered, live or not —
        // binding a switch before its receiver is powered is ordinary setup.
        {
            char preview[16];
            snprintf(preview, sizeof(preview), "CH %d", st().aux_channel);

            ImGui::SetNextItemWidth(92.0f);
            if (ImGui::BeginCombo("##aux_ch", preview)) {
                for (int c = 1; c <= RcChannels::MAX_CHANNELS; ++c) {
                    char item[40];
                    const uint16_t pwm = rc.at(c);
                    // The live reading beside each entry is what tells the
                    // operator which channel is the switch they just flicked.
                    if (pwm > 0)
                        snprintf(item, sizeof(item), "CH %-2d  %u", c, (unsigned)pwm);
                    else
                        snprintf(item, sizeof(item), "CH %-2d  --", c);

                    if (ImGui::Selectable(item, c == st().aux_channel))
                        st().aux_channel = c;
                    if (c == st().aux_channel) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
        }

        // Built after the picker, so the row below always names the channel the
        // list is currently on.
        char aux_id[24];
        if (!rc_aux_param(st().aux_channel, dialect, aux_id, sizeof(aux_id))) {
            ImGui::TextDisabled("Channel %d has no option parameter.", st().aux_channel);
            ImGui::PopID();
            return;
        }

        float val = staged_value(params, aux_id, 0.0f);

        ImGui::SameLine(0, 6);
        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("%s", aux_id);
        if (rc.at(st().aux_channel) > 0) {
            ImGui::SameLine(0, 6);
            ImGui::TextColored(col_data(), "%u", (unsigned)rc.at(st().aux_channel));
        }

        const char* preview = rc_aux_option_label((int)val);
        char preview_buf[48];
        if (!preview) {
            snprintf(preview_buf, sizeof(preview_buf), "Option %d", (int)val);
            preview = preview_buf;
        }

        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::BeginCombo("##aux_opt", preview, ImGuiComboFlags_HeightLargest)) {
            for (const RcAuxOption& o : rc_aux_options()) {
                // Only what this firmware implements. The full table covers four
                // vehicles and most of it is meaningless on any one of them.
                if (!o.supported(fw)) continue;
                const bool sel = (o.value == (uint16_t)val);
                char item[80];
                snprintf(item, sizeof(item), "%3u  %s", (unsigned)o.value, o.label);
                if (ImGui::Selectable(item, sel)) {
                    st().bind_edits[aux_id] = (float)o.value;
                    val = (float)o.value;
                }
                if (sel) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        {
            const float row_w = ImGui::GetContentRegionAvail().x;
            const float in_w  = 70.0f;

            ImGui::SetNextItemWidth(in_w);
            int opt = (int)val;
            if (ImGui::InputInt("##aux_num", &opt, 0, 0)) {
                opt = std::max(0, opt);
                st().bind_edits[aux_id] = (float)opt;
                val = (float)opt;
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("RCn_OPTION value \xe2\x80\x94 any number the\n"
                                  "firmware accepts, listed or not");

            ImGui::SameLine(0, 4);
            if (draw_write_row(sender, params, aux_id, val, connected, tsys, tcomp,
                               row_w - in_w - 4.0f)) {
                const char* name = rc_aux_option_label((int)val);
                gcs_log("rc: %s = %d%s%s", aux_id, (int)val,
                        name ? " " : "", name ? name : "");
            }
        }
    }
    ImGui::PopID();
}

// ── Joystick ─────────────────────────────────────────────────────────────────
//
// Separate from the radio's sections below, because its results go somewhere
// else: this calibration stays on this computer, and the radio's goes to the
// vehicle. The device is global, so nothing here lives in RcTabState.

// The whole section wears the plugin rail's accent in place of amber, by the rail's
// own rule: the same panels and wells, blue wherever amber would be. That tells it
// from the radio below at a glance. The chrome reads its colours from g_theme all
// through ui_kit, so they are swapped for this one section and put straight back.
class JoystickTint {
public:
    JoystickTint()
        : accent_(g_theme.accent),
          active_base_(g_theme.btn_tab_active_base), active_hov_(g_theme.btn_tab_active_hov),
          write_base_(g_theme.btn_write_base), write_hov_(g_theme.btn_write_hov)
    {
        const ImVec4 a = g_theme.plugin_accent;
        const auto shade = [&](float k, float alpha) {
            return ImVec4{ a.x * k, a.y * k, a.z * k, alpha };
        };
        g_theme.accent              = a;
        g_theme.btn_tab_active_base = shade(0.92f, active_base_.w);
        g_theme.btn_tab_active_hov  = shade(1.00f, active_hov_.w);
        g_theme.btn_write_base      = shade(0.36f, write_base_.w);
        g_theme.btn_write_hov       = shade(0.50f, write_hov_.w);
        ImGui::PushStyleColor(ImGuiCol_CheckMark, a);
    }
    ~JoystickTint()
    {
        ImGui::PopStyleColor();
        g_theme.accent              = accent_;
        g_theme.btn_tab_active_base = active_base_;
        g_theme.btn_tab_active_hov  = active_hov_;
        g_theme.btn_write_base      = write_base_;
        g_theme.btn_write_hov       = write_hov_;
    }
    JoystickTint(const JoystickTint&)            = delete;
    JoystickTint& operator=(const JoystickTint&) = delete;

private:
    ImVec4 accent_, active_base_, active_hov_, write_base_, write_hov_;
};

static constexpr float JOY_BTN_W   = 30.0f;
static constexpr float JOY_BTN_H   = 20.0f;
static constexpr float JOY_BTN_GAP = 3.0f;

// Every button in a grid under the axes, lit while held. A coloured number means it
// has a job, which is how to tell a spare one from across the panel. For display:
// numbers are read here, and chosen in BUTTON ACTIONS.
static void draw_joystick_button_grid(int count, int cols, const JoystickProfile& profile)
{
    ImDrawList*  dl   = ImGui::GetWindowDrawList();
    const ImVec2 p0   = ImGui::GetCursorScreenPos();
    const int    rows = (count + cols - 1) / cols;

    for (int i = 0; i < count; ++i) {
        const ImVec2 a = { p0.x + (i % cols) * (JOY_BTN_W + JOY_BTN_GAP),
                           p0.y + (i / cols) * (JOY_BTN_H + JOY_BTN_GAP) };
        const ImVec2 b = { a.x + JOY_BTN_W, a.y + JOY_BTN_H };

        const bool down  = (joystick_buttons() & (1u << i)) != 0;
        const int  owner = joy_virtual_of_button(profile, i);
        const bool bound = owner >= 0 || profile.buttons[i].usable();

        dl->AddRectFilled(a, b, down ? ui_col_accent() : ui_col_well());
        dl->AddRect(a, b, down ? ui_col_accent() : ui_col_seam());

        char num[8];
        snprintf(num, sizeof(num), "%d", i + 1);
        const ImVec2 sz = ImGui::CalcTextSize(num);
        const ImU32 col = down  ? ui_col(g_theme.bg_child_darker)
                        : bound ? ui_col_accent()
                                : ui_col(g_theme.col_log, 0.70f);
        dl->AddText({ a.x + (JOY_BTN_W - sz.x) * 0.5f, a.y + (JOY_BTN_H - sz.y) * 0.5f },
                    col, num);

        if (ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(a, b)) {
            if (owner >= 0)      ImGui::SetTooltip("Button %d, on V%d", i + 1, owner + 1);
            else if (bound)      ImGui::SetTooltip("Button %d, has an action", i + 1);
            else                 ImGui::SetTooltip("Button %d", i + 1);
        }
    }
    ImGui::Dummy({ 1.0f, rows * (JOY_BTN_H + JOY_BTN_GAP) - JOY_BTN_GAP });
}

static void draw_joystick_axes(const RcChannels& frame)
{
    const JoystickProfile& profile = joystick_profile();
    const RcCalibration&   cal     = joystick_calibration();
    const int              n       = frame.usable_count();
    const int              buttons = joystick_button_count();

    if (n == 0 && buttons == 0) {
        ImGui::TextColored(col_warning(), "This device reports no axes or buttons.");
        return;
    }

    ImGui::PushStyleColor(ImGuiCol_ChildBg, bg_param_list());
    ImGui::PushStyleColor(ImGuiCol_Border,  col_separator());
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, FRAME_BORDER_NORMAL);
    int rows = n;
    for (const JoyVirtualChannel& v : profile.virtuals)
        if (v.used()) ++rows;
    float list_h = rows * (ROW_H + 2.0f) + ImGui::GetStyle().WindowPadding.y * 2.0f;

    // How many fit across depends on the sidebar, so the rows are worked out here,
    // where the box's height is decided. A 2 px rule and spacing sit above the grid.
    int btn_cols = 1;
    if (buttons > 0) {
        const float inner = ImGui::GetContentRegionAvail().x - ImGui::GetStyle().WindowPadding.x * 2.0f;
        btn_cols = std::max(1, (int)((inner + JOY_BTN_GAP) / (JOY_BTN_W + JOY_BTN_GAP)));
        const int btn_rows = (buttons + btn_cols - 1) / btn_cols;
        list_h += 8.0f + btn_rows * (JOY_BTN_H + JOY_BTN_GAP);
    }

    if (ImGui::BeginChild("##joy_axes", { -1.0f, list_h }, true,
                          ImGuiWindowFlags_NoScrollbar)) {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 2.0f, 2.0f });
        for (int i = 0; i < n; ++i) {
            const char* bound = nullptr;
            for (int f = 0; f < JOY_FN_COUNT; ++f)
                if (profile.bindings[f].axis == i)
                    bound = joy_fn_info((JoyFn)f).label;
            char lbl[8];
            snprintf(lbl, sizeof(lbl), "%d", i + 1);
            draw_channel_row(lbl, frame.chan[i],
                             cal.phase() == RcCalibration::Phase::Idle
                                 ? nullptr : &cal.channel(i),
                             bound);
        }
        // Named V rather than numbered on: these are not more of the stick's axes.
        for (int i = 0; i < JOY_MAX_VIRTUAL; ++i) {
            const JoyVirtualChannel& v = profile.virtuals[i];
            if (!v.used()) continue;
            const int pos = joystick_virtual_position(i);
            char lbl[8], where[32];
            snprintf(lbl, sizeof(lbl), "V%d", i + 1);
            if (pos >= 0) snprintf(where, sizeof(where), "position %d of %d", pos + 1, v.positions);
            else          snprintf(where, sizeof(where), "no position yet");
            draw_channel_row(lbl, v.pwm(pos), nullptr, where);
        }
        if (buttons > 0) {
            ImGui::Dummy({ 1.0f, 2.0f });
            const ImVec2 r = ImGui::GetCursorScreenPos();
            ImGui::GetWindowDrawList()->AddLine(
                { r.x, r.y }, { r.x + ImGui::GetContentRegionAvail().x, r.y }, ui_col_seam());
            ImGui::Dummy({ 1.0f, 2.0f });
            draw_joystick_button_grid(buttons, btn_cols, profile);
        }
        ImGui::PopStyleVar();
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);
}

static void draw_joystick_calibration(const RcChannels& frame, bool available)
{
    RcCalibration& cal = joystick_calibration();

    switch (cal.phase()) {

    case RcCalibration::Phase::Idle: {
        const char* label = joystick_profile().calibrated
                          ? "RECALIBRATE JOYSTICK###joy_cal"
                          : "CALIBRATE JOYSTICK###joy_cal";
        ImGui::BeginDisabled(!available);
        if (ui_grid_button(label, { -1.0f, 26.0f }) && available) {
            cal.begin(RcChannels::MAX_CHANNELS);
            gcs_log("joystick calibration started");
        }
        ImGui::EndDisabled();
        break;
    }

    case RcCalibration::Phase::Center: {
        ImGui::TextColored(col_warning(), "1 / 3  CENTRE");
        ImGui::TextWrapped("Let the sticks return to centre, then capture.");
        ImGui::Spacing();

        const float half = (ImGui::GetContentRegionAvail().x - 4.0f) * 0.5f;
        if (ui_solid_button("CAPTURE##joy", { half, 26.0f },
                            btn_write_base(), btn_write_hov()) &&
            !cal.capture_center(frame)) {
            gcs_log("joystick calibration: no usable axes to capture");
            gcs_tone(GcsTone::Failure);
        }
        ImGui::SameLine(0, 4);
        if (ui_grid_button("CANCEL##joycal", { -1.0f, 26.0f })) cal.cancel();
        break;
    }

    case RcCalibration::Phase::Sweep: {
        ImGui::TextColored(col_warning(), "2 / 3  SWEEP");
        ImGui::TextWrapped("Move every axis to both stops. The amber band on each "
                           "bar is the travel recorded so far.");
        ImGui::Spacing();

        const int swept = cal.usable_count();
        const int total = cal.active_count();
        char prog[48];
        snprintf(prog, sizeof(prog), "%d / %d axes", swept, total);
        ui_progress_bar(prog, total > 0 ? (float)swept / total : 0.0f, 12.0f);
        ImGui::Spacing();

        const float half = (ImGui::GetContentRegionAvail().x - 4.0f) * 0.5f;
        ImGui::BeginDisabled(swept == 0);
        if (ui_solid_button("FINISH##joy", { half, 26.0f },
                            btn_write_base(), btn_write_hov()))
            cal.finish();
        ImGui::EndDisabled();
        ImGui::SameLine(0, 4);
        if (ui_grid_button("CANCEL##joycal", { -1.0f, 26.0f })) cal.cancel();
        break;
    }

    case RcCalibration::Phase::Review: {
        ImGui::TextColored(col_warning(), "3 / 3  REVIEW");
        ImGui::TextWrapped("%d of %d axes recorded. An axis that was not moved "
                           "cannot be bound. Saved on this computer; nothing is "
                           "written to the vehicle.",
                           cal.usable_count(), cal.active_count());
        ImGui::Spacing();

        const float half = (ImGui::GetContentRegionAvail().x - 4.0f) * 0.5f;
        if (ui_solid_button("SAVE##joy", { half, 26.0f },
                            btn_write_base(), btn_write_hov()))
            joystick_save_calibration();
        ImGui::SameLine(0, 4);
        if (ui_grid_button("DISCARD##joy", { -1.0f, 26.0f })) cal.cancel();
        break;
    }
    }
}

static void draw_joystick_bindings(const RcChannels& frame, bool editable)
{
    const JoystickProfile& profile = joystick_profile();

    constexpr float LBL_W = 74.0f;
    constexpr float REV_W = 52.0f;
    constexpr float DET_W = 58.0f;
    const float row_w = ImGui::GetContentRegionAvail().x;
    const float cmb_w = std::max(50.0f, row_w - LBL_W - REV_W - DET_W - 8.0f);

    ImGui::PushID("joybind");
    ImGui::BeginDisabled(!editable);
    for (int f = 0; f < JOY_FN_COUNT; ++f) {
        const JoyFn         fn   = (JoyFn)f;
        const JoyFnInfo&    info = joy_fn_info(fn);
        const JoyFnBinding& b    = profile.bindings[f];

        ImGui::PushID(f);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(info.label);

        char preview[16];
        if (b.axis >= 0) snprintf(preview, sizeof(preview), "AXIS %d", b.axis + 1);
        else             snprintf(preview, sizeof(preview), "--");

        ImGui::SameLine(LBL_W);
        ImGui::SetNextItemWidth(cmb_w);
        if (ImGui::BeginCombo("##axis", preview)) {
            if (ImGui::Selectable("--", b.axis < 0))
                joystick_set_binding(fn, -1, false);
            for (int i = 0; i < frame.usable_count(); ++i) {
                char item[32];
                snprintf(item, sizeof(item), "AXIS %-2d  %u", i + 1,
                         (unsigned)frame.chan[i]);
                if (ImGui::Selectable(item, b.axis == i))
                    joystick_set_binding(fn, i, b.reversed);
            }
            ImGui::EndCombo();
        }

        ImGui::SameLine(0, 4);
        bool rev = b.reversed;
        ImGui::BeginDisabled(b.axis < 0);
        if (ImGui::Checkbox("REV", &rev))
            joystick_set_binding(fn, b.axis, rev);
        ImGui::EndDisabled();

        ImGui::SameLine(row_w - DET_W);
        const bool armed = joystick_detect_target() == f;
        if (ui_grid_button(armed ? "PUSH" : "DETECT", { DET_W, 0.0f }, armed)) {
            if (armed) {
                joystick_detect_cancel();
            } else {
                joystick_detect_start(fn);
                gcs_log("joystick: push %s %s", info.label, info.positive);
            }
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Bind by pushing the stick %s", info.positive);

        ImGui::PopID();
    }
    ImGui::EndDisabled();
    ImGui::PopID();
}

// The one line a bound button gets in the list.
static std::string joy_button_summary(const JoyButtonBinding& b)
{
    static const char* const POS[] = { "LOW", "MID", "HIGH" };
    static const char* const TRIG[] = { "", " hold", " toggle" };
    const char* trig = TRIG[(int)b.trigger];

    char out[96] = "";
    switch (b.action) {
    case JoyBtnAction::None:
        break;
    case JoyBtnAction::Mode:
        snprintf(out, sizeof(out), "Mode %s", b.mode_name.c_str());
        break;
    case JoyBtnAction::Aux: {
        const char* name = rc_aux_option_label(b.aux_fn);
        if (b.trigger == JoyBtnTrigger::Press)
            snprintf(out, sizeof(out), "%s %s", name ? name : "Aux", POS[std::min<int>(b.on, 2)]);
        else
            snprintf(out, sizeof(out), "%s%s", name ? name : "Aux", trig);
        break;
    }
    case JoyBtnAction::Servo:
        if (b.trigger == JoyBtnTrigger::Press)
            snprintf(out, sizeof(out), "Servo %u %u", (unsigned)b.servo, (unsigned)b.on);
        else
            snprintf(out, sizeof(out), "Servo %u %u/%u%s", (unsigned)b.servo,
                     (unsigned)b.on, (unsigned)b.off, trig);
        break;
    }
    return out;
}

// Row of buttons acting as one choice. True if the choice changed.
template <class T>
static bool joy_choice_row(const char* const* labels, int count, T& value)
{
    const float w = (ImGui::GetContentRegionAvail().x - 4.0f * (count - 1)) / count;
    bool changed = false;
    for (int i = 0; i < count; ++i) {
        if (i) ImGui::SameLine(0, 4);
        if (ui_grid_button(labels[i], { w, 0.0f }, (int)value == i) && (int)value != i) {
            value   = (T)i;
            changed = true;
        }
    }
    return changed;
}

static bool joy_trigger_row(JoyButtonBinding& b)
{
    static const char* const LABELS[] = { "PRESS##joyt", "HOLD##joyt", "TOGGLE##joyt" };
    const bool changed = joy_choice_row(LABELS, 3, b.trigger);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("PRESS: sends ON, release does nothing\n"
                          "HOLD: ON while held, OFF on release\n"
                          "TOGGLE: ON and OFF on alternate presses");
    return changed;
}

static bool joy_edit_mode(JoyButtonBinding& b, const VehicleState* vs, bool connected)
{
    if (!connected) {
        ImGui::TextDisabled(b.mode_name.empty() ? "Connect to a vehicle to choose a mode."
                                                : "Mode %s. Connect to change it.",
                            b.mode_name.c_str());
        return false;
    }

    struct Choice { std::string name; uint32_t custom; uint8_t standard; };
    std::vector<Choice> choices;
    if (!vs->available_modes.empty()) {
        for (const FlightModeInfo& m : vs->available_modes)
            if (m.user_selectable())
                choices.push_back({ m.name, m.custom_mode, m.standard_mode });
    } else {
        for (const FallbackMode& m : firmware_profile(*vs).fallback_modes(vs->type))
            choices.push_back({ m.label, m.custom_mode, 0 });
    }

    bool changed = false;
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::BeginCombo("##joy_mode", b.mode_name.empty() ? "Choose a mode"
                                                            : b.mode_name.c_str(),
                          ImGuiComboFlags_HeightLargest)) {
        for (size_t i = 0; i < choices.size(); ++i) {
            const std::string item = choices[i].name + "##" + std::to_string(i);
            if (ImGui::Selectable(item.c_str(), choices[i].name == b.mode_name)) {
                b.mode_name      = choices[i].name;
                b.mode_custom    = choices[i].custom;
                b.mode_standard  = choices[i].standard;
                b.mode_autopilot = (uint8_t)vs->autopilot;
                b.mode_type      = (uint8_t)vs->type;
                changed = true;
            }
        }
        ImGui::EndCombo();
    }
    return changed;
}

// `switch_position` is for a virtual channel's position, which has no press or
// release of its own: it arrives, and sends one value.
static bool joy_edit_aux(JoyButtonBinding& b, const VehicleState* vs, bool connected,
                         bool switch_position)
{
    const RcFirmware fw = rc_firmware_for(connected ? vs->type : MAV_TYPE_QUADROTOR);
    const Capability cap = firmware_profile(connected ? vs->autopilot
                                                      : MAV_AUTOPILOT_GENERIC).aux_functions();
    if (connected && !cap.supported && cap.reason)
        ImGui::TextColored(col_warning(), "%s", cap.reason);

    bool changed = false;
    const char* name = rc_aux_option_label(b.aux_fn);
    char preview[64];
    if (b.aux_fn == 0)  snprintf(preview, sizeof(preview), "Choose a function");
    else if (name)      snprintf(preview, sizeof(preview), "%s", name);
    else                snprintf(preview, sizeof(preview), "Option %u", (unsigned)b.aux_fn);

    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::BeginCombo("##joy_aux", preview, ImGuiComboFlags_HeightLargest)) {
        for (const RcAuxOption& o : rc_aux_options()) {
            if (o.value == 0 || !o.supported(fw)) continue;
            char item[80];
            snprintf(item, sizeof(item), "%3u  %s", (unsigned)o.value, o.label);
            if (ImGui::Selectable(item, o.value == b.aux_fn)) {
                b.aux_fn = o.value;
                changed  = true;
            }
        }
        ImGui::EndCombo();
    }

    if (joy_aux_is_critical(b.aux_fn))
        ImGui::TextColored(col_error(), "Can stop the motors in flight.");

    if (!switch_position) changed |= joy_trigger_row(b);

    static const char* const POS[] = { "LOW", "MID", "HIGH" };
    const auto pos_row = [&](const char* label, const char* id, uint16_t& pos) {
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(label);
        ImGui::SameLine(36.0f);
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::BeginCombo(id, POS[std::min<int>(pos, 2)])) {
            for (int p = 0; p < 3; ++p)
                if (ImGui::Selectable(POS[p], pos == p)) {
                    pos     = (uint16_t)p;
                    changed = true;
                }
            ImGui::EndCombo();
        }
    };
    pos_row(switch_position ? "SET" : "ON", "##joy_aux_on", b.on);
    if (!switch_position && b.trigger != JoyBtnTrigger::Press)
        pos_row("OFF", "##joy_aux_off", b.off);
    return changed;
}

static bool joy_edit_servo(JoyButtonBinding& b, bool switch_position)
{
    bool changed = switch_position ? false : joy_trigger_row(b);

    const auto num_row = [&](const char* label, const char* id, int lo, int hi, int value,
                             auto store) {
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(label);
        ImGui::SameLine(52.0f);
        ImGui::SetNextItemWidth(-1.0f);
        int v = value;
        if (ImGui::InputInt(id, &v, 0, 0)) {
            store(std::max(lo, std::min(v, hi)));
            changed = true;
        }
    };
    num_row("SERVO", "##joy_srv", 1, 16, b.servo, [&](int v) { b.servo = (uint8_t)v; });
    num_row(switch_position ? "PWM" : "ON", "##joy_on", 0, 2200, b.on,
            [&](int v) { b.on = (uint16_t)v; });
    if (!switch_position && b.trigger != JoyBtnTrigger::Press)
        num_row("OFF", "##joy_off", 0, 2200, b.off, [&](int v) { b.off = (uint16_t)v; });
    return changed;
}

static void joy_edit_action(JoyButtonBinding& b, const VehicleState* vs, bool connected,
                            bool switch_position, bool& changed)
{
    switch (b.action) {
    case JoyBtnAction::None:  break;
    case JoyBtnAction::Mode:  changed |= joy_edit_mode(b, vs, connected); break;
    case JoyBtnAction::Aux:   changed |= joy_edit_aux(b, vs, connected, switch_position); break;
    case JoyBtnAction::Servo: changed |= joy_edit_servo(b, switch_position); break;
    }
}

static void draw_joystick_buttons(const VehicleState* vs, bool connected, bool editable)
{
    const int count = joystick_button_count();
    if (count == 0) return;

    const JoystickProfile& profile = joystick_profile();

    ImGui::Spacing();
    ImGui::TextColored(accent_col(), "BUTTON ACTIONS");
    ImGui::SameLine(0, 6);
    ImGui::TextDisabled("(act only while sending)");

    ImGui::PushID("joybtn");
    ImGui::BeginDisabled(!editable);

    const int  sel       = std::min(joystick_button_selected(), count - 1);
    const bool listening = joystick_button_detecting();

    // Which number it is matters less than which one is under the thumb, so the
    // picker marks what is down and DETECT takes the next press.
    char preview[24];
    snprintf(preview, sizeof(preview), "BUTTON %d%s", sel + 1,
             (joystick_buttons() & (1u << sel)) ? " \xe2\x80\xa2" : "");

    constexpr float DET_W = 58.0f;
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - DET_W - 4.0f);
    if (ImGui::BeginCombo("##joy_btn_pick", preview)) {
        for (int i = 0; i < count; ++i) {
            const int owner = joy_virtual_of_button(profile, i);
            char item[32];
            if (owner >= 0)
                snprintf(item, sizeof(item), "BUTTON %d  on V%d", i + 1, owner + 1);
            else
                snprintf(item, sizeof(item), "BUTTON %d%s", i + 1,
                         (joystick_buttons() & (1u << i)) ? " \xe2\x80\xa2" : "");
            if (ImGui::Selectable(item, i == sel, owner >= 0 ? ImGuiSelectableFlags_Disabled : 0))
                joystick_button_select(i);
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine(0, 4);
    if (ui_grid_button(listening ? "PUSH" : "DETECT", { DET_W, 0.0f }, listening))
        joystick_button_detect(!listening);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Choose a button by pressing it");

    const int owner = joy_virtual_of_button(profile, sel);
    if (owner >= 0) {
        // DETECT can land here by pressing a switch, which is worth explaining.
        ImGui::TextDisabled("Button %d belongs to V%d. Its action is set there.",
                            sel + 1, owner + 1);
    } else {
        JoyButtonBinding b = profile.buttons[sel];

        static const char* const ACTIONS[] = { "NONE##joya", "MODE##joya", "AUX##joya", "SERVO##joya" };
        bool changed = joy_choice_row(ACTIONS, 4, b.action);
        if (changed) {
            // Starting points that do nothing surprising: PRESS, and a servo's usual
            // travel. An aux switch goes HIGH, and LOW on the way back if asked to.
            const JoyBtnAction a = b.action;
            b = JoyButtonBinding{};
            b.action = a;
            if (a == JoyBtnAction::Aux)   { b.on = 2;    b.off = 0;    }
            if (a == JoyBtnAction::Servo) { b.on = 1900; b.off = 1100; }
        }
        joy_edit_action(b, vs, connected, false, changed);
        if (changed) joystick_set_button(sel, b);
    }

    ImGui::EndDisabled();

    // Everything assigned, so nobody has to walk the picker to find out. Boxed and
    // titled: directly under the editor, a bare row reads as one more of its options.
    int assigned = 0;
    for (int i = 0; i < count; ++i)
        if (profile.buttons[i].usable()) ++assigned;

    if (assigned > 0) {
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::TextColored(accent_col(), "ASSIGNED BUTTONS");
        ImGui::SameLine(0, 6);
        ImGui::TextColored(col_data(), "%d", assigned);

        ImGui::PushStyleColor(ImGuiCol_ChildBg, bg_param_list());
        ImGui::PushStyleColor(ImGuiCol_Border,  col_separator());
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, FRAME_BORDER_NORMAL);

        const float row_h = ImGui::GetTextLineHeightWithSpacing();
        const float box_h = assigned * row_h + ImGui::GetStyle().WindowPadding.y * 2.0f;
        if (ImGui::BeginChild("##joy_assigned", { -1.0f, box_h }, true,
                              ImGuiWindowFlags_NoScrollbar)) {
            for (int i = 0; i < count; ++i) {
                const JoyButtonBinding& bound = profile.buttons[i];
                if (!bound.usable()) continue;
                ImGui::PushID(i);
                if (ImGui::Selectable("##assigned", i == sel)) joystick_button_select(i);
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Edit this button");
                ImGui::SameLine(ImGui::GetStyle().WindowPadding.x + 2.0f);
                ImGui::TextColored(accent_col(), "BTN %-2d", i + 1);
                ImGui::SameLine(0, 10);
                ImGui::TextUnformatted(joy_button_summary(bound).c_str());
                ImGui::PopID();
            }
        }
        ImGui::EndChild();

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);
    }
    ImGui::PopID();
}

// "button 4", "buttons 4+5", or "none" for the position with nothing pressed.
static std::string joy_pattern_text(uint32_t pattern)
{
    std::string out;
    int n = 0;
    for (int i = 0; i < JOY_MAX_BUTTONS; ++i) {
        if (!(pattern & (1u << i))) continue;
        out += (n++ ? "+" : "") + std::to_string(i + 1);
    }
    if (n == 0) return "none";
    return (n == 1 ? "button " : "buttons ") + out;
}

static void joy_wrapped_error(const char* text)
{
    ImGui::PushStyleColor(ImGuiCol_Text, col_error());
    ImGui::TextWrapped("%s", text);
    ImGui::PopStyleColor();
}

static void draw_joystick_virtual_capture()
{
    const int step  = joystick_virtual_capture_step();
    const int total = joystick_virtual_capture_total();

    ImGui::TextColored(col_warning(), "POSITION %d OF %d", step + 1, total);
    ImGui::TextWrapped("Put the switch in position %d and leave it there. Now reading: %s.",
                       step + 1, joy_pattern_text(joystick_buttons()).c_str());
    if (const char* why = joystick_virtual_capture_problem()) joy_wrapped_error(why);
    ImGui::Spacing();

    const float half = (ImGui::GetContentRegionAvail().x - 4.0f) * 0.5f;
    if (ui_solid_button("CAPTURE##joyv", { half, 26.0f }, btn_write_base(), btn_write_hov()))
        joystick_virtual_capture();
    ImGui::SameLine(0, 4);
    if (ui_grid_button("CANCEL##joyv", { -1.0f, 26.0f }))
        joystick_virtual_cancel();
}

static void draw_joystick_virtuals(const VehicleState* vs, bool connected, bool editable)
{
    if (joystick_button_count() == 0) return;

    const JoystickProfile& profile = joystick_profile();

    static int s_new_positions = 3;
    static int s_open          = -1;   // the channel being edited
    static int s_open_position = 0;

    ImGui::Spacing();
    ImGui::TextColored(accent_col(), "VIRTUAL CHANNELS");
    ImGui::SameLine(0, 6);
    ImGui::TextDisabled("(multi-position switches)");

    ImGui::PushID("joyvirt");
    ImGui::BeginDisabled(!editable);

    int free_slots = 0;
    for (const JoyVirtualChannel& v : profile.virtuals)
        if (!v.used()) ++free_slots;

    if (joystick_virtual_capturing()) {
        draw_joystick_virtual_capture();
    } else {
        constexpr float NUM_W = 76.0f;
        char count_lbl[24];
        snprintf(count_lbl, sizeof(count_lbl), "%d POS", s_new_positions);
        ImGui::SetNextItemWidth(NUM_W);
        if (ImGui::BeginCombo("##joyv_count", count_lbl)) {
            for (int n = JOY_VIRTUAL_MIN_POS; n <= JOY_VIRTUAL_MAX_POS; ++n) {
                char item[24];
                snprintf(item, sizeof(item), "%d POS", n);
                if (ImGui::Selectable(item, n == s_new_positions)) s_new_positions = n;
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine(0, 4);
        ImGui::BeginDisabled(free_slots == 0);
        if (ui_grid_button("ADD VIRTUAL CHANNEL", { -1.0f, 0.0f }))
            joystick_virtual_begin(s_new_positions);
        ImGui::EndDisabled();
        if (const char* why = joystick_virtual_capture_problem()) joy_wrapped_error(why);
    }

    for (int i = 0; i < JOY_MAX_VIRTUAL; ++i) {
        const JoyVirtualChannel& v = profile.virtuals[i];
        if (!v.used()) continue;
        ImGui::PushID(i);

        const bool open = (s_open == i);
        char head[32];
        snprintf(head, sizeof(head), "V%d", i + 1);
        constexpr float REMOVE_W = 64.0f, EDIT_W = 46.0f;
        ImGui::Spacing();
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(accent_col(), "%s", head);
        ImGui::SameLine(0, 6);
        ImGui::TextDisabled("%s", joy_pattern_text(v.mask()).c_str());

        const float right = ImGui::GetWindowContentRegionMax().x;
        ImGui::SameLine(right - REMOVE_W - EDIT_W - 4.0f);
        if (ui_grid_button("EDIT", { EDIT_W, 0.0f }, open)) {
            s_open          = open ? -1 : i;
            s_open_position = 0;
        }
        ImGui::SameLine(right - REMOVE_W);
        if (ui_grid_button("REMOVE", { REMOVE_W, 0.0f }))
            ImGui::OpenPopup("##confirm_joyv_remove");

        char q[96];
        snprintf(q, sizeof(q), "REMOVE V%d AND ITS POSITION ACTIONS?", i + 1);
        if (ui_confirm_popup("##confirm_joyv_remove", "REMOVE VIRTUAL CHANNEL", q,
                             "REMOVE", btn_stop_base()) == UiConfirm::Confirmed) {
            joystick_virtual_remove(i);
            if (s_open == i) s_open = -1;
            ImGui::PopID();
            continue;
        }

        // Each position, what makes it and what it does, with the live one marked.
        const int live_pos = joystick_virtual_position(i);
        for (int p = 0; p < v.positions; ++p) {
            const std::string pat = joy_pattern_text(v.patterns[p]);
            const std::string act = v.actions[p].usable()
                                  ? joy_button_summary(v.actions[p]) : std::string("no action");
            const ImVec4 col = (p == live_pos) ? accent_col() : ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);
            ImGui::TextColored(col, "%s %d  %u  %s", p == live_pos ? ">" : " ", p + 1,
                               (unsigned)v.pwm(p), pat.c_str());
            ImGui::TextColored(col, "      %s", act.c_str());
        }

        if (open) {
            ImGui::Spacing();
            s_open_position = std::min(s_open_position, v.positions - 1);

            char pos_lbl[JOY_VIRTUAL_MAX_POS][16];
            const char* pos_ptrs[JOY_VIRTUAL_MAX_POS];
            for (int p = 0; p < v.positions; ++p) {
                snprintf(pos_lbl[p], sizeof(pos_lbl[p]), "POS %d##joyp", p + 1);
                pos_ptrs[p] = pos_lbl[p];
            }
            joy_choice_row(pos_ptrs, v.positions, s_open_position);

            JoyButtonBinding b = v.actions[s_open_position];
            static const char* const ACTIONS[] = { "NONE##joyva", "MODE##joyva", "AUX##joyva", "SERVO##joyva" };
            bool changed = joy_choice_row(ACTIONS, 4, b.action);
            if (changed) {
                // A switch usually drives one function through its positions, so
                // the other positions' choice carries over, and the position
                // itself suggests where it goes: LOW, MID, HIGH, or an even PWM.
                const JoyBtnAction a = b.action;
                b = JoyButtonBinding{};
                b.action = a;
                for (int p = 0; p < v.positions; ++p) {
                    const JoyButtonBinding& o = v.actions[p];
                    if (o.action != a) continue;
                    b.aux_fn = o.aux_fn;
                    b.servo  = o.servo;
                    break;
                }
                if (a == JoyBtnAction::Aux)
                    b.on = s_open_position == 0 ? 0
                         : s_open_position == v.positions - 1 ? 2 : 1;
                if (a == JoyBtnAction::Servo) b.on = v.pwm(s_open_position);
            }
            joy_edit_action(b, vs, connected, true, changed);
            if (changed) joystick_virtual_set_action(i, s_open_position, b);
        }

        ImGui::PopID();
    }

    ImGui::EndDisabled();
    ImGui::PopID();
}

// One channel's row: the RC channel it fills, its direction, and what it sends.
// `taken` marks RC channels another row has, which cannot be chosen twice.
static void joy_rc_row(const char* lbl, JoyRcOut out, uint32_t taken, uint16_t value,
                       bool is_virtual, int index)
{
    constexpr float LBL_W = 32.0f, CMB_W = 92.0f;

    ImGui::PushID(is_virtual ? 100 + index : index);
    ImGui::AlignTextToFramePadding();
    if (out.channel) ImGui::TextColored(accent_col(), "%s", lbl);
    else             ImGui::TextDisabled("%s", lbl);

    char preview[16] = "--";
    if (out.channel) snprintf(preview, sizeof(preview), "RC %d", out.channel);

    bool changed = false;
    ImGui::SameLine(LBL_W);
    ImGui::SetNextItemWidth(CMB_W);
    if (ImGui::BeginCombo("##rc_ch", preview, ImGuiComboFlags_HeightLargest)) {
        if (ImGui::Selectable("--", out.channel == 0)) { out.channel = 0; changed = true; }
        for (int ch = 1; ch <= JOY_RC_MAX_CHANNEL; ++ch) {
            const bool other = (taken & (1u << ch)) != 0 && ch != out.channel;
            char item[16];
            snprintf(item, sizeof(item), "RC %d", ch);
            if (ImGui::Selectable(item, ch == out.channel,
                                  other ? ImGuiSelectableFlags_Disabled : 0)) {
                out.channel = ch;
                changed = true;
            }
        }
        ImGui::EndCombo();
    }

    ImGui::SameLine(0, 4);
    ImGui::BeginDisabled(out.channel == 0);
    changed |= ImGui::Checkbox("REV", &out.reversed);
    ImGui::EndDisabled();

    if (out.channel && value != JOY_RC_IGNORE) {
        ImGui::SameLine(0, 8);
        ImGui::TextColored(col_data(), "%u", (unsigned)value);
    }

    if (changed) {
        if (is_virtual) joystick_set_virtual_rc(index, out);
        else            joystick_set_axis_rc(index, out);
    }
    ImGui::PopID();
}

static void draw_joystick_rc_override(const RcChannels& frame, bool editable)
{
    const JoystickProfile& profile = joystick_profile();

    ImGui::Spacing();
    ImGui::PushID("joyrc");
    ImGui::BeginDisabled(!editable);

    bool on = profile.rc_override;
    if (ImGui::Checkbox("RC OVERRIDE", &on)) joystick_set_rc_override(on);
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        ImGui::SetTooltip("Disconnect any other RC transmitter before using this.\n"
                          "A radio left on is overridden while this sends, and takes\n"
                          "those channels back a few seconds after it stops.\n\n"
                          "Sends RC_CHANNELS_OVERRIDE (#70) instead of MANUAL_CONTROL.");

    if (on) {
        uint16_t values[RcChannels::MAX_CHANNELS];
        joystick_rc_values(values);

        uint32_t taken = 0;
        for (int a = 0; a < frame.usable_count(); ++a)
            taken |= 1u << profile.rc_axes[a].channel;
        for (const JoyVirtualChannel& v : profile.virtuals)
            if (v.used()) taken |= 1u << v.rc.channel;

        const auto value_of = [&](const JoyRcOut& o) {
            return o.channel >= 1 && o.channel <= JOY_RC_MAX_CHANNEL
                 ? values[o.channel - 1] : JOY_RC_IGNORE;
        };
        for (int a = 0; a < frame.usable_count(); ++a) {
            char lbl[8];
            snprintf(lbl, sizeof(lbl), "%d", a + 1);
            joy_rc_row(lbl, profile.rc_axes[a], taken, value_of(profile.rc_axes[a]), false, a);
        }
        for (int i = 0; i < JOY_MAX_VIRTUAL; ++i) {
            const JoyVirtualChannel& v = profile.virtuals[i];
            if (!v.used()) continue;
            char lbl[8];
            snprintf(lbl, sizeof(lbl), "V%d", i + 1);
            joy_rc_row(lbl, v.rc, taken, value_of(v.rc), true, i);
        }
    }

    ImGui::EndDisabled();
    ImGui::PopID();
}

// What is on its way to the vehicle: the fields of the message itself, not the
// stick. Off unless asked for.
static void draw_joystick_output(bool profile_ok)
{
    static bool s_show = false;

    ImGui::Spacing();
    if (ui_grid_button("READ OUTPUT##joy", { -1.0f, 0.0f }, s_show)) s_show = !s_show;
    if (!s_show) return;

    const bool live = joystick_live();
    const bool rc   = joystick_profile().rc_override;

    // While sending this is the frame the link last wrote. Otherwise nothing is
    // going anywhere, so it is a preview, and has to say so: the numbers look the
    // same either way.
    MavlinkSender::ManualControlFrame f;
    if (live) {
        f = joystick_output_frame();
        ImGui::TextColored(col_warning(), "SENDING to vehicle (%u)",
                           (unsigned)joystick_live_number());
    } else {
        const JoyCommand c = joystick_command();
        const uint32_t   b = joystick_output_buttons();
        f.x = c.x; f.y = c.y; f.z = c.z; f.r = c.r;
        f.buttons  = (uint16_t)(b & 0xFFFFu);
        f.buttons2 = (uint16_t)(b >> 16);
        ImGui::TextDisabled("PREVIEW. Nothing is being sent.");
    }

    ImGui::PushStyleColor(ImGuiCol_ChildBg, bg_param_list());
    ImGui::PushStyleColor(ImGuiCol_Border,  col_separator());
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, FRAME_BORDER_NORMAL);

    const float line = ImGui::GetTextLineHeightWithSpacing();
    // The same frame's other half. Only the message the profile sends is shown:
    // the other is not going out, however its numbers would read.
    uint16_t rc_chan[RcChannels::MAX_CHANNELS];
    int      rc_rows = 0;
    if (rc) {
        if (live) {
            const MavlinkSender::RcOverrideFrame rf = joystick_output_rc_frame();
            std::copy(rf.chan, rf.chan + RcChannels::MAX_CHANNELS, rc_chan);
            f.target = rf.target;
        } else {
            joystick_rc_values(rc_chan);
        }
        for (uint16_t v : rc_chan)
            if (v != JOY_RC_IGNORE && v != 0) ++rc_rows;
    }

    const float mc_lines = rc ? 3.0f + (float)rc_rows : 8.0f;
    if (ImGui::BeginChild("##joy_out_mc", { -1.0f, line * mc_lines + 10.0f }, true,
                          ImGuiWindowFlags_NoScrollbar)) {
        ImGui::TextColored(accent_col(), rc ? "RC_CHANNELS_OVERRIDE #70"
                                            : "MANUAL_CONTROL #69");
        if (live) {
            const char* rate_hint = joystick_output_has_frame() ? "" : "  waiting for link";
            ImGui::SameLine(0, 6);
            ImGui::TextColored(col_data(), "%.1f Hz%s", joystick_output_hz(), rate_hint);
        }

        const auto field = [](const char* name, const char* meaning, int value) {
            ImGui::TextDisabled("%-8s", name);
            ImGui::SameLine(72.0f);
            ImGui::TextColored(col_data(), "%+6d", value);
            ImGui::SameLine(0, 8);
            ImGui::TextDisabled("%s", meaning);
        };
        if (live) {
            ImGui::TextDisabled("%-8s", "target");
            ImGui::SameLine(72.0f);
            ImGui::TextColored(col_data(), "%6u", (unsigned)f.target);
        } else {
            ImGui::TextDisabled("target    none");
        }

        if (rc) {
            for (int i = 0; i < RcChannels::MAX_CHANNELS; ++i) {
                if (rc_chan[i] == JOY_RC_IGNORE || rc_chan[i] == 0) continue;
                char name[16];
                snprintf(name, sizeof(name), "chan%d_raw", i + 1);
                ImGui::TextDisabled("%-11s", name);
                ImGui::SameLine(104.0f);
                ImGui::TextColored(col_data(), "%6u", (unsigned)rc_chan[i]);
            }
            ImGui::TextDisabled("All others UINT16_MAX: left alone.");
        } else if (!live && !profile_ok) {
            ImGui::TextDisabled("Calibrate and bind the sticks");
            ImGui::TextDisabled("to see their values.");
        } else {
            field("x", "pitch",    f.x);
            field("y", "roll",     f.y);
            field("z", "throttle", f.z);
            field("r", "yaw",      f.r);
        }
        if (!rc) {
            ImGui::TextDisabled("%-8s", "buttons");
            ImGui::SameLine(72.0f);
            ImGui::TextColored(col_data(), "0x%04X", (unsigned)f.buttons);
            ImGui::TextDisabled("%-8s", "buttons2");
            ImGui::SameLine(72.0f);
            ImGui::TextColored(col_data(), "0x%04X", (unsigned)f.buttons2);
        }
    }
    ImGui::EndChild();

    // Commands that have actually gone, whether or not sending has since stopped.
    const auto& cmds = joystick_output_commands();
    if (ImGui::BeginChild("##joy_out_cmd", { -1.0f, line * 5.0f + 10.0f }, true)) {
        ImGui::TextColored(accent_col(), "COMMAND_LONG #76");
        if (cmds.empty()) ImGui::TextDisabled("None sent yet.");
        const double now = joystick_now();
        for (const JoyOutputCommand& c : cmds) {
            ImGui::TextDisabled("%4.0fs ago  %s", now - c.time, c.source.c_str());
            ImGui::TextWrapped("%s", c.text.c_str());
        }
    }
    ImGui::EndChild();

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);
}

static void draw_joystick_send(const VehicleState* vs, bool connected)
{
    if (joystick_live()) {
        // The chip may have moved on. Say which aircraft the stick is flying
        // when it is not the one this panel is showing.
        if (joystick_live_target() != ui_bound_vehicle())
            ImGui::TextColored(col_warning(), "Flying vehicle (%u), not this one.",
                               (unsigned)joystick_live_number());
        if (ui_solid_button("STOP SENDING##joy", { -1.0f, 26.0f },
                            btn_stop_base(), btn_stop_hov()))
            joystick_disengage();
        return;
    }

    const char* why = joystick_engage_problem();
    if (!connected)
        why = "Connect to a vehicle first.";
    else if (st().cal.phase() != RcCalibration::Phase::Idle)
        why = "Finish the radio calibration first.";

    ImGui::BeginDisabled(why != nullptr);
    if (ui_solid_button("SEND TO VEHICLE##joy", { -1.0f, 26.0f },
                        btn_write_base(), btn_write_hov()) && !why) {
        // Taking over an armed aircraft is what the confirmation is for, and
        // the throttle is what the operator most needs to look at first: this
        // is the one axis the centred-sticks check cannot speak for.
        if (vs && vs->armed) ImGui::OpenPopup("##confirm_joy_send");
        else                 joystick_engage(ui_bound_vehicle());
    }
    ImGui::EndDisabled();
    if (why) ImGui::TextDisabled("%s", why);

    // Under override no one here knows which channel is the throttle, so it shows
    // every value about to go out.
    char q[320];
    if (joystick_profile().rc_override) {
        uint16_t values[RcChannels::MAX_CHANNELS];
        joystick_rc_values(values);
        int used = snprintf(q, sizeof(q), "THE VEHICLE IS ARMED. TAKE CONTROL WITH RC OVERRIDE?");
        int shown = 0;
        for (int i = 0; i < RcChannels::MAX_CHANNELS; ++i)
            if (values[i] != JOY_RC_IGNORE && used < (int)sizeof(q) - 18)
                used += snprintf(q + used, sizeof(q) - used, "%sCH%d %u",
                                 shown++ % 4 == 0 ? "\n" : "   ", i + 1, (unsigned)values[i]);
    } else {
        snprintf(q, sizeof(q), "THE VEHICLE IS ARMED. TAKE CONTROL WITH THROTTLE AT %d%%?",
                 joystick_command().z / 10);
    }
    if (ui_confirm_popup("##confirm_joy_send", "SEND JOYSTICK TO VEHICLE", q,
                         "TAKE CONTROL", btn_write_base()) == UiConfirm::Confirmed)
        joystick_engage(ui_bound_vehicle());
}

// Virtual channels and RC override: most sticks need neither, so the section
// starts closed, whatever the profile has in it.
static void draw_joystick_advanced(const VehicleState* vs, bool connected,
                                   const RcChannels& frame, bool editable)
{
    static bool s_open = false;

    ImGui::Spacing();
    if (ui_grid_button("ADVANCED##joy", { -1.0f, 0.0f }, s_open)) s_open = !s_open;
    if (!s_open) return;

    draw_joystick_virtuals(vs, connected, editable);
    draw_joystick_rc_override(frame, editable);
}

static void draw_joystick(const VehicleState* vs, bool connected)
{
    const JoystickTint tint;

    ui_panel_header("JOYSTICK", joystick_connected() ? nullptr : "NOT CONNECTED");
    ImGui::Spacing();

    if (!joystick_connected()) {
        draw_joystick_calibration(RcChannels{}, false);
        return;
    }

    const std::vector<int> devices = joystick_devices();
    if (devices.size() > 1) {
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::BeginCombo("##joy_device",
                              joystick_device_name(joystick_selected()))) {
            for (int jid : devices) {
                ImGui::PushID(jid);
                if (ImGui::Selectable(joystick_device_name(jid),
                                      jid == joystick_selected()))
                    joystick_select(jid);
                ImGui::PopID();
            }
            ImGui::EndCombo();
        }
    } else {
        ImGui::TextWrapped("%s", joystick_device_name(joystick_selected()));
    }
    ImGui::Spacing();

    // Read after the picker, so a different device is never drawn with the old
    // one's frame.
    const RcChannels frame = joystick_frame();
    const bool       live  = joystick_live();

    draw_joystick_axes(frame);
    ImGui::Spacing();

    draw_joystick_calibration(frame, !live);
    ImGui::Spacing();
    draw_joystick_bindings(frame, !live);
    draw_joystick_buttons(vs, connected, !live);
    draw_joystick_advanced(vs, connected, frame, !live);

    if (!joystick_profile().rc_override &&
        !joy_profile_problem(joystick_profile(), frame.usable_count())) {
        const JoyCommand c = joystick_command();
        ImGui::Spacing();
        ImGui::TextDisabled("R %+5d  P %+5d  T %4d  Y %+5d",
                            (int)c.y, (int)c.x, (int)c.z, (int)c.r);
    }
    ImGui::Spacing();

    draw_joystick_send(vs, connected);
    draw_joystick_output(!joy_profile_problem(joystick_profile(), frame.usable_count()));
}

// ─────────────────────────────────────────────────────────────────────────────

void rc_tab_pump(const VehicleState* vs)
{
    if (!vs || !vs->has_heartbeat || !vs->rc.valid) {
        st().seen_mask = 0;
        return;
    }

    for (int i = 0; i < RcChannels::MAX_CHANNELS; ++i)
        if (vs->rc.chan[i] > 0)
            st().seen_mask |= (1u << i);

    st().cal.update(vs->rc);
}

void draw_tab_rc(MavlinkSender* sender, const VehicleState* vs,
                 const std::unordered_map<std::string, ParamEntry>* params)
{
    const bool    connected = (vs && vs->has_heartbeat);
    const uint8_t tsys      = connected ? vs->sysid  : 1;
    const uint8_t tcomp     = connected ? vs->compid : 1;

    const RcChannels     rc      = vs ? vs->rc : RcChannels{};
    const RcParamDialect dialect = RcCalibration::dialect_for(
        vs ? vs->autopilot : MAV_AUTOPILOT_GENERIC);

    ImGui::Spacing();

    // ── Nothing fetched yet ──────────────────────────────────────────────────
    //
    // Pinned above the scroll region rather than placed inside it: it explains
    // why every WRITE further down is dead, and an explanation you have to
    // scroll back up to find is one the operator will not connect to the
    // greyed-out button in front of them.
    //
    // Only shown on a live link. With no vehicle the parameters are missing for
    // the obvious reason, the header already says "(no link)", and telling
    // someone to fetch from a vehicle that is not there is just noise.
    if (connected && !params_ready(params)) {
        const ImVec4 err = col_error();

        ImGui::PushStyleColor(ImGuiCol_ChildBg, { err.x, err.y, err.z, 0.14f });
        ImGui::PushStyleColor(ImGuiCol_Border,  err);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, FRAME_BORDER_NORMAL);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,   { 6.0f, 5.0f });

        // Measured rather than guessed: the sidebar is a quarter of the window,
        // so how many lines the message wraps to depends on the display, and a
        // fixed height would clip it on a small one. The same splitter does the
        // measuring and the drawing, so the two cannot disagree.
        const char* title = "PARAMETERS NOT FETCHED";
        const char* msg   = "Open the PARAMS tab and press FETCH ALL. "
                            "Writing is disabled until the vehicle's current "
                            "values are known.";

        const float text_w = ImGui::GetContentRegionAvail().x - 12.0f;
        const int   n_line = centered_wrapped(title, text_w, err, false)
                           + centered_wrapped(msg,   text_w, err, false);
        const float box_h  = n_line * ImGui::GetTextLineHeightWithSpacing() + 10.0f;

        if (ImGui::BeginChild("##rc_no_params", { -1.0f, box_h }, true)) {
            centered_wrapped(title, text_w, err, true);
            centered_wrapped(msg,   text_w, err, true);
        }
        ImGui::EndChild();

        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);
        ImGui::Spacing();
    }

    // ── Which channel each axis is bound to, for the monitor's labels ────────
    //
    // Read from the vehicle's parameters with the staged edits on top, so a
    // channel the operator has just re-bound lights up before it is written.
    const char* bind_of_channel[RcChannels::MAX_CHANNELS] = {};
    for (const RcStickBinding& b : rc_stick_bindings()) {
        const char* id = rc_stick_param(b, dialect);
        if (!id) continue;
        const int idx = (int)staged_value(params, id, (float)b.default_ch) - 1;
        if (idx >= 0 && idx < RcChannels::MAX_CHANNELS && !bind_of_channel[idx])
            bind_of_channel[idx] = b.label;
    }

    // ── Channel detection ────────────────────────────────────────────────────
    //
    // Run before anything draws so the result is visible in the same frame the
    // stick moved, and outside the binding section so it keeps working while
    // the operator scrolls away from the row that armed it.
    if (st().detect_target >= 0) {
        if (!rc.valid) {
            st().detect_target = -1;
        } else {
            const int moved = rc_detect_moved_channel(st().detect_baseline, rc);
            if (moved > 0) {
                const auto& bindings = rc_stick_bindings();
                if (st().detect_target < (int)bindings.size()) {
                    const char* id = rc_stick_param(bindings[st().detect_target],
                                                    dialect);
                    if (id) {
                        st().bind_edits[id] = (float)moved;
                        gcs_log("rc: %s detected on channel %d \xe2\x80\x94 press WRITE to bind",
                                bindings[st().detect_target].label, moved);
                        gcs_tone(GcsTone::Success);
                    }
                }
                st().detect_target = -1;
            }
        }
    }

    // Eighteen channel bars, a three-step calibration and six mode slots do not
    // fit a sidebar at any sensible font size, and the sidebar window itself is
    // created with NoScrollbar — so the panel brings its own scroll region
    // rather than quietly clipping everything past the fold.
    if (ImGui::BeginChild("##rc_scroll", { -1.0f, -1.0f }, false)) {
        draw_joystick(vs, connected);

        // A section of its own: everything below is the radio wired to the
        // vehicle, and everything above is the joystick on this computer.
        ImGui::Spacing();
        ImGui::Spacing();
        ui_panel_header("RADIO", connected ? nullptr : "NO LINK");
        ImGui::Spacing();

        draw_monitor(rc, bind_of_channel);

        ImGui::Spacing();
        themed_sep();
        ImGui::Spacing();

        draw_calibration(sender, vs, params, rc, dialect, connected, tsys, tcomp);

        ImGui::Spacing();
        themed_sep();
        ImGui::Spacing();

        draw_binding(sender, vs, params, rc, dialect, connected, tsys, tcomp);
    }
    ImGui::EndChild();
}
