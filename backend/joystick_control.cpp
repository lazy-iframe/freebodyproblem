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


#include "joystick_control.hpp"
#include "rc_calibration.hpp"   // MIN_TRAVEL_US

#include <algorithm>
#include <cmath>

namespace {

float clamp1(float v) { return std::max(-1.0f, std::min(v, 1.0f)); }

// -1..1 around the captured centre, measured on each side separately: few sticks
// rest exactly halfway between their stops.
float stick_norm(const JoyAxisRange& r, uint16_t v, bool reversed)
{
    float n = 0.0f;
    if (v > r.trim && r.max > r.trim)
        n = (float)(v - r.trim) / (float)(r.max - r.trim);
    else if (v < r.trim && r.trim > r.min)
        n = -(float)(r.trim - v) / (float)(r.trim - r.min);
    n = clamp1(n);

    // Rescaled past the deadzone, so the output starts from zero at its edge
    // instead of stepping straight to 5%.
    const float a = std::fabs(n);
    n = (a <= JOY_DEADZONE) ? 0.0f
                            : std::copysign((a - JOY_DEADZONE) / (1.0f - JOY_DEADZONE), n);
    return reversed ? -n : n;
}

// 0..1 between the stops. The centre plays no part: a throttle's rest position
// says nothing about where zero is.
float throttle_norm(const JoyAxisRange& r, uint16_t v, bool reversed)
{
    float n = (float)((int)v - (int)r.min) / (float)(r.max - r.min);
    n = std::max(0.0f, std::min(n, 1.0f));
    return reversed ? 1.0f - n : n;
}

int16_t to_cmd(float n) { return (int16_t)std::lround(n * 1000.0f); }

float fn_stick(const JoystickProfile& p, const RcChannels& f, JoyFn fn)
{
    const JoyFnBinding& b = p.bindings[fn];
    return stick_norm(p.ranges[b.axis], f.chan[b.axis], b.reversed);
}

} // namespace

const JoyFnInfo& joy_fn_info(JoyFn fn)
{
    static const JoyFnInfo table[JOY_FN_COUNT] = {
        { "ROLL",     "right"   },
        { "PITCH",    "forward" },
        { "THROTTLE", "up"      },
        { "YAW",      "right"   },
    };
    return table[fn];
}

bool JoyAxisRange::valid() const
{
    return min <= trim && trim <= max && max - min >= RcCalibration::MIN_TRAVEL_US;
}

RcChannels joy_axes_to_channels(const float* axes, int axis_count)
{
    RcChannels rc;
    const int n = std::max(0, std::min(axis_count, (int)RcChannels::MAX_CHANNELS));
    for (int i = 0; i < n; ++i)
        rc.chan[i] = (uint16_t)std::lround(1500.0f + 500.0f * clamp1(axes[i]));
    rc.count = (uint8_t)n;
    rc.valid = n > 0;
    return rc;
}

const char* joy_profile_problem(const JoystickProfile& p, int axis_count)
{
    if (!p.calibrated) return "Calibrate the joystick first.";

    for (int f = 0; f < JOY_FN_COUNT; ++f) {
        const int axis = p.bindings[f].axis;
        if (axis < 0 || axis >= axis_count ||
            axis >= (int)RcChannels::MAX_CHANNELS)
            return "Bind roll, pitch, throttle and yaw to axes.";
        if (!p.ranges[axis].valid())
            return "A bound axis was not moved during calibration.";
        for (int g = 0; g < f; ++g)
            if (p.bindings[g].axis == axis)
                return "Two controls are bound to the same axis.";
    }
    return nullptr;
}

JoyCommand joy_compute_command(const JoystickProfile& p, const RcChannels& f)
{
    const JoyFnBinding& t = p.bindings[JOY_THROTTLE];

    JoyCommand c;
    c.x = to_cmd(fn_stick(p, f, JOY_PITCH));
    c.y = to_cmd(fn_stick(p, f, JOY_ROLL));
    c.z = to_cmd(throttle_norm(p.ranges[t.axis], f.chan[t.axis], t.reversed));
    c.r = to_cmd(fn_stick(p, f, JOY_YAW));
    return c;
}

bool JoyButtonBinding::usable() const
{
    switch (action) {
    case JoyBtnAction::None:  return false;
    case JoyBtnAction::Mode:  return !mode_name.empty();
    case JoyBtnAction::Aux:   return aux_fn != 0;
    case JoyBtnAction::Servo: return servo >= 1;
    }
    return false;
}

