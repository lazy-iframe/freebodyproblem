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

// frontend/components/sidebar_left/tab_sensors.cpp
//
// The SENSORS tab: the pump that keeps the calibrations fed while the tab is
// not on screen, and the tab itself, laying out the sections in sensors/.

#include "frontend/components/sidebar_left/sensors/sensors_internal.hpp"
#include "frontend/source/app_log.hpp"
#include "frontend/source/audio.hpp"
#include <cstdio>
#include <vector>

namespace sensors_tab {

static VehicleUiState<SensorTabState> s_state;

SensorTabState& st() { return *s_state; }

} // namespace sensors_tab

using namespace sensors_tab;

// ─────────────────────────────────────────────────────────────────────────────

void sensors_tab_pump(const VehicleState* vs, const std::vector<StatusText>& texts,
                      double now)
{
    if (!vs || !vs->has_heartbeat) {
        // The exchange cannot continue without the vehicle on the other end of
        // it, and a half-finished calibration left on screen would invite the
        // operator to keep turning the airframe for nobody.
        if (st().accel.running()) {
            st().accel.cancel();
            gcs_log("accel cal: abandoned \xe2\x80\x94 link lost");
        }
        if (st().gyro.running()) {
            st().gyro.cancel();
            gcs_log("gyro cal: abandoned \xe2\x80\x94 link lost");
        }
        if (st().mag.running()) {
            st().mag.cancel();
            gcs_log("compass cal: abandoned \xe2\x80\x94 link lost");
        }
        st().seen_accelcal_seq    = 0;
        st().seen_calib_ack_seq   = 0;
        st().seen_magcal_ack_seq  = 0;
        st().consumed_statustexts = 0;
        st().traced_magcal_seq    = 0;
        st().traced_mag_pct       = -1;
        return;
    }

    // Noted before anything is fed in: a run ends on one of several inputs
    // below, and this is the one place that sees all of them.
    const bool gyro_was_running = st().gyro.running();
    const bool mag_was_running  = st().mag.running();

    // New STATUSTEXTs first, so the vehicle's wording is in hand before the
    // command below settles which position it actually meant.
    //
    // Counted against the running total rather than indexed into the window:
    // the window drops its oldest at 200, so an index into it would quietly
    // start pointing at a different message.
    const uint32_t total = vs->statustext_total;
    if (total < st().consumed_statustexts) {
        st().consumed_statustexts = total;      // link restarted, counter reset
    } else if (total > st().consumed_statustexts) {
        const uint32_t fresh = total - st().consumed_statustexts;
        const size_t   have  = texts.size();
        // More new than the window holds means some were dropped before we
        // looked; replay what survived rather than nothing.
        const size_t   start = (fresh >= have) ? 0 : have - (size_t)fresh;
        for (size_t i = start; i < have; ++i) {
            st().accel.on_statustext(texts[i]);
            st().mag.on_statustext(texts[i], now);
        }
        st().consumed_statustexts = total;
    }

    if (vs->accelcal_seq != st().seen_accelcal_seq) {
        st().seen_accelcal_seq = vs->accelcal_seq;
        st().accel.on_vehicle_pos(vs->accelcal_pos);
    }

    // The ACK for PREFLIGHT_CALIBRATION — the gyro's verdict, and on PX4 the
    // compass's too, so it goes to both and whichever is not running ignores
    // it. Watched by sequence rather than by value: two runs can end the same
    // way, and the second would otherwise look like silence.
    if (vs->calib_ack_seq != st().seen_calib_ack_seq) {
        st().seen_calib_ack_seq = vs->calib_ack_seq;
        st().gyro.on_command_ack(vs->calib_ack_result, now);
        st().mag.on_command_ack(vs->calib_ack_result, now);
    }

    // The compass calibration's own commands. Only the start is the machine's
    // business: an accept or a cancel is answered too, and neither says
    // anything about a run that is still going.
    if (vs->magcal_ack_seq != st().seen_magcal_ack_seq) {
        st().seen_magcal_ack_seq = vs->magcal_ack_seq;
        if (vs->magcal_ack_cmd == 42424)
            st().mag.on_start_ack(vs->magcal_ack_result, now);
        else if (vs->magcal_ack_cmd == 42425)
            gcs_log("compass cal: accept %s",
                    vs->magcal_ack_result == 0 ? "accepted by the vehicle"
                                               : "rejected by the vehicle");
    }

    // Per-compass slots, read whole every frame rather than watched for
    // changes: they are a latch, and what matters is what they all say now.
    st().mag.on_mag_cal(vs->magcal, VehicleState::MAX_COMPASSES, now);

    if (trace_mag_cal && st().mag.running()) {
        // The slots, whenever the vehicle has written one. Printed as the
        // machine reads them, so a run that ignores a compass because its slot
        // predates the run shows up as a sequence number rather than as
        // silence.
        if (vs->magcal_seq != st().traced_magcal_seq) {
            st().traced_magcal_seq = vs->magcal_seq;
            for (int i = 0; i < VehicleState::MAX_COMPASSES; ++i) {
                const auto& c = vs->magcal[i];
                if (!c.seen) continue;
                gcs_log("magcal slot %d: status=%u pct=%u reported=%d autosaved=%d "
                        "fitness=%.1f seq=%u",
                        i, (unsigned)c.status, (unsigned)c.completion_pct,
                        (int)c.reported, (int)c.autosaved, (double)c.fitness,
                        (unsigned)c.seq);
            }
        }
        if (st().mag.percent() != st().traced_mag_pct) {
            st().traced_mag_pct = st().mag.percent();
            gcs_log("magcal state: %d %%%s%s", st().traced_mag_pct,
                    st().mag.message().empty() ? "" : " \xe2\x80\x94 ",
                    st().mag.message().c_str());
        }
    }

    st().gyro.tick(now);
    if (gyro_was_running && !st().gyro.running())
        gcs_log("gyro cal: %s", st().gyro.result_text());

    st().mag.tick(now);
    if (mag_was_running && !st().mag.running()) {
        if (st().mag.phase() == MagCalibration::Phase::Succeeded)
            gcs_log("compass cal: complete%s",
                    st().mag.needs_save() ? " \xe2\x80\x94 not saved by the vehicle" : "");
        else
            gcs_log("compass cal: failed \xe2\x80\x94 %s", st().mag.failure_text());
    }
}

