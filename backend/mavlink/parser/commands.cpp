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

// MavlinkParser: commands from the vehicle, COMMAND_ACK, and compass-calibration reports.
// Every MAVLink message received is decoded by MavlinkParser; handle() in
// parser.cpp dispatches here.

#include "backend/mavlink/parser/parser.hpp"
#include "backend/vehicle/firmware_profile.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "backend/mavlink/signing.hpp"
#include "frontend/source/app_log.hpp"   // gcs_log — thread-safe, see the header

namespace {

// ── Compass-calibration tracing ──────────────────────────────────────────────
// Every MAG_CAL_PROGRESS, MAG_CAL_REPORT and calibration COMMAND_ACK, printed
// to the GCS log exactly as it came off the wire. The SENSORS panel only ever
// sees the latched summary of these, which is no use when the question is
// whether the vehicle said anything at all. Set to false to quieten it.
constexpr bool trace_mag_cal = false;

const char* mag_cal_status_name(uint8_t s)
{
    switch (s) {
    case MAG_CAL_NOT_STARTED:      return "NOT_STARTED";
    case MAG_CAL_WAITING_TO_START: return "WAITING_TO_START";
    case MAG_CAL_RUNNING_STEP_ONE: return "RUNNING_STEP_ONE";
    case MAG_CAL_RUNNING_STEP_TWO: return "RUNNING_STEP_TWO";
    case MAG_CAL_SUCCESS:          return "SUCCESS";
    case MAG_CAL_FAILED:           return "FAILED";
    case MAG_CAL_BAD_ORIENTATION:  return "BAD_ORIENTATION";
    case MAG_CAL_BAD_RADIUS:       return "BAD_RADIUS";
    default:                       return "?";
    }
}

const char* mav_result_name(uint8_t r)
{
    switch (r) {
    case MAV_RESULT_ACCEPTED:             return "ACCEPTED";
    case MAV_RESULT_TEMPORARILY_REJECTED: return "TEMPORARILY_REJECTED";
    case MAV_RESULT_DENIED:               return "DENIED";
    case MAV_RESULT_UNSUPPORTED:          return "UNSUPPORTED";
    case MAV_RESULT_FAILED:               return "FAILED";
    case MAV_RESULT_IN_PROGRESS:          return "IN_PROGRESS";
    case MAV_RESULT_CANCELLED:            return "CANCELLED";
    default:                              return "?";
    }
}

} // namespace

void MavlinkParser::on_command_long(const mavlink_message_t& msg)
{
    // Vehicles mostly receive COMMAND_LONG; during an accelerometer
    // calibration ArduPilot also sends one, to tell the GCS which way to
    // turn the airframe next. Nothing else this GCS cares about arrives
    // this way, so only that command is decoded.
    mavlink_command_long_t cl;
    mavlink_msg_command_long_decode(&msg, &cl);
    if (cl.command == MAV_CMD_ACCELCAL_VEHICLE_POS) {
        state_.accelcal_pos = (uint32_t)cl.param1;
        ++state_.accelcal_seq;
    }
}

void MavlinkParser::on_command_ack(const mavlink_message_t& msg)
{
    mavlink_command_ack_t ack;
    mavlink_msg_command_ack_decode(&msg, &ack);
    ack_queue_.push_back({ack.command, ack.result});
    // MAV_CMD_PREFLIGHT_CALIBRATION. Also latched into the state, where the
    // SENSORS panel can still find it after the queue above has been
    // drained and cleared by the link thread.
    if (ack.command == 241) {
        state_.calib_ack_result = ack.result;
        ++state_.calib_ack_seq;
    }
    // The compass calibration's own commands, latched the same way.
    if (ack.command == 42424 || ack.command == 42425 || ack.command == 42426) {
        state_.magcal_ack_cmd    = ack.command;
        state_.magcal_ack_result = ack.result;
        ++state_.magcal_ack_seq;
    }
    if (trace_mag_cal &&
        (ack.command == 241 || ack.command == 511 || ack.command == 42424 ||
         ack.command == 42425 || ack.command == 42426)) {
        // 511 is in here because the compass messages have to be asked for
        // with it, and a refusal there is the difference between a vehicle
        // that says nothing and a vehicle that was never asked to speak.
        const char* which = ack.command == 241   ? "PREFLIGHT_CALIBRATION"
                          : ack.command == 511   ? "SET_MESSAGE_INTERVAL"
                          : ack.command == 42424 ? "DO_START_MAG_CAL"
                          : ack.command == 42425 ? "DO_ACCEPT_MAG_CAL"
                                                 : "DO_CANCEL_MAG_CAL";
        gcs_log("magcal rx: ACK %s (%u) result=%s (%u) from %u/%u",
                which, (unsigned)ack.command, mav_result_name(ack.result),
                (unsigned)ack.result, (unsigned)msg.sysid, (unsigned)msg.compid);
    }
}

