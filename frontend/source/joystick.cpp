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


#include "frontend/source/joystick.hpp"

#include "frontend/source/app_log.hpp"
#include "frontend/source/audio.hpp"
#include "frontend/source/settings.hpp"
#include "frontend/source/layout.hpp"
#include "frontend/components/others/ui_kit.hpp"
#include "lib/text.hpp"
#include "backend/vehicle/firmware_profile.hpp"
#include "backend/input/rc_binding.hpp"   // rc_detect_moved_channel, rc_aux_option_label

#include "imgui.h"
#include "imgui_internal.h"            // BringWindowToDisplayFront
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <string>
#include <utility>

#ifdef __linux__
#  include <fcntl.h>
#  include <linux/input.h>
#  include <sys/ioctl.h>
#  include <unistd.h>
#  include <filesystem>
#endif

namespace {

#ifdef __linux__
// The buttons as the kernel has them right now.
//
// GLFW cannot say. On opening a device it reads the axes' positions but not the
// buttons, and after that learns of a button only from a change event. A latching
// switch left on therefore reads as off, and turning it off changes nothing on
// screen: the operator's first movement appears to go missing, and every position
// shown until then is wrong. Asking the kernel each frame has neither problem, and
// EVIOCGKEY takes nothing from the event stream GLFW is reading.
//
// Windows and macOS poll the device's current state, so they have no such gap.
class KernelButtons {
public:
    ~KernelButtons() { close_device(); }

    void close_device()
    {
        if (fd_ >= 0) ::close(fd_);
        fd_ = -1;
        codes_.clear();
    }

