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


#include "rc_calibration.hpp"

#include <algorithm>
#include <cstdio>

namespace {

// A pulse worth believing. Zero means "channel not received" and every value
// outside the window is a dropout or a decode error; both must be dropped
// before they can become an endpoint.
inline bool plausible(uint16_t pwm)
{
    return pwm >= RcCalibration::PWM_FLOOR && pwm <= RcCalibration::PWM_CEIL;
}

const RcChannelCal kEmptyChannel{};

} // namespace

// ── Lifecycle ────────────────────────────────────────────────────────────────

void RcCalibration::begin(int channel_count)
{
    for (RcChannelCal& c : chans_) c = RcChannelCal{};

    channel_count_ = std::max(0, std::min(channel_count,
                                          (int)RcChannels::MAX_CHANNELS));
    phase_ = Phase::Center;
    // throttle_channel_ deliberately survives: it comes from the vehicle's
    // stick mapping, not from this run, and re-deriving it on every restart
    // would lose it the moment the operator cancels and tries again.
}

bool RcCalibration::capture_center(const RcChannels& rc)
{
    if (phase_ != Phase::Center) return false;

    const int n = std::min(channel_count_, rc.usable_count());

    // Refuse a centre taken from a receiver that is not talking. Every later
    // step builds on these numbers, and a frame of zeroes would set every trim
    // to the PWM floor without anything downstream being able to tell.
    int live = 0;
    for (int i = 0; i < n; ++i)
        if (plausible(rc.chan[i])) ++live;
    if (live == 0) return false;

    for (int i = 0; i < n; ++i) {
        const uint16_t v = rc.chan[i];
        if (!plausible(v)) continue;

        RcChannelCal& c = chans_[i];
        c.trim = v;
        // The centre is also the first sample of the sweep. Seeding the range
        // with it rather than with the PWM window means an untouched channel
        // ends the run with zero travel and is reported as unusable, instead of
        // inheriting an 800-2200 range nobody demonstrated.
        c.min  = v;
        c.max  = v;
        c.seen = true;
    }

    phase_ = Phase::Sweep;
    return true;
}

void RcCalibration::update(const RcChannels& rc)
{
    if (phase_ != Phase::Sweep) return;

    const int n = std::min(channel_count_, rc.usable_count());
    for (int i = 0; i < n; ++i) {
        const uint16_t v = rc.chan[i];
        if (!plausible(v)) continue;

        RcChannelCal& c = chans_[i];
        if (!c.seen) {
            // A channel that was dead at centre-capture and has since come
            // alive — a switch on a second receiver bank, say. It has no trim,
            // so it starts its range here and stays out of the trim writes
            // unless it is moved enough to count.
            c.min  = v;
            c.max  = v;
            c.seen = true;
            continue;
        }
        if (v < c.min) c.min = v;
        if (v > c.max) c.max = v;
    }
}

bool RcCalibration::finish()
{
    if (phase_ != Phase::Sweep) return false;
    phase_ = Phase::Review;
    return true;
}

void RcCalibration::cancel()
{
    for (RcChannelCal& c : chans_) c = RcChannelCal{};
    channel_count_ = 0;
    phase_         = Phase::Idle;
}

// ── Inspection ───────────────────────────────────────────────────────────────

const RcChannelCal& RcCalibration::channel(int idx) const
{
    if (idx < 0 || idx >= channel_count_) return kEmptyChannel;
    return chans_[idx];
}

bool RcCalibration::channel_usable(int idx) const
{
    if (idx < 0 || idx >= channel_count_) return false;
    const RcChannelCal& c = chans_[idx];
    return c.seen && (c.max - c.min) >= MIN_TRAVEL_US;
}

int RcCalibration::usable_count() const
{
    int n = 0;
    for (int i = 0; i < channel_count_; ++i)
        if (channel_usable(i)) ++n;
    return n;
}

bool RcCalibration::channel_active(int idx) const
{
    if (idx < 0 || idx >= channel_count_) return false;
    return chans_[idx].seen;
}

int RcCalibration::active_count() const
{
    int n = 0;
    for (int i = 0; i < channel_count_; ++i)
        if (channel_active(i)) ++n;
    return n;
}

void RcCalibration::set_reversed(int idx, bool rev)
{
    if (idx < 0 || idx >= channel_count_) return;
    chans_[idx].reversed = rev;
}

void RcCalibration::set_throttle_channel(int channel_1based)
{
    if (channel_1based < 0 || channel_1based > RcChannels::MAX_CHANNELS) return;
    throttle_channel_ = channel_1based;
}

// ── Result ───────────────────────────────────────────────────────────────────

std::vector<RcParamWrite> RcCalibration::params(RcParamDialect dialect) const
{
    std::vector<RcParamWrite> out;
    if (phase_ != Phase::Review) return out;

    // MAV_PARAM_TYPE_REAL32. Both stacks store these as integers internally but
    // both accept — and ArduPilot's own parameter table advertises — the float
    // form over the classic protocol. A caller that has the vehicle's fetched
    // parameter table can substitute the type it actually reported.
    constexpr uint8_t REAL32 = 9;

    const char* rev_name = (dialect == RcParamDialect::PX4) ? "REV" : "REVERSED";

    out.reserve((size_t)usable_count() * 4);

    for (int i = 0; i < channel_count_; ++i) {
        if (!channel_usable(i)) continue;

        const RcChannelCal& c  = chans_[i];
        const int           ch = i + 1;   // RCn_* names are 1-based

        char id[24];

        snprintf(id, sizeof(id), "RC%d_MIN", ch);
        out.push_back({ id, (float)c.min, REAL32 });

        snprintf(id, sizeof(id), "RC%d_MAX", ch);
        out.push_back({ id, (float)c.max, REAL32 });

        // Throttle rests at the bottom, not in the middle, so its trim is the
        // floor of its travel. Writing a mid-stick trim there is what makes a
        // freshly calibrated vehicle believe half throttle is idle.
        const uint16_t trim = (ch == throttle_channel_) ? c.min : c.trim;
        snprintf(id, sizeof(id), "RC%d_TRIM", ch);
        // A trim outside the measured travel is a centre captured before the
        // sweep widened past it in the other direction; clamp rather than write
        // a trim the endpoints exclude.
        out.push_back({ id,
                        (float)std::max(c.min, std::min(trim, c.max)),
                        REAL32 });

        // PX4's RCn_REV is a float that is -1 or 1; ArduPilot's RCn_REVERSED is
        // a 0/1 flag. Same intent, different spelling of "backwards".
        snprintf(id, sizeof(id), "RC%d_%s", ch, rev_name);
        const float rev_val = (dialect == RcParamDialect::PX4)
                            ? (c.reversed ? -1.f : 1.f)
                            : (c.reversed ?  1.f : 0.f);
        out.push_back({ id, rev_val, REAL32 });
    }

    return out;
}

RcParamDialect RcCalibration::dialect_for(MAV_AUTOPILOT autopilot)
{
    return (autopilot == MAV_AUTOPILOT_PX4) ? RcParamDialect::PX4
                                            : RcParamDialect::ArduPilot;
}
