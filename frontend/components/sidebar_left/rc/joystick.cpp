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

// RC tab, joystick: the section's frame — axes, calibration, axis bindings, sending.

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
#include "frontend/components/sidebar_left/rc/joystick_internal.hpp"

namespace rc_tab {

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

void draw_joystick(const VehicleState* vs, bool connected)
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

} // namespace rc_tab
