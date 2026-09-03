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
#include "../../../backend/accel_calibration.hpp"
#include "../../../backend/gyro_calibration.hpp"
#include "../../../backend/mag_calibration.hpp"
#include "../../../backend/sensor_inventory.hpp"
#include "../../../backend/geodesic_grid.hpp"
#include "imgui.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

// The calibrations outlive any one frame and, via sensors_tab_pump() below, any
// one tab: the vehicle is mid-conversation and will keep asking for positions
// whether or not this panel is the one on screen.
static AccelCalibration s_accel;
static GyroCalibration  s_gyro;
static MagCalibration   s_mag;

// What the pump has already handed to the state machines.
static uint32_t s_seen_accelcal_seq = 0;
static uint32_t s_seen_calib_ack_seq = 0;
static uint32_t s_seen_magcal_ack_seq = 0;
static uint32_t s_consumed_statustexts = 0;

// Compass-calibration tracing, the panel's half of it: what the state machine
// made of what arrived. The wire itself is traced in mavlink_parser.cpp and in
// mavlink_sender.cpp; this is here to catch the case where the messages are
// arriving and the machine is not moving. Set to false to quieten it.
static constexpr bool trace_mag_cal = true;
static uint32_t s_traced_magcal_seq = 0;
static int      s_traced_mag_pct    = -1;

// Whether MAG_CAL_PROGRESS and MAG_CAL_REPORT have been asked for. They are
// requested when a run starts and let go of when it ends — see
// start_mag_cal_for() for why they have to be asked for at all.
static bool s_magcal_stream_on = false;

// ─────────────────────────────────────────────────────────────────────────────

