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
#include "frontend/components/sidebar_left/rc/rc_internal.hpp"

namespace rc_tab {

static VehicleUiState<RcTabState> s_state;

// The vehicle whose state the panel and the pump are working on; the render
// loop binds it before either runs. See vehicle_ui_state.hpp.
RcTabState& st() { return *s_state; }

} // namespace rc_tab

using namespace rc_tab;

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
