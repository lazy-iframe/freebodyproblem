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

// Shared by the SENSORS tab's files (tab_sensors.cpp and sensors/). Not part
// of the public API.

#include "frontend/components/sidebar_left/sidebar_internal.hpp"
#include "frontend/source/vehicle_ui_state.hpp"
#include "frontend/components/others/sidebar_themes.hpp"
#include "frontend/source/app_log.hpp"
#include "frontend/source/audio.hpp"
#include "backend/calibration/accel.hpp"
#include "backend/calibration/gyro.hpp"
#include "backend/calibration/mag.hpp"
#include "backend/vehicle/sensor_inventory.hpp"
#include "backend/calibration/geodesic_grid.hpp"
#include "imgui.h"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace sensors_tab {

// The calibrations outlive any one frame and, via sensors_tab_pump(), any
// one tab: the vehicle is mid-conversation and will keep asking for positions
// whether or not this panel is the one on screen.
// Per vehicle, not per panel — see the note on RcTabState in rc/rc_internal.hpp. An
// accelerometer calibration is six positions of one airframe, and the operator
// glancing at another aircraft in the fleet must not reset it.
struct SensorTabState {
    AccelCalibration accel;
    GyroCalibration  gyro;
    MagCalibration   mag;

    // What the pump has already handed to the state machines.
    uint32_t seen_accelcal_seq    = 0;
    uint32_t seen_calib_ack_seq   = 0;
    uint32_t seen_magcal_ack_seq  = 0;
    uint32_t consumed_statustexts = 0;

    uint32_t traced_magcal_seq = 0;
    int      traced_mag_pct    = -1;
    bool     magcal_stream_on  = false;
};

// The state of the vehicle the render loop has bound; see vehicle_ui_state.hpp.
SensorTabState& st();

// Compass-calibration tracing, the panel's half of it: what the state machine
// made of what arrived. The wire itself is traced in mavlink/parser/parser.cpp and in
// mavlink/sender/sender.cpp; this is here to catch the case where the messages are
// arriving and the machine is not moving. Set to false to quieten it.
inline constexpr bool trace_mag_cal = true;

// Whether MAG_CAL_PROGRESS and MAG_CAL_REPORT have been asked for. They are
// requested when a run starts and let go of when it ends — see
// start_mag_cal_for() for why they have to be asked for at all.

using ParamMap = std::unordered_map<std::string, ParamEntry>;

// Every boxed list on this tab is sized to its own contents, so a scrollbar
// inside one would be a bar with nothing to scroll — and a second scroll target
// under the pointer, stealing the wheel from the page that does scroll. The
// page's own region, in draw_tab_sensors(), is the only one that scrolls.
inline constexpr ImGuiWindowFlags kNoScroll = ImGuiWindowFlags_NoScrollbar |
                                       ImGuiWindowFlags_NoScrollWithMouse;

// common.cpp
void draw_sensor_devices(const ParamMap* params, SensorKind kind,
                         const char* id, const char* noun);
void draw_vehicle_message(const char* id, const std::string& msg, ImVec4 col);

// coverage_sphere.cpp
void draw_coverage_sphere(const uint8_t* mask, float height);

// accel.cpp, gyro.cpp, mag.cpp: one section each
void draw_accel_section(MavlinkSender* sender, bool connected, bool armed,
                        uint8_t tsys, uint8_t tcomp, const ParamMap* params);
void draw_gyro_section(MavlinkSender* sender, bool connected, bool armed,
                       uint8_t tsys, uint8_t tcomp, const ParamMap* params);
void draw_mag_section(MavlinkSender* sender, bool connected, bool armed,
                      uint8_t tsys, uint8_t tcomp, const VehicleState* vs,
                      const ParamMap* params);

} // namespace sensors_tab