void MavlinkParser::on_mag_cal_progress(const mavlink_message_t& msg)
{
    mavlink_mag_cal_progress_t p;
    mavlink_msg_mag_cal_progress_decode(&msg, &p);
    if (trace_mag_cal)
        gcs_log("magcal rx: PROGRESS compass=%u mask=0x%02X status=%s (%u) "
                "attempt=%u pct=%u dir=(%.2f %.2f %.2f) from %u/%u",
                (unsigned)p.compass_id, (unsigned)p.cal_mask,
                mag_cal_status_name(p.cal_status), (unsigned)p.cal_status,
                (unsigned)p.attempt, (unsigned)p.completion_pct,
                (double)p.direction_x, (double)p.direction_y, (double)p.direction_z,
                (unsigned)msg.sysid, (unsigned)msg.compid);
    if (p.compass_id >= VehicleState::MAX_COMPASSES && trace_mag_cal)
        gcs_log("magcal rx: PROGRESS compass=%u ignored \xe2\x80\x94 only %d slots",
                (unsigned)p.compass_id, VehicleState::MAX_COMPASSES);
    if (p.compass_id < VehicleState::MAX_COMPASSES) {
        auto& c = state_.magcal[p.compass_id];
        c.seen           = true;
        c.status         = p.cal_status;
        c.completion_pct = p.completion_pct;
        std::memcpy(c.completion_mask, p.completion_mask,
                    sizeof(c.completion_mask));
        // Progress for a compass that has already reported means a second
        // calibration is under way; the old verdict is not this run's.
        c.reported       = false;
        c.seq            = ++state_.magcal_seq;
    }
}

void MavlinkParser::on_mag_cal_report(const mavlink_message_t& msg)
{
    mavlink_mag_cal_report_t r;
    mavlink_msg_mag_cal_report_decode(&msg, &r);
    if (trace_mag_cal)
        gcs_log("magcal rx: REPORT compass=%u mask=0x%02X status=%s (%u) "
                "autosaved=%u fitness=%.1f ofs=(%.1f %.1f %.1f) "
                "orientation %u\xe2\x86\x92%u conf=%.2f scale=%.3f from %u/%u",
                (unsigned)r.compass_id, (unsigned)r.cal_mask,
                mag_cal_status_name(r.cal_status), (unsigned)r.cal_status,
                (unsigned)r.autosaved, (double)r.fitness,
                (double)r.ofs_x, (double)r.ofs_y, (double)r.ofs_z,
                (unsigned)r.old_orientation, (unsigned)r.new_orientation,
                (double)r.orientation_confidence, (double)r.scale_factor,
                (unsigned)msg.sysid, (unsigned)msg.compid);
    if (r.compass_id >= VehicleState::MAX_COMPASSES && trace_mag_cal)
        gcs_log("magcal rx: REPORT compass=%u ignored \xe2\x80\x94 only %d slots",
                (unsigned)r.compass_id, VehicleState::MAX_COMPASSES);
    if (r.compass_id < VehicleState::MAX_COMPASSES) {
        auto& c = state_.magcal[r.compass_id];
        c.seen      = true;
        c.status    = r.cal_status;
        c.reported  = true;
        c.autosaved = (r.autosaved != 0);
        c.fitness   = r.fitness;
        // A finished compass is a finished compass, whatever the last
        // progress message happened to say.
        if (r.cal_status == MAG_CAL_SUCCESS) c.completion_pct = 100;
        c.seq       = ++state_.magcal_seq;
    }
}
