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

#include "mavlink_parser.hpp"   // StatusText, VehicleState::MagCalCompass

// ─────────────────────────────────────────────────────────────────────────────
// Magnetometer calibration — the GCS half of MAV_CMD_PREFLIGHT_CALIBRATION with
// the magnetometer parameter set.
//
// Neither a conversation like the accelerometer's nor a moment's wait like the
// gyro's. The vehicle samples continuously while the operator turns the
// airframe through every orientation, and it is done when it has seen enough.
// Nothing is asked of the GCS but to show how far along it is.
//
// The two stacks say so differently, and this tracks both because a GCS does
// not get to pick which one it is talking to:
//
//   • ArduPilot sends MAG_CAL_PROGRESS and MAG_CAL_REPORT, one per compass, a
//     percentage and then a verdict per sensor. Started with
//     MAV_CMD_DO_START_MAG_CAL, whose COMMAND_ACK is not the verdict — it
//     arrives at once, acknowledging the request. A NACK there is, though:
//     nothing has started, and nothing further will arrive.
//   • PX4 sends neither, and narrates in STATUSTEXT: "[cal] progress <30>",
//     "[cal] nose down orientation detected". Its verdict is the COMMAND_ACK,
//     which does not arrive until the calibration has finished.
//
// So: percentage and prose from whichever is talking, and a verdict from
// MAG_CAL_REPORT where there is one and from the ACK where there is not. The
// ack-versus-receipt ambiguity is settled by kAckGraceSeconds below.
//
// Holds no link and sends nothing — it is fed messages and a clock.
// ─────────────────────────────────────────────────────────────────────────────

// The orientations the airframe has to be turned through. PX4 announces each as
// it recognises it; ArduPilot does not, and there the list is a hint rather
// than a checklist.
int         mag_cal_orientation_count();
const char* mag_cal_orientation_label(int index);   // nullptr if out of range

class MagCalibration {
public:
    enum class Phase {
        Idle,
        Running,
        Succeeded,
        Failed,
    };

    // How long after the command an ACCEPTED ack is read as "request received"
    // rather than "calibration finished". ArduPilot answers immediately and
    // then calibrates for a minute or more; PX4 does not answer until it is
    // done. Well clear of a round trip, nowhere near a real calibration.
    static constexpr double kAckGraceSeconds = 5.0;

    // How long the vehicle may say nothing at all before the run is given up
    // on. Generous: it is the operator doing the work, and a slow one turning
    // an airframe through six orientations is not a fault.
    static constexpr double kQuietSeconds = 120.0;

    // A calibration was requested. `magcal_seq` is VehicleState::magcal_seq as
    // it stands now, the mark against which the per-compass slots are judged to
    // belong to this run rather than to a previous one.
    void begin(uint32_t magcal_seq, double now);

    // Abandon it locally. The caller decides whether to tell the vehicle.
    void cancel();

    // Feed the per-compass slots every frame; `count` is how many the array
    // holds. Slots written before begin() are ignored.
    void on_mag_cal(const VehicleState::MagCalCompass* slots, int count, double now);

    // Feed each new STATUSTEXT — PX4's progress and orientation lines, and
    // whatever ArduPilot says about the compasses. Never decides the verdict:
    // it is the readable half, and the messages above are the authoritative one.
    void on_statustext(const StatusText& st, double now);

    // Feed the COMMAND_ACK for MAV_CMD_PREFLIGHT_CALIBRATION — PX4's verdict,
    // and on ArduPilot a receipt for a start this panel did not send that way.
    void on_command_ack(uint8_t result, double now);

    // Feed the COMMAND_ACK for DO_START_MAG_CAL (42424). A rejection means the
    // calibration never began; an acceptance is a receipt and decides nothing,
    // because on this path the verdict is MAG_CAL_REPORT.
    void on_start_ack(uint8_t result, double now);

    // Call once a frame with the same clock as begin().
    void tick(double now);

    Phase phase() const { return phase_; }
    bool  running() const { return phase_ == Phase::Running; }

    // 0-100. The least far along of the compasses reporting, because the run is
    // not finished until the slowest of them is.
    int percent() const { return percent_; }

    // The vehicle's own words for the last thing it did, empty until it has
    // spoken. Shown verbatim.
    const std::string& message() const { return message_; }

    // Orientations PX4 has said it recognised, as a bitmask over the indices of
    // mag_cal_orientation_label(). Always 0 on ArduPilot, which does not say.
    uint32_t detected_mask() const { return detected_; }

    // After a successful run: true when a compass finished well but the vehicle
    // has not stored the result — which is every ArduPilot run this panel
    // starts, since it starts them with autosave off. The offsets exist and are
    // discarded unless DO_ACCEPT_MAG_CAL follows.
    bool needs_save() const { return needs_save_; }

    // Why it failed. Never null; meaningless unless the phase is Failed.
    const char* failure_text() const;

    // The coverage mask of the compass the percentage above came from — the
    // one holding the run up, which is the one whose gaps the operator has to
    // fill. 80 bits over the geodesic grid; see geodesic_grid.hpp for what a
    // bit means. Null until a MAG_CAL_PROGRESS of this run has arrived, and
    // always null on PX4, which does not send one.
    const uint8_t* completion_mask() const { return has_mask_ ? mask_ : nullptr; }

    // The worst fitness reported, in milligauss — the number that says how good
    // the fit actually was. Negative when no compass reported one.
    float worst_fitness() const { return worst_fitness_; }

private:
    Phase       phase_      = Phase::Idle;
    int         percent_    = 0;
    std::string message_;
    uint32_t    detected_   = 0;
    bool        needs_save_ = false;
    float       worst_fitness_ = -1.0f;

    uint8_t     mask_[10]   = {};
    bool        has_mask_   = false;

    uint32_t    begin_seq_  = 0;
    // Highest per-compass slot sequence already accounted for. The slots are a
    // latch and are read whole every frame, so this is what separates "the
    // vehicle said something" from "it is still saying nothing" — without it
    // the quiet watchdog would be reset by its own re-reading.
    uint32_t    last_slot_seq_ = 0;
    double      started_at_ = 0.0;
    double      last_word_  = 0.0;   // last time the vehicle said anything

    uint8_t     ack_result_ = 255;   // MAV_RESULT of a rejecting ack
    uint8_t     cal_status_ = 0;     // MAG_CAL_STATUS behind a failure
    bool        timed_out_  = false;

    void fail(uint8_t cal_status, uint8_t ack_result);
};
