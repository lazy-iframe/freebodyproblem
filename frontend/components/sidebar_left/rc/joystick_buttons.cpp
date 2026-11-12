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

// RC tab, joystick: what each button does.

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
#include "frontend/components/sidebar_left/rc/joystick_internal.hpp"

namespace rc_tab {

// The one line a bound button gets in the list.
std::string joy_button_summary(const JoyButtonBinding& b)
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
    const RcFirmware fw = rc_firmware_for(connected ? vehicle_kind(*vs) : VehicleKind::Copter);
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

void joy_edit_action(JoyButtonBinding& b, const VehicleState* vs, bool connected,
                     bool switch_position, bool& changed)
{
    switch (b.action) {
    case JoyBtnAction::None:  break;
    case JoyBtnAction::Mode:  changed |= joy_edit_mode(b, vs, connected); break;
    case JoyBtnAction::Aux:   changed |= joy_edit_aux(b, vs, connected, switch_position); break;
    case JoyBtnAction::Servo: changed |= joy_edit_servo(b, switch_position); break;
    }
}

void draw_joystick_buttons(const VehicleState* vs, bool connected, bool editable)
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

} // namespace rc_tab
