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

#include <cstddef>
#include <cstdint>

// ── Parameter metadata ───────────────────────────────────────────────────────
//
// What ArduPilot documents about each parameter: what it is, its units and
// range, the values it accepts. The table itself is param_meta_generated.cpp,
// produced by scripts/gen_param_meta.py from the published Copter parameter
// list of every stable release from 4.0.0 on
// (https://ardupilot.org/copter/docs/parameters.html, "another version") and
// checked in — rerun the script when a new stable release appears.
//
// Parameters come and go between releases and their documentation changes —
// ARMING_CHECK became ARMING_SKIPCHK, WPNAV_SPEED in cm/s became WP_SPD in
// m/s — so the table is the superset of all of them. Each record carries the
// releases it was published for, and the lookup takes the release the
// vehicle runs.

// What kind of value a parameter holds, and so which widget edits it.
//
// Bitmask is the page's own word. Enum nearly is: the page's Values list,
// unless it also gives a Range or a fractional value — then the values are
// landmarks on a continuum (ATC_INPUT_TC's 0.5 "Very Soft" in 0 to 1), the
// parameter is a plain number, and enum_entries holds them as suggestions.
// Int and Float are not the page's word at all: the page
// never names a storage type, so the generator calls a plain number an Int
// when its step and range are whole. The vehicle does know — PARAM_VALUE
// carries the type — and param_effective_kind() prefers that.
enum class ParamKind : uint8_t { Float, Int, Enum, Bitmask };

struct ParamEnumEntry  { float   value; const char* label; };
struct ParamBitmaskBit { uint8_t bit;   const char* label; };

enum : uint8_t {
    PARAM_FLAG_REBOOT        = 1 << 0,   // takes effect only after a reboot
    PARAM_FLAG_ADVANCED      = 1 << 1,   // "for advanced users"
    PARAM_FLAG_READONLY      = 1 << 2,
    PARAM_FLAG_CALIBRATION   = 1 << 3,   // set by a calibration routine
    PARAM_FLAG_VOLATILE      = 1 << 4,   // changed by the firmware itself
    PARAM_FLAG_HAS_RANGE     = 1 << 5,
    PARAM_FLAG_HAS_INCREMENT = 1 << 6,
};

struct ParamMeta {
    const char*            name;
    // Library that owns this documentation when the page has several for one
    // name — "AC_AttitudeControl_Multi" against "..._Heli". nullptr otherwise.
    const char*            variant;
    const char*            display_name;
    const char*            group;          // the page's section, "ARMING_ Parameters"
    const char*            description;
    const char*            units;          // nullptr = dimensionless
    float                  min_val;        // valid with PARAM_FLAG_HAS_RANGE
    float                  max_val;
    float                  increment;      // valid with PARAM_FLAG_HAS_INCREMENT
    uint8_t                flags;
    ParamKind              kind;
    const ParamEnumEntry*  enum_entries;   // Enum's values, or a number's suggested ones
    int                    enum_count;
    const ParamBitmaskBit* bitmask_bits;   // set for Bitmask, else nullptr
    int                    bitmask_count;
    // Bit i set: release i of param_doc_versions() documents this parameter
    // with exactly this text.
    uint64_t               versions;

    bool has(uint8_t f) const { return (flags & f) != 0; }
};

struct ParamDocVersion { uint8_t major, minor, patch; };

// The whole table, sorted by name; a name's variants and per-release
// revisions sit together. Defined in param_meta_generated.cpp.
const ParamMeta* param_meta_table(size_t* count);

// The stable releases the table was built from, oldest first.
const ParamDocVersion* param_doc_versions(size_t* count);

// Index into param_doc_versions() of the reference that fits a firmware
// version string such as "4.5.7": the newest documented release not newer
// than it, so a 4.5.8 that has no page of its own reads 4.5.7's. Firmware
// older than every page gets the oldest. -1 when the string is not a version
// (no AUTOPILOT_VERSION yet).
int param_doc_version_for(const char* fw_version);

// "V4.5.7" for a param_doc_versions() index.
void param_doc_version_label(int index, char* out, size_t n);

// Metadata for a parameter ID, or nullptr if no stable release documents it.
//
// `version` is a param_doc_versions() index, or -1 for unknown, which reads
// as the newest. The record published for that release wins; failing that —
// a parameter the vehicle has but its release's page left out — the one
// published nearest to it. `*doc_version`, when given, receives the release
// whose text was returned, which differs from `version` exactly when the
// match was not exact.
//
// `heli` picks the helicopter variant where the page documents one, so a
// TradHeli's ATC_RAT_RLL_P shows its own range rather than a multirotor's.
//
// A numbered instance no page lists on its own falls back to the first
// instance's entry: indexed families share their documentation.
const ParamMeta* get_param_meta(const char* param_id, bool heli, int version,
                                int* doc_version = nullptr);

// The kind to edit a parameter as: the page's Enum/Bitmask as they are, a
// plain number as Int or Float by what the vehicle says it stores
// (MAV_PARAM_TYPE), the page's guess only when the vehicle has not said.
// Works without metadata too, for parameters the page does not list.
ParamKind param_effective_kind(const ParamMeta* meta, uint8_t mav_param_type);

// Display name for a kind, "Bitmask", and for a MAV_PARAM_TYPE, "int16".
const char* param_kind_name(ParamKind k);
const char* param_mav_type_name(uint8_t mav_param_type);
