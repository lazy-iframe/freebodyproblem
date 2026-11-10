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

// MavlinkParser: the mission protocol's incoming half — downloads, upload requests, ACKs.
// Every MAVLink message received is decoded by MavlinkParser; handle() in
// parser.cpp dispatches here.

#include "backend/mavlink/parser/parser.hpp"
#include "backend/mission/download.hpp"
#include "backend/vehicle/firmware_profile.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "backend/mavlink/signing.hpp"
#include "frontend/source/app_log.hpp"   // gcs_log — thread-safe, see the header

void MavlinkParser::on_mission_count(const mavlink_message_t& msg)
{
    mavlink_mission_count_t mc;
    mavlink_msg_mission_count_decode(&msg, &mc);
    // The plan and the fence each have a list of their own on the same
    // messages; rally points are not handled.
    const bool fence = mc.mission_type == MAV_MISSION_TYPE_FENCE;
    if (!fence && mc.mission_type != MAV_MISSION_TYPE_MISSION) return;

    MissionDownload& dl = fence ? fence_dl_ : mission_dl_;
    const bool more = dl.begin(mc.count);
    publish_download_progress(fence);
    if (more)
        mission_reqs_.push_back({msg.sysid, msg.compid, 0, mc.mission_type});
    else
        finish_download(fence);
}

void MavlinkParser::on_mission_item_int(const mavlink_message_t& msg)
{
    mavlink_mission_item_int_t mi;
    mavlink_msg_mission_item_int_decode(&msg, &mi);
    const bool fence = mi.mission_type == MAV_MISSION_TYPE_FENCE;
    if (!fence && mi.mission_type != MAV_MISSION_TYPE_MISSION) return;

    MissionItem item{};
    item.seq            = mi.seq;
    item.lat            = mi.x / 1e7;
    item.lon            = mi.y / 1e7;
    item.alt            = mi.z;
    item.param1         = mi.param1;
    item.param2         = mi.param2;
    item.param3         = mi.param3;
    item.param4         = mi.param4;
    item.command        = mi.command;
    item.frame          = mi.frame;
    item.is_current     = (mi.current != 0);
    item.autocontinue   = (mi.autocontinue != 0);

    MissionDownload& dl = fence ? fence_dl_ : mission_dl_;
    if (!dl.on_item(item)) return;
    publish_download_progress(fence);
    if (!dl.complete())
        mission_reqs_.push_back({msg.sysid, msg.compid,
                                 (uint16_t)(mi.seq + 1), mi.mission_type});
    else
        finish_download(fence);
}

void MavlinkParser::on_mission_request_int(const mavlink_message_t& msg)
{
    // FC is requesting a mission item from GCS (upload protocol, new style)
    mavlink_mission_request_int_t mr;
    mavlink_msg_mission_request_int_decode(&msg, &mr);
    if (mr.seq == 0) {
        if (mr.mission_type == MAV_MISSION_TYPE_FENCE)
            state_.fence_upload_status = VehicleState::UploadStatus::InProgress;
        else
            state_.upload_status = VehicleState::UploadStatus::InProgress;
    }
    item_reqs_.push_back({msg.sysid, msg.compid, mr.seq, mr.mission_type});
}

void MavlinkParser::on_mission_request(const mavlink_message_t& msg)
{
    // FC is requesting a mission item from GCS (upload protocol, legacy style)
    // Some ArduPilot versions send this instead of MISSION_REQUEST_INT.
    mavlink_mission_request_t mr;
    mavlink_msg_mission_request_decode(&msg, &mr);
    if (mr.seq == 0) {
        if (mr.mission_type == MAV_MISSION_TYPE_FENCE)
            state_.fence_upload_status = VehicleState::UploadStatus::InProgress;
        else
            state_.upload_status = VehicleState::UploadStatus::InProgress;
    }
    item_reqs_.push_back({msg.sysid, msg.compid, mr.seq, mr.mission_type});
}

void MavlinkParser::on_mission_ack(const mavlink_message_t& msg)
{
    // FC acknowledges completed upload
    mavlink_mission_ack_t ma;
    mavlink_msg_mission_ack_decode(&msg, &ma);
    if (ma.mission_type == MAV_MISSION_TYPE_FENCE) {
        state_.fence_upload_ack_result = ma.type;
        state_.fence_upload_status = (ma.type == MAV_MISSION_ACCEPTED)
                                   ? VehicleState::UploadStatus::Accepted
                                   : VehicleState::UploadStatus::Failed;
        return;
    }
    if (ma.mission_type != MAV_MISSION_TYPE_MISSION) return;
    state_.upload_ack_result = ma.type;
    state_.upload_status = (ma.type == MAV_MISSION_ACCEPTED)
                         ? VehicleState::UploadStatus::Accepted
                         : VehicleState::UploadStatus::Failed;
}

// ── Mission download ──────────────────────────────────────────────────────────

void MavlinkParser::publish_download_progress(bool fence)
{
    const MissionDownload& dl = fence ? fence_dl_ : mission_dl_;
    (fence ? state_.fence_count    : state_.mission_count)    = dl.count();
    (fence ? state_.fence_received : state_.mission_received) = dl.received();
}

void MavlinkParser::finish_download(bool fence)
{
    if (fence) {
        // No home slot: that is the plan's alone.
        state_.fence     = fence_dl_.take(false);
        state_.has_fence = true;
        ++state_.fence_generation;
        return;
    }
    state_.mission = mission_dl_.take(firmware_profile(state_.autopilot).mission_home_at_seq0(),
                                      &state_.mission_home, &state_.has_mission_home);
    state_.has_mission = true;
    ++state_.mission_generation;
}
