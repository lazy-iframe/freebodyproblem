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

// Shared by the PARAMS tab's files (tab_params.cpp and params/). Not part of
// the public API.

#include "frontend/components/sidebar_left/sidebar_internal.hpp"
#include "frontend/components/others/sidebar_themes.hpp"
#include "frontend/source/vehicle_ui_state.hpp"
#include "frontend/source/param_meta.hpp"
#include "frontend/source/app_log.hpp"
#include "backend/input/rc_binding.hpp"
#include "imgui.h"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace params_tab {

// One line of a loaded file, waiting to be accepted or dropped. Only parameters
// the vehicle actually has and whose value differs get this far.
struct LoadRow {
    std::string id;
    float       file_val;
    float       vehicle_val;
    bool        selected;
};

// The pending review. Lives across frames because the popup does — cleared when
// the operator applies it or walks away.
// Per vehicle, not per panel — see source/vehicle_ui_state.hpp.
//
// Staged edits are the sharp one: a value typed against one aircraft and then
// written after switching the callsign chip would go to a different aircraft.
// The sorted name list matters for a quieter reason — it is rebuilt only when
// the parameter count changes, and two vehicles running the same firmware have
// the same count, so a shared list would keep showing the first vehicle's
// parameter names against the second's values.
struct ParamsTabState {
    std::vector<LoadRow> load_rows;
    int  load_matched = 0;   // in the file, already equal on the vehicle
    int  load_unknown = 0;   // in the file, not on this vehicle
    int  load_skipped = 0;   // unparseable lines

    std::vector<std::string> sorted_ids;
    size_t                   last_param_count = 0;

    std::unordered_map<std::string, float> edited_vals;

    // Enums and bitmasks switched to a typed number from their info popup:
    // for a value the reference does not list, or one known by heart.
    std::unordered_set<std::string> manual;
};

// The state of the vehicle the render loop has bound; see vehicle_ui_state.hpp.
ParamsTabState& st();

// info.cpp
const ParamDocSource* param_docs_for(const VehicleState* vs);
void draw_param_info(const ParamEntry& p, const ParamMeta* meta,
                     ParamKind kind, float shown, const ParamDocSource* docs,
                     int fw_doc, int doc, bool* manual);

// editors.cpp
//
// The row's value editor for `ui_kind` (the parameter's kind, or the plain
// number it is stored as under manual entry). True when the operator changed
// `edit_val`.
bool draw_value_editor(const ParamEntry& p, const ParamMeta* meta, ParamKind kind,
                       ParamKind ui_kind, bool manual, float& edit_val, float in_w);

// file.cpp
//
// The PARAM FILE buttons, their browsers and the load picker. Loaded values
// are staged into st().edited_vals; a save writes the list as shown.
void draw_param_file(const VehicleState* vs,
                     const std::unordered_map<std::string, ParamEntry>* params,
                     uint8_t tsys, uint8_t tcomp);

} // namespace params_tab
