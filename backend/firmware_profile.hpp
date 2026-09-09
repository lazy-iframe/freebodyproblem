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

#include <cstdint>
#include <string>
#include <vector>

#include "mavlink_parser.hpp"   // MAV_AUTOPILOT, MAV_TYPE, VehicleState

// ── What differs between flight stacks, in one place ─────────────────────────
//
// ArduPilot and PX4 speak the same MAVLink, and this GCS builds against the
// ardupilotmega dialect, which includes common.xml — so a PX4 vehicle's
// messages already decode. What does not carry across is narrower and sharper
// than "the dialect": which message reports estimator health, how a mode number
// is written on the wire, what a mode number *means*, and which commands exist
// at all.
//
// Those live here rather than as `if (autopilot == PX4)` at a dozen call sites,
// because the differences are not independent — a stack's mode numbering and
// its DO_SET_MODE encoding are the same fact seen twice, and splitting them is
// how they drift apart.
//
// Everything here is const and stateless. See firmware_profile() below.

class MavlinkSender;

// A DO_SET_MODE, already in the shape COMMAND_LONG wants.
//
// The three params are not interchangeable between stacks. common.xml gives
// param2 "Custom Mode" and param3 "Custom Submode"; ArduPilot puts its whole
// flat mode number in param2 and ignores param3, while PX4 splits main and sub
// across the two. PX4's packed (main<<16)|(sub<<24) word is what it *reports*
// in HEARTBEAT and never what it accepts here — sending that word in param2 is
// how every PX4 mode button silently lands somewhere else.
struct SetModeCommand {
    uint16_t command = 176;   // MAV_CMD_DO_SET_MODE, or 262 DO_SET_STANDARD_MODE
    float    param1  = 1.0f;  // base_mode, or the MAV_STANDARD_MODE for 262
    float    param2  = 0.0f;  // custom mode  / PX4 main mode
    float    param3  = 0.0f;  // custom submode — PX4 only
};

// One entry of a stack's built-in mode table, used only when the vehicle never
// answers AVAILABLE_MODES.
struct FallbackMode {
    const char* label;        // short, already uppercase: "STAB"
    uint32_t    custom_mode;  // as it appears in HEARTBEAT.custom_mode
};

// Why something cannot be done, so a panel can say so rather than fail
// silently. `reason` points at a string literal owned by the profile and
// outlives every caller; it is null exactly when `supported` is true.
struct Capability {
    bool        supported = false;
    const char* reason    = nullptr;
};

// ── PX4 mode packing ──────────────────────────────────────────────────────────
//
// PX4's custom_mode is a packed union rather than a flat number: HEARTBEAT
// reports (main_mode << 16) | (sub_mode << 24). Unlike ArduPilot's, the
// numbering does not change with the airframe — a PX4 quad and a PX4 plane
// agree on what mode 3 is.
namespace px4_mode {

enum Main : uint32_t {
    MANUAL = 1, ALTCTL = 2, POSCTL = 3, AUTO = 4,
    ACRO   = 5, OFFBOARD = 6, STABILIZED = 7,
};

enum AutoSub : uint32_t {
    READY = 1, TAKEOFF = 2, LOITER = 3, MISSION = 4,
    RTL   = 5, LAND    = 6, FOLLOW_TARGET = 8, PRECLAND = 9,
};

constexpr uint32_t pack(uint32_t main, uint32_t sub = 0)
{ return (main << 16) | (sub << 24); }

constexpr uint32_t main_of(uint32_t custom_mode) { return (custom_mode >> 16) & 0xFFu; }
constexpr uint32_t sub_of (uint32_t custom_mode) { return (custom_mode >> 24) & 0xFFu; }

// A shift typo here would be a mode button that commands the wrong mode without
// saying so, which is the worst thing this file could do. Pinned at compile
// time against values read off a real PX4 heartbeat.
static_assert(pack(AUTO, RTL) == 0x05040000u, "PX4 AUTO.RTL packing");
static_assert(pack(POSCTL)    == 0x00030000u, "PX4 POSCTL packing");
static_assert(main_of(0x05040000u) == AUTO,   "PX4 main_mode unpacking");
static_assert(sub_of (0x05040000u) == RTL,    "PX4 sub_mode unpacking");

} // namespace px4_mode

