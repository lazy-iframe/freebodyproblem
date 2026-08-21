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
#include <vector>

#include "mavlink_parser.hpp"   // RcChannels, MAV_AUTOPILOT

// ─────────────────────────────────────────────────────────────────────────────
// RC calibration — measure each channel's travel from the raw receiver stream,
// then hand back the parameter writes that record it on the vehicle.
//
// This holds no link and sends nothing. It is fed RcChannels and produces
// RcParamWrite; the caller owns the MavlinkSender and decides whether those
// writes go out as PARAM_SET or PARAM_EXT_SET. That split is what makes the
// measurement testable without a vehicle, and what keeps the UI free of any
// opinion about parameter naming.
// ─────────────────────────────────────────────────────────────────────────────

// Which flight stack's parameter names to write. The measurement is identical
// either way; only the names it lands under differ.
enum class RcParamDialect {
    ArduPilot,   // RCn_MIN / RCn_MAX / RCn_TRIM / RCn_REVERSED
    PX4,         // RCn_MIN / RCn_MAX / RCn_TRIM / RCn_REV
};

// One parameter write, named and typed. `type` is the classic MAV_PARAM_TYPE;
// a caller committing over PARAM_EXT_SET maps it to the matching
// MAV_PARAM_EXT_TYPE, which for everything here is the same numeric value.
struct RcParamWrite {
    std::string id;
    float       value = 0.f;
    uint8_t     type  = 9;   // MAV_PARAM_TYPE_REAL32
};

// One channel's measured travel.
struct RcChannelCal {
    uint16_t min      = 0;
    uint16_t max      = 0;
    uint16_t trim     = 0;
    bool     reversed = false;
    bool     seen     = false;   // carried at least one plausible pulse
};

class RcCalibration {
public:
    // Sanity window for a receiver pulse. Anything outside it is a dropout, a
    // failsafe hold or a decode error, and must not be allowed to become a
    // channel's endpoint — one stray 0 recorded as a minimum is a channel the
    // vehicle then believes has 1100 µs of travel it does not have.
    static constexpr uint16_t PWM_FLOOR = 800;
    static constexpr uint16_t PWM_CEIL  = 2200;

    // Travel below this is a channel that was never moved: a switch left alone,
    // an unused slot sitting at a constant value, or a stick the operator
    // forgot. Writing its endpoints would pin it to a sliver of its real range.
    static constexpr uint16_t MIN_TRAVEL_US = 200;

    enum class Phase {
        Idle,     // nothing running; result() is empty
        Center,   // sticks at neutral, waiting for capture_center()
        Sweep,    // widening every channel's min/max as the operator moves them
        Review,   // sweep stopped; the result is inspectable and committable
    };

    // Start a run. Any previous result is discarded. `channel_count` is what the
    // receiver reports — sweeping channels the vehicle does not have would just
    // produce writes for parameters nobody reads.
    void begin(int channel_count);

    // Take the current positions as neutral and move to Sweep.
    // Returns false (and changes nothing) outside Center, or if the frame holds
    // no plausible pulse at all — capturing a trim from a dead receiver is the
    // one way to silently write a whole radio's worth of wrong centres.
    bool capture_center(const RcChannels& rc);

    // Feed every RC update. Widens min/max in Sweep and is ignored in every
    // other phase, so a caller can hand it the stream unconditionally.
    void update(const RcChannels& rc);

    // Stop widening and move to Review. Returns false outside Sweep.
    bool finish();

    // Throw the run away and return to Idle.
    void cancel();

    Phase phase()         const { return phase_; }
    int   channel_count() const { return channel_count_; }

    // 0-based. Out-of-range reads return a default-constructed entry rather
    // than reading past the array, so a UI drawing a fixed grid stays simple.
    const RcChannelCal& channel(int idx) const;

    // A channel with enough travel to be worth writing.
    bool channel_usable(int idx) const;
    int  usable_count() const;

    // A channel that is carrying a signal at all — it has shown at least one
    // plausible pulse since the run began. This is the honest denominator for
    // "how much of the sweep is done": a receiver reports its full channel
    // count whether or not a transmitter is filling every slot, so measuring
    // progress against channel_count() would stall a perfectly finished
    // calibration at eight bars out of eighteen.
    bool channel_active(int idx) const;
    int  active_count() const;

    // Reversal cannot be measured by a sweep — the two endpoints are the same
    // pair of numbers whichever way the stick travels to reach them. It is the
    // operator's call, taken from watching the bar move, and this is where they
    // record it. Seed it from the vehicle's existing RCn_REVERSED before the
    // operator sees it, so an already-reversed channel does not silently
    // un-reverse itself on commit.
    void set_reversed(int idx, bool rev);

    // ArduPilot expects the throttle channel's trim at the bottom of its travel
    // rather than at the stick's rest position, because a throttle's "neutral"
    // is off. Naming the channel here makes that one substitution happen inside
    // the result instead of in whatever code assembles the writes.
    // 1-based; 0 means no throttle channel is known and every trim is the
    // captured centre.
    void set_throttle_channel(int channel_1based);
    int  throttle_channel() const { return throttle_channel_; }

    // The writes this run implies, in channel order, skipping channels that
    // were never really moved. Empty outside Review.
    std::vector<RcParamWrite> params(RcParamDialect dialect) const;

    // Which naming a vehicle wants, from its HEARTBEAT autopilot field.
    // Anything that is not PX4 gets the ArduPilot names: they are what this GCS
    // is built against, and a wrong guess writes parameters that simply do not
    // exist rather than the wrong value into ones that do.
    static RcParamDialect dialect_for(MAV_AUTOPILOT autopilot);

private:
    Phase phase_            = Phase::Idle;
    int   channel_count_    = 0;
    int   throttle_channel_ = 0;

    RcChannelCal chans_[RcChannels::MAX_CHANNELS];
};
