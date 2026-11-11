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

// What one vehicle's protocol loop tells the operator: the log lines and tones
// for what the vehicle said or did. Reporting only — nothing here sends.

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>

#include "backend/mission/watch.hpp"
#include "backend/vehicle/state.hpp"

class VehicleReports {
public:
    explicit VehicleReports(uint8_t sysid) : sysid_(sysid) {}

    // Once per pass: new STATUSTEXTs, and the one-time lines for the first
    // heartbeat, the firmware version and the clock sync.
    void tick(const VehicleState& vs, const std::deque<StatusText>& texts);

    // COMMAND_ACK. Logged, and toned when `tones`, only for the commands a
    // person pressed a button for.
    void command_ack(const CommandAck& ack, bool tones) const;

    // PARAM_EXT_ACK — the extended parameter protocol's reply. Unlike a
    // PARAM_SET, which is answered with a PARAM_VALUE echo the table picks up
    // on its own, this one goes nowhere unless it is logged: it names the
    // parameter and says outright whether the write took.
    void param_ext_ack(const ParamExtAck& ack, bool tones) const;

    // What one list's MissionWatch decided this pass.
    void mission(bool fence, const MissionWatch::Actions& a,
                 const MissionWatch::Progress& p, const VehicleState& vs, bool tones) const;

private:
    uint8_t sysid_;
    size_t  last_status_count_ = 0;
    bool    logged_heartbeat_  = false;
    bool    logged_fw_info_    = false;
    // When AUTOPILOT_VERSION came in; the firmware line waits on the banner
    // from then, a little, since only the banner names the firmware for sure.
    std::chrono::steady_clock::time_point fw_info_at_{};
    bool    logged_timesync_   = false;
};
