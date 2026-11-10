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

#include "backend/mavlink/parser/parser.hpp"   // RcChannels

// ─────────────────────────────────────────────────────────────────────────────
// Joystick control — turn raw joystick axes into MANUAL_CONTROL values.
//
// Holds no device and no link, like rc_calibration: it is fed numbers and hands
// numbers back, so the mapping can be checked without a stick or a vehicle.
//
// Axes travel as RcChannels, scaled to 1000-2000, so a joystick goes through the
// same RcCalibration and channel-detection code as a receiver does. That is a
// convenience inside the GCS only. A joystick's calibration is never written to
// the vehicle: RCn_MIN and friends describe the radio wired to the autopilot,
// and MANUAL_CONTROL does not pass through them in joystick units anyway.
// ─────────────────────────────────────────────────────────────────────────────

enum JoyFn : int { JOY_ROLL, JOY_PITCH, JOY_THROTTLE, JOY_YAW, JOY_FN_COUNT };

// `positive` is the way the operator is asked to push the stick when binding it,
// which is also the direction MANUAL_CONTROL defines as positive for that axis.
struct JoyFnInfo {
    const char* label;
    const char* positive;
};
const JoyFnInfo& joy_fn_info(JoyFn fn);

// Drift a stick may show at rest and still read as zero, as a fraction of travel.
constexpr float JOY_DEADZONE = 0.05f;

// How far from centre roll, pitch and yaw may be when sending starts. Anything
// further is a stick being held or a binding that is wrong, and either would
// reach the vehicle as a sudden command.
constexpr float JOY_ENGAGE_CENTER_MAX = 0.15f;

struct JoyAxisRange {
    uint16_t min = 0, trim = 0, max = 0;

    // Measured, ordered, and with enough travel to divide by. Also what stands
    // between a hand-edited settings file and a nonsense stick command.
    bool valid() const;
};

struct JoyFnBinding {
    int  axis     = -1;      // 0-based; -1 is unbound
    bool reversed = false;   // true when the raw value falls as the stick goes positive
};

// What a button does.
constexpr int JOY_MAX_BUTTONS = 32;

enum class JoyBtnAction : uint8_t { None, Mode, Aux, Servo };

// A button has one travel and a switch has positions, so this says how one acts
// as the other. Press is the default because it sends nothing on release:
// falling back to LOW when a thumb lifts is the disarm half of an ARM/DISARM
// function, and the aircraft may be in the air by then.
enum class JoyBtnTrigger : uint8_t {
    Press,    // `on` when pressed; release does nothing
    Hold,     // `on` when pressed, `off` when released
    Toggle,   // `on` and `off` on alternate presses, starting with `on`
};

struct JoyButtonBinding {
    JoyBtnAction  action  = JoyBtnAction::None;
    JoyBtnTrigger trigger = JoyBtnTrigger::Press;

    // Mode. The name is what counts: a profile follows the joystick from
    // vehicle to vehicle, and custom-mode numbers mean different things on each
    // frame. The numbers are only for a vehicle of the same stack and type that
    // does not publish a mode list carrying that name.
    std::string mode_name;
    uint32_t    mode_custom    = 0;
    uint8_t     mode_standard  = 0;
    uint8_t     mode_autopilot = 0;   // MAV_AUTOPILOT it was chosen on
    uint8_t     mode_type      = 0;   // MAV_TYPE it was chosen on

    uint16_t aux_fn = 0;      // RCn_OPTION value
    uint8_t  servo  = 1;      // 1-based output

    // Aux: switch position, 0 LOW, 1 MID, 2 HIGH. Servo: PWM. Unused for a mode.
    uint16_t on  = 0;
    uint16_t off = 0;

    // Whether it has a complete action to carry out.
    bool usable() const;
};

// RC_CHANNELS_OVERRIDE: which vehicle RC channel a joystick channel fills.
//
// The message has fields for 18 channels, but ArduPilot reads only the first 16,
// so a binding to 17 or 18 would look like it worked and do nothing.
constexpr int      JOY_RC_MAX_CHANNEL = 16;
constexpr uint16_t JOY_RC_IGNORE      = 0xFFFF;   // the message's "leave this channel alone"

struct JoyRcOut {
    int  channel  = 0;       // 1-based; 0 is unbound
    bool reversed = false;
};

// A multi-position switch, which a joystick reports as separate buttons: up is
// button 4, down is button 5, and the middle is neither. To the operator that is
// one control with one state, so it is modelled as one virtual channel.
//
// Each position is a captured pattern of buttons rather than a chosen button, so
// "none pressed" is an ordinary position, and a switch encoding four positions in
// two buttons needs nothing special. Buttons that differ between the patterns are
// the channel's members, and can have no action of their own.
//
// What reaches the vehicle is each position's action, and the virtual PWM too if
// the channel is bound to an RC channel for override.
constexpr int JOY_MAX_VIRTUAL        = 8;
constexpr int JOY_VIRTUAL_MIN_POS    = 2;
constexpr int JOY_VIRTUAL_MAX_POS    = 6;

// How long a position must hold before it counts. Going from up to down passes
// through the middle, and a switch between detents can read as anything: acting
// on that would fire the middle position's action, perhaps a mode change, on the
// way to somewhere else.
constexpr double JOY_VIRTUAL_SETTLE_S = 0.12;

struct JoyVirtualChannel {
    JoyRcOut         rc;
    int              positions = 0;   // 0 is an unused slot
    uint32_t         patterns[JOY_VIRTUAL_MAX_POS] = {};
    JoyButtonBinding actions[JOY_VIRTUAL_MAX_POS];

