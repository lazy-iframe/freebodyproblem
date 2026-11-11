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

// frontend/components/sidebar_left/sensors/mag.cpp
//
// The compass calibration: started the way the vehicle's stack expects, then
// followed by its progress and the coverage sphere until it reports.

#include "frontend/components/sidebar_left/sensors/sensors_internal.hpp"
#include "frontend/source/app_log.hpp"
#include "frontend/source/audio.hpp"
#include <cstdio>
#include <cstring>

namespace sensors_tab {

// ── Magnetometer ─────────────────────────────────────────────────────────────

// Start a compass calibration the way this vehicle expects one.
//
// ArduPilot has a command written for the job — DO_START_MAG_CAL, with a
// bitmask, a retry flag and an autosave flag — and takes PREFLIGHT_CALIBRATION
// only as a blunter synonym for it. PX4 has no such command and knows only the
// PREFLIGHT_CALIBRATION form. So the stack decides, and it is the heartbeat that
// says which stack it is.
//
// Anything that is neither gets the portable form: it is the one every stack
// implementing MAVLink calibration at all implements.
static void start_mag_cal_for(MavlinkSender* sender, const VehicleState* vs,
                              uint8_t tsys, uint8_t tcomp)
{
    const bool ardupilot = vs && vs->autopilot == MAV_AUTOPILOT_ARDUPILOTMEGA;

    if (trace_mag_cal) {
        st().traced_magcal_seq = vs ? vs->magcal_seq : 0;
        st().traced_mag_pct    = -1;
        gcs_log("magcal: run begun \xe2\x80\x94 target %u/%u, autopilot=%u, "
                "baseline seq=%u",
                (unsigned)tsys, (unsigned)tcomp,
                (unsigned)(vs ? vs->autopilot : 255),
                (unsigned)(vs ? vs->magcal_seq : 0));
    }

    if (ardupilot) {
        // MAG_CAL_PROGRESS and MAG_CAL_REPORT are not sent unasked. ArduPilot
        // carries both in the EXTRA3 stream, which a GCS that has not asked for
        // it does not get — so a calibration runs to completion, the vehicle
        // says so in STATUSTEXT, and the two messages that actually carry the
        // percentage and the verdict never leave the autopilot. Asked for here
        // rather than at connect: they are of no use at any other time, and
        // ArduPilot sends nothing on either while no calibration is running.
        sender->request_message_interval(tsys, tcomp, 191,  200000);  // 5 Hz
        sender->request_message_interval(tsys, tcomp, 192, 1000000);  // 1 Hz
        st().magcal_stream_on = true;
        if (trace_mag_cal)
            gcs_log("magcal tx: SET_MESSAGE_INTERVAL 191 @ 5 Hz, 192 @ 1 Hz");

        sender->start_mag_cal(tsys, tcomp);
        gcs_log("compass cal: requested (DO_START_MAG_CAL, autosave off)");
    } else {
        sender->calibrate_magnetometer(tsys, tcomp);
        gcs_log("compass cal: requested (PREFLIGHT_CALIBRATION)");
    }
}


void draw_mag_section(MavlinkSender* sender, bool connected, bool armed,
                             uint8_t tsys, uint8_t tcomp, const VehicleState* vs,
                             const ParamMap* params)
{
    ImGui::TextColored(accent_col(), "MAGNETOMETER");
    ImGui::Spacing();

    // The run is over, however it ended: let the two streams go again. Both are
    // silent outside a calibration, so this is tidiness rather than bandwidth —
    // and it means the next run asks for them afresh on a vehicle that has
    // rebooted in between and forgotten.
    if (st().magcal_stream_on && !st().mag.running()) {
        st().magcal_stream_on = false;
        if (connected) {
            sender->request_message_interval(tsys, tcomp, 191, -1);
            sender->request_message_interval(tsys, tcomp, 192, -1);
            if (trace_mag_cal)
                gcs_log("magcal tx: SET_MESSAGE_INTERVAL 191, 192 disabled");
        }
    }

    if (st().mag.phase() == MagCalibration::Phase::Idle) {
        draw_sensor_devices(params, SensorKind::Compass, "##mag_devs", "compasses");
        ImGui::Spacing();
    }

    switch (st().mag.phase()) {

    case MagCalibration::Phase::Idle: {
        ImGui::TextWrapped("Rotate the vehicle slowly through every "
                           "orientation \xe2\x80\x94 nose down and up, both "
                           "sides, level and inverted \xe2\x80\x94 turning it "
                           "about each axis until the vehicle says it has "
                           "enough. Keep clear of metal, magnets and wiring.");
        ImGui::Spacing();

        ImGui::BeginDisabled(!connected || armed);
        if (ui_solid_button("CALIBRATE MAGNETOMETER", { -1.0f, 30.0f },
                            btn_write_base(), btn_write_hov())) {
            start_mag_cal_for(sender, vs, tsys, tcomp);
            st().mag.begin(vs ? vs->magcal_seq : 0, ImGui::GetTime());
        }
        ImGui::EndDisabled();

        if (!connected)  ImGui::TextDisabled("Connect to a vehicle first.");
        else if (armed)  ImGui::TextColored(col_error(), "Disarm before calibrating.");
        break;
    }

    case MagCalibration::Phase::Running: {
        const int pct = st().mag.percent();

        char prog[32];
        snprintf(prog, sizeof(prog), "%d %%", pct);
        ui_progress_bar(prog, pct / 100.0f, 12.0f);

        // ArduPilot only. PX4 sends no mask, and there the percentage and the
        // vehicle's own words are the whole of what the panel knows.
        if (const uint8_t* covered = st().mag.completion_mask()) {
            ImGui::Spacing();
            draw_coverage_sphere(covered, 190.0f);
        }

        ImGui::Spacing();
        draw_vehicle_message("##mag_msg", st().mag.message(), accent_col());
        ImGui::Spacing();
        ImGui::TextColored(col_warning(), "KEEP ROTATING");

        ImGui::Spacing();
        if (ui_solid_button("CANCEL##mag", { -1.0f, 26.0f },
                            btn_disconnect_base(), btn_disconnect_hov())) {
            // DO_CANCEL_MAG_CAL, not the all-zero PREFLIGHT_CALIBRATION the
            // other two cancel with: ArduPilot reads that one as cancelling the
            // accelerometer routine and would leave this running. PX4 ignores
            // it, and its calibration ends on its own timeout.
            if (connected) sender->cancel_mag_cal(tsys, tcomp);
            st().mag.cancel();
            gcs_log("compass cal: cancelled");
        }
        break;
    }

    case MagCalibration::Phase::Succeeded: {
        ImGui::TextColored(col_ok(), "CALIBRATION SUCCESSFUL");
        if (st().mag.worst_fitness() >= 0.0f)
            ImGui::TextDisabled("Worst fit: %.1f mGauss", st().mag.worst_fitness());

        if (const uint8_t* covered = st().mag.completion_mask()) {
            ImGui::Spacing();
            draw_coverage_sphere(covered, 190.0f);
            ImGui::Spacing();
        }

        // The offsets are written but the vehicle goes on flying with the ones
        // it booted with, exactly as for the accelerometer.
        ImGui::TextWrapped("The new offsets take effect after a reboot.");
        ImGui::Spacing();

        // Only when a compass finished well and the vehicle did not store it.
        // Offering the force-save the rest of the time would invite pressing it
        // on a calibration already saved, which does nothing, or on none at
        // all, which saves nothing.
        // Not optional on the ArduPilot path: the calibration is started with
        // autosave off, so the offsets exist only in the vehicle's head until
        // this is pressed, and are lost on the next boot if it is not.
        if (st().mag.needs_save()) {
            ImGui::TextColored(col_warning(), "Not stored yet \xe2\x80\x94 accept "
                                              "to keep it.");
            ImGui::BeginDisabled(!connected || armed);
            if (ui_solid_button("ACCEPT CALIBRATION", { -1.0f, 28.0f },
                                btn_write_base(), btn_write_hov())) {
                sender->accept_mag_cal(tsys, tcomp);
                gcs_log("compass cal: accepting offsets");
            }
            ImGui::EndDisabled();
            ImGui::Spacing();
        }

        const float half = (ImGui::GetContentRegionAvail().x - 4.0f) * 0.5f;

        ImGui::BeginDisabled(!connected || armed);
        if (ui_solid_button("REBOOT VEHICLE##mag", { half, 28.0f },
                            btn_write_base(), btn_write_hov()))
            ImGui::OpenPopup("##confirm_reboot_mag");
        ImGui::EndDisabled();

        ImGui::SameLine(0, 4);
        // Named for what it does. With offsets still waiting to be accepted,
        // closing the panel throws them away, and a button saying DONE next to
        // a calibration about to be lost would be the wrong word for it.
        if (ui_grid_button(st().mag.needs_save() ? "DISCARD##mag" : "DONE##mag",
                           { -1.0f, 28.0f })) {
            if (st().mag.needs_save())
                gcs_log("compass cal: discarded \xe2\x80\x94 offsets not accepted");
            st().mag.cancel();
        }

        if (ui_confirm_popup("##confirm_reboot_mag", "REBOOT VEHICLE",
                             "RESTART THE AUTOPILOT NOW?",
                             "REBOOT", btn_write_base()) == UiConfirm::Confirmed) {
            sender->reboot_autopilot(tsys, tcomp);
            st().mag.cancel();
            gcs_log("reboot requested \xe2\x80\x94 the link will drop and must be "
                    "reconnected");
        }
        break;
    }

    case MagCalibration::Phase::Failed: {
        draw_vehicle_message("##mag_msg", st().mag.message(), col_error());
        ImGui::Spacing();
        ImGui::TextColored(col_error(), "CALIBRATION FAILED");
        ImGui::TextColored(col_error(), "Vehicle said: %s", st().mag.failure_text());
        ImGui::Spacing();
        ImGui::TextWrapped("Nothing was changed. Move away from metal and "
                           "wiring, turn the vehicle more slowly, and cover "
                           "every orientation.");
        ImGui::Spacing();

        const float half = (ImGui::GetContentRegionAvail().x - 4.0f) * 0.5f;
        ImGui::BeginDisabled(!connected || armed);
        if (ui_solid_button("RETRY##mag", { half, 28.0f },
                            btn_write_base(), btn_write_hov())) {
            start_mag_cal_for(sender, vs, tsys, tcomp);
            st().mag.begin(vs ? vs->magcal_seq : 0, ImGui::GetTime());
        }
        ImGui::EndDisabled();
        ImGui::SameLine(0, 4);
        if (ui_grid_button("CLOSE##mag", { -1.0f, 28.0f }))
            st().mag.cancel();
        break;
    }
    }
}

} // namespace sensors_tab
