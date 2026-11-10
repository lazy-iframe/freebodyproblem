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
#include "backend/vehicle/identity.hpp"

// ── ModeListWalk ──────────────────────────────────────────────────────────────

void ModeListWalk::on_probe_sent(Clock::time_point now)
{
    last_req_ = now;
    ++probes_;
}

void ModeListWalk::restart()
{
    cursor_ = 0;
    sweeps_ = 0;
    probes_ = 0;
    logged_ = false;
}

ModeListWalk::Action ModeListWalk::tick(const VehicleState& ms, Clock::time_point now)
{
    Action a;
    const double since = std::chrono::duration<double>(now - last_req_).count();

    if (!ms.modes_complete && sweeps_ < MAX_SWEEPS) {
        if (ms.modes_expected == 0) {
            // No number_modes yet: either the probe was lost or the firmware
            // does not implement the protocol at all.
            if (since >= PROBE_S && probes_ < MAX_PROBES) {
                a.kind    = Action::Kind::Probe;
                last_req_ = now;
                ++probes_;
            }
        } else if (since >= REQ_S) {
            // Next index we have not received yet. Indices already in hand are
            // skipped, so a retry sweep only re-asks for the genuine gaps.
            int next = 0;
            for (int i = cursor_ + 1; i <= ms.modes_expected; ++i) {
                if (!ms.has_mode_index(static_cast<uint8_t>(i))) {
                    next = i;
                    break;
                }
            }
            if (next != 0) {
                a.kind    = Action::Kind::RequestIndex;
                a.index   = static_cast<uint8_t>(next);
                cursor_   = next;
                last_req_ = now;
            } else {
                // Reached the end with gaps still open — sweep again.
                cursor_ = 0;
                ++sweeps_;
            }
        }
    }

    if (ms.modes_complete && !logged_) {
        logged_          = true;
        a.just_completed = true;
    }
    return a;
}

// ── VersionProbe ──────────────────────────────────────────────────────────────

void VersionProbe::on_first_request(Clock::time_point now)
{
    last_req_ = now;
    probes_   = 1;
}

VersionProbe::Ask VersionProbe::tick(bool burst_sent, bool has_fw_info, Clock::time_point now)
{
    if (!burst_sent || has_fw_info || probes_ >= MAX_PROBES) return Ask::None;
    if (std::chrono::duration<double>(now - last_req_).count() < PROBE_S) return Ask::None;
    const Ask ask = (probes_ % 2 == 0) ? Ask::RequestMessage : Ask::RequestCapabilities;
    last_req_ = now;
    ++probes_;
    return ask;
}