    // GLFW does not say which device node it opened, so it is found by the GUID
    // GLFW made from it. Two identical sticks cannot be told apart that way, and
    // reading the wrong one would show another stick's buttons: leave both to GLFW.
    bool open_for(const char* guid)
    {
        close_device();
        if (!guid) return false;

        std::error_code ec;
        int found = -1, matches = 0;
        for (const auto& entry : std::filesystem::directory_iterator("/dev/input", ec)) {
            if (entry.path().filename().string().rfind("event", 0) != 0) continue;
            const int fd = ::open(entry.path().c_str(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
            if (fd < 0) continue;
            if (guid_of(fd) == guid) {
                ++matches;
                if (found >= 0) ::close(found);
                found = fd;
            } else {
                ::close(fd);
            }
        }
        if (matches != 1) {
            if (found >= 0) ::close(found);
            return false;
        }

        // The same walk GLFW makes to number its buttons, so bit n is button n+1.
        unsigned char bits[(KEY_CNT + 7) / 8] = {};
        if (ioctl(found, EVIOCGBIT(EV_KEY, sizeof(bits)), bits) < 0) {
            ::close(found);
            return false;
        }
        for (int code = BTN_MISC; code < KEY_CNT; ++code)
            if (bits[code / 8] & (1u << (code % 8))) codes_.push_back(code);
        fd_ = found;
        return true;
    }

    // False if the device has gone, which leaves the caller with GLFW's reading.
    bool read(uint32_t& buttons, int button_count)
    {
        if (fd_ < 0) return false;
        unsigned char state[(KEY_CNT + 7) / 8] = {};
        if (ioctl(fd_, EVIOCGKEY(sizeof(state)), state) < 0) {
            close_device();
            return false;
        }
        // Past these come the hats GLFW reports as buttons, which are not keys.
        const int n = std::min({ (int)codes_.size(), button_count, JOY_MAX_BUTTONS });
        for (int i = 0; i < n; ++i) {
            const uint32_t bit = 1u << i;
            if (state[codes_[i] / 8] & (1u << (codes_[i] % 8))) buttons |= bit;
            else                                                buttons &= ~bit;
        }
        return true;
    }

private:
    // As GLFW builds it in linux_joystick.c.
    static std::string guid_of(int fd)
    {
        input_id id{};
        char name[256] = "";
        if (ioctl(fd, EVIOCGID, &id) < 0) return {};
        if (ioctl(fd, EVIOCGNAME(sizeof(name)), name) < 0) return {};
        char guid[33] = "";
        if (id.vendor && id.product && id.version)
            snprintf(guid, sizeof(guid), "%02x%02x0000%02x%02x0000%02x%02x0000%02x%02x0000",
                     id.bustype & 0xff, id.bustype >> 8, id.vendor & 0xff, id.vendor >> 8,
                     id.product & 0xff, id.product >> 8, id.version & 0xff, id.version >> 8);
        else
            snprintf(guid, sizeof(guid), "%02x%02x0000%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x00",
                     id.bustype & 0xff, id.bustype >> 8,
                     name[0], name[1], name[2], name[3], name[4], name[5],
                     name[6], name[7], name[8], name[9], name[10]);
        return guid;
    }

    int              fd_ = -1;
    std::vector<int> codes_;
};
#endif

// 25 Hz: well inside the half second PX4 allows by default between frames, and
// about 600 bytes a second, which a 57600 telemetry radio can spare.
constexpr double SEND_INTERVAL_S = 0.04;

// A frame loop stalled this long means the operator has been flying blind and
// the vehicle has been holding an old command. Carrying on as if nothing had
// happened would be a surprise to both of them.
constexpr double STALL_LIMIT_S = 2.0;

struct State {
    AppSettings*    settings = nullptr;

    int             jid = -1;
    std::string     guid;
    JoystickProfile profile;
    RcCalibration   cal;
#ifdef __linux__
    KernelButtons   kernel_buttons;
#endif

    RcChannels      frame;
    uint32_t        buttons      = 0;
    int             button_count = 0;

    int             detect_fn = -1;
    RcChannels      detect_baseline;

    int             button_selected = 0;
    bool            button_detect   = false;
    uint32_t        buttons_prev    = 0;     // last frame, for the detector

    JoyVirtualTracker vtrack[JOY_MAX_VIRTUAL];
    int             vcap_total = 0;          // 0 when no capture is running
    int             vcap_step  = 0;
    uint32_t        vcap_patterns[JOY_VIRTUAL_MAX_POS] = {};
    const char*     vcap_problem = nullptr;

    MavlinkSender::ManualControlFrame out_frame;
    MavlinkSender::RcOverrideFrame    out_rc_frame;
    double          out_hz         = 0.0;
    double          out_rate_since = 0.0;
    uint32_t        out_rate_count = 0;
    uint32_t        out_base_count = 0;      // the sender's count when sending began
    bool            out_primed     = false;
    std::deque<JoyOutputCommand> out_commands;

    uint32_t        sent_buttons = 0;        // as last acted on while sending
    bool            toggles[JOY_MAX_BUTTONS] = {};

    // Held buttons whose release was still owed when sending stopped. Copies,
    // because an unplugged joystick takes its profile with it.
    struct Owed { std::string source; JoyButtonBinding binding; uint16_t value; };
    std::vector<Owed> owed;

    // Last frame's fleet, as ids. The panel starts sending partway through a
    // frame, and needs to know its target exists and what the map calls it.
    std::vector<std::pair<VehicleId, uint32_t>> known;

    bool            live = false;
    VehicleId       target;
    uint32_t        target_number = 0;
    double          last_send     = 0.0;
    double          last_pump     = 0.0;
} s;

void stop_sending(const char* why, bool fault)
{
    if (!s.live) return;
    s.live = false;

    // A HOLD still down would otherwise never see its release, and the servo or
    // aux function would stay wherever the press left it.
    //
    // Not for a function that can stop the motors. Nobody moved anything: the
    // operator pressed STOP, or a cable came out, with a latching switch still on.
    // On ARM/DISARM the release is a disarm, and the aircraft may be in the air.
    for (int i = 0; i < JOY_MAX_BUTTONS; ++i) {
        const JoyButtonBinding& b = s.profile.buttons[i];
        if ((s.sent_buttons & (1u << i)) && b.usable() &&
            b.action != JoyBtnAction::Mode && b.trigger == JoyBtnTrigger::Hold &&
            !(b.action == JoyBtnAction::Aux && joy_aux_is_critical(b.aux_fn)))
            s.owed.push_back({ "button " + std::to_string(i + 1), b, b.off });
    }

    gcs_log("joystick: stopped sending to vehicle (%u) \xe2\x80\x94 %s",
            (unsigned)s.target_number, why);
    if (fault) gcs_tone(GcsTone::Failure);
}

void adopt_device(int jid)
{
    s.jid = jid;
    const char* guid = jid >= 0 ? glfwGetJoystickGUID(jid) : nullptr;
    s.guid = guid ? guid : "unknown";

#ifdef __linux__
    s.kernel_buttons.close_device();
    if (jid >= 0 && !s.kernel_buttons.open_for(guid))
        gcs_log("joystick: cannot read its button state directly; a switch already "
                "on will show as off until it is moved");
#endif

    s.profile = JoystickProfile{};
    if (jid >= 0 && s.settings) {
        auto it = s.settings->joystick_profiles.find(s.guid);
        if (it != s.settings->joystick_profiles.end()) s.profile = it->second;
    }
    s.cal.cancel();
    s.detect_fn     = -1;
    s.button_detect = false;
    s.frame         = RcChannels{};
    s.buttons       = 0;
    s.buttons_prev  = 0;
    s.vcap_total    = 0;
    s.vcap_problem  = nullptr;
    for (JoyVirtualTracker& t : s.vtrack) t = JoyVirtualTracker{};
}

void commit_profile()
{
    if (s.jid < 0 || !s.settings) return;
    s.settings->joystick_profiles[s.guid] = s.profile;
    settings_save(*s.settings);
}

// Follows the plug and the unplug. The connected device is left alone while it
// stays connected, however many others come and go.
void track_device()
{
    if (s.jid >= 0 && !glfwJoystickPresent(s.jid)) {
        gcs_log("joystick disconnected");
        stop_sending("joystick disconnected", true);
        adopt_device(-1);
    }
    if (s.jid < 0) {
        const std::vector<int> found = joystick_devices();
        if (!found.empty()) {
            adopt_device(found.front());
            gcs_log("joystick connected: %s", joystick_device_name(s.jid));
        }
    }
}

void read_device()
{
    if (s.jid < 0) return;

    int axis_count = 0;
    const float* axes = glfwGetJoystickAxes(s.jid, &axis_count);
    s.frame = joy_axes_to_channels(axes, axes ? axis_count : 0);

    int button_count = 0;
    const unsigned char* buttons = glfwGetJoystickButtons(s.jid, &button_count);
    s.button_count = buttons ? std::min(button_count, 32) : 0;
    s.buttons = 0;
    for (int i = 0; i < s.button_count; ++i)
        if (buttons[i] == GLFW_PRESS) s.buttons |= (1u << i);
#ifdef __linux__
    s.kernel_buttons.read(s.buttons, s.button_count);
#endif
}

void run_detection()
{
    if (s.detect_fn < 0) return;

    const int moved = rc_detect_moved_channel(s.detect_baseline, s.frame);
    if (moved <= 0) return;

    const JoyFn fn = (JoyFn)s.detect_fn;
    s.detect_fn = -1;
    const bool reversed = s.frame.at(moved) < s.detect_baseline.at(moved);
    joystick_set_binding(fn, moved - 1, reversed);
    gcs_log("joystick: %s bound to axis %d%s", joy_fn_info(fn).label, moved,
            reversed ? " (reversed)" : "");
    gcs_tone(GcsTone::Success);
}

// The vehicle's own number for a named mode, or false if there is no telling.
// Sending a number anyway is how a button labelled Loiter lands a Plane in
// something else entirely.
bool resolve_mode(const JoyButtonBinding& b, const VehicleState& vs,
                  uint32_t& custom, uint8_t& standard)
{
    standard = 0;
    if (!vs.available_modes.empty()) {
        for (const FlightModeInfo& m : vs.available_modes)
            if (m.user_selectable() && text_iequals(b.mode_name, m.name)) {
                custom   = m.custom_mode;
                standard = m.standard_mode;
                return true;
            }
    } else {
        for (const FallbackMode& m : firmware_profile(vs).fallback_modes(vs.type))
            if (text_iequals(b.mode_name, m.label)) {
                custom = m.custom_mode;
                return true;
            }
    }
    if (b.mode_autopilot == (uint8_t)vs.autopilot && b.mode_type == (uint8_t)vs.type) {
        custom   = b.mode_custom;
        standard = b.mode_standard;
        return true;
    }
    return false;
}

void note_command(const std::string& source, const std::string& text)
{
    constexpr size_t KEEP = 6;
    s.out_commands.push_front({ glfwGetTime(), source, text });
    if (s.out_commands.size() > KEEP) s.out_commands.pop_back();
}

std::string format_param(float v)
{
    char buf[24];
    snprintf(buf, sizeof(buf), "%g", (double)v);
    return buf;
}

void fire_button(Vehicle& v, const std::string& source, const JoyButtonBinding& b,
                 uint16_t value)
{
    switch (b.action) {
    case JoyBtnAction::None:
        break;

    case JoyBtnAction::Mode: {
        VehicleSnapshot snap;
        v.snapshot(snap);
        uint32_t custom = 0;
        uint8_t  standard = 0;
        if (!resolve_mode(b, snap.state, custom, standard)) {
            gcs_log("joystick: %s \xe2\x80\x94 this vehicle has no mode %s",
                    source.c_str(), b.mode_name.c_str());
            gcs_tone(GcsTone::Failure);
            break;
        }
        const FirmwareProfile& prof = firmware_profile(snap.state);
        send_set_mode(v.sender(), v.sysid(), v.compid(), prof, custom, standard);
        gcs_log("joystick: %s \xe2\x86\x92 mode %s", source.c_str(), b.mode_name.c_str());

        // The same encoding send_set_mode just used, so this is what went out.
        const SetModeCommand c = prof.encode_set_mode(custom, standard);
        note_command(source, std::string(c.command == 176 ? "DO_SET_MODE (176)"
                                                          : "DO_SET_STANDARD_MODE (262)") +
                     " p1=" + format_param(c.param1) + " p2=" + format_param(c.param2) +
                     " p3=" + format_param(c.param3) + "  " + b.mode_name);
        break;
    }

    case JoyBtnAction::Aux: {
        VehicleSnapshot snap;
        v.snapshot(snap);
        const Capability cap = firmware_profile(snap.state).aux_functions();
        if (!cap.supported) {
            gcs_log("joystick: %s \xe2\x80\x94 %s", source.c_str(),
                    cap.reason ? cap.reason : "no aux functions on this vehicle");
            gcs_tone(GcsTone::Failure);
            break;
        }
        v.sender().do_aux_function(v.sysid(), v.compid(), b.aux_fn, (uint8_t)value);
        const char* name = rc_aux_option_label(b.aux_fn);
        gcs_log("joystick: %s \xe2\x86\x92 aux %s %s", source.c_str(),
                name ? name : std::to_string(b.aux_fn).c_str(),
                value == 2 ? "HIGH" : value == 1 ? "MID" : "LOW");
        note_command(source, "DO_AUX_FUNCTION (218) p1=" + std::to_string(b.aux_fn) +
                     " p2=" + std::to_string(value) + "  " + (name ? name : ""));
        break;
    }

    case JoyBtnAction::Servo:
        v.sender().do_set_servo(v.sysid(), v.compid(), b.servo, value);
        gcs_log("joystick: %s \xe2\x86\x92 servo %u %u \xc2\xb5s", source.c_str(),
                (unsigned)b.servo, (unsigned)value);
        note_command(source, "DO_SET_SERVO (183) p1=" + std::to_string(b.servo) +
                     " p2=" + std::to_string(value));
        break;
    }
}

// What stands in the way of the profile's own kind of output.
const char* output_problem()
{
    return s.profile.rc_override ? joy_rc_problem(s.profile, s.frame.usable_count())
                                 : joy_profile_problem(s.profile, s.frame.usable_count());
}

void current_rc_values(uint16_t out[RcChannels::MAX_CHANNELS])
{
    int positions[JOY_MAX_VIRTUAL];
    for (int i = 0; i < JOY_MAX_VIRTUAL; ++i) positions[i] = s.vtrack[i].stable;
    joy_rc_compute(s.profile, s.frame, positions, out);
}

void run_buttons(Vehicle& v, const bool settled[JOY_MAX_VIRTUAL])
{
    const uint32_t members = joy_virtual_member_mask(s.profile);
    for (int i = 0; i < JOY_MAX_BUTTONS; ++i) {
        const uint32_t bit = 1u << i;
        if (members & bit) continue;
        uint16_t value = 0;
        if (joy_button_step(s.profile.buttons[i], (s.sent_buttons & bit) != 0,
                            (s.buttons & bit) != 0, s.toggles[i], value))
            fire_button(v, "button " + std::to_string(i + 1), s.profile.buttons[i], value);
    }
    s.sent_buttons = s.buttons;

    for (int i = 0; i < JOY_MAX_VIRTUAL; ++i) {
        if (!settled[i]) continue;
        const int pos = s.vtrack[i].stable;
        const JoyButtonBinding& b = s.profile.virtuals[i].actions[pos];
        if (b.usable())
            fire_button(v, "V" + std::to_string(i + 1) + " position " + std::to_string(pos + 1),
                        b, b.on);
    }
}

void pay_owed_releases(const std::vector<std::shared_ptr<Vehicle>>& vehicles)
{
    if (s.owed.empty()) return;
    for (const auto& v : vehicles)
        if (v->id() == s.target && !v->stale())
            for (const auto& o : s.owed)
                fire_button(*v, o.source, o.binding, o.value);
    s.owed.clear();
}

void run_button_detection()
{
    const uint32_t rising = s.buttons & ~s.buttons_prev;
    s.buttons_prev = s.buttons;
    if (!s.button_detect || rising == 0) return;

    for (int i = 0; i < JOY_MAX_BUTTONS; ++i)
        if (rising & (1u << i)) { s.button_selected = i; break; }
    s.button_detect = false;
}

void run_sending(const std::vector<std::shared_ptr<Vehicle>>& vehicles, double now,
                 const bool settled[JOY_MAX_VIRTUAL])
{
    if (!s.live) return;

    std::shared_ptr<Vehicle> v;
    for (const auto& c : vehicles)
        if (c->id() == s.target) { v = c; break; }

    if (!v)         { stop_sending("vehicle gone", true);           return; }
    if (v->stale()) { stop_sending("vehicle not responding", true); return; }

    // Rechecked on every frame only because it costs nothing: the profile cannot
    // change while sending, but the device can report fewer axes after a glitch.
    if (output_problem()) {
        stop_sending("joystick lost its axes", true);
        return;
    }

    // Every frame rather than at the send rate: a quick press must not fall
    // between two stick frames.
    run_buttons(*v, settled);

    s.out_frame    = v->sender().manual_control_sent();
    s.out_rc_frame = v->sender().rc_override_sent();
    const uint32_t out_count = s.profile.rc_override ? s.out_rc_frame.count : s.out_frame.count;
    if (!s.out_primed) {
        // The sender's count runs from the vehicle's first frame, not this run's.
        s.out_primed     = true;
        s.out_base_count = s.out_rate_count = out_count;
        s.out_rate_since = now;
    }
    if (now - s.out_rate_since >= 1.0) {
        // Counted at the sender, so this is the rate the link managed.
        s.out_hz = (out_count - s.out_rate_count) / (now - s.out_rate_since);
        s.out_rate_since = now;
        s.out_rate_count = out_count;
    }

    if (now - s.last_send < SEND_INTERVAL_S) return;
    s.last_send = std::max(s.last_send + SEND_INTERVAL_S, now - SEND_INTERVAL_S);

    if (s.profile.rc_override) {
        uint16_t chan[RcChannels::MAX_CHANNELS];
        current_rc_values(chan);
        v->sender().send_rc_override(v->sysid(), v->compid(), chan);
        return;
    }

    const JoyCommand c = joy_compute_command(s.profile, s.frame);
    v->sender().send_manual_control(v->sysid(), c.x, c.y, c.z, c.r,
                                    s.buttons & ~joy_bound_button_mask(s.profile));
}

} // namespace

void joystick_init(AppSettings* settings) { s.settings = settings; }

void joystick_pump(const std::vector<std::shared_ptr<Vehicle>>& vehicles)
{
    s.known.clear();
    for (const auto& v : vehicles) s.known.emplace_back(v->id(), v->number());

    const double now = glfwGetTime();
    if (s.live && s.last_pump > 0.0 && now - s.last_pump > STALL_LIMIT_S)
        stop_sending("the display stalled", true);
    s.last_pump = now;

    track_device();
    read_device();

    s.cal.update(s.frame);
    run_detection();
    run_button_detection();

    // Followed all the time, for the display. Only sending acts on a change.
    bool settled[JOY_MAX_VIRTUAL] = {};
    for (int i = 0; i < JOY_MAX_VIRTUAL; ++i)
        if (s.profile.virtuals[i].used())
            settled[i] = joy_virtual_step(s.profile.virtuals[i], s.buttons, now, s.vtrack[i]);

    run_sending(vehicles, now, settled);
    pay_owed_releases(vehicles);
}

std::vector<int> joystick_devices()
{
    std::vector<int> out;
    for (int jid = GLFW_JOYSTICK_1; jid <= GLFW_JOYSTICK_LAST; ++jid)
        if (glfwJoystickPresent(jid)) out.push_back(jid);
    return out;
}

const char* joystick_device_name(int jid)
{
    const char* name = jid >= 0 ? glfwGetJoystickName(jid) : nullptr;
    return name ? name : "Joystick";
}

void joystick_select(int jid)
{
    if (jid == s.jid || !glfwJoystickPresent(jid)) return;
    stop_sending("a different joystick was selected", false);
    adopt_device(jid);
}

int  joystick_selected()     { return s.jid; }
bool joystick_connected()    { return s.jid >= 0; }
int  joystick_button_count() { return s.button_count; }
uint32_t          joystick_buttons() { return s.buttons; }
const RcChannels& joystick_frame()   { return s.frame; }

const JoystickProfile& joystick_profile()     { return s.profile; }
RcCalibration&         joystick_calibration() { return s.cal; }

void joystick_save_calibration()
{
    if (s.cal.phase() != RcCalibration::Phase::Review) return;

    int saved = 0;
    for (int i = 0; i < RcChannels::MAX_CHANNELS; ++i) {
        s.profile.ranges[i] = JoyAxisRange{};
        if (!s.cal.channel_usable(i)) continue;
        const RcChannelCal& c = s.cal.channel(i);
        s.profile.ranges[i] = { c.min, c.trim, c.max };
        ++saved;
    }
    s.profile.calibrated = true;
    s.cal.cancel();
    commit_profile();
    gcs_log("joystick calibration saved: %d axes", saved);
    gcs_tone(GcsTone::Success);
}

void joystick_set_binding(JoyFn fn, int axis, bool reversed)
{
    if (s.live) return;
    s.profile.bindings[fn] = { axis, reversed };
    commit_profile();
}

void joystick_set_button(int button, const JoyButtonBinding& binding)
{
    if (s.live || button < 0 || button >= JOY_MAX_BUTTONS) return;
    if (joy_virtual_of_button(s.profile, button) >= 0) return;
    s.profile.buttons[button] = binding;
    commit_profile();
}

int  joystick_button_selected()   { return s.button_selected; }
void joystick_button_select(int b)
{
    if (b >= 0 && b < JOY_MAX_BUTTONS) s.button_selected = b;
}
void joystick_button_detect(bool on) { s.button_detect = on && !s.live; }
bool joystick_button_detecting()     { return s.button_detect; }

void joystick_set_rc_override(bool on)
{
    if (s.live) return;
    s.profile.rc_override = on;
    commit_profile();
}

void joystick_set_axis_rc(int axis, const JoyRcOut& out)
{
    if (s.live || axis < 0 || axis >= RcChannels::MAX_CHANNELS) return;
    s.profile.rc_axes[axis] = out;
    commit_profile();
}

void joystick_set_virtual_rc(int index, const JoyRcOut& out)
{
    if (s.live || index < 0 || index >= JOY_MAX_VIRTUAL) return;
    s.profile.virtuals[index].rc = out;
    commit_profile();
}

void joystick_rc_values(uint16_t out[RcChannels::MAX_CHANNELS]) { current_rc_values(out); }

void joystick_virtual_begin(int positions)
{
    if (s.live || s.jid < 0) return;
    s.vcap_total   = std::max(JOY_VIRTUAL_MIN_POS, std::min(positions, JOY_VIRTUAL_MAX_POS));
    s.vcap_step    = 0;
    s.vcap_problem = nullptr;
}

void joystick_virtual_cancel()
{
    s.vcap_total   = 0;
    s.vcap_problem = nullptr;
}

void joystick_virtual_capture()
{
    if (s.vcap_total == 0) return;

    for (int i = 0; i < s.vcap_step; ++i)
        if (s.vcap_patterns[i] == s.buttons) {
            s.vcap_problem = "Same buttons as an earlier position. Move the switch. "
                             "A switch that only pulses a button when moved cannot "
                             "be read this way.";
            gcs_tone(GcsTone::Failure);
            return;
        }
    s.vcap_problem = nullptr;
    s.vcap_patterns[s.vcap_step++] = s.buttons;
    if (s.vcap_step < s.vcap_total) return;

    JoyVirtualChannel v;
    v.positions = s.vcap_total;
    for (int i = 0; i < s.vcap_total; ++i) v.patterns[i] = s.vcap_patterns[i];
    // Only what the switch changes. A button that happened to be held throughout
    // is not part of it.
    const uint32_t m = v.mask();
    for (int i = 0; i < s.vcap_total; ++i) v.patterns[i] &= m;
    s.vcap_total = 0;

    if (joy_virtual_member_mask(s.profile) & m) {
        s.vcap_problem = "Those buttons already belong to a virtual channel.";
        gcs_tone(GcsTone::Failure);
        return;
    }
    int slot = -1;
    for (int i = JOY_MAX_VIRTUAL - 1; i >= 0; --i)
        if (!s.profile.virtuals[i].used()) slot = i;
    if (slot < 0 || !v.valid()) {
        s.vcap_problem = "No room for another virtual channel.";
        gcs_tone(GcsTone::Failure);
        return;
    }

    for (int i = 0; i < JOY_MAX_BUTTONS; ++i)
        if (m & (1u << i)) s.profile.buttons[i] = JoyButtonBinding{};
    s.profile.virtuals[slot] = v;
    joy_virtual_sync(v, s.buttons, s.vtrack[slot]);
    commit_profile();
    gcs_log("joystick: V%d created with %d positions", slot + 1, v.positions);
    gcs_tone(GcsTone::Success);
}

bool joystick_virtual_capturing()      { return s.vcap_total > 0; }
int  joystick_virtual_capture_step()   { return s.vcap_step; }
int  joystick_virtual_capture_total()  { return s.vcap_total; }
const char* joystick_virtual_capture_problem() { return s.vcap_problem; }

void joystick_virtual_remove(int index)
{
    if (s.live || index < 0 || index >= JOY_MAX_VIRTUAL) return;
    s.profile.virtuals[index] = JoyVirtualChannel{};
    s.vtrack[index]           = JoyVirtualTracker{};
    commit_profile();
}

void joystick_virtual_set_action(int index, int position, const JoyButtonBinding& action)
{
    if (s.live || index < 0 || index >= JOY_MAX_VIRTUAL) return;
    JoyVirtualChannel& v = s.profile.virtuals[index];
    if (position < 0 || position >= v.positions) return;
    v.actions[position] = action;
    v.actions[position].trigger = JoyBtnTrigger::Press;
    commit_profile();
}

int joystick_virtual_position(int index)
{
    if (index < 0 || index >= JOY_MAX_VIRTUAL) return -1;
    const JoyVirtualChannel& v = s.profile.virtuals[index];
    const int now = v.used() ? v.position_of(s.buttons) : -1;
    return now >= 0 ? now : s.vtrack[index].stable;
}

void joystick_detect_start(JoyFn fn)
{
    if (s.live || !s.frame.valid) return;
    s.detect_baseline = s.frame;
    s.detect_fn       = fn;
}

void joystick_detect_cancel() { s.detect_fn = -1; }
int  joystick_detect_target() { return s.detect_fn; }

const char* joystick_engage_problem()
{
    if (s.jid < 0) return "No joystick connected.";
    if (s.cal.phase() != RcCalibration::Phase::Idle)
        return "Finish the joystick calibration first.";
    if (s.vcap_total > 0)
        return "Finish the virtual channel first.";
    if (const char* p = output_problem()) return p;
    if (s.profile.rc_override) {
        if (!joy_rc_at_rest(s.profile, s.frame))
            return "Return the bound sticks to centre to start.";
        return nullptr;
    }
    if (!joy_sticks_centered(s.profile, s.frame))
        return "Centre roll, pitch and yaw to start.";
    return nullptr;
}

bool joystick_engage(VehicleId target)
{
    if (s.live) return false;
    if (const char* why = joystick_engage_problem()) {
        gcs_log("joystick: not sending \xe2\x80\x94 %s", why);
        gcs_tone(GcsTone::Failure);
        return false;
    }
    const auto it = std::find_if(s.known.begin(), s.known.end(),
                                 [&](const auto& k) { return k.first == target; });
    if (it == s.known.end()) {
        gcs_log("joystick: not sending \xe2\x80\x94 that vehicle has gone");
        gcs_tone(GcsTone::Failure);
        return false;
    }
    s.detect_fn     = -1;
    s.button_detect = false;
    // Starts from what is down now, so a button already held does not fire.
    s.sent_buttons  = s.buttons;
    for (bool& t : s.toggles) t = false;
    // Likewise a switch: where it sits now is not a request to go there.
    for (int i = 0; i < JOY_MAX_VIRTUAL; ++i)
        if (s.profile.virtuals[i].used())
            joy_virtual_sync(s.profile.virtuals[i], s.buttons, s.vtrack[i]);
    s.out_frame      = MavlinkSender::ManualControlFrame{};
    s.out_rc_frame   = MavlinkSender::RcOverrideFrame{};
    s.out_hz         = 0.0;
    s.out_primed     = false;
    s.target        = target;
    s.target_number = it->second;
    s.last_send     = glfwGetTime() - SEND_INTERVAL_S;
    s.live          = true;
    gcs_log("joystick: sending %s to vehicle (%u)",
            s.profile.rc_override ? "RC_CHANNELS_OVERRIDE" : "MANUAL_CONTROL",
            (unsigned)s.target_number);
    gcs_tone(GcsTone::Success);
    return true;
}

void joystick_disengage() { stop_sending("stopped by the operator", false); }

bool      joystick_live()        { return s.live; }
VehicleId joystick_live_target() { return s.live ? s.target : VehicleId{}; }
uint32_t  joystick_live_number() { return s.target_number; }

JoyCommand joystick_command()
{
    if (joy_profile_problem(s.profile, s.frame.usable_count())) return JoyCommand{};
    return joy_compute_command(s.profile, s.frame);
}

MavlinkSender::ManualControlFrame joystick_output_frame() { return s.out_frame; }
MavlinkSender::RcOverrideFrame joystick_output_rc_frame() { return s.out_rc_frame; }
bool     joystick_output_has_frame()
{
    return s.out_primed &&
           (s.profile.rc_override ? s.out_rc_frame.count : s.out_frame.count) != s.out_base_count;
}
double   joystick_output_hz()      { return s.out_hz; }
uint32_t joystick_output_buttons() { return s.buttons & ~joy_bound_button_mask(s.profile); }
const std::deque<JoyOutputCommand>& joystick_output_commands() { return s.out_commands; }
double   joystick_now()            { return glfwGetTime(); }

void draw_joystick_live_banner()
{
    if (!s.live) return;

    const ImGuiIO& io = ImGui::GetIO();
    const ImVec2   size { 340.0f, 34.0f };
    // Below the centre view's VIDEO / MAP tabs, which sit just under the top bar.
    ImGui::SetNextWindowPos({ (io.DisplaySize.x - size.x) * 0.5f, TOPBAR_H + 30.0f });
    ImGui::SetNextWindowSize(size);

    const ImVec4 warn = g_theme.col_warning;
    ImGui::PushStyleColor(ImGuiCol_WindowBg, g_theme.bg_topbar);
    ImGui::PushStyleColor(ImGuiCol_Border,   warn);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,    { 8.0f, 5.0f });

    constexpr ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav;
    if (ImGui::Begin("##joystick_live", nullptr, flags)) {
        // Every frame: clicking the map or a sidebar raises that window, and
        // this must not end up underneath it.
        ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());

        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(warn, "JOYSTICK LIVE \xe2\x86\x92 VEHICLE (%u)",
                           (unsigned)s.target_number);
        const float btn_w = 76.0f;
        ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - btn_w);
        if (ui_solid_button("STOP##joystick_live", { btn_w, 0.0f },
                            g_theme.btn_stop_base, btn_stop_hov()))
            joystick_disengage();
    }
    ImGui::End();

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
}