void sensors_tab_pump(const VehicleState* vs, const std::vector<StatusText>& texts,
                      double now)
{
    if (!vs || !vs->has_heartbeat) {
        // The exchange cannot continue without the vehicle on the other end of
        // it, and a half-finished calibration left on screen would invite the
        // operator to keep turning the airframe for nobody.
        if (s_accel.running()) {
            s_accel.cancel();
            gcs_log("accel cal: abandoned \xe2\x80\x94 link lost");
        }
        if (s_gyro.running()) {
            s_gyro.cancel();
            gcs_log("gyro cal: abandoned \xe2\x80\x94 link lost");
        }
        if (s_mag.running()) {
            s_mag.cancel();
            gcs_log("compass cal: abandoned \xe2\x80\x94 link lost");
        }
        s_seen_accelcal_seq    = 0;
        s_seen_calib_ack_seq   = 0;
        s_seen_magcal_ack_seq  = 0;
        s_consumed_statustexts = 0;
        s_traced_magcal_seq    = 0;
        s_traced_mag_pct       = -1;
        return;
    }

    // Noted before anything is fed in: a run ends on one of several inputs
    // below, and this is the one place that sees all of them.
    const bool gyro_was_running = s_gyro.running();
    const bool mag_was_running  = s_mag.running();

    // New STATUSTEXTs first, so the vehicle's wording is in hand before the
    // command below settles which position it actually meant.
    //
    // Counted against the running total rather than indexed into the window:
    // the window drops its oldest at 200, so an index into it would quietly
    // start pointing at a different message.
    const uint32_t total = vs->statustext_total;
    if (total < s_consumed_statustexts) {
        s_consumed_statustexts = total;      // link restarted, counter reset
    } else if (total > s_consumed_statustexts) {
        const uint32_t fresh = total - s_consumed_statustexts;
        const size_t   have  = texts.size();
        // More new than the window holds means some were dropped before we
        // looked; replay what survived rather than nothing.
        const size_t   start = (fresh >= have) ? 0 : have - (size_t)fresh;
        for (size_t i = start; i < have; ++i) {
            s_accel.on_statustext(texts[i]);
            s_mag.on_statustext(texts[i], now);
        }
        s_consumed_statustexts = total;
    }

    if (vs->accelcal_seq != s_seen_accelcal_seq) {
        s_seen_accelcal_seq = vs->accelcal_seq;
        s_accel.on_vehicle_pos(vs->accelcal_pos);
    }

    // The ACK for PREFLIGHT_CALIBRATION — the gyro's verdict, and on PX4 the
    // compass's too, so it goes to both and whichever is not running ignores
    // it. Watched by sequence rather than by value: two runs can end the same
    // way, and the second would otherwise look like silence.
    if (vs->calib_ack_seq != s_seen_calib_ack_seq) {
        s_seen_calib_ack_seq = vs->calib_ack_seq;
        s_gyro.on_command_ack(vs->calib_ack_result, now);
        s_mag.on_command_ack(vs->calib_ack_result, now);
    }

    // The compass calibration's own commands. Only the start is the machine's
    // business: an accept or a cancel is answered too, and neither says
    // anything about a run that is still going.
    if (vs->magcal_ack_seq != s_seen_magcal_ack_seq) {
        s_seen_magcal_ack_seq = vs->magcal_ack_seq;
        if (vs->magcal_ack_cmd == 42424)
            s_mag.on_start_ack(vs->magcal_ack_result, now);
        else if (vs->magcal_ack_cmd == 42425)
            gcs_log("compass cal: accept %s",
                    vs->magcal_ack_result == 0 ? "accepted by the vehicle"
                                               : "rejected by the vehicle");
    }

    // Per-compass slots, read whole every frame rather than watched for
    // changes: they are a latch, and what matters is what they all say now.
    s_mag.on_mag_cal(vs->magcal, VehicleState::MAX_COMPASSES, now);

    if (trace_mag_cal && s_mag.running()) {
        // The slots, whenever the vehicle has written one. Printed as the
        // machine reads them, so a run that ignores a compass because its slot
        // predates the run shows up as a sequence number rather than as
        // silence.
        if (vs->magcal_seq != s_traced_magcal_seq) {
            s_traced_magcal_seq = vs->magcal_seq;
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
        if (s_mag.percent() != s_traced_mag_pct) {
            s_traced_mag_pct = s_mag.percent();
            gcs_log("magcal state: %d %%%s%s", s_traced_mag_pct,
                    s_mag.message().empty() ? "" : " \xe2\x80\x94 ",
                    s_mag.message().c_str());
        }
    }

    s_gyro.tick(now);
    if (gyro_was_running && !s_gyro.running())
        gcs_log("gyro cal: %s", s_gyro.result_text());

    s_mag.tick(now);
    if (mag_was_running && !s_mag.running()) {
        if (s_mag.phase() == MagCalibration::Phase::Succeeded)
            gcs_log("compass cal: complete%s",
                    s_mag.needs_save() ? " \xe2\x80\x94 not saved by the vehicle" : "");
        else
            gcs_log("compass cal: failed \xe2\x80\x94 %s", s_mag.failure_text());
    }
}

// ── What the vehicle is made of ──────────────────────────────────────────────

using ParamMap = std::unordered_map<std::string, ParamEntry>;

// Every boxed list on this tab is sized to its own contents, so a scrollbar
// inside one would be a bar with nothing to scroll — and a second scroll target
// under the pointer, stealing the wheel from the page that does scroll. The
// page's own region, at the bottom of this file, is the only one that scrolls.
constexpr ImGuiWindowFlags kNoScroll = ImGuiWindowFlags_NoScrollbar |
                                       ImGuiWindowFlags_NoScrollWithMouse;

// The sensors of one kind, as the parameter table describes them. This is where
// the numbers come from and it is worth being plain about it: there is no
// MAVLink message that lists a vehicle's compasses, so an empty table here
// means the parameters have not been fetched, not that the vehicle has none.
static void draw_sensor_devices(const ParamMap* params, SensorKind kind,
                                const char* id, const char* noun)
{
    const std::vector<SensorDevice> devices = sensor_inventory(params, kind);

    if (devices.empty()) {
        if (!params || params->empty())
            ImGui::TextDisabled("Fetch the parameters to list the %s.", noun);
        else
            ImGui::TextDisabled("No %s found in the parameters.", noun);
        return;
    }

    ImGui::PushStyleColor(ImGuiCol_ChildBg, bg_param_list());
    ImGui::PushStyleColor(ImGuiCol_Border,  col_separator());
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, FRAME_BORDER_NORMAL);

    const float h = devices.size() * ImGui::GetTextLineHeightWithSpacing() + 10.0f;
    if (ImGui::BeginChild(id, { -1.0f, h }, true, kNoScroll)) {
        for (const SensorDevice& d : devices) {
            // A device type the table does not know is shown as its number
            // rather than left blank: it is still the chip's identity, and a
            // blank would read as a sensor that failed to identify itself.
            char chip[32];
            if (d.devtype_name[0]) snprintf(chip, sizeof(chip), "%s", d.devtype_name);
            else                   snprintf(chip, sizeof(chip), "type 0x%02X", d.devtype);

            // Greyed when the vehicle is not using it, which is a thing worth
            // seeing before calibrating: a disabled compass calibrates and
            // then goes on being ignored.
            const bool dim = d.has_use && !d.in_use;
            if (dim) ImGui::PushStyleColor(ImGuiCol_Text, ui_col_label());

            ImGui::Text("%d  %-12s %s bus %u  0x%02X%s%s",
                        d.instance, chip, d.bus_name, d.bus, d.address,
                        (d.has_external && d.external) ? "  ext" : "",
                        dim ? "  unused" : "");

            if (dim) ImGui::PopStyleColor();

            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s = %u", d.param.c_str(), d.devid);
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);
}

// ── The six positions, ticked off as they are confirmed ──────────────────────

static void draw_position_checklist()
{
    const AccelCalPos now  = s_accel.requested();
    const int         done = s_accel.confirmed_count();

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

// The vehicle's own sentence, boxed. Shown verbatim rather than paraphrased —
// it is the vehicle that knows what it wants, and a GCS that rewrote the
// instruction would have one more thing to keep in step with firmware.
static void draw_vehicle_message(const char* id, const std::string& msg, ImVec4 col)
{
    ImGui::PushStyleColor(ImGuiCol_ChildBg, bg_child_dark());
    ImGui::PushStyleColor(ImGuiCol_Border,  col);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, FRAME_BORDER_NORMAL);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,   { 6.0f, 5.0f });

    const char* text = msg.empty() ? "Waiting for the vehicle\xe2\x80\xa6" : msg.c_str();
    const float text_w = ImGui::GetContentRegionAvail().x - 12.0f;
    const float box_h  = ImGui::CalcTextSize(text, nullptr, false, text_w).y + 10.0f;

    if (ImGui::BeginChild(id, { -1.0f, box_h }, true, kNoScroll)) {
        ImGui::PushTextWrapPos(text_w);
        ImGui::PushStyleColor(ImGuiCol_Text, msg.empty() ? ui_col_label() : ui_col(col));
        ImGui::TextUnformatted(text);
        ImGui::PopStyleColor();
        ImGui::PopTextWrapPos();
    }
    ImGui::EndChild();

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
}

