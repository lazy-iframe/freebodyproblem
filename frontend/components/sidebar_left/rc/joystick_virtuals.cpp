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

// RC tab, joystick: virtual channels, set from button patterns.

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

// "button 4", "buttons 4+5", or "none" for the position with nothing pressed.
static std::string joy_pattern_text(uint32_t pattern)
{
    std::string out;
    int n = 0;
    for (int i = 0; i < JOY_MAX_BUTTONS; ++i) {
        if (!(pattern & (1u << i))) continue;
        out += (n++ ? "+" : "") + std::to_string(i + 1);
    }
    if (n == 0) return "none";
    return (n == 1 ? "button " : "buttons ") + out;
}

static void joy_wrapped_error(const char* text)
{
    ImGui::PushStyleColor(ImGuiCol_Text, col_error());
    ImGui::TextWrapped("%s", text);
    ImGui::PopStyleColor();
}

static void draw_joystick_virtual_capture()
{
    const int step  = joystick_virtual_capture_step();
    const int total = joystick_virtual_capture_total();

    ImGui::TextColored(col_warning(), "POSITION %d OF %d", step + 1, total);
    ImGui::TextWrapped("Put the switch in position %d and leave it there. Now reading: %s.",
                       step + 1, joy_pattern_text(joystick_buttons()).c_str());
    if (const char* why = joystick_virtual_capture_problem()) joy_wrapped_error(why);
    ImGui::Spacing();

    const float half = (ImGui::GetContentRegionAvail().x - 4.0f) * 0.5f;
    if (ui_solid_button("CAPTURE##joyv", { half, 26.0f }, btn_write_base(), btn_write_hov()))
        joystick_virtual_capture();
    ImGui::SameLine(0, 4);
    if (ui_grid_button("CANCEL##joyv", { -1.0f, 26.0f }))
        joystick_virtual_cancel();
}

