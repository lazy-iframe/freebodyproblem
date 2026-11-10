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
// What a firmware's documentation says about each parameter: what it is, its
// units and range, the values it accepts. One table per documented firmware,
// each a ParamDocSource generated from that firmware's published reference
// for every release and checked in — rerun the script when a release appears:
//
//   param_docs_arducopter   param_meta_arducopter.cpp, scripts/gen_param_meta_ardupilot.py copter
//                           https://ardupilot.org/copter/docs/parameters.html,
//                           every stable release from 4.0.0
//   param_docs_arduplane    param_meta_arduplane.cpp, scripts/gen_param_meta_ardupilot.py plane
//                           https://ardupilot.org/plane/docs/parameters.html,
//                           every stable release from 4.0.0
//   param_docs_px4          param_meta_px4.cpp, scripts/gen_param_meta_px4.py
//                           https://docs.px4.io/main/en/advanced_config/parameter_reference,
//                           every release from v1.12 ("main" left out)
//
// Parameters come and go between releases and their documentation changes —
// ARMING_CHECK became ARMING_SKIPCHK, WPNAV_SPEED in cm/s became WP_SPD in
// m/s — so a table is the superset of all of them. Each record carries the
// releases it was published for, and the lookup takes the release the
// vehicle runs.

// What kind of value a parameter holds, and so which widget edits it.
//
// Bitmask is the page's own word. Enum nearly is: the page's Values list,
// unless the range admits values the list does not name or a value is
// fractional — then the values are landmarks on a continuum (ATC_INPUT_TC's
// 0.5 "Very Soft" in 0 to 1), the parameter is a plain number, and
// enum_entries holds them as suggestions. A PX4 boolean is an Enum of
// Disabled/Enabled. Int and Float are PX4's word; ArduPilot's page
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
    PARAM_FLAG_HAS_DEFAULT   = 1 << 7,   // PX4 documents defaults, ArduPilot does not
};

// A range end the page leaves open ("[0, ?]") is stored as ±FLT_MAX.
constexpr float PARAM_RANGE_OPEN = 3.4028234663852886e38f;

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
    float                  default_val;    // valid with PARAM_FLAG_HAS_DEFAULT
    uint8_t                flags;
    ParamKind              kind;
    const ParamEnumEntry*  enum_entries;   // Enum's values, or a number's suggested ones
    int                    enum_count;
    const ParamBitmaskBit* bitmask_bits;   // set for Bitmask, else nullptr
    int                    bitmask_count;
    // Bit i set: release i of its source's versions documents this parameter
    // with exactly this text.
    uint64_t               versions;

    bool has(uint8_t f) const { return (flags & f) != 0; }
};

struct ParamDocVersion { uint8_t major, minor, patch; };

// One firmware's documentation.
struct ParamDocSource {
    const char*            name;           // "Copter", "Plane", "PX4"
    // Sorted by name; a name's variants and per-release revisions sit together.
    const ParamMeta*       params;
    size_t                 param_count;
    // The releases documented, oldest first.
    const ParamDocVersion* versions;
    size_t                 version_count;
    // One page per minor release (PX4): labelled "v1.15", and a 1.15.4
    // vehicle reads it. Otherwise one per patch release, "V4.5.7".
    bool                   minor_releases;
};

extern const ParamDocSource param_docs_arducopter;
extern const ParamDocSource param_docs_arduplane;
extern const ParamDocSource param_docs_px4;

// Index into src.versions of the reference that fits a firmware version
// string such as "4.5.7": the newest documented release not newer than it,
// so a 4.5.8 that has no page of its own reads 4.5.7's. Firmware older than
// every page gets the oldest. -1 when the string is not a version (no
// AUTOPILOT_VERSION yet).
int param_doc_version_for(const ParamDocSource& src, const char* fw_version);

// "V4.5.7" or "v1.15" for a src.versions index.
void param_doc_version_label(const ParamDocSource& src, int index, char* out, size_t n);

// Metadata for a parameter ID, or nullptr if no release documents it.
//
// `version` is a src.versions index, or -1 for unknown, which reads as the
// newest. The record published for that release wins; failing that — a
// parameter the vehicle has but its release's page left out — the one
// published nearest to it. `*doc_version`, when given, receives the release
// whose text was returned, which differs from `version` exactly when the
// match was not exact.
//
// `heli` picks the helicopter variant where the page documents one, so a
// TradHeli's ATC_RAT_RLL_P shows its own range rather than a multirotor's.
//
// A numbered instance no page lists on its own falls back to the first
// instance's entry: indexed families share their documentation.
const ParamMeta* get_param_meta(const ParamDocSource& src, const char* param_id,
                                bool heli, int version, int* doc_version = nullptr);

// The kind to edit a parameter as: the page's Enum/Bitmask as they are, a
// plain number as Int or Float by what the vehicle says it stores
// (MAV_PARAM_TYPE), the page's word only when the vehicle has not said.
// Works without metadata too, for parameters the page does not list.
ParamKind param_effective_kind(const ParamMeta* meta, uint8_t mav_param_type);

// Display name for a kind, "Bitmask", and for a MAV_PARAM_TYPE, "int16".
const char* param_kind_name(ParamKind k);
const char* param_mav_type_name(uint8_t mav_param_type);