bool joy_button_step(const JoyButtonBinding& b, bool was_down, bool is_down,
                     bool& toggle_on, uint16_t& value)
{
    if (was_down == is_down || !b.usable()) return false;

    // A mode has no "off" to go back to, whatever the trigger says.
    if (b.action == JoyBtnAction::Mode || b.trigger == JoyBtnTrigger::Press) {
        value = b.on;
        return is_down;
    }
    if (b.trigger == JoyBtnTrigger::Hold) {
        value = is_down ? b.on : b.off;
        return true;
    }
    if (!is_down) return false;
    toggle_on = !toggle_on;
    value     = toggle_on ? b.on : b.off;
    return true;
}

uint32_t JoyVirtualChannel::mask() const
{
    uint32_t m = 0;
    for (int i = 1; i < positions; ++i) m |= patterns[i] ^ patterns[0];
    return m;
}

bool JoyVirtualChannel::valid() const
{
    if (positions < JOY_VIRTUAL_MIN_POS || positions > JOY_VIRTUAL_MAX_POS) return false;
    const uint32_t m = mask();
    for (int i = 0; i < positions; ++i)
        for (int k = 0; k < i; ++k)
            if ((patterns[i] & m) == (patterns[k] & m)) return false;
    return true;
}

int JoyVirtualChannel::position_of(uint32_t buttons) const
{
    const uint32_t m = mask();
    for (int i = 0; i < positions; ++i)
        if ((buttons & m) == (patterns[i] & m)) return i;
    return -1;
}

uint16_t JoyVirtualChannel::pwm(int position) const
{
    if (position < 0 || position >= positions || positions < 2) return 0;
    return (uint16_t)(1000 + (position * 1000 + (positions - 1) / 2) / (positions - 1));
}

uint32_t joy_virtual_member_mask(const JoystickProfile& p)
{
    uint32_t m = 0;
    for (const JoyVirtualChannel& v : p.virtuals)
        if (v.used()) m |= v.mask();
    return m;
}

int joy_virtual_of_button(const JoystickProfile& p, int button)
{
    if (button < 0 || button >= JOY_MAX_BUTTONS) return -1;
    for (int i = 0; i < JOY_MAX_VIRTUAL; ++i)
        if (p.virtuals[i].used() && (p.virtuals[i].mask() & (1u << button))) return i;
    return -1;
}

void joy_virtual_sync(const JoyVirtualChannel& v, uint32_t buttons, JoyVirtualTracker& t)
{
    t = JoyVirtualTracker{};
    t.stable = v.position_of(buttons);
}

bool joy_virtual_step(const JoyVirtualChannel& v, uint32_t buttons, double now,
                      JoyVirtualTracker& t)
{
    const int p = v.position_of(buttons);

    // A pattern that is no position says nothing about where the switch is going.
    if (p < 0 || p == t.stable) {
        t.candidate = -1;
        return false;
    }
    if (p != t.candidate) {
        t.candidate = p;
        t.since     = now;
        return false;
    }
    if (now - t.since < JOY_VIRTUAL_SETTLE_S) return false;

    t.stable    = p;
    t.candidate = -1;
    return true;
}

uint32_t joy_bound_button_mask(const JoystickProfile& p)
{
    uint32_t mask = joy_virtual_member_mask(p);
    for (int i = 0; i < JOY_MAX_BUTTONS; ++i)
        if (p.buttons[i].usable()) mask |= (1u << i);
    return mask;
}

bool joy_aux_is_critical(uint16_t aux_fn)
{
    switch (aux_fn) {
    case 22:    // Parachute Release
    case 31:    // Motor Emergency Stop
    case 32:    // Motor Interlock
    case 41:    // Arm/Disarm (4.1-)
    case 81:    // Disarm
    case 153:   // ARM/DISARM (4.2+)
    case 154:   // ARM/DISARM + AIRMODE
    case 165:   // ARM / Motor E-Stop
        return true;
    default:
        return false;
    }
}

