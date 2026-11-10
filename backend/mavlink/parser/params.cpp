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

// MavlinkParser: parameter traffic — PARAM_VALUE and PARAM_EXT_ACK.
// Every MAVLink message received is decoded by MavlinkParser; handle() in
// parser.cpp dispatches here.

#include "backend/mavlink/parser/parser.hpp"
#include "backend/mission/geofence.hpp"
#include "backend/vehicle/firmware_profile.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "backend/mavlink/signing.hpp"
#include "frontend/source/app_log.hpp"   // gcs_log — thread-safe, see the header

void MavlinkParser::on_param_ext_ack(const mavlink_message_t& msg)
{
    mavlink_param_ext_ack_t pa;
    mavlink_msg_param_ext_ack_decode(&msg, &pa);

    ParamExtAck entry{};
    // Neither field is guaranteed NUL-terminated when it fills its array.
    std::memcpy(entry.param_id, pa.param_id, 16);
    entry.param_id[16] = '\0';
    std::memcpy(entry.value, pa.param_value, 128);
    entry.value[128] = '\0';
    entry.param_type = pa.param_type;
    entry.result     = pa.param_result;
    param_ext_acks_.push_back(entry);
}

void MavlinkParser::on_param_value(const mavlink_message_t& msg)
{
    mavlink_param_value_t pv;
    mavlink_msg_param_value_decode(&msg, &pv);
    ParamEntry entry{};
    // param_id is 16 chars, not necessarily null-terminated
    std::memcpy(entry.param_id, pv.param_id, 16);
    entry.param_id[16] = '\0';
    entry.value = pv.param_value;
    entry.type  = pv.param_type;
    entry.index = pv.param_index;
    params_[entry.param_id]     = entry;
    apply_fence_param(state_.fence_params, entry.param_id, entry.value);
    state_.param_count          = pv.param_count;
    state_.params_received      = (uint16_t)params_.size();
    ++state_.params_generation;
}