// ── The profile ───────────────────────────────────────────────────────────────

class FirmwareProfile {
public:
    virtual ~FirmwareProfile() = default;

    virtual const char* name() const = 0;          // "ArduPilot", "PX4"

    // ── Estimator health ─────────────────────────────────────────────────────

    // Which message this stack reports estimator health in: ArduPilot's own
    // EKF_STATUS_REPORT (#193, ardupilotmega.xml), or the portable
    // ESTIMATOR_STATUS (#230, common.xml) that PX4 sends.
    virtual uint32_t estimator_msgid() const = 0;

    // What the six bars are actually measuring. They are the same struct fields
    // either way, but not the same quantity — ArduPilot reports normalised
    // variances, PX4 innovation test ratios — so the panel has to say which.
    virtual const char* estimator_units() const = 0;

    // ── Flight modes ─────────────────────────────────────────────────────────

    // Used only when the vehicle publishes no AVAILABLE_MODES. Returns a
    // reference to a table with static lifetime.
    virtual const std::vector<FallbackMode>& fallback_modes(MAV_TYPE type) const = 0;

    // Short label for the topbar chip, under the same condition.
    virtual std::string mode_short_name(uint32_t custom_mode, MAV_TYPE type) const = 0;

    // The command that puts the vehicle in a mode. `custom_mode` is always the
    // HEARTBEAT / AVAILABLE_MODES form; `standard_mode` is that mode's
    // MAV_STANDARD_MODE, or 0 for a stack-specific one.
    //
    // Both are needed because neither alone is enough. A custom mode number
    // does not always reach the mode it names — PX4 reports Orbit as POSCTL
    // sub-mode 1 and then ignores that sub-mode from DO_SET_MODE, accepting
    // the command and going to Position Hold instead. The standard mode number
    // is what actually gets there.
    virtual SetModeCommand encode_set_mode(uint32_t custom_mode,
                                           uint8_t  standard_mode) const = 0;

    // ── Guided position targets ──────────────────────────────────────────────

    // How "fly to this point" is expressed to this stack. Not interchangeable:
    // ArduPilot honours the setpoint message in GUIDED, PX4 ignores it outside
    // OFFBOARD and wants the command instead.
    enum class GotoMethod {
        PositionTarget,   // SET_POSITION_TARGET_GLOBAL_INT (#86)
        Reposition,       // MAV_CMD_DO_REPOSITION (192), as COMMAND_INT
    };
    virtual GotoMethod goto_method() const = 0;

    // Will this vehicle, as it stands right now, act on a one-shot
    // SET_POSITION_TARGET_GLOBAL_INT?
    virtual Capability position_target(const VehicleState& vs) const = 0;

    // ── Auxiliary functions ──────────────────────────────────────────────────

    // MAV_CMD_DO_AUX_FUNCTION (218) and the RCn_OPTION function numbers behind
    // it. An ArduPilot invention; PX4 implements neither.
    virtual Capability aux_functions() const = 0;
};

// The profile for an autopilot. Never null, and never a dangling reference: the
// profiles are immortal const singletons, so this is safe to call from any
// thread and safe to hold. An unknown stack gets ArduPilot's, which is what
// every call site assumed before this file existed.
const FirmwareProfile& firmware_profile(MAV_AUTOPILOT autopilot);

inline const FirmwareProfile& firmware_profile(const VehicleState& vs)
{ return firmware_profile(vs.autopilot); }

// Put a vehicle in a mode, encoding it the way its stack expects.
//
// Declared here because this is the one place a profile and a sender meet. The
// profiles themselves stay sender-free: they are shared constants, and one that
// owned a link could not be.
void send_set_mode(MavlinkSender& sender, uint8_t tsys, uint8_t tcomp,
                   const FirmwareProfile& profile, uint32_t custom_mode,
                   uint8_t standard_mode = 0);

// Send this vehicle to a point, in whichever form its stack acts on.
// `altitude_m` is metres above home either way.
void send_goto(MavlinkSender& sender, uint8_t tsys, uint8_t tcomp,
               const FirmwareProfile& profile,
               double lat, double lon, float altitude_m);
