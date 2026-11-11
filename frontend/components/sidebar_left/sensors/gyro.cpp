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

// frontend/components/sidebar_left/sensors/gyro.cpp
//
// The gyroscope calibration: one command, the airframe held still.

#include "frontend/components/sidebar_left/sensors/sensors_internal.hpp"
#include "frontend/source/app_log.hpp"
#include "frontend/source/audio.hpp"
#include <cstdio>

namespace sensors_tab {

// ── Gyroscope ────────────────────────────────────────────────────────────────

void draw_gyro_section(MavlinkSender* sender, bool connected, bool armed,
                              uint8_t tsys, uint8_t tcomp, const ParamMap* params)
{
    ImGui::TextColored(accent_col(), "GYROSCOPE");
    ImGui::Spacing();

    if (st().gyro.phase() == GyroCalibration::Phase::Idle) {
        draw_sensor_devices(params, SensorKind::Gyro, "##gyro_devs", "gyroscopes");
        ImGui::Spacing();
    }

    switch (st().gyro.phase()) {

    case GyroCalibration::Phase::Idle: {
        ImGui::TextWrapped("The vehicle samples its own gyros and needs a few "
                           "seconds of stillness \xe2\x80\x94 set it down level, "
                           "hands off, and do not move it.");
        ImGui::Spacing();

        ImGui::BeginDisabled(!connected || armed);
        if (ui_solid_button("CALIBRATE GYROSCOPE", { -1.0f, 30.0f },
                            btn_write_base(), btn_write_hov())) {
            sender->calibrate_gyroscope(tsys, tcomp);
            st().gyro.begin(ImGui::GetTime());
            gcs_log("gyro cal: requested");
        }
        ImGui::EndDisabled();

        if (!connected)  ImGui::TextDisabled("Connect to a vehicle first.");
        else if (armed)  ImGui::TextColored(col_error(), "Disarm before calibrating.");
        break;
    }

    case GyroCalibration::Phase::Running: {
        // Against the timeout rather than against the calibration's own
        // progress, which the vehicle does not report. It reads as "how long
        // before this gives up", which is the only thing actually known.
        const double elapsed = st().gyro.elapsed(ImGui::GetTime());
        const float  frac    = (float)(elapsed / GyroCalibration::kTimeoutSeconds);

        char prog[32];
        snprintf(prog, sizeof(prog), "%.0f s", elapsed);
        ui_progress_bar(prog, frac < 1.0f ? frac : 1.0f, 12.0f);

        ImGui::Spacing();
        ImGui::TextColored(col_warning(), "HOLD STILL");
        ImGui::TextWrapped("Waiting for the vehicle to report back.");

        ImGui::Spacing();
        if (ui_solid_button("CANCEL##gyro", { -1.0f, 26.0f },
                            btn_disconnect_base(), btn_disconnect_hov())) {
            // Told to the vehicle as well as forgotten here, the same as the
            // accelerometer: it is in a calibration it was asked for, and
            // dropping the panel alone would leave it there.
            if (connected) sender->cancel_calibration(tsys, tcomp);
            st().gyro.cancel();
            gcs_log("gyro cal: cancelled");
        }
        break;
    }

    case GyroCalibration::Phase::Succeeded: {
        ImGui::TextColored(col_ok(), "CALIBRATION SUCCESSFUL");
        // No reboot notice here: unlike the accelerometer offsets, the new gyro
        // offsets are the ones the vehicle goes on flying with.
        ImGui::Spacing();
        if (ui_grid_button("DONE##gyro", { -1.0f, 28.0f }))
            st().gyro.cancel();
        break;
    }

    case GyroCalibration::Phase::Failed: {
        ImGui::TextColored(col_error(), "CALIBRATION FAILED");
        // The vehicle's own verdict, named. "denied" and "no reply" are
        // different problems from a wobbly table, and the advice below is only
        // right for one of them.
        ImGui::TextColored(col_error(), "Vehicle said: %s", st().gyro.result_text());
        ImGui::Spacing();
        ImGui::TextWrapped("The vehicle has to be completely still. Set it on "
                           "something solid, wait for it to settle, then try "
                           "again.");
        ImGui::Spacing();

        const float half = (ImGui::GetContentRegionAvail().x - 4.0f) * 0.5f;
        ImGui::BeginDisabled(!connected || armed);
        if (ui_solid_button("RETRY##gyro", { half, 28.0f },
                            btn_write_base(), btn_write_hov())) {
            sender->calibrate_gyroscope(tsys, tcomp);
            st().gyro.begin(ImGui::GetTime());
            gcs_log("gyro cal: retrying");
        }
        ImGui::EndDisabled();
        ImGui::SameLine(0, 4);
        if (ui_grid_button("CLOSE##gyro", { -1.0f, 28.0f }))
            st().gyro.cancel();
        break;
    }
    }
}

} // namespace sensors_tab