// ─────────────────────────────────────────────────────────────────────────────

// `armed` is passed in rather than read here because it blocks both
// calibrations for the same reason: motors spinning under an airframe the
// operator has been told to pick up, or to leave perfectly still.
static void draw_accel_section(MavlinkSender* sender, bool connected, bool armed,
                               uint8_t tsys, uint8_t tcomp, const ParamMap* params)
{
    ImGui::TextColored(accent_col(), "ACCELEROMETER");
    ImGui::Spacing();

    // Only while idle. Mid-calibration the operator is being asked to do
    // something, and an inventory below the instruction is one more thing in
    // the way of reading it.
    if (s_accel.phase() == AccelCalibration::Phase::Idle) {
        draw_sensor_devices(params, SensorKind::Accel, "##accel_devs",
                            "accelerometers");
        ImGui::Spacing();
    }

    switch (s_accel.phase()) {

    case AccelCalibration::Phase::Idle: {
        ImGui::TextWrapped("Six-position calibration. The vehicle asks for each "
                           "orientation in turn \xe2\x80\x94 hold it still in that "
                           "position, then confirm.");
        ImGui::Spacing();

        ImGui::BeginDisabled(!connected || armed);
        if (ui_solid_button("CALIBRATE ACCELEROMETER", { -1.0f, 30.0f },
                            btn_write_base(), btn_write_hov())) {
            sender->calibrate_accelerometer(tsys, tcomp);
            s_accel.begin();
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
        const int done  = s_accel.confirmed_count();
        const int total = accel_cal_pos_count();

        char prog[32];
        snprintf(prog, sizeof(prog), "%d / %d positions", done, total);
        ui_progress_bar(prog, total > 0 ? (float)done / total : 0.0f, 12.0f);

        ImGui::Spacing();
        draw_vehicle_message("##accel_msg", s_accel.message(), accent_col());
        ImGui::Spacing();

        const AccelCalPos pos   = s_accel.requested();
        const char*       label = accel_cal_pos_label(pos);
        const bool        ready = (s_accel.phase() == AccelCalibration::Phase::Waiting)
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
            const AccelCalPos sent = s_accel.confirm();
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
            s_accel.cancel();
            gcs_log("accel cal: cancelled");
        }

        ImGui::Spacing();
        draw_position_checklist();
        break;
    }

    case AccelCalibration::Phase::Succeeded: {
        draw_vehicle_message("##accel_msg", s_accel.message(), col_ok());
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
            s_accel.cancel();

        // Confirmed, and never automatic. Rebooting is the GCS reaching out and
        // stopping the thing it is talking to: the link goes with it, and over
        // USB the serial device disappears entirely. That is the operator's
        // call to make, not a step to slip into the end of a wizard.
        if (ui_confirm_popup("##confirm_reboot", "REBOOT VEHICLE",
                             "RESTART THE AUTOPILOT NOW?",
                             "REBOOT", btn_write_base()) == UiConfirm::Confirmed) {
            sender->reboot_autopilot(tsys, tcomp);
            s_accel.cancel();
            gcs_log("reboot requested \xe2\x80\x94 the link will drop and must be "
                    "reconnected");
        }
        break;
    }

    case AccelCalibration::Phase::Failed: {
        draw_vehicle_message("##accel_msg", s_accel.message(), col_error());
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
            s_accel.begin();
            gcs_log("accel cal: retrying");
        }
        ImGui::EndDisabled();
        ImGui::SameLine(0, 4);
        if (ui_grid_button("CLOSE##accel", { -1.0f, 28.0f }))
            s_accel.cancel();
        break;
    }
    }
}