    bool     used()  const { return positions > 0; }
    uint32_t mask()  const;           // the member buttons
    bool     valid() const;           // a position count in range, patterns all different

    // Which position these buttons mean, or -1 for a pattern that is none of them.
    int position_of(uint32_t buttons) const;

    // Evenly spaced over 1000-2000: 1000, 1500, 2000 for three positions.
    uint16_t pwm(int position) const;
};

// One channel's position as it settles over time.
struct JoyVirtualTracker {
    int    stable    = -1;
    int    candidate = -1;
    double since     = 0.0;
};

// One device's calibration and bindings, kept in settings by device GUID.
//
// Nothing is bound by default. Axis numbering differs by device and by operating
// system, so a guessed default is a guess about which way an aircraft goes.
struct JoystickProfile {
    JoyAxisRange ranges[RcChannels::MAX_CHANNELS];
    JoyFnBinding bindings[JOY_FN_COUNT];
    JoyButtonBinding buttons[JOY_MAX_BUTTONS];
    JoyVirtualChannel virtuals[JOY_MAX_VIRTUAL];
    JoyRcOut     rc_axes[RcChannels::MAX_CHANNELS];

    // Send RC_CHANNELS_OVERRIDE from these bindings instead of MANUAL_CONTROL.
    // Never both: ArduPilot turns MANUAL_CONTROL into overrides of its own on the
    // stick channels, and the two would fight over them at the send rate.
    bool         rc_override = false;
    bool         calibrated = false;
};

// MANUAL_CONTROL's four axes: x pitch (+forward), y roll (+right), r yaw
// (+clockwise), each -1000..1000, and z throttle 0..1000, which is what both
// ArduPilot and PX4 read it as.
struct JoyCommand {
    int16_t x = 0, y = 0, z = 0, r = 0;
};

// Raw GLFW axes, -1..1, as a frame of 1000-2000 values.
RcChannels joy_axes_to_channels(const float* axes, int axis_count);

// Why this profile cannot drive a vehicle with this many axes, or nullptr if it
// can. Written to be shown to the operator.
const char* joy_profile_problem(const JoystickProfile& p, int axis_count);

// The command a frame implies. Only meaningful once joy_profile_problem() has
// returned nullptr.
JoyCommand joy_compute_command(const JoystickProfile& p, const RcChannels& frame);

// One button's state change. True when it calls for a command, with `value` the
// binding's on or off to send. `toggle_on` is that button's toggle position, kept
// by the caller between calls.
bool joy_button_step(const JoyButtonBinding& b, bool was_down, bool is_down,
                     bool& toggle_on, uint16_t& value);

// The buttons belonging to any virtual channel, one bit each.
uint32_t joy_virtual_member_mask(const JoystickProfile& p);

// The virtual channel a button belongs to, or -1.
int joy_virtual_of_button(const JoystickProfile& p, int button);

// Takes the current position as settled, with no change reported. For starting
// to send, where the switch's position is not a request to do anything.
void joy_virtual_sync(const JoyVirtualChannel& v, uint32_t buttons, JoyVirtualTracker& t);

// One frame's reading. True when a new position has just settled.
bool joy_virtual_step(const JoyVirtualChannel& v, uint32_t buttons, double now,
                      JoyVirtualTracker& t);

// The buttons carrying an action, directly or through a virtual channel, one bit
// each. These are left out of the bitmask MANUAL_CONTROL carries, so a vehicle
// that gives buttons a meaning of its own does not act on the same press twice.
uint32_t joy_bound_button_mask(const JoystickProfile& p);

// An aux function that can stop the motors or end the flight, for a warning where
// it is assigned. A stray press on a button is far easier than on a guarded switch.
bool joy_aux_is_critical(uint16_t aux_fn);

// ── RC override ──────────────────────────────────────────────────────────────

// An axis's calibrated value as a pulse, reaching a full 1000-2000 whatever part
// of its raw range the stick really uses. `linear` scales from stop to stop, for
// an axis with no centre. Otherwise the captured rest position is 1500, measured
// on each side separately so a stick not resting midway still sits at neutral.
uint16_t joy_axis_rc_value(const JoyAxisRange& r, uint16_t v, bool reversed, bool linear);

// No centre: bound as the throttle, or resting at one of its stops.
bool joy_axis_rc_linear(const JoystickProfile& p, int axis);

// Why these bindings cannot drive a vehicle, or nullptr if they can.
const char* joy_rc_problem(const JoystickProfile& p, int axis_count);

// All 18 message fields, JOY_RC_IGNORE where nothing is bound. `positions` is each
// virtual channel's settled position; a channel with none yet is left out.
void joy_rc_compute(const JoystickProfile& p, const RcChannels& frame,
                    const int positions[JOY_MAX_VIRTUAL],
                    uint16_t out[RcChannels::MAX_CHANNELS]);

// Every bound axis with a centre is near it. The override's counterpart of
// joy_sticks_centered(): RC channels mean whatever the vehicle's map says, so the
// check has no idea which is roll, only which axes have a position to return to.
bool joy_rc_at_rest(const JoystickProfile& p, const RcChannels& frame);

// Roll, pitch and yaw all within JOY_ENGAGE_CENTER_MAX of centre. Throttle is
// not in it: a sprung throttle rests in the middle and a slider rests anywhere.
bool joy_sticks_centered(const JoystickProfile& p, const RcChannels& frame);
