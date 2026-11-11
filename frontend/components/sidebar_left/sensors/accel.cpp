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

// frontend/components/sidebar_left/sensors/accel.cpp
//
// The accelerometer calibration: six positions, each confirmed by the
// vehicle, ticked off as they go.

#include "frontend/components/sidebar_left/sensors/sensors_internal.hpp"
#include "frontend/source/app_log.hpp"
#include "frontend/source/audio.hpp"
#include <cstdio>

namespace sensors_tab {

// ── The six positions, ticked off as they are confirmed ──────────────────────

static void draw_position_checklist()
{
    const AccelCalPos now  = st().accel.requested();
    const int         done = st().accel.confirmed_count();

    ImGui::PushStyleColor(ImGuiCol_ChildBg, bg_param_list());
    ImGui::PushStyleColor(ImGuiCol_Border,  col_separator());
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, FRAME_BORDER_NORMAL);

    const float h = accel_cal_pos_count() * ImGui::GetTextLineHeightWithSpacing() + 10.0f;
    if (ImGui::BeginChild("##accel_steps", { -1.0f, h }, true, kNoScroll)) {
        for (int i = 0; i < accel_cal_pos_count(); ++i) {
            const AccelCalPos p    = accel_cal_pos_at(i);
            const bool        is_now = (p == now);
            // Positions are counted, not matched by name: the vehicle asks in
            // its own order and is free to repeat one, so "everything before
            // the count" is what has actually been answered.
            const bool        past = (i < done);

            if (is_now)     ImGui::TextColored(accent_col(), "\xe2\x96\xb6 %s", accel_cal_pos_label(p));
            else if (past)  ImGui::TextColored(col_ok(),     "\xe2\x9c\x93 %s", accel_cal_pos_label(p));
            else            ImGui::TextDisabled("  %s", accel_cal_pos_label(p));
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);
}

// `armed` is passed in rather than read here because it blocks both
// calibrations for the same reason: motors spinning under an airframe the
// operator has been told to pick up, or to leave perfectly still.
void draw_accel_section(MavlinkSender* sender, bool connected, bool armed,
                               uint8_t tsys, uint8_t tcomp, const ParamMap* params)
{
    ImGui::TextColored(accent_col(), "ACCELEROMETER");
    ImGui::Spacing();

    // Only while idle. Mid-calibration the operator is being asked to do
    // something, and an inventory below the instruction is one more thing in
    // the way of reading it.
    if (st().accel.phase() == AccelCalibration::Phase::Idle) {
        draw_sensor_devices(params, SensorKind::Accel, "##accel_devs",
                            "accelerometers");
        ImGui::Spacing();
    }

    switch (st().accel.phase()) {

    case AccelCalibration::Phase::Idle: {
        ImGui::TextWrapped("Six-position calibration. The vehicle asks for each "
                           "orientation in turn \xe2\x80\x94 hold it still in that "
                           "position, then confirm.");
        ImGui::Spacing();

        ImGui::BeginDisabled(!connected || armed);
        if (ui_solid_button("CALIBRATE ACCELEROMETER", { -1.0f, 30.0f },
                            btn_write_base(), btn_write_hov())) {
            sender->calibrate_accelerometer(tsys, tcomp);
            st().accel.begin();
            gcs_log("accel cal: requested six-position calibration");
        }
        ImGui::EndDisabled();

        if (!connected)     ImGui::TextDisabled("Connect to a vehicle first.");
        else if (armed)     ImGui::TextColored(col_error(), "Disarm before calibrating.");
        break;
    }

    case AccelCalibration::Phase::Starting:
    case AccelCalibration::Phase::Waiting:
    case AccelCalibration::Phase::Confirming: {
        const int done  = st().accel.confirmed_count();
        const int total = accel_cal_pos_count();

        char prog[32];
        snprintf(prog, sizeof(prog), "%d / %d positions", done, total);
        ui_progress_bar(prog, total > 0 ? (float)done / total : 0.0f, 12.0f);

        ImGui::Spacing();
        draw_vehicle_message("##accel_msg", st().accel.message(), accent_col());
        ImGui::Spacing();

        const AccelCalPos pos   = st().accel.requested();
        const char*       label = accel_cal_pos_label(pos);
        const bool        ready = (st().accel.phase() == AccelCalibration::Phase::Waiting)
                                  && label != nullptr;

        // The button names the position it is confirming. A bare "OK" next to a
        // wrapped instruction is exactly the control an operator presses
        // without rereading which way round the airframe was supposed to be.
        char confirm_lbl[64];
        if (ready) snprintf(confirm_lbl, sizeof(confirm_lbl), "VEHICLE IS %s", label);
        else       snprintf(confirm_lbl, sizeof(confirm_lbl), "WAITING\xe2\x80\xa6");

        ImGui::BeginDisabled(!connected || !ready);
        if (ready
            ? ui_solid_button(confirm_lbl, { -1.0f, 32.0f },
                              btn_write_base(), btn_write_hov())
            : ui_grid_button(confirm_lbl, { -1.0f, 32.0f }))
        {
            const AccelCalPos sent = st().accel.confirm();
            if (sent != AccelCalPos::None) {
                sender->send_accelcal_vehicle_pos(tsys, tcomp, (uint32_t)sent);
                gcs_log("accel cal: reported %s", accel_cal_pos_label(sent));
            }
        }
        ImGui::EndDisabled();

        ImGui::Spacing();
        if (ui_solid_button("CANCEL##accel", { -1.0f, 26.0f },
                            btn_disconnect_base(), btn_disconnect_hov())) {
            // Told to the vehicle as well as forgotten here: it is sitting in a
            // calibration waiting to be answered, and dropping the panel alone
            // would leave it there.
            if (connected) sender->cancel_calibration(tsys, tcomp);
            st().accel.cancel();
            gcs_log("accel cal: cancelled");
        }

        ImGui::Spacing();
        draw_position_checklist();
        break;
    }

    case AccelCalibration::Phase::Succeeded: {
        draw_vehicle_message("##accel_msg", st().accel.message(), col_ok());
        ImGui::Spacing();
        ImGui::TextColored(col_ok(), "CALIBRATION SUCCESSFUL");
        // Not advice, a requirement: ArduPilot writes the new offsets to
        // storage but goes on flying with the ones it booted with, so until it
        // restarts the calibration has changed nothing about how it flies.
        ImGui::TextWrapped("The new offsets take effect after a reboot. Until "
                           "then the vehicle is still flying on the old ones.");
        ImGui::Spacing();

        const float half = (ImGui::GetContentRegionAvail().x - 4.0f) * 0.5f;

        ImGui::BeginDisabled(!connected || armed);
        if (ui_solid_button("REBOOT VEHICLE", { half, 28.0f },
                            btn_write_base(), btn_write_hov()))
            ImGui::OpenPopup("##confirm_reboot");
        ImGui::EndDisabled();

        ImGui::SameLine(0, 4);
        if (ui_grid_button("DONE##accel", { -1.0f, 28.0f }))
            st().accel.cancel();

        // Confirmed, and never automatic. Rebooting is the GCS reaching out and
        // stopping the thing it is talking to: the link goes with it, and over
        // USB the serial device disappears entirely. That is the operator's
        // call to make, not a step to slip into the end of a wizard.
        if (ui_confirm_popup("##confirm_reboot", "REBOOT VEHICLE",
                             "RESTART THE AUTOPILOT NOW?",
                             "REBOOT", btn_write_base()) == UiConfirm::Confirmed) {
            sender->reboot_autopilot(tsys, tcomp);
            st().accel.cancel();
            gcs_log("reboot requested \xe2\x80\x94 the link will drop and must be "
                    "reconnected");
        }
        break;
    }

    case AccelCalibration::Phase::Failed: {
        draw_vehicle_message("##accel_msg", st().accel.message(), col_error());
        ImGui::Spacing();
        ImGui::TextColored(col_error(), "CALIBRATION FAILED");
        ImGui::TextWrapped("Nothing was changed. Check the airframe was still "
                           "and square in each position, then try again.");
        ImGui::Spacing();

        const float half = (ImGui::GetContentRegionAvail().x - 4.0f) * 0.5f;
        ImGui::BeginDisabled(!connected || armed);
        if (ui_solid_button("RETRY##accel", { half, 28.0f },
                            btn_write_base(), btn_write_hov())) {
            sender->calibrate_accelerometer(tsys, tcomp);
            st().accel.begin();
            gcs_log("accel cal: retrying");
        }
        ImGui::EndDisabled();
        ImGui::SameLine(0, 4);
        if (ui_grid_button("CLOSE##accel", { -1.0f, 28.0f }))
            st().accel.cancel();
        break;
    }
    }
}

} // namespace sensors_tab