// ── Gyroscope ────────────────────────────────────────────────────────────────

static void draw_gyro_section(MavlinkSender* sender, bool connected, bool armed,
                              uint8_t tsys, uint8_t tcomp, const ParamMap* params)
{
    ImGui::TextColored(accent_col(), "GYROSCOPE");
    ImGui::Spacing();

    if (s_gyro.phase() == GyroCalibration::Phase::Idle) {
        draw_sensor_devices(params, SensorKind::Gyro, "##gyro_devs", "gyroscopes");
        ImGui::Spacing();
    }

    switch (s_gyro.phase()) {

    case GyroCalibration::Phase::Idle: {
        ImGui::TextWrapped("The vehicle samples its own gyros and needs a few "
                           "seconds of stillness \xe2\x80\x94 set it down level, "
                           "hands off, and do not move it.");
        ImGui::Spacing();

        ImGui::BeginDisabled(!connected || armed);
        if (ui_solid_button("CALIBRATE GYROSCOPE", { -1.0f, 30.0f },
                            btn_write_base(), btn_write_hov())) {
            sender->calibrate_gyroscope(tsys, tcomp);
            s_gyro.begin(ImGui::GetTime());
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
        const double elapsed = s_gyro.elapsed(ImGui::GetTime());
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
            s_gyro.cancel();
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
            s_gyro.cancel();
        break;
    }

    case GyroCalibration::Phase::Failed: {
        ImGui::TextColored(col_error(), "CALIBRATION FAILED");
        // The vehicle's own verdict, named. "denied" and "no reply" are
        // different problems from a wobbly table, and the advice below is only
        // right for one of them.
        ImGui::TextColored(col_error(), "Vehicle said: %s", s_gyro.result_text());
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
            s_gyro.begin(ImGui::GetTime());
            gcs_log("gyro cal: retrying");
        }
        ImGui::EndDisabled();
        ImGui::SameLine(0, 4);
        if (ui_grid_button("CLOSE##gyro", { -1.0f, 28.0f }))
            s_gyro.cancel();
        break;
    }
    }
}

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
        s_traced_magcal_seq = vs ? vs->magcal_seq : 0;
        s_traced_mag_pct    = -1;
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
        s_magcal_stream_on = true;
        if (trace_mag_cal)
            gcs_log("magcal tx: SET_MESSAGE_INTERVAL 191 @ 5 Hz, 192 @ 1 Hz");

        sender->start_mag_cal(tsys, tcomp);
        gcs_log("compass cal: requested (DO_START_MAG_CAL, autosave off)");
    } else {
        sender->calibrate_magnetometer(tsys, tcomp);
        gcs_log("compass cal: requested (PREFLIGHT_CALIBRATION)");
    }
}


