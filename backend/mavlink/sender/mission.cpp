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

// MavlinkSender: the mission protocol's outgoing half — list requests, uploads, ACKs.
// Every MAVLink message sent is packed and queued by MavlinkSender; the queue,
// transport and retries are in sender.cpp.

#include "backend/mavlink/sender/sender.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <mutex>
#include <mavlink/ardupilotmega/mavlink.h>
#include "frontend/source/app_log.hpp"   // gcs_log — thread-safe, see the header

using Clock = std::chrono::steady_clock;

void MavlinkSender::request_mission_list(uint8_t tsys, uint8_t tcomp, uint8_t type)
{
    mavlink_message_t msg;
    std::lock_guard<std::mutex> _tx(tx_mtx_);
    mavlink_msg_mission_request_list_pack_status(gcs_sysid(), GCS_COMPID, &tx_status_, &msg,
                                                tsys, tcomp, type);
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    const uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
    std::lock_guard<std::mutex> lk(mtx_);
    queue_.emplace(buf, buf + len);
}

void MavlinkSender::request_mission_item(uint8_t tsys, uint8_t tcomp, uint16_t seq,
                                         uint8_t type)
{
    mavlink_message_t msg;
    std::lock_guard<std::mutex> _tx(tx_mtx_);
    mavlink_msg_mission_request_int_pack_status(gcs_sysid(), GCS_COMPID, &tx_status_, &msg,
                                              tsys, tcomp, seq, type);
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    const uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
    std::lock_guard<std::mutex> lk(mtx_);
    queue_.emplace(buf, buf + len);
}

void MavlinkSender::send_mission_ack(uint8_t tsys, uint8_t tcomp, uint8_t type)
{
    mavlink_message_t msg;
    std::lock_guard<std::mutex> _tx(tx_mtx_);
    mavlink_msg_mission_ack_pack_status(gcs_sysid(), GCS_COMPID, &tx_status_, &msg,
                                       tsys, tcomp,
                                       MAV_MISSION_ACCEPTED,
                                       type,
                                       0);
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    const uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
    std::lock_guard<std::mutex> lk(mtx_);
    queue_.emplace(buf, buf + len);
}

void MavlinkSender::start_upload(uint8_t tsys, uint8_t tcomp,
                                  std::vector<MissionItem> items, uint8_t type)
{
    if (type > MAV_MISSION_TYPE_RALLY) return;
    mavlink_message_t msg;
    std::lock_guard<std::mutex> _tx(tx_mtx_);
    mavlink_msg_mission_count_pack_status(gcs_sysid(), GCS_COMPID, &tx_status_, &msg,
                                        tsys, tcomp,
                                        (uint16_t)items.size(),
                                        type,
                                        0 /* opaque_id */);
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    const uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);

    std::lock_guard<std::mutex> lk(mtx_);
    queue_.emplace(buf, buf + len);
    upload_items_[type] = std::move(items);
}

void MavlinkSender::send_mission_item_at(uint8_t tsys, uint8_t tcomp, uint16_t seq,
                                         uint8_t type)
{
    std::lock_guard<std::mutex> lk(mtx_);
    if (type > MAV_MISSION_TYPE_RALLY || seq >= upload_items_[type].size()) return;

    const MissionItem& it = upload_items_[type][seq];
    mavlink_message_t msg;
    std::lock_guard<std::mutex> _tx(tx_mtx_);
    mavlink_msg_mission_item_int_pack_status(
        gcs_sysid(), GCS_COMPID, &tx_status_, &msg,
        tsys, tcomp,
        it.seq,
        it.frame,
        it.command,
        it.is_current ? 1 : 0,
        it.autocontinue ? 1 : 0,
        it.param1, it.param2, it.param3, it.param4,
        (int32_t)(it.lat * 1e7),
        (int32_t)(it.lon * 1e7),
        it.alt,
        type
    );
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    const uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
    queue_.emplace(buf, buf + len);
}

void MavlinkSender::clear_upload(uint8_t type)
{
    if (type > MAV_MISSION_TYPE_RALLY) return;
    std::lock_guard<std::mutex> lk(mtx_);
    upload_items_[type].clear();
}

bool MavlinkSender::upload_active(uint8_t type) const
{
    if (type > MAV_MISSION_TYPE_RALLY) return false;
    std::lock_guard<std::mutex> lk(mtx_);
    return !upload_items_[type].empty();
}
