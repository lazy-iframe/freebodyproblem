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

// Shared by the RC tab's files. Not part of the public API.

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

namespace rc_tab {

// ─────────────────────────────────────────────────────────────────────────────
// Panel state.
//
// The calibration outlives any one frame and — via rc_tab_pump(), in tab_rc.cpp — any
// one tab: a sweep started here keeps recording while the operator is looking
// at PARAMS or the map, which is the difference between a calibration you can
// walk away from and one that silently stops the moment you glance elsewhere.
// ─────────────────────────────────────────────────────────────────────────────

// Per vehicle, not per panel.
//
// A calibration is a conversation with one airframe, and switching the callsign
// chip to look at another aircraft must not disturb it — the whole point of
// rc_tab_pump() is that a sweep survives the operator looking elsewhere, and
// with a fleet on screen "elsewhere" now includes another vehicle. Pending
// binding edits are keyed here for a sharper reason: committing edits staged
// against one aircraft to a different one would write the wrong RC map to
// flying hardware.
struct RcTabState {
    RcCalibration cal;
    uint32_t      seen_mask       = 0;
    bool          use_param_ext   = false;
    int           detect_target   = -1;
    RcChannels    detect_baseline;
    std::unordered_map<std::string, float> bind_edits;
    int           aux_channel     = 5;
};

// The extended parameter protocol is opt-in. Neither ArduPilot nor PX4 answers
// PARAM_EXT_SET for RC parameters — both speak the classic protocol for these,
// and MAVLink advertises no capability bit that would let us detect otherwise —
// so the default has to be PARAM_SET or a commit would land nowhere on the
// hardware this GCS actually flies. The switch is here for MAVLink components
// that do implement it, and because a commit is exactly where you want to be
// able to choose.

// Channel-detection ("move the stick you want") state. -1 when nothing is armed;
// otherwise the index into rc_stick_bindings() waiting for a movement.

// Binding values the operator has typed or demonstrated but not yet written.
// Keyed by parameter name, so ArduPilot and PX4 runs cannot collide.

// Aux-function channel the numeric editor is pointed at, 1-based.

// The state of the vehicle the render loop has bound; see vehicle_ui_state.hpp.
RcTabState& st();

inline constexpr float ROW_H = 15.0f;   // one channel row

// helpers.cpp
const ParamEntry* find_param(
    const std::unordered_map<std::string, ParamEntry>* params, const char* id);
bool params_ready(const std::unordered_map<std::string, ParamEntry>* params);
uint8_t param_type_for(
    const std::unordered_map<std::string, ParamEntry>* params, const char* id);
void commit_write(MavlinkSender* sender, uint8_t tsys, uint8_t tcomp,
                  const char* id, float value, uint8_t type);
float staged_value(const std::unordered_map<std::string, ParamEntry>* params,
                   const char* id, float fallback);
int centered_wrapped(const char* text, float wrap_w, ImVec4 col, bool draw);
void draw_channel_row(const char* lbl, uint16_t pwm,
                      const RcChannelCal* cal, const char* bind_label);

// radio.cpp
void draw_monitor(const RcChannels& rc,
                  const char* const bind_of_channel[RcChannels::MAX_CHANNELS]);
void draw_calibration(MavlinkSender* sender, const VehicleState* vs,
                      const std::unordered_map<std::string, ParamEntry>* params,
                      const RcChannels& rc, RcParamDialect dialect,
                      bool connected, uint8_t tsys, uint8_t tcomp);

// radio_binding.cpp
void draw_binding(MavlinkSender* sender, const VehicleState* vs,
                  const std::unordered_map<std::string, ParamEntry>* params,
                  const RcChannels& rc, RcParamDialect dialect,
                  bool connected, uint8_t tsys, uint8_t tcomp);

// joystick.cpp
void draw_joystick(const VehicleState* vs, bool connected);

} // namespace rc_tab
