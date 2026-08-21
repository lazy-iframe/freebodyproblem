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
#include "../sidebar_themes.hpp"
#include "../../app_log.hpp"
#include "../../audio.hpp"
#include "../../../backend/rc_binding.hpp"
#include "../../../backend/rc_calibration.hpp"
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

static RcCalibration s_cal;

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
static uint32_t s_seen_mask = 0;

static bool channel_seen(int idx_0based)
{
    return (s_seen_mask & (1u << idx_0based)) != 0u;
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
static bool s_use_param_ext = false;

// Channel-detection ("move the stick you want") state. -1 when nothing is armed;
// otherwise the index into rc_stick_bindings() waiting for a movement.
static int        s_detect_target   = -1;
static RcChannels s_detect_baseline;

// Binding values the operator has typed or demonstrated but not yet written.
// Keyed by parameter name, so ArduPilot and PX4 runs cannot collide.
static std::unordered_map<std::string, float> s_bind_edits;

// Aux-function channel the numeric editor is pointed at, 1-based.
static int s_aux_channel = 5;

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
    if (s_use_param_ext) sender->set_param_ext(tsys, tcomp, id, value, type);
    else                 sender->set_param    (tsys, tcomp, id, value, type);
}

// The value a parameter row should show: the operator's staged edit if there is
// one, otherwise what the vehicle reported, otherwise the given default.
static float staged_value(const std::unordered_map<std::string, ParamEntry>* params,
                          const char* id, float fallback)
{
    auto ed = s_bind_edits.find(id);
    if (ed != s_bind_edits.end()) return ed->second;
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
static void draw_channel_row(int ch_1based, uint16_t pwm,
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

    char lbl[8];
    snprintf(lbl, sizeof(lbl), "%d", ch_1based);
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
        ImGui::SetTooltip("Channel %d \xe2\x80\x94 %s", ch_1based, bind_label);
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
    const float list_h = active_n * (ROW_H + 2.0f) + 8.0f;
    if (ImGui::BeginChild("##rc_channels", { -1.0f, list_h }, true)) {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 2.0f, 2.0f });
        for (int i = 0; i < RcChannels::MAX_CHANNELS; ++i) {
            if (!channel_seen(i)) continue;
            const RcChannelCal& cal = s_cal.channel(i);
            // The channel keeps its own number, not its position in the list:
            // a gap where a dead channel would be is the point, since RCn_*
            // parameters and every binding are named by that number.
            draw_channel_row(i + 1, rc.chan[i],
                             s_cal.phase() == RcCalibration::Phase::Idle
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

    const bool can_calibrate = connected && rc.valid && rc.usable_count() > 0;

    switch (s_cal.phase()) {

    case RcCalibration::Phase::Idle: {
        ImGui::BeginDisabled(!can_calibrate);
        if (ui_grid_button("START CALIBRATION", { -1.0f, 26.0f }) && can_calibrate) {
            // Every channel the protocol carries, not the count the vehicle
            // reports. Which of them are really there is then decided by what
            // arrives, so an under-reported chancount cannot quietly drop a
            // channel out of the calibration.
            s_cal.begin(RcChannels::MAX_CHANNELS);

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
                    s_cal.set_reversed(i, rev);
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
                s_cal.set_throttle_channel((int)ch);
            }

            gcs_log("rc calibration started");
        }
        ImGui::EndDisabled();

        // The missing-parameters case is covered by the banner at the top of
        // the panel; repeating it here would say the same thing twice.
        if (!connected)
            ImGui::TextDisabled("Connect to a vehicle first.");
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
            if (s_cal.capture_center(rc)) {
                gcs_log("rc calibration: centre captured on %d channels \xe2\x80\x94 "
                        "now move every stick and switch to both stops",
                        s_cal.active_count());
            } else {
                // capture_center() refuses a frame with nothing plausible in it.
                gcs_log("rc calibration: no usable RC input to capture");
                gcs_tone(GcsTone::Failure);
            }
        }
        ImGui::EndDisabled();

        ImGui::SameLine(0, 4);
        if (ui_grid_button("CANCEL##cal", { -1.0f, 26.0f })) {
            s_cal.cancel();
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
        const int swept = s_cal.usable_count();
        const int total = s_cal.active_count();
        char prog[48];
        snprintf(prog, sizeof(prog), "%d / %d active channel%s",
                 swept, total, total == 1 ? "" : "s");
        ui_progress_bar(prog, total > 0 ? (float)swept / total : 0.0f, 12.0f);

        ImGui::Spacing();
        const float half = (ImGui::GetContentRegionAvail().x - 4.0f) * 0.5f;

        ImGui::BeginDisabled(swept == 0);
        if (ui_solid_button("FINISH", { half, 26.0f },
                            btn_write_base(), btn_write_hov())) {
            s_cal.finish();
            gcs_log("rc calibration: %d of %d active channels swept", swept, total);
        }
        ImGui::EndDisabled();
        if (swept == 0 && ImGui::IsItemHovered())
            ImGui::SetTooltip("Nothing has moved far enough to record yet");

        ImGui::SameLine(0, 4);
        if (ui_grid_button("CANCEL##cal", { -1.0f, 26.0f })) {
            s_cal.cancel();
            gcs_log("rc calibration cancelled");
        }
        break;
    }

    case RcCalibration::Phase::Review: {
        const std::vector<RcParamWrite> writes = s_cal.params(dialect);

        ImGui::TextColored(col_warning(), "3 / 3  REVIEW");
        ImGui::TextWrapped("Check each channel's travel, then commit. Nothing has "
                           "been written to the vehicle yet.");
        ImGui::Spacing();

        // Per-channel table: measured endpoints, trim, and the reversal flag —
        // the one value a sweep cannot determine and the operator must set.
        ImGui::PushStyleColor(ImGuiCol_ChildBg, bg_param_list());
        ImGui::PushStyleColor(ImGuiCol_Border,  col_separator());
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, FRAME_BORDER_NORMAL);
        if (ImGui::BeginChild("##cal_review", { -1.0f, 150.0f }, true)) {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 4.0f, 3.0f });

            const float rev_x = ImGui::GetContentRegionAvail().x - 46.0f;

            for (int i = 0; i < s_cal.channel_count(); ++i) {
                // Channels that never carried a pulse are not a finding — they
                // are the empty half of an eighteen-slot protocol. Only a
                // channel that is live but was not moved is worth a line.
                if (!s_cal.channel_active(i)) continue;

                const RcChannelCal& c = s_cal.channel(i);
                ImGui::PushID(i);

                if (!s_cal.channel_usable(i)) {
                    ImGui::AlignTextToFramePadding();
                    ImGui::TextDisabled("%2d  not moved", i + 1);
                    ImGui::PopID();
                    continue;
                }

                const uint16_t trim = (i + 1 == s_cal.throttle_channel())
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
                    s_cal.set_reversed(i, rev);

                ImGui::PopID();
            }
            ImGui::PopStyleVar();
        }
        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);

        ImGui::Spacing();

        // Protocol switch — see the note on s_use_param_ext.
        ImGui::Checkbox("Commit over PARAM_EXT_SET", &s_use_param_ext);
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
            s_cal.cancel();
            gcs_log("rc calibration discarded \xe2\x80\x94 vehicle untouched");
        }

        {
            char q[128];
            snprintf(q, sizeof(q), "WRITE %d PARAMETERS FOR %d CHANNELS?",
                     (int)writes.size(), s_cal.usable_count());
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
                        s_use_param_ext ? "PARAM_EXT_SET" : "PARAM_SET");
                gcs_tone(GcsTone::Success);
                s_cal.cancel();
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
        s_bind_edits.erase(id);
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
                s_bind_edits[id] = (float)ch;
                val = (float)ch;
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", id);

            // Arm detection: take a baseline now, then watch for the first
            // channel to move by more than the noise threshold.
            ImGui::SameLine(0, 4);
            const bool armed = (s_detect_target == (int)i);
            if (ui_grid_button(armed ? "MOVE" : "DETECT", { det_w, 0.0f }, armed)) {
                if (armed) {
                    s_detect_target = -1;
                } else if (rc.valid) {
                    s_detect_baseline = rc;
                    s_detect_target   = (int)i;
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
                        s_bind_edits[slot.param] = (float)choices[c].mode;
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
                s_bind_edits[slot.param] = (float)mode_num;
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
            snprintf(preview, sizeof(preview), "CH %d", s_aux_channel);

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

                    if (ImGui::Selectable(item, c == s_aux_channel))
                        s_aux_channel = c;
                    if (c == s_aux_channel) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
        }

        // Built after the picker, so the row below always names the channel the
        // list is currently on.
        char aux_id[24];
        if (!rc_aux_param(s_aux_channel, dialect, aux_id, sizeof(aux_id))) {
            ImGui::TextDisabled("Channel %d has no option parameter.", s_aux_channel);
            ImGui::PopID();
            return;
        }

        float val = staged_value(params, aux_id, 0.0f);

        ImGui::SameLine(0, 6);
        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("%s", aux_id);
        if (rc.at(s_aux_channel) > 0) {
            ImGui::SameLine(0, 6);
            ImGui::TextColored(col_data(), "%u", (unsigned)rc.at(s_aux_channel));
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
                    s_bind_edits[aux_id] = (float)o.value;
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
                s_bind_edits[aux_id] = (float)opt;
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

// ─────────────────────────────────────────────────────────────────────────────

void rc_tab_pump(const VehicleState* vs)
{
    if (!vs || !vs->has_heartbeat || !vs->rc.valid) {
        s_seen_mask = 0;
        return;
    }

    for (int i = 0; i < RcChannels::MAX_CHANNELS; ++i)
        if (vs->rc.chan[i] > 0)
            s_seen_mask |= (1u << i);

    s_cal.update(vs->rc);
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
    ImGui::TextColored(accent_col(), "RADIO CONTROL");
    if (!connected) {
        ImGui::SameLine(0, 6);
        ImGui::TextColored(col_no_link_muted(), "(no link)");
    }
    themed_sep();
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
    if (s_detect_target >= 0) {
        if (!rc.valid) {
            s_detect_target = -1;
        } else {
            const int moved = rc_detect_moved_channel(s_detect_baseline, rc);
            if (moved > 0) {
                const auto& bindings = rc_stick_bindings();
                if (s_detect_target < (int)bindings.size()) {
                    const char* id = rc_stick_param(bindings[s_detect_target],
                                                    dialect);
                    if (id) {
                        s_bind_edits[id] = (float)moved;
                        gcs_log("rc: %s detected on channel %d \xe2\x80\x94 press WRITE to bind",
                                bindings[s_detect_target].label, moved);
                        gcs_tone(GcsTone::Success);
                    }
                }
                s_detect_target = -1;
            }
        }
    }

    // Eighteen channel bars, a three-step calibration and six mode slots do not
    // fit a sidebar at any sensible font size, and the sidebar window itself is
    // created with NoScrollbar — so the panel brings its own scroll region
    // rather than quietly clipping everything past the fold.
    if (ImGui::BeginChild("##rc_scroll", { -1.0f, -1.0f }, false)) {
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
