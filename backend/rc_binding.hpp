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
#include <vector>

#include "mavlink_parser.hpp"    // RcChannels
#include "rc_calibration.hpp"    // RcParamDialect, RcParamWrite

// ─────────────────────────────────────────────────────────────────────────────
// RC binding — which physical channel drives which vehicle function.
//
// Two separate things wear that name on a radio, and this covers both:
//
//   • stick binding: the four control axes and the mode switch, held in the
//     stack's channel-map parameters (RCMAP_* on ArduPilot, RC_MAP_* on PX4).
//     The value is a channel number.
//
//   • auxiliary binding: what a spare switch does, held per channel in
//     RCn_OPTION. The value is a function number.
//
// Receiver *pairing* — the bind-button ritual on a DSM or SRXL receiver — is
// deliberately not here. It is MAV_CMD_START_RX_PAIR, which the MAVLink
// dialect this GCS builds against no longer defines, and which neither of the
// two stacks we target implements over the link.
//
// Like rc_calibration, this sends nothing: it names parameters and produces
// RcParamWrite for the caller to commit.
// ─────────────────────────────────────────────────────────────────────────────

// One control axis, and where each stack keeps its channel number.
struct RcStickBinding {
    const char* label;        // "ROLL"
    const char* ardupilot;    // "RCMAP_ROLL"
    const char* px4;          // "RC_MAP_ROLL", or nullptr where PX4 has no analogue
    int         default_ch;   // 1-based, the stack's own default
};

// The axes in the order a radio lays them out. Stable across calls; safe to
// hold a reference to.
const std::vector<RcStickBinding>& rc_stick_bindings();

// The parameter name for `b` under `dialect`, or nullptr when that stack has no
// parameter for the axis — the caller must skip it rather than write a name
// from the other dialect.
const char* rc_stick_param(const RcStickBinding& b, RcParamDialect dialect);

// The throttle axis's index in rc_stick_bindings(), so a caller can find the
// throttle channel without matching on the label string.
int rc_stick_throttle_index();

// ── Auxiliary functions ──────────────────────────────────────────────────────

// Which ArduPilot firmware is flying. The auxiliary function table is shared
// across the four, but most entries only mean something on one or two of them —
// a Rover has no ALTHOLD to switch into.
enum class RcFirmware { Copter, Plane, Rover, Sub };

// From the vehicle's HEARTBEAT type. Anything unrecognised is treated as a
// Copter, which is both the largest table and this GCS's common case.
RcFirmware  rc_firmware_for(MAV_TYPE type);
const char* rc_firmware_label(RcFirmware fw);

// One RCn_OPTION value: what it does, and which firmwares implement it.
struct RcAuxOption {
    uint16_t    value;
    const char* label;
    bool        copter, plane, rover, sub;

    bool supported(RcFirmware fw) const
    {
        switch (fw) {
        case RcFirmware::Copter: return copter;
        case RcFirmware::Plane:  return plane;
        case RcFirmware::Rover:  return rover;
        case RcFirmware::Sub:    return sub;
        }
        return false;
    }
};

// The ArduPilot auxiliary function table. One copy, shared by the FLIGHT tab's
// AUX pad (which triggers these live over MAV_CMD_DO_AUX_FUNCTION) and the RC
// tab's binding editor (which writes them into RCn_OPTION). They are two views
// of the same list, and a GCS that let them drift would offer a function on one
// screen that the other did not believe in.
//
// PX4 has no equivalent per-channel option parameter; it binds spare switches
// through RC_MAP_AUXn instead, which is a stick binding above.
const std::vector<RcAuxOption>& rc_aux_options();

// The label for `value`, or nullptr when it is not in the table.
const char* rc_aux_option_label(int value);

// The RCn_OPTION parameter name for a 1-based channel. Returns false when the
// dialect has no such parameter, leaving `out` untouched.
bool rc_aux_param(int channel_1based, RcParamDialect dialect,
                  char* out, size_t out_len);

// ── Flight-mode switch slots ─────────────────────────────────────────────────
//
// ArduPilot does not bind a mode to a channel; it bands one channel's travel
// into six positions and keeps a mode number for each in FLTMODE1..FLTMODE6.
// Binding a mode switch therefore means editing those six parameters, and
// knowing which band the stick is sitting in right now.

struct RcModeSlot {
    int         index;      // 1-6, matching the parameter's name
    const char* param;      // "FLTMODE1"
    uint16_t    pwm_lo;     // inclusive
    uint16_t    pwm_hi;     // inclusive
};

// The six slots in order. Stable across calls.
const std::vector<RcModeSlot>& rc_mode_slots();

// Which slot a mode-channel reading falls in, 1-6, or 0 when there is no
// reading. The bands are contiguous and cover the whole range, so any plausible
// pulse lands somewhere.
int rc_mode_slot_for_pwm(uint16_t pwm);

// Fallback mode numbers for a vehicle that does not publish AVAILABLE_MODES
// (#435). Prefer the vehicle's own list wherever it has one: custom_mode
// numbering is per-frame, so this table is only correct for Copter, and only
// by accident for anything else.
struct RcModeOption {
    uint32_t    custom_mode;
    const char* label;
};
const std::vector<RcModeOption>& rc_copter_modes();

// ── Channel detection ────────────────────────────────────────────────────────
//
// "Move the stick you want to bind." Compares a live frame against a baseline
// taken before the operator touched anything and names the channel that moved
// furthest, so a binding can be set by demonstration rather than by counting
// channels on a transmitter.

// Movement below this is noise, a trim knob nudge, or a stick that was already
// drifting — not a deliberate demonstration.
constexpr uint16_t RC_DETECT_THRESHOLD_US = 200;

// Returns the 1-based channel that moved furthest from `baseline`, or 0 when
// nothing moved by more than `threshold`. Channels absent from either frame are
// skipped: a channel appearing for the first time has not "moved".
int rc_detect_moved_channel(const RcChannels& baseline, const RcChannels& live,
                            uint16_t threshold = RC_DETECT_THRESHOLD_US);