void draw_joystick_virtuals(const VehicleState* vs, bool connected, bool editable)
{
    if (joystick_button_count() == 0) return;

    const JoystickProfile& profile = joystick_profile();

    static int s_new_positions = 3;
    static int s_open          = -1;   // the channel being edited
    static int s_open_position = 0;

    ImGui::Spacing();
    ImGui::TextColored(accent_col(), "VIRTUAL CHANNELS");
    ImGui::SameLine(0, 6);
    ImGui::TextDisabled("(multi-position switches)");

    ImGui::PushID("joyvirt");
    ImGui::BeginDisabled(!editable);

    int free_slots = 0;
    for (const JoyVirtualChannel& v : profile.virtuals)
        if (!v.used()) ++free_slots;

    if (joystick_virtual_capturing()) {
        draw_joystick_virtual_capture();
    } else {
        constexpr float NUM_W = 76.0f;
        char count_lbl[24];
        snprintf(count_lbl, sizeof(count_lbl), "%d POS", s_new_positions);
        ImGui::SetNextItemWidth(NUM_W);
        if (ImGui::BeginCombo("##joyv_count", count_lbl)) {
            for (int n = JOY_VIRTUAL_MIN_POS; n <= JOY_VIRTUAL_MAX_POS; ++n) {
                char item[24];
                snprintf(item, sizeof(item), "%d POS", n);
                if (ImGui::Selectable(item, n == s_new_positions)) s_new_positions = n;
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine(0, 4);
        ImGui::BeginDisabled(free_slots == 0);
        if (ui_grid_button("ADD VIRTUAL CHANNEL", { -1.0f, 0.0f }))
            joystick_virtual_begin(s_new_positions);
        ImGui::EndDisabled();
        if (const char* why = joystick_virtual_capture_problem()) joy_wrapped_error(why);
    }

    for (int i = 0; i < JOY_MAX_VIRTUAL; ++i) {
        const JoyVirtualChannel& v = profile.virtuals[i];
        if (!v.used()) continue;
        ImGui::PushID(i);

        const bool open = (s_open == i);
        char head[32];
        snprintf(head, sizeof(head), "V%d", i + 1);
        constexpr float REMOVE_W = 64.0f, EDIT_W = 46.0f;
        ImGui::Spacing();
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(accent_col(), "%s", head);
        ImGui::SameLine(0, 6);
        ImGui::TextDisabled("%s", joy_pattern_text(v.mask()).c_str());

        const float right = ImGui::GetWindowContentRegionMax().x;
        ImGui::SameLine(right - REMOVE_W - EDIT_W - 4.0f);
        if (ui_grid_button("EDIT", { EDIT_W, 0.0f }, open)) {
            s_open          = open ? -1 : i;
            s_open_position = 0;
        }
        ImGui::SameLine(right - REMOVE_W);
        if (ui_grid_button("REMOVE", { REMOVE_W, 0.0f }))
            ImGui::OpenPopup("##confirm_joyv_remove");

        char q[96];
        snprintf(q, sizeof(q), "REMOVE V%d AND ITS POSITION ACTIONS?", i + 1);
        if (ui_confirm_popup("##confirm_joyv_remove", "REMOVE VIRTUAL CHANNEL", q,
                             "REMOVE", btn_stop_base()) == UiConfirm::Confirmed) {
            joystick_virtual_remove(i);
            if (s_open == i) s_open = -1;
            ImGui::PopID();
            continue;
        }

        // Each position, what makes it and what it does, with the live one marked.
        const int live_pos = joystick_virtual_position(i);
        for (int p = 0; p < v.positions; ++p) {
            const std::string pat = joy_pattern_text(v.patterns[p]);
            const std::string act = v.actions[p].usable()
                                  ? joy_button_summary(v.actions[p]) : std::string("no action");
            const ImVec4 col = (p == live_pos) ? accent_col() : ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);
            ImGui::TextColored(col, "%s %d  %u  %s", p == live_pos ? ">" : " ", p + 1,
                               (unsigned)v.pwm(p), pat.c_str());
            ImGui::TextColored(col, "      %s", act.c_str());
        }

        if (open) {
            ImGui::Spacing();
            s_open_position = std::min(s_open_position, v.positions - 1);

            char pos_lbl[JOY_VIRTUAL_MAX_POS][16];
            const char* pos_ptrs[JOY_VIRTUAL_MAX_POS];
            for (int p = 0; p < v.positions; ++p) {
                snprintf(pos_lbl[p], sizeof(pos_lbl[p]), "POS %d##joyp", p + 1);
                pos_ptrs[p] = pos_lbl[p];
            }
            joy_choice_row(pos_ptrs, v.positions, s_open_position);

            JoyButtonBinding b = v.actions[s_open_position];
            static const char* const ACTIONS[] = { "NONE##joyva", "MODE##joyva", "AUX##joyva", "SERVO##joyva" };
            bool changed = joy_choice_row(ACTIONS, 4, b.action);
            if (changed) {
                // A switch usually drives one function through its positions, so
                // the other positions' choice carries over, and the position
                // itself suggests where it goes: LOW, MID, HIGH, or an even PWM.
                const JoyBtnAction a = b.action;
                b = JoyButtonBinding{};
                b.action = a;
                for (int p = 0; p < v.positions; ++p) {
                    const JoyButtonBinding& o = v.actions[p];
                    if (o.action != a) continue;
                    b.aux_fn = o.aux_fn;
                    b.servo  = o.servo;
                    break;
                }
                if (a == JoyBtnAction::Aux)
                    b.on = s_open_position == 0 ? 0
                         : s_open_position == v.positions - 1 ? 2 : 1;
                if (a == JoyBtnAction::Servo) b.on = v.pwm(s_open_position);
            }
            joy_edit_action(b, vs, connected, true, changed);
            if (changed) joystick_virtual_set_action(i, s_open_position, b);
        }

        ImGui::PopID();
    }

    ImGui::EndDisabled();
    ImGui::PopID();
}

} // namespace rc_tab
