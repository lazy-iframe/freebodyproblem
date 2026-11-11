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

#pragma once

// Shared by the RC tab's joystick files. Not part of the public API.

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

namespace rc_tab {

// joystick_buttons.cpp
std::string joy_button_summary(const JoyButtonBinding& b);
void joy_edit_action(JoyButtonBinding& b, const VehicleState* vs, bool connected,
                     bool switch_position, bool& changed);
void draw_joystick_buttons(const VehicleState* vs, bool connected, bool editable);

// Row of buttons acting as one choice. True if the choice changed.
template <class T>
inline bool joy_choice_row(const char* const* labels, int count, T& value)
{
    const float w = (ImGui::GetContentRegionAvail().x - 4.0f * (count - 1)) / count;
    bool changed = false;
    for (int i = 0; i < count; ++i) {
        if (i) ImGui::SameLine(0, 4);
        if (ui_grid_button(labels[i], { w, 0.0f }, (int)value == i) && (int)value != i) {
            value   = (T)i;
            changed = true;
        }
    }
    return changed;
}

// joystick_virtuals.cpp
void draw_joystick_virtuals(const VehicleState* vs, bool connected, bool editable);

// joystick_output.cpp
void draw_joystick_rc_override(const RcChannels& frame, bool editable);
void draw_joystick_output(bool profile_ok);

} // namespace rc_tab
