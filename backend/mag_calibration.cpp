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


#include "mag_calibration.hpp"

#include <cctype>
#include <cstring>

namespace {

bool contains(const std::string& haystack_lower, const char* needle_lower)
{
    return haystack_lower.find(needle_lower) != std::string::npos;
}

std::string lowered(const char* text)
{
    std::string out;
    if (!text) return out;
    for (const char* p = text; *p; ++p)
        out += (char)std::tolower((unsigned char)*p);
    return out;
}

// First run of digits after `key`, or -1. PX4 writes its percentage as
// "[cal] progress <30>", and the brackets have moved between releases while the
// number after the word has not. Read from the keyword rather than from the
// start of the line, because a line that also names the sensor — "[cal] mag 0:
// progress <30>" — would otherwise yield the sensor's number.
int number_after(const std::string& t, const char* key)
{
    const size_t at = t.find(key);
    if (at == std::string::npos) return -1;

    for (size_t i = at; i < t.size(); ++i) {
        if (!std::isdigit((unsigned char)t[i])) continue;
        int v = 0;
        while (i < t.size() && std::isdigit((unsigned char)t[i]) && v < 1000)
            v = v * 10 + (t[i++] - '0');
        return v;
    }
    return -1;
}

// MAG_CAL_STATUS values that end a compass badly.
bool status_is_failure(uint8_t s)
{
    return s == MAG_CAL_FAILED || s == MAG_CAL_BAD_ORIENTATION ||
           s == MAG_CAL_BAD_RADIUS;
}

} // namespace

// ── Lifecycle ────────────────────────────────────────────────────────────────

void MagCalibration::begin(uint32_t magcal_seq, double now)
{
    phase_         = Phase::Running;
    percent_       = 0;
    message_.clear();
    needs_save_    = false;
    worst_fitness_ = -1.0f;
    has_mask_      = false;
    std::memset(mask_, 0, sizeof(mask_));
    begin_seq_     = magcal_seq;
    last_slot_seq_ = magcal_seq;
    started_at_    = now;
    last_word_     = now;
    ack_result_    = 255;
    cal_status_    = 0;
    timed_out_     = false;
}

void MagCalibration::cancel()
{
    phase_      = Phase::Idle;
    percent_    = 0;
    message_.clear();
    needs_save_ = false;
    worst_fitness_ = -1.0f;
    has_mask_   = false;
    std::memset(mask_, 0, sizeof(mask_));
    timed_out_  = false;
}

void MagCalibration::fail(uint8_t cal_status, uint8_t ack_result)
{
    phase_      = Phase::Failed;
    cal_status_ = cal_status;
    ack_result_ = ack_result;
}

// ── ArduPilot: MAG_CAL_PROGRESS / MAG_CAL_REPORT ─────────────────────────────

void MagCalibration::on_mag_cal(const VehicleState::MagCalCompass* slots, int count,
                                double now)
{
    if (!running() || !slots) return;

    int  worst        = 101;   // above any percentage, so "none seen" is visible
    int  participating = 0;
    int  finished     = 0;
    bool any_failed   = false;
    uint8_t failure   = 0;
    bool any_unsaved  = false;
    float worst_fit   = -1.0f;
    uint32_t newest   = last_slot_seq_;
    const VehicleState::MagCalCompass* laggard = nullptr;

    for (int i = 0; i < count; ++i) {
        const auto& c = slots[i];
        // Written before this run started: another calibration's leftovers.
        if (!c.seen || c.seq <= begin_seq_) continue;

        ++participating;
        if (c.seq > newest) newest = c.seq;
        if ((int)c.completion_pct < worst) {
            worst   = c.completion_pct;
            laggard = &c;
        }

        if (status_is_failure(c.status)) { any_failed = true; failure = c.status; }

        if (c.reported) {
            ++finished;
            if (c.fitness > worst_fit) worst_fit = c.fitness;
            if (c.status == MAG_CAL_SUCCESS && !c.autosaved) any_unsaved = true;
        }
    }

    if (participating == 0) return;   // nothing from this run yet

    // Only an actually new message counts as the vehicle having spoken.
    if (newest > last_slot_seq_) {
        last_slot_seq_ = newest;
        last_word_     = now;
    }

    percent_       = (worst <= 100) ? worst : 0;
    worst_fitness_ = worst_fit;

    // The coverage shown is the laggard's: filling its gaps is what finishes
    // the run, and the compasses turn together anyway.
    if (laggard) {
        std::memcpy(mask_, laggard->completion_mask, sizeof(mask_));
        has_mask_ = true;
    }

    // A single compass failing fails the calibration. The vehicle flies on all
    // of them, and "two out of three" is not a compass calibration.
    if (any_failed) {
        fail(failure, 255);
        return;
    }

    // Every compass that joined in has reported. Nothing further is coming.
    if (finished == participating) {
        phase_      = Phase::Succeeded;
        percent_    = 100;
        needs_save_ = any_unsaved;
    }
}

