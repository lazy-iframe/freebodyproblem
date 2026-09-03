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


#include "gyro_calibration.hpp"

void GyroCalibration::begin(double now)
{
    phase_      = Phase::Running;
    result_     = kNoResult;
    started_at_ = now;
}

void GyroCalibration::cancel()
{
    phase_      = Phase::Idle;
    result_     = kNoResult;
    started_at_ = 0.0;
}

void GyroCalibration::on_command_ack(uint8_t result, double now)
{
    if (!running()) return;

    // MAV_RESULT_IN_PROGRESS. Not a verdict: the vehicle is saying it has the
    // command and is working on it, and the one that decides the run is still
    // to come. The clock restarts on it — a vehicle that keeps reporting
    // progress has not gone quiet, which is the only thing the timeout is for.
    if (result == 5) {
        started_at_ = now;
        return;
    }

    result_ = result;
    phase_  = (result == 0) ? Phase::Succeeded : Phase::Failed;
}

void GyroCalibration::tick(double now)
{
    if (!running()) return;

    if (now - started_at_ >= kTimeoutSeconds) {
        result_ = kNoResult;
        phase_  = Phase::Failed;
    }
}

const char* GyroCalibration::result_text() const
{
    switch (result_) {
    case 0:  return "accepted";
    case 1:  return "temporarily rejected";
    case 2:  return "denied";
    case 3:  return "unsupported";
    case 4:  return "failed";
    case 6:  return "cancelled";
    case kNoResult: return "no reply from the vehicle";
    default: return "rejected";
    }
}
