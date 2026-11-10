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

// MavlinkSender: parameter traffic — fetches, reads and writes.
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

void MavlinkSender::request_param_list(uint8_t tsys, uint8_t tcomp)
{
    // PARAM_REQUEST_LIST (#21) — FC responds with a PARAM_VALUE stream
    mavlink_message_t msg;
    std::lock_guard<std::mutex> _tx(tx_mtx_);
    mavlink_msg_param_request_list_pack_status(gcs_sysid(), GCS_COMPID, &tx_status_, &msg, tsys, tcomp);

    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    const uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);

    param_list_seq_.fetch_add(1, std::memory_order_release);

    std::lock_guard<std::mutex> lk(mtx_);
    queue_.emplace(buf, buf + len);
}

uint32_t MavlinkSender::param_list_seq() const
{
    return param_list_seq_.load(std::memory_order_acquire);
}

void MavlinkSender::request_param_read(uint8_t tsys, uint8_t tcomp,
                                        int16_t param_index)
{
    // PARAM_REQUEST_READ (#20) — empty param_id means "look up by index"
    mavlink_message_t msg;
    std::lock_guard<std::mutex> _tx(tx_mtx_);
    mavlink_msg_param_request_read_pack_status(gcs_sysid(), GCS_COMPID, &tx_status_, &msg,
                                             tsys, tcomp, "", param_index);
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    const uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
    std::lock_guard<std::mutex> lk(mtx_);
    queue_.emplace(buf, buf + len);
}

void MavlinkSender::request_param_read(uint8_t tsys, uint8_t tcomp,
                                        const char* param_id)
{
    // An index of -1 means "look up by name".
    mavlink_message_t msg;
    std::lock_guard<std::mutex> _tx(tx_mtx_);
    mavlink_msg_param_request_read_pack_status(gcs_sysid(), GCS_COMPID, &tx_status_, &msg,
                                             tsys, tcomp, param_id, -1);
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    const uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
    std::lock_guard<std::mutex> lk(mtx_);
    queue_.emplace(buf, buf + len);
}

void MavlinkSender::set_param(uint8_t tsys, uint8_t tcomp,
                               const char* param_id, float value, uint8_t param_type)
{
    // PARAM_SET (#23) — FC echoes the accepted value back as PARAM_VALUE
    mavlink_message_t msg;
    std::lock_guard<std::mutex> _tx(tx_mtx_);
    mavlink_msg_param_set_pack_status(gcs_sysid(), GCS_COMPID, &tx_status_, &msg,
                                    tsys, tcomp, param_id, value, param_type);

    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    const uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);

    std::lock_guard<std::mutex> lk(mtx_);
    queue_.emplace(buf, buf + len);
}

// PARAM_EXT_SET carries its value as raw bytes rather than a float, so the
// caller's number has to be narrowed to the parameter's declared type first and
// then copied out little-endian — MAVLink is little-endian on the wire, and the
// payload is opaque bytes that no field-level byte swap will touch.
static void encode_param_ext_value(char out[128], float value, uint8_t ext_type)
{
    std::memset(out, 0, 128);

    auto put = [&](const void* src, size_t n) { std::memcpy(out, src, n); };

    switch (ext_type) {
    case MAV_PARAM_EXT_TYPE_UINT8:  { uint8_t  v = (uint8_t) value; put(&v, 1); break; }
    case MAV_PARAM_EXT_TYPE_INT8:   { int8_t   v = (int8_t)  value; put(&v, 1); break; }
    case MAV_PARAM_EXT_TYPE_UINT16: { uint16_t v = (uint16_t)value; put(&v, 2); break; }
    case MAV_PARAM_EXT_TYPE_INT16:  { int16_t  v = (int16_t) value; put(&v, 2); break; }
    case MAV_PARAM_EXT_TYPE_UINT32: { uint32_t v = (uint32_t)value; put(&v, 4); break; }
    case MAV_PARAM_EXT_TYPE_INT32:  { int32_t  v = (int32_t) value; put(&v, 4); break; }
    case MAV_PARAM_EXT_TYPE_UINT64: { uint64_t v = (uint64_t)value; put(&v, 8); break; }
    case MAV_PARAM_EXT_TYPE_INT64:  { int64_t  v = (int64_t) value; put(&v, 8); break; }
    case MAV_PARAM_EXT_TYPE_REAL64: { double   v = (double)  value; put(&v, 8); break; }
    case MAV_PARAM_EXT_TYPE_CUSTOM:
        // CUSTOM is an opaque string the component defines for itself. There is
        // no meaning to give a float here, so the payload stays zeroed rather
        // than inventing a text encoding the receiver never agreed to.
        break;
    case MAV_PARAM_EXT_TYPE_REAL32:
    default:                        { float    v =           value; put(&v, 4); break; }
    }
}

void MavlinkSender::set_param_ext(uint8_t tsys, uint8_t tcomp,
                                   const char* param_id, float value,
                                   uint8_t param_type)
{
    char payload[128];
    encode_param_ext_value(payload, value, param_type);

    mavlink_message_t msg;
    std::lock_guard<std::mutex> _tx(tx_mtx_);
    mavlink_msg_param_ext_set_pack_status(gcs_sysid(), GCS_COMPID, &tx_status_, &msg,
                                        tsys, tcomp, param_id, payload, param_type);

    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    const uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);

    std::lock_guard<std::mutex> lk(mtx_);
    queue_.emplace(buf, buf + len);
}
