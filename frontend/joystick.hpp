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
#include <deque>
#include <memory>
#include <string>
#include <vector>

#include "../backend/joystick_control.hpp"
#include "../backend/rc_calibration.hpp"
#include "../backend/vehicle.hpp"

struct AppSettings;

// ─────────────────────────────────────────────────────────────────────────────
// The joystick: reading the device, running its calibration, and streaming
// MANUAL_CONTROL while the operator has asked for that.
//
// One joystick for the whole program, not one per vehicle, so this state lives
// here and not in a VehicleUiState. UI thread only: GLFW allows its joystick
// calls nowhere else.
//
// Sending is an explicit act and starts off every time. It is tied to the one
// vehicle it was started on, so switching the callsign chip to look at another
// aircraft never moves the sticks over to it. It stops by itself when the device
// is unplugged, the vehicle goes, or the frame loop stalls, and it does not
// resume by itself after any of those.
// ─────────────────────────────────────────────────────────────────────────────

void joystick_init(AppSettings* settings);

// Once per frame, whichever tab is showing.
void joystick_pump(const std::vector<std::shared_ptr<Vehicle>>& vehicles);

// What is plugged in. Zero or one of these is selected; the panel lists them
// only when there is a choice to make.
std::vector<int> joystick_devices();
const char*      joystick_device_name(int jid);
void             joystick_select(int jid);
int              joystick_selected();     // -1 when nothing is connected

bool              joystick_connected();
const RcChannels& joystick_frame();       // axes as 1000-2000 values
uint32_t          joystick_buttons();     // bit 0 is button 1
int               joystick_button_count();

// ── Calibration and binding ──────────────────────────────────────────────────

const JoystickProfile& joystick_profile();

// Fed by the pump, so a sweep keeps recording while another tab is showing.
RcCalibration& joystick_calibration();

// Copies a finished calibration into the profile and saves it, then ends the run.
void joystick_save_calibration();

void joystick_set_binding(JoyFn fn, int axis, bool reversed);

// "Push the stick". The first axis to move is bound to `fn`, and the way it moved
// decides whether it is reversed — unlike a radio's sweep, this can tell, because
// the operator was asked to push in a known direction.
void joystick_detect_start(JoyFn fn);
void joystick_detect_cancel();
int  joystick_detect_target();            // -1 when not listening

// Buttons act only while sending, on the vehicle being sent to. A press with
// nothing live does nothing, so setting one up cannot change anything.
void joystick_set_button(int button, const JoyButtonBinding& binding);

// The button the panel is editing, and "press the one you mean" to choose it.
int  joystick_button_selected();
void joystick_button_select(int button);
void joystick_button_detect(bool on);
bool joystick_button_detecting();

// ── Virtual channels ─────────────────────────────────────────────────────────
//
// See JoyVirtualChannel. Positions are captured one at a time by putting the
// switch there; the channel exists once the last one is taken, and from then on
// its buttons have no actions of their own.

void joystick_virtual_begin(int positions);
void joystick_virtual_cancel();
void joystick_virtual_capture();          // takes the position being asked for
bool joystick_virtual_capturing();
int  joystick_virtual_capture_step();     // 0-based position being asked for
int  joystick_virtual_capture_total();
const char* joystick_virtual_capture_problem();   // why the last capture failed, or nullptr

void joystick_virtual_remove(int index);
void joystick_virtual_set_action(int index, int position, const JoyButtonBinding& action);

// Where the switch is this frame, for showing it. Actions and the RC override wait
// for a position to settle; the display does not, so it answers the hand at once.
// -1 when the switch has not matched a position yet.
int  joystick_virtual_position(int index);

// ── RC override ──────────────────────────────────────────────────────────────

void joystick_set_rc_override(bool on);
void joystick_set_axis_rc(int axis, const JoyRcOut& out);
void joystick_set_virtual_rc(int index, const JoyRcOut& out);

// All 18 RC_CHANNELS_OVERRIDE fields as the sticks and switches have them now.
void joystick_rc_values(uint16_t out[RcChannels::MAX_CHANNELS]);

// ── Sending ──────────────────────────────────────────────────────────────────

// Why sending cannot start right now, or nullptr if it can.
const char* joystick_engage_problem();

// Starts sending to `target`. False, logged, if joystick_engage_problem() has an
// objection by the time this is called.
bool joystick_engage(VehicleId target);
void joystick_disengage();

bool       joystick_live();
VehicleId  joystick_live_target();        // invalid while not sending
uint32_t   joystick_live_number();        // the map's number for it
JoyCommand joystick_command();            // what the stick says now; zero until usable

// ── Read output ──────────────────────────────────────────────────────────────

// The frame last written to the link, for as long as sending lasts: whichever of
// MANUAL_CONTROL and RC_CHANNELS_OVERRIDE the profile sends.
MavlinkSender::ManualControlFrame joystick_output_frame();
MavlinkSender::RcOverrideFrame joystick_output_rc_frame();
bool   joystick_output_has_frame();       // false until this run's first frame leaves
double joystick_output_hz();              // frames written per second, measured

// The buttons as MANUAL_CONTROL would carry them, without those that have actions.
uint32_t joystick_output_buttons();

// Button and switch commands, newest first. These fire once and are gone, so the
// last few are kept on screen.
struct JoyOutputCommand {
    double      time = 0.0;               // glfwGetTime()
    std::string source;                   // "button 3", "V1 position 2"
    std::string text;                     // the message and its fields
};
const std::deque<JoyOutputCommand>& joystick_output_commands();
double joystick_now();

// Drawn over every view while sending, with a STOP. SWARM and the fullscreen
// views have no RADIO tab to go back to, and a stream of stick commands is not
// something to lose track of.
void draw_joystick_live_banner();
