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

// Shared by the MISSION tab's files (tab_mission.cpp and mission/). Not part
// of the public API.

#include "frontend/components/sidebar_left/sidebar_internal.hpp"
#include "backend/vehicle/firmware_profile.hpp"
#include "frontend/source/vehicle_ui_state.hpp"
#include "frontend/components/sidebar_themes.hpp"
#include "frontend/source/app_log.hpp"
#include "frontend/source/audio.hpp"
#include "frontend/source/mission_alt.hpp"
#include "frontend/source/terrain.hpp"
#include "frontend/source/terrain_clearance.hpp"
#include "imgui.h"
#include "frontend/source/mission_cmds.hpp"
#include <cstdint>
#include <optional>
#include <vector>

namespace mission_tab {

// ── Editor state ─────────────────────────────────────────────────────────────

// Per vehicle, not per panel — see source/vehicle_ui_state.hpp.
//
// The editor holds a mission the operator is building. Switching the callsign
// chip to look at another aircraft and then pressing UPLOAD would otherwise
// send this aircraft's plan to that one; keying it also means the plan is still
// there when they switch back.
struct MissionTabState {
    std::vector<bool>        open;   // per-item expand state, one per waypoint
    std::vector<MissionItem> edit;
    uint32_t fetch_gen        = 0;      // mission_generation when last synced
    bool     has_edit         = false;
    bool     upload_done      = false;  // show result badge once
    bool     cleared          = false;  // user pressed CLEAR; don't auto-re-sync
    bool     fetching         = false;  // FETCH pressed; awaiting fresh reply

    // ── FENCE view ────────────────────────────────────────────────────────────
    // The uploaded geofence, edited as shapes and kept in step with the
    // vehicle's own fence list exactly as `edit` is with its mission.
    bool      fence_view     = false;
    FencePlan fence;
    uint32_t  fence_gen      = 0;   // fence_generation when last synced
    bool      fence_has_edit = false;
    bool      fence_cleared  = false;
    bool      fence_fetching = false;
    int       fence_open     = -1;  // the zone expanded in the list
};

// The state of the vehicle the render loop has bound; see vehicle_ui_state.hpp.
MissionTabState& st();

// fields.cpp
void labeled_float(const char* label, float* v, const char* unit = nullptr,
                   float /*speed*/ = 0.1f);
void labeled_double(const char* label, double* v, const char* unit = nullptr);

// The ground under an item and its height above it; see fields.cpp.
struct ItemTerrain {
    std::optional<float> ground;   // metres AMSL under the item
    std::optional<float> agl;      // the item's height above that
    bool                 relative = false;   // agl missing for want of home
};
ItemTerrain item_terrain(const MissionItem& item, std::optional<float> home);
void draw_wp_fields(MissionItem& item, std::optional<float> home, const MissionLeg* leg);

// add.cpp
void append_item(uint16_t cmd, double lat, double lon,
                 std::optional<float> alt = std::nullopt);
void draw_add_grid(MissionPickState* pick);

// fence.cpp
void finish_fence_polygon(MissionPickState* pick);
void consume_fence_pick(MissionPickState* pick);
void draw_fence_view(MavlinkSender* sender, const VehicleState* vs,
                     MissionPickState* pick, bool connected,
                     uint8_t tsys, uint8_t tcomp);

// list.cpp
void draw_waypoint_list(std::optional<float> plan_home,
                        const std::vector<const MissionLeg*>& leg_to);

} // namespace mission_tab
