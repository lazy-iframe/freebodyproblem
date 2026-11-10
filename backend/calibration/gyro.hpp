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

// ─────────────────────────────────────────────────────────────────────────────
// Gyroscope calibration — the GCS half of MAV_CMD_PREFLIGHT_CALIBRATION with
// the gyro parameter set.
//
// Nothing like the accelerometer's six-position conversation: the vehicle is
// told to sample its own gyros while it sits still, and it does the whole thing
// on its own in a few seconds. There is no position to report and nothing to
// answer.
//
// The COMMAND_ACK is the verdict. The vehicle runs this one to completion
// before it replies, so ACCEPTED means the gyros were calibrated and anything
// else means they were not — no prose to parse, and no wording that a firmware
// release is free to change underneath this.
//
// A run that is never answered is failed on a timeout rather than left
// spinning. See kTimeoutSeconds.
//
// Holds no link and sends nothing — it is fed the ACK and a clock.
// ─────────────────────────────────────────────────────────────────────────────

class GyroCalibration {
public:
    enum class Phase {
        Idle,       // nothing running
        Running,    // command sent, waiting for the vehicle's verdict
        Succeeded,
        Failed,
    };

    // How long to wait for the ACK before calling it failed. Generous: the
    // sampling itself is a few seconds on every stack seen, and the rest is
    // slack for a slow or lossy link.
    static constexpr double kTimeoutSeconds = 30.0;

    // No ACK seen. Distinct from every MAV_RESULT, so result() can say
    // "timed out" rather than name a verdict the vehicle never gave.
    static constexpr uint8_t kNoResult = 255;

    // A calibration was requested; `now` is any monotonic seconds clock, the
    // same one passed to tick(). The caller sends the command — this only
    // starts expecting the reply.
    void begin(double now);

    // Abandon it locally. The caller decides whether to tell the vehicle.
    void cancel();

    // Feed the COMMAND_ACK for MAV_CMD_PREFLIGHT_CALIBRATION, `result` being
    // its MAV_RESULT verbatim. Ignored unless a calibration is running.
    void on_command_ack(uint8_t result, double now);

    // Call once a frame with the same clock as begin(). Fails a run that has
    // gone unanswered past the timeout; does nothing otherwise.
    void tick(double now);

    Phase phase() const { return phase_; }

    bool running() const { return phase_ == Phase::Running; }

    // The vehicle's MAV_RESULT, or kNoResult when it never answered.
    uint8_t result() const { return result_; }

    // What result() means, for the panel to show: "accepted", "denied",
    // "timed out", … Never null.
    const char* result_text() const;

    // Seconds since begin(), for a progress readout. 0 when not running.
    double elapsed(double now) const {
        return running() ? (now - started_at_) : 0.0;
    }

private:
    Phase   phase_      = Phase::Idle;
    uint8_t result_     = kNoResult;
    double  started_at_ = 0.0;
};
