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

#include "mavlink_parser.hpp"   // StatusText

// ─────────────────────────────────────────────────────────────────────────────
// Accelerometer calibration — the GCS half of ArduPilot's six-position routine.
//
// The vehicle drives it. After MAV_CMD_PREFLIGHT_CALIBRATION with the
// accelerometer parameter set, it asks for one orientation at a time and waits;
// the GCS answers each with MAV_CMD_ACCELCAL_VEHICLE_POS once the airframe has
// actually been moved. Six positions, then a verdict.
//
// Each request arrives twice, in two forms:
//
//   • STATUSTEXT — "Place vehicle level and press any key." Prose, meant for a
//     person, and what the panel puts on screen.
//   • MAV_CMD_ACCELCAL_VEHICLE_POS — the same request as an enum, and the only
//     form that reports SUCCESS and FAILED as values rather than wording.
//
// This tracks both, because neither alone is enough: the command is
// unambiguous but says nothing a human wants to read, and the text is readable
// but is a sentence that firmware is free to rephrase. The command wins where
// they disagree.
//
// Holds no link and sends nothing — it is fed messages and reports what the
// caller should transmit.
// ─────────────────────────────────────────────────────────────────────────────

// ACCELCAL_VEHICLE_POS, with the two out-of-band results folded in.
enum class AccelCalPos : uint32_t {
    None     = 0,
    Level    = 1,
    Left     = 2,
    Right    = 3,
    NoseDown = 4,
    NoseUp   = 5,
    Back     = 6,
    Success  = 16777215,
    Failed   = 16777216,
};

// "LEVEL", "LEFT SIDE", … — a short label for a button, not the vehicle's own
// sentence. Returns nullptr for None and for the two result values.
const char* accel_cal_pos_label(AccelCalPos pos);

// The six orientations in the order ArduPilot asks for them, for a progress
// readout that can show what is still to come.
int         accel_cal_pos_count();
AccelCalPos accel_cal_pos_at(int index);          // 0-based; None if out of range
int         accel_cal_pos_index(AccelCalPos pos); // 0-based; -1 if not one of the six

class AccelCalibration {
public:
    enum class Phase {
        Idle,        // nothing running
        Starting,    // command sent, waiting for the vehicle's first request
        Waiting,     // the vehicle has asked for a position; move the airframe
        Confirming,  // position reported, waiting for the vehicle to move on
        Succeeded,
        Failed,
    };

    // A calibration was requested. The caller sends the command; this only
    // starts expecting the traffic that follows.
    void begin();

    // Abandon it locally. The caller decides whether to tell the vehicle.
    void cancel();

    // Feed MAV_CMD_ACCELCAL_VEHICLE_POS as it arrives, param1 verbatim.
    // Ignored unless a calibration is running.
    void on_vehicle_pos(uint32_t position);

    // Feed each new STATUSTEXT. Kept as the on-screen instruction when it reads
    // like a position request, and used to infer the position when no command
    // has arrived for it. Ignored unless a calibration is running.
    void on_statustext(const StatusText& st);

    // The operator says the airframe is in the requested position. Returns the
    // value to send as MAV_CMD_ACCELCAL_VEHICLE_POS, or None when there is
    // nothing outstanding to confirm.
    AccelCalPos confirm();

    Phase       phase()     const { return phase_; }
    AccelCalPos requested() const { return requested_; }

    // The vehicle's own words for the current step, empty until it has spoken.
    // Shown verbatim: it is the vehicle that knows what it wants, and
    // paraphrasing it here would only add a second thing to keep in step.
    const std::string& message() const { return message_; }

    // Positions confirmed so far, 0-6.
    int confirmed_count() const { return confirmed_; }

    bool running() const { return phase_ == Phase::Starting ||
                                  phase_ == Phase::Waiting  ||
                                  phase_ == Phase::Confirming; }

    // Reads `text` as a position request. Exposed for the fallback path and
    // because the wording is the fragile part worth testing on its own.
    // Returns None when the line is not one.
    static AccelCalPos parse_statustext(const char* text);

private:
    Phase       phase_     = Phase::Idle;
    AccelCalPos requested_ = AccelCalPos::None;
    AccelCalPos last_done_ = AccelCalPos::None;
    std::string message_;
    int         confirmed_ = 0;

    void set_requested(AccelCalPos pos);
};
