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


#include "backend/calibration/accel.hpp"
#include "lib/text.hpp"

#include <cctype>
#include <cstring>

namespace {

// The six, in the order ArduPilot walks them.
const AccelCalPos kOrder[] = {
    AccelCalPos::Level,
    AccelCalPos::Left,
    AccelCalPos::Right,
    AccelCalPos::NoseDown,
    AccelCalPos::NoseUp,
    AccelCalPos::Back,
};
constexpr int kOrderCount = (int)(sizeof(kOrder) / sizeof(kOrder[0]));

} // namespace

// ── Position labels ──────────────────────────────────────────────────────────

const char* accel_cal_pos_label(AccelCalPos pos)
{
    switch (pos) {
    case AccelCalPos::Level:    return "LEVEL";
    case AccelCalPos::Left:     return "LEFT SIDE";
    case AccelCalPos::Right:    return "RIGHT SIDE";
    case AccelCalPos::NoseDown: return "NOSE DOWN";
    case AccelCalPos::NoseUp:   return "NOSE UP";
    case AccelCalPos::Back:     return "ON ITS BACK";
    default:                    return nullptr;
    }
}

int accel_cal_pos_count() { return kOrderCount; }

AccelCalPos accel_cal_pos_at(int index)
{
    if (index < 0 || index >= kOrderCount) return AccelCalPos::None;
    return kOrder[index];
}

// ── STATUSTEXT parsing ───────────────────────────────────────────────────────

AccelCalPos AccelCalibration::parse_statustext(const char* text)
{
    // Searched for the distinguishing word only, without regard to case: the
    // vehicle's wording is not stable enough to match exactly — it has changed
    // across releases and differs between vehicle types.
    const std::string_view t = text ? text : "";
    if (t.empty()) return AccelCalPos::None;

    // Results first. "Calibration successful" and "Calibration FAILED" are
    // verdicts, and testing them before the orientations keeps a word that
    // happens to appear in both from being read as a request to move.
    if (text_icontains(t, "successful") || text_icontains(t, "success"))
        return AccelCalPos::Success;
    if (text_icontains(t, "failed") || text_icontains(t, "failure"))
        return AccelCalPos::Failed;

    // Only lines that are actually asking for an orientation. Without this the
    // words below would match any passing message — a vehicle says "left" and
    // "up" for plenty of reasons that have nothing to do with a calibration.
    if (!text_icontains(t, "place") && !text_icontains(t, "position"))
        return AccelCalPos::None;

    // "nose down" / "nose up" before the bare directions: both contain a word
    // that on its own would be ambiguous, and the two-word form is the specific
    // one. "back" and "level" have no such overlap.
    if (text_icontains(t, "nose down") || text_icontains(t, "nosedown")) return AccelCalPos::NoseDown;
    if (text_icontains(t, "nose up")   || text_icontains(t, "noseup"))   return AccelCalPos::NoseUp;
    if (text_icontains(t, "level"))                                      return AccelCalPos::Level;
    if (text_icontains(t, "left"))                                       return AccelCalPos::Left;
    if (text_icontains(t, "right"))                                      return AccelCalPos::Right;
    if (text_icontains(t, "back"))                                       return AccelCalPos::Back;
    if (text_icontains(t, "down"))                                       return AccelCalPos::NoseDown;
    if (text_icontains(t, "up"))                                         return AccelCalPos::NoseUp;

    return AccelCalPos::None;
}

// ── Lifecycle ────────────────────────────────────────────────────────────────

void AccelCalibration::begin()
{
    phase_     = Phase::Starting;
    requested_ = AccelCalPos::None;
    last_done_ = AccelCalPos::None;
    message_.clear();
    confirmed_ = 0;
}

void AccelCalibration::cancel()
{
    phase_     = Phase::Idle;
    requested_ = AccelCalPos::None;
    last_done_ = AccelCalPos::None;
    message_.clear();
    confirmed_ = 0;
}

void AccelCalibration::set_requested(AccelCalPos pos)
{
    // The vehicle re-sends a request it thinks went unanswered. Treating the
    // repeat as a new step would advance the count without the airframe having
    // moved, so a position already confirmed is only a reminder.
    if (pos == last_done_ && phase_ == Phase::Confirming) return;

    requested_ = pos;
    phase_     = Phase::Waiting;
}

void AccelCalibration::on_vehicle_pos(uint32_t position)
{
    if (!running()) return;

    const AccelCalPos pos = (AccelCalPos)position;

    switch (pos) {
    case AccelCalPos::Success:
        phase_     = Phase::Succeeded;
        requested_ = AccelCalPos::None;
        return;
    case AccelCalPos::Failed:
        phase_     = Phase::Failed;
        requested_ = AccelCalPos::None;
        return;
    case AccelCalPos::Level:
    case AccelCalPos::Left:
    case AccelCalPos::Right:
    case AccelCalPos::NoseDown:
    case AccelCalPos::NoseUp:
    case AccelCalPos::Back:
        set_requested(pos);
        return;
    default:
        // A value outside the enum. Nothing sensible to do with it, and
        // guessing would be worse than waiting for the next message.
        return;
    }
}

void AccelCalibration::on_statustext(const StatusText& st)
{
    if (!running()) return;

    const AccelCalPos pos = parse_statustext(st.text);
    if (pos == AccelCalPos::None) return;

    if (pos == AccelCalPos::Success) { phase_ = Phase::Succeeded; requested_ = AccelCalPos::None; }
    else if (pos == AccelCalPos::Failed) { phase_ = Phase::Failed; requested_ = AccelCalPos::None; }
    else set_requested(pos);

    // Kept whatever the phase became, so the panel can show the vehicle's own
    // wording for the verdict as readily as for a step.
    message_ = st.text;
}

AccelCalPos AccelCalibration::confirm()
{
    if (phase_ != Phase::Waiting || requested_ == AccelCalPos::None)
        return AccelCalPos::None;

    const AccelCalPos pos = requested_;
    last_done_ = pos;
    phase_     = Phase::Confirming;
    ++confirmed_;
    return pos;
}