// ── The coverage sphere ──────────────────────────────────────────────────────
//
// MAG_CAL_PROGRESS carries 80 bits, one for each section of the sphere of
// directions the compass has been turned through — ArduPilot's geodesic grid,
// an icosahedron cut once. A percentage says how much is left; this says which
// part, which is the thing the operator can actually act on.
//
// Drawn rather than modelled: 80 triangles, projected orthographically, painted
// far-to-near so the far side shows through the near one. The vectors are in
// body frame and so is the view — the screen is the vehicle seen from in front,
// +Y to the right and +Z down, turned by dragging.
//
// Nothing here rotates with the vehicle's attitude. The mask is in body frame,
// and a sphere that turned with the airframe would be showing the operator
// gaps that are not where it says they are.

static float s_sphere_yaw   = 0.6f;
static float s_sphere_pitch = 0.35f;

static void draw_coverage_sphere(const uint8_t* mask, float height)
{
    const GeodesicSection* sections = geodesic_sections();

    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const float  width  = ImGui::GetContentRegionAvail().x;
    ImGui::InvisibleButton("##mag_sphere", { width, height });

    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        const ImVec2 d = ImGui::GetIO().MouseDelta;
        s_sphere_yaw   += d.x * 0.012f;
        // Minus: the sphere follows the cursor. Dragging down brings the near
        // face down and the top of the sphere towards the viewer, which is what
        // a hand on a globe does.
        s_sphere_pitch -= d.y * 0.012f;
        if (s_sphere_pitch >  1.45f) s_sphere_pitch =  1.45f;
        if (s_sphere_pitch < -1.45f) s_sphere_pitch = -1.45f;
    }

    const ImVec2 centre { origin.x + width * 0.5f, origin.y + height * 0.5f };
    const float  radius = (width < height ? width : height) * 0.44f;

    const float cy = cosf(s_sphere_yaw),   sy = sinf(s_sphere_yaw);
    const float cp = cosf(s_sphere_pitch), sp = sinf(s_sphere_pitch);

    // Body frame in, screen out. Yaw about z (down), then pitch about y (right);
    // what is left of x points at the viewer and serves as depth.
    auto project = [&](const float* v, ImVec2& out, float& depth) {
        const float x1 =  v[0] * cy - v[1] * sy;
        const float y1 =  v[0] * sy + v[1] * cy;
        const float z1 =  v[2];
        const float x2 =  x1 * cp + z1 * sp;
        const float z2 = -x1 * sp + z1 * cp;
        out   = { centre.x + radius * y1, centre.y + radius * z2 };
        depth = x2;
    };

    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddCircleFilled(centre, radius * 1.02f,
                        ImGui::GetColorU32(bg_param_list()), 48);

    struct Facet { ImVec2 p[3]; float depth; int index; };
    Facet facets[GEODESIC_SECTION_COUNT];
    for (int s = 0; s < GEODESIC_SECTION_COUNT; ++s) {
        Facet& f = facets[s];
        f.index  = s;
        f.depth  = 0.0f;
        for (int k = 0; k < 3; ++k) {
            float d;
            project(sections[s].v[k], f.p[k], d);
            f.depth += d / 3.0f;
        }
    }

    // Painter's algorithm, far first. An insertion sort over eighty facets is
    // nothing next to the eighty filled triangles that follow it.
    for (int i = 1; i < GEODESIC_SECTION_COUNT; ++i) {
        Facet f = facets[i];
        int j = i - 1;
        while (j >= 0 && facets[j].depth > f.depth) { facets[j + 1] = facets[j]; --j; }
        facets[j + 1] = f;
    }

    const ImVec4 lit   = col_ok();
    const ImVec4 unlit = col_no_link_muted();
    const ImU32  edge  = ImGui::GetColorU32(col_separator());

    for (const Facet& f : facets) {
        const bool covered = geodesic_section_covered(mask, f.index);
        const bool near_   = f.depth >= 0.0f;

        // The far side is drawn faint rather than not at all: a gap behind the
        // vehicle is exactly the one the operator cannot see, and making them
        // orbit the view to find it defeats the point of drawing it.
        ImVec4 c = covered ? lit : unlit;
        c.w = near_ ? (covered ? 0.92f : 0.55f)
                    : (covered ? 0.30f : 0.14f);

        ImVec2 pts[3] = { f.p[0], f.p[1], f.p[2] };
        dl->AddConvexPolyFilled(pts, 3, ImGui::GetColorU32(c));
        if (near_)
            dl->AddPolyline(pts, 3, edge, ImDrawFlags_Closed, 1.0f);
    }

    dl->AddCircle(centre, radius * 1.02f, ImGui::GetColorU32(col_separator()),
                  48, 1.0f);

    // Which way the vehicle is facing in all this. Only the ends pointing at
    // the viewer are labelled; the others are behind the sphere.
    struct Axis { float v[3]; const char* name; };
    const Axis axes[] = {
        { {  1.f,  0.f,  0.f }, "NOSE"  },
        { {  0.f,  1.f,  0.f }, "RIGHT" },
        { {  0.f,  0.f,  1.f }, "DOWN"  },
    };
    for (const Axis& a : axes) {
        ImVec2 p; float d;
        project(a.v, p, d);
        if (d < 0.0f) continue;
        const ImVec2 at { p.x + 4.0f, p.y - ImGui::GetTextLineHeight() * 0.5f };
        dl->AddText(at, ImGui::GetColorU32(accent_col()), a.name);
    }

    ImGui::TextDisabled("%d / %d sections \xe2\x80\x94 drag to turn",
                        geodesic_covered_count(mask), GEODESIC_SECTION_COUNT);
}