uint16_t joy_axis_rc_value(const JoyAxisRange& r, uint16_t v, bool reversed, bool linear)
{
    float pulse;
    if (linear) {
        const float n = (float)((int)v - (int)r.min) / (float)((int)r.max - (int)r.min);
        pulse = 1000.0f + 1000.0f * std::max(0.0f, std::min(n, 1.0f));
    } else {
        float n = 0.0f;
        if (v > r.trim && r.max > r.trim)
            n = (float)(v - r.trim) / (float)(r.max - r.trim);
        else if (v < r.trim && r.trim > r.min)
            n = -(float)(r.trim - v) / (float)(r.trim - r.min);
        pulse = 1500.0f + 500.0f * clamp1(n);
    }
    if (reversed) pulse = 3000.0f - pulse;
    return (uint16_t)std::lround(pulse);
}

bool joy_axis_rc_linear(const JoystickProfile& p, int axis)
{
    if (p.bindings[JOY_THROTTLE].axis == axis) return true;
    const JoyAxisRange& r = p.ranges[axis];
    const int margin = ((int)r.max - (int)r.min) / 10;
    return (int)r.trim - (int)r.min <= margin || (int)r.max - (int)r.trim <= margin;
}

const char* joy_rc_problem(const JoystickProfile& p, int axis_count)
{
    bool     any  = false;
    uint32_t used = 0;

    const auto claim = [&](int channel) {
        const uint32_t bit = 1u << channel;
        const bool dup = (used & bit) != 0;
        used |= bit;
        any = true;
        return !dup;
    };

    for (int a = 0; a < RcChannels::MAX_CHANNELS; ++a) {
        const int ch = p.rc_axes[a].channel;
        if (ch == 0) continue;
        if (ch < 1 || ch > JOY_RC_MAX_CHANNEL) return "An RC channel is out of range.";
        if (a >= axis_count) return "A bound axis is not on this joystick.";
        if (!p.calibrated) return "Calibrate the joystick first.";
        if (!p.ranges[a].valid()) return "A bound axis was not moved during calibration.";
        if (!claim(ch)) return "Two channels are bound to the same RC channel.";
    }
    for (const JoyVirtualChannel& v : p.virtuals) {
        if (!v.used() || v.rc.channel == 0) continue;
        if (v.rc.channel < 1 || v.rc.channel > JOY_RC_MAX_CHANNEL)
            return "An RC channel is out of range.";
        if (!claim(v.rc.channel)) return "Two channels are bound to the same RC channel.";
    }
    return any ? nullptr : "Bind at least one channel to an RC channel.";
}

void joy_rc_compute(const JoystickProfile& p, const RcChannels& f,
                    const int positions[JOY_MAX_VIRTUAL],
                    uint16_t out[RcChannels::MAX_CHANNELS])
{
    for (int i = 0; i < RcChannels::MAX_CHANNELS; ++i) out[i] = JOY_RC_IGNORE;

    for (int a = 0; a < f.usable_count(); ++a) {
        const JoyRcOut& o = p.rc_axes[a];
        if (o.channel < 1 || o.channel > JOY_RC_MAX_CHANNEL || !p.ranges[a].valid()) continue;
        out[o.channel - 1] = joy_axis_rc_value(p.ranges[a], f.chan[a], o.reversed,
                                               joy_axis_rc_linear(p, a));
    }
    for (int i = 0; i < JOY_MAX_VIRTUAL; ++i) {
        const JoyVirtualChannel& v = p.virtuals[i];
        if (!v.used() || v.rc.channel < 1 || v.rc.channel > JOY_RC_MAX_CHANNEL) continue;
        const uint16_t pulse = v.pwm(positions[i]);
        if (pulse == 0) continue;
        out[v.rc.channel - 1] = v.rc.reversed ? (uint16_t)(3000 - pulse) : pulse;
    }
}

bool joy_rc_at_rest(const JoystickProfile& p, const RcChannels& f)
{
    for (int a = 0; a < f.usable_count(); ++a) {
        if (p.rc_axes[a].channel == 0 || !p.ranges[a].valid() || joy_axis_rc_linear(p, a))
            continue;
        const float n = ((float)joy_axis_rc_value(p.ranges[a], f.chan[a], false, false) - 1500.0f) / 500.0f;
        if (std::fabs(n) > JOY_ENGAGE_CENTER_MAX) return false;
    }
    return true;
}

bool joy_sticks_centered(const JoystickProfile& p, const RcChannels& f)
{
    for (JoyFn fn : { JOY_ROLL, JOY_PITCH, JOY_YAW })
        if (std::fabs(fn_stick(p, f, fn)) > JOY_ENGAGE_CENTER_MAX) return false;
    return true;
}
