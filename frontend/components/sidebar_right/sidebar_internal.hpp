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

// Shared by the right sidebar's files. Not part of the public API; that is
// frontend/elements/sidebar_right.hpp.

#include "frontend/elements/center_view.hpp"
#include "frontend/source/vehicle_ui_state.hpp"
#include "frontend/source/layout.hpp"
#include "frontend/source/theme.hpp"
#include "frontend/components/others/ui_kit.hpp"
#include "imgui.h"
#include "frontend/components/sidebar_left/mavlink_field_catalog_generated.hpp"
#include "frontend/source/settings.hpp"
#include <vector>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <limits>
#include <cfloat>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <deque>
#include <cctype>
#include <ctime>
#include <string>
#include <cmath>

namespace right_sidebar {

// Height of one telemetry tile in the 3×3 grid.
inline constexpr float GRID_CELL_H = 66.0f;

// attitude_ball.cpp
void draw_classic_horizon(float roll_deg, float pitch_deg, float yaw_deg,
                           float airspeed_ms, int throttle_pct, bool has_vfr,
                           float size, float text_size = 0.0f);

// hud.cpp
void draw_artificial_horizon(const VehicleState& vs, ImVec2 sz,
                             float text_size = 0.0f);
void draw_hud_body(const VehicleState& vs, ImVec2 avail, bool tapes,
                   float text_size, float strip_value_size);

// event_log.cpp
// ── Fleet event log ───────────────────────────────────────────────────────────
//
// One log for the whole fleet rather than one per vehicle. A STATUSTEXT is the
// vehicle saying something went wrong, and the aircraft that most needs saying
// so is rarely the one already on screen — a per-vehicle log hides exactly the
// message the operator needed to see. So this does not follow the callsign
// chip: switching vehicles leaves it alone.

// What a line in the log is. Most are the vehicle speaking; the rest are the
// GCS drawing a line across the log where that vehicle's story broke — so what
// it said before a reboot or a dropped link is never read as being about the
// aircraft as it is now. The log cannot simply be cleared at those points: it
// is the whole fleet's, and the other vehicles' messages are still current.
enum class FleetLogKind : uint8_t {
    Message,
    RebootRequested,   // the operator asked; the vehicle may not have acted yet
    Rebooted,          // the vehicle's boot clock went back to zero
    LinkLost,          // stopped hearing it, or its link was dropped
    Assistant,         // an agent's call through MCP; reads like a message, tagged MCP
};

struct FleetLogEntry {
    FleetLogKind kind;
    VehicleId    vid;        // which vehicle, for placing a reboot marker
    uint32_t     number;     // Fleet's display number for the vehicle that spoke
    uint32_t     ordinal;    // Message only: its statustext_total, 1-based
    uint8_t      severity;
    std::string  stamp;      // wall clock at the moment it was taken in
    std::string  text;
    uint64_t     seq = 0;    // identity for the selection; see fleet_log_add()
};
extern std::deque<FleetLogEntry> g_fleet_log;
extern std::unordered_set<uint32_t> g_fleet_log_numbers;

// event_log_view.cpp
extern bool g_log_sel_dragging;
void draw_event_log_header(const char* meta);
void draw_event_log_body(float bg_alpha, bool small_text = false,
                         bool wrap = false);

// telemetry_grid.cpp
extern bool s_cells_loaded;
void cells_load(const AppSettings& settings);
void draw_data_grid(const VehicleState* vs, MavlinkSender* sender,
                     AppSettings* settings);

} // namespace right_sidebar