static void draw_mag_section(MavlinkSender* sender, bool connected, bool armed,
                             uint8_t tsys, uint8_t tcomp, const VehicleState* vs,
                             const ParamMap* params)
{
    ImGui::TextColored(accent_col(), "MAGNETOMETER");
    ImGui::Spacing();

    // The run is over, however it ended: let the two streams go again. Both are
    // silent outside a calibration, so this is tidiness rather than bandwidth —
    // and it means the next run asks for them afresh on a vehicle that has
    // rebooted in between and forgotten.
    if (s_magcal_stream_on && !s_mag.running()) {
        s_magcal_stream_on = false;
        if (connected) {
            sender->request_message_interval(tsys, tcomp, 191, -1);
            sender->request_message_interval(tsys, tcomp, 192, -1);
            if (trace_mag_cal)
                gcs_log("magcal tx: SET_MESSAGE_INTERVAL 191, 192 disabled");
        }
    }

    if (s_mag.phase() == MagCalibration::Phase::Idle) {
        draw_sensor_devices(params, SensorKind::Compass, "##mag_devs", "compasses");
        ImGui::Spacing();
    }

    switch (s_mag.phase()) {

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
            s_mag.begin(vs ? vs->magcal_seq : 0, ImGui::GetTime());
        }
        ImGui::EndDisabled();

        if (!connected)  ImGui::TextDisabled("Connect to a vehicle first.");
        else if (armed)  ImGui::TextColored(col_error(), "Disarm before calibrating.");
        break;
    }

    case MagCalibration::Phase::Running: {
        const int pct = s_mag.percent();

        char prog[32];
        snprintf(prog, sizeof(prog), "%d %%", pct);
        ui_progress_bar(prog, pct / 100.0f, 12.0f);

        // ArduPilot only. PX4 sends no mask, and there the percentage and the
        // vehicle's own words are the whole of what the panel knows.
        if (const uint8_t* covered = s_mag.completion_mask()) {
            ImGui::Spacing();
            draw_coverage_sphere(covered, 190.0f);
        }

        ImGui::Spacing();
        draw_vehicle_message("##mag_msg", s_mag.message(), accent_col());
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
            s_mag.cancel();
            gcs_log("compass cal: cancelled");
        }
        break;
    }

    case MagCalibration::Phase::Succeeded: {
        ImGui::TextColored(col_ok(), "CALIBRATION SUCCESSFUL");
        if (s_mag.worst_fitness() >= 0.0f)
            ImGui::TextDisabled("Worst fit: %.1f mGauss", s_mag.worst_fitness());

        if (const uint8_t* covered = s_mag.completion_mask()) {
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
        if (s_mag.needs_save()) {
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
        if (ui_grid_button(s_mag.needs_save() ? "DISCARD##mag" : "DONE##mag",
                           { -1.0f, 28.0f })) {
            if (s_mag.needs_save())
                gcs_log("compass cal: discarded \xe2\x80\x94 offsets not accepted");
            s_mag.cancel();
        }

        if (ui_confirm_popup("##confirm_reboot_mag", "REBOOT VEHICLE",
                             "RESTART THE AUTOPILOT NOW?",
                             "REBOOT", btn_write_base()) == UiConfirm::Confirmed) {
            sender->reboot_autopilot(tsys, tcomp);
            s_mag.cancel();
            gcs_log("reboot requested \xe2\x80\x94 the link will drop and must be "
                    "reconnected");
        }
        break;
    }

    case MagCalibration::Phase::Failed: {
        draw_vehicle_message("##mag_msg", s_mag.message(), col_error());
        ImGui::Spacing();
        ImGui::TextColored(col_error(), "CALIBRATION FAILED");
        ImGui::TextColored(col_error(), "Vehicle said: %s", s_mag.failure_text());
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
            s_mag.begin(vs ? vs->magcal_seq : 0, ImGui::GetTime());
        }
        ImGui::EndDisabled();
        ImGui::SameLine(0, 4);
        if (ui_grid_button("CLOSE##mag", { -1.0f, 28.0f }))
            s_mag.cancel();
        break;
    }
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
    const bool accel_busy = s_accel.running();
    const bool gyro_busy  = s_gyro.running();
    const bool mag_busy   = s_mag.running();

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