// ── PX4: STATUSTEXT ──────────────────────────────────────────────────────────

void MagCalibration::on_statustext(const StatusText& st, double now)
{
    if (!running()) return;

    const std::string t = lowered(st.text);
    if (t.empty()) return;

    // Only lines that are about this. PX4 prefixes its calibration chatter with
    // "[cal]"; ArduPilot names the sensor. Everything else a busy vehicle says
    // would otherwise land in the panel as though it were an instruction.
    const bool ours = contains(t, "[cal]") || contains(t, "compass") ||
                      contains(t, "mag");
    if (!ours) return;

    last_word_ = now;
    message_   = st.text;

    if (contains(t, "progress")) {
        const int pct = number_after(t, "progress");
        if (pct >= 0 && pct <= 100) percent_ = pct;
    }
}

// ── The ACK ──────────────────────────────────────────────────────────────────

void MagCalibration::on_command_ack(uint8_t result, double now)
{
    if (!running()) return;

    last_word_ = now;

    // MAV_RESULT_IN_PROGRESS: the vehicle has the command and is working.
    if (result == 5) return;

    if (result != 0) {                 // rejected outright, by either stack
        fail(0, result);
        return;
    }

    // ACCEPTED. Two very different things wear this: ArduPilot acknowledging
    // the request within a moment of it being sent, and PX4 announcing a
    // finished calibration a minute later. The clock tells them apart, and on
    // ArduPilot the real verdict arrives later as MAG_CAL_REPORT anyway.
    if (now - started_at_ < kAckGraceSeconds) return;

    phase_   = Phase::Succeeded;
    percent_ = 100;
    // PX4 stores what it calibrated; there is nothing left to force-save, and
    // the command that would do it is ArduPilot's.
    needs_save_ = false;
}

void MagCalibration::on_start_ack(uint8_t result, double now)
{
    if (!running()) return;

    last_word_ = now;

    if (result == 5) return;   // IN_PROGRESS, same as above

    // ACCEPTED here is a receipt: the vehicle has begun, and MAG_CAL_REPORT
    // will say how it went. Anything else means it did not begin at all, and
    // waiting out the quiet timeout for a run that never started would be two
    // minutes of the panel telling the operator to keep rotating.
    if (result != 0) fail(0, result);
}

void MagCalibration::tick(double now)
{
    if (!running()) return;

    if (now - last_word_ >= kQuietSeconds) {
        timed_out_ = true;
        fail(0, 255);
    }
}

const char* MagCalibration::failure_text() const
{
    if (timed_out_) return "no reply from the vehicle";

    switch (cal_status_) {
    case MAG_CAL_FAILED:          return "the fit was not good enough";
    case MAG_CAL_BAD_ORIENTATION: return "the compass is mounted the wrong way round";
    case MAG_CAL_BAD_RADIUS:      return "the field strength is wrong \xe2\x80\x94 "
                                         "look for magnets and current-carrying wire";
    default: break;
    }

    switch (ack_result_) {
    case 1:  return "temporarily rejected";
    case 2:  return "denied";
    case 3:  return "unsupported";
    case 4:  return "failed";
    case 6:  return "cancelled";
    case 255: break;
    default: return "rejected";
    }

    return "the vehicle would not finish";
}
