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

// ── .params files ─────────────────────────────────────────────────────────────
//
// The parameter-file format Mission Planner and QGroundControl both write, so a
// file saved here loads there and vice versa. Tab-separated, one parameter per
// line, '#' comments:
//
//   # Onboard parameters for Vehicle 1
//   #
//   # Stack: ArduPilot
//   # Vehicle: Quadrotor
//   # Version: 4.8.0 dev
//   # Git Revision: 1ea89b0b
//   #
//   # Vehicle-Id Component-Id Name Value Type
//   1	1	ACRO_BAL_PITCH	1.000000000000000000	9
//   1	1	ACRO_OPTIONS	0	2
//
// The trailing column is MAV_PARAM_TYPE. Floating types are written at full
// precision — a REAL32 rounded to a few decimals comes back as a different
// number, and the vehicle would take the rounding as an edit.

#include <cstdint>
#include <string>
#include <vector>

struct ParamFileRow {
    std::string id;       // parameter name, max 16 chars
    float       value;
    uint8_t     type;     // MAV_PARAM_TYPE (9 = REAL32, 2 = UINT8, …)
    uint8_t     sysid;
    uint8_t     compid;
};

// Comment block written above the parameters. Empty fields are left out rather
// than written blank, so a file saved before AUTOPILOT_VERSION arrived does not
// claim a firmware it never saw.
struct ParamFileHeader {
    uint8_t     sysid  = 1;
    std::string stack;      // "ArduPilot", "PX4", …
    std::string vehicle;    // "Quadrotor", "Fixed Wing", …
    std::string version;    // firmware version string
    std::string git_hash;
};

// Write `rows` to `path`. Returns false and fills `err` on failure; the file is
// left untouched if it could not be opened.
bool param_file_save(const std::string& path,
                     const std::vector<ParamFileRow>& rows,
                     const ParamFileHeader& header,
                     std::string* err);

// Read a .params file. Malformed lines are skipped rather than failing the
// whole load — files are hand-edited often enough that one bad line should not
// cost the other 1400 — and `skipped` counts them. Returns false only when the
// file itself could not be read.
bool param_file_load(const std::string& path,
                     std::vector<ParamFileRow>* out,
                     int* skipped,
                     std::string* err);