// ─────────────────────────────────────────────────────────────────────────────

void draw_tab_sensors(MavlinkSender* sender, const VehicleState* vs,
                      const ParamMap* params)
{
    const bool    connected = (vs && vs->has_heartbeat);
    const uint8_t tsys      = connected ? vs->sysid  : 1;
    const uint8_t tcomp     = connected ? vs->compid : 1;

    ImGui::Spacing();
    ImGui::TextColored(accent_col(), "SENSORS");
    if (!connected) {
        ImGui::SameLine(0, 6);
        ImGui::TextColored(col_no_link_muted(), "(no link)");
    }
    themed_sep();
    ImGui::Spacing();

    // An inventory, three calibration panels and a six-step checklist do not
    // fit a sidebar at any sensible font size, and the sidebar window itself is
    // created with NoScrollbar — so the panel brings its own scroll region
    // rather than quietly clipping everything past the fold. The heading above
    // stays outside it and holds still while the rest scrolls.
    if (!ImGui::BeginChild("##sensors_scroll", { -1.0f, -1.0f }, false)) {
        ImGui::EndChild();
        return;
    }

    const bool armed = connected && vs->armed;

    // One at a time. Both are PREFLIGHT_CALIBRATION on the same vehicle and the
    // command carries one sensor per message, so starting the second would be
    // the GCS withdrawing the first without saying so — and the operator would
    // be reading instructions for a calibration no longer running.
    const bool accel_busy = st().accel.running();
    const bool gyro_busy  = st().gyro.running();
    const bool mag_busy   = st().mag.running();

    ImGui::BeginDisabled(gyro_busy || mag_busy);
    draw_accel_section(sender, connected, armed, tsys, tcomp, params);
    ImGui::EndDisabled();

    ImGui::Spacing();
    themed_sep();
    ImGui::Spacing();

    ImGui::BeginDisabled(accel_busy || mag_busy);
    draw_gyro_section(sender, connected, armed, tsys, tcomp, params);
    ImGui::EndDisabled();

    ImGui::Spacing();
    themed_sep();
    ImGui::Spacing();

    ImGui::BeginDisabled(accel_busy || gyro_busy);
    draw_mag_section(sender, connected, armed, tsys, tcomp, vs, params);
    ImGui::EndDisabled();

    ImGui::EndChild();
}
