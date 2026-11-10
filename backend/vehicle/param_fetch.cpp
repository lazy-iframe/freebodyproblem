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
#include "backend/vehicle/param_fetch.hpp"

#include <unordered_set>

ParamFetch::Publish ParamFetch::on_state(const VehicleState& ps, Clock::time_point now)
{
    // Track last-param-arrival time for the retransmit logic in tick().
    if (ps.params_generation != last_params_gen_) {
        last_params_gen_ = ps.params_generation;
        last_param_time_ = now;
    }

    Publish p;
    const bool complete = ps.param_count > 0 && ps.params_received >= ps.param_count;
    const bool changed  = ps.params_generation != known_params_gen_;
    p.just_completed = complete && !complete_latch_;
    complete_latch_  = complete;

    p.republish = complete && changed;
    if (p.republish) known_params_gen_ = ps.params_generation;
    return p;
}

bool ParamFetch::open_if_requested(uint32_t list_seq, Clock::time_point now)
{
    if (list_seq == seen_list_seq_) return false;
    seen_list_seq_   = list_seq;
    active_          = true;
    stall_rounds_    = 0;
    stall_mark_      = 0;
    last_param_time_ = now;
    return true;
}

ParamFetch::Actions ParamFetch::tick(const VehicleState& ps,
                                     const std::unordered_map<std::string, ParamEntry>& params,
                                     Clock::time_point now)
{
    Actions a;

    if (active_ && ps.param_count > 0 && ps.params_received >= ps.param_count)
        active_ = false;   // complete

    if (!(active_ && ps.param_count > 0 && ps.params_received < ps.param_count))
        return a;
    if (std::chrono::duration<double>(now - last_param_time_).count() < STALL_S)
        return a;

    // Rounds are only counted as lost when nothing at all arrived, so a slow
    // link retries as long as it is moving.
    if (ps.params_received == stall_mark_) ++stall_rounds_;
    else                                   stall_rounds_ = 0;
    stall_mark_ = ps.params_received;

    if (stall_rounds_ >= GIVEUP_ROUNDS) {
        a.kind  = Actions::Kind::Abandoned;
        active_ = false;
    } else {
        std::unordered_set<uint16_t> have;
        have.reserve(params.size());
        for (const auto& [key, e] : params) have.insert(e.index);

        for (uint16_t idx = 0; idx < ps.param_count; ++idx)
            if (have.find(idx) == have.end()) a.missing.push_back(idx);
        a.missing_count = a.missing.size();

        if ((int)a.missing.size() > READ_BURST) {
            a.kind = Actions::Kind::RequestList;
            a.missing.clear();
        } else {
            a.kind = Actions::Kind::RequestReads;
        }
    }
    last_param_time_ = now;   // rate-limit to once per STALL_S
    return a;
}
