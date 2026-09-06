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

#include <cstddef>
#include <cstdint>

#include <mavlink/ardupilotmega/mavlink.h>

// MavlinkFramer — byte stream in, whole MAVLink messages out.
//
// The reassembly buffer lives in this object rather than in the library's
// per-channel statics. mavlink_parse_char() indexes a global array by
// MAVLINK_COMM_n, so two links framing at once on the same channel interleave
// their half-built frames into each other; mavlink_frame_char_buffer() takes
// the buffer as an argument and has no such state. One framer per link is then
// the whole of the synchronisation story for inbound bytes.
//
// Counting lives here too because it is a property of the wire, not of any
// vehicle on it: bytes arriving and frames failing their CRC belong to the
// link, while what a message *means* belongs to whichever vehicle sent it.
class MavlinkFramer {
public:
    // Feeds `len` bytes, invoking cb(const mavlink_message_t&) once per complete
    // frame. Returns the number of frames handed to cb.
    template <class Fn>
    int feed(const uint8_t* buf, size_t len, Fn&& cb)
    {
        total_bytes_ += len;

        int parsed = 0;
        mavlink_message_t msg;
        mavlink_status_t  st;

        for (size_t i = 0; i < len; ++i) {
            const uint8_t result =
                mavlink_frame_char_buffer(&rxmsg_, &status_, buf[i], &msg, &st);
            if (result == MAVLINK_FRAMING_OK) {
                ++total_messages_;
                ++parsed;
                cb(static_cast<const mavlink_message_t&>(msg));
            } else if (result == MAVLINK_FRAMING_BAD_CRC ||
                       result == MAVLINK_FRAMING_BAD_SIGNATURE) {
                ++parse_errors_;
            }
        }

        return parsed;
    }

    uint64_t total_bytes()    const { return total_bytes_;    }
    uint64_t total_messages() const { return total_messages_; }
    uint64_t parse_errors()   const { return parse_errors_;   }

private:
    mavlink_message_t rxmsg_{};
    mavlink_status_t  status_{};

    uint64_t total_bytes_    = 0;
    uint64_t total_messages_ = 0;
    uint64_t parse_errors_   = 0;
};
