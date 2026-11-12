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

// RC tab, radio: the live channel monitor and the endpoint calibration.

#include "frontend/components/sidebar_left/sidebar_internal.hpp"
#include "frontend/components/others/sidebar_themes.hpp"
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
#include "frontend/components/sidebar_left/rc/rc_internal.hpp"

namespace rc_tab {

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

// ── Sections ─────────────────────────────────────────────────────────────────

void draw_monitor(const RcChannels& rc,
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

void draw_calibration(MavlinkSender* sender, const VehicleState* vs,
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

} // namespace rc_tab
