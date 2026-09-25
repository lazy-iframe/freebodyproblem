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
                // A frame carrying a message this dialect does not define
                // fails its CRC for a reason that is not corruption: the check
                // is seeded with a per-message constant, and there is none for
                // a message we have never heard of. Counting those as link
                // errors makes the error counter measure our own vocabulary
                // rather than the quality of the wire — and PX4 sends two such
                // messages continuously (GNSS_INTEGRITY #441 and
                // ESTIMATOR_SENSOR_FUSION_STATUS #514, both development-dialect
                // and neither in ardupilotmega), so the count climbs forever on
                // a link that is in fact perfect.
                //
                // The id is readable either way: it comes out of the header,
                // which is parsed before the CRC is checked.
                if (mavlink_get_msg_entry(msg.msgid) == nullptr)
                    ++unknown_msgs_;
                else
                    ++parse_errors_;
            }
        }

        return parsed;
    }

    uint64_t total_bytes()    const { return total_bytes_;    }
    uint64_t total_messages() const { return total_messages_; }

    // Frames that failed their CRC and carried a message this build knows.
    // This is the one that means something is wrong with the link.
    uint64_t parse_errors()   const { return parse_errors_;   }

    // Well-formed frames skipped for carrying a message this dialect does not
    // define. Not an error — nothing is lost that this GCS could have used.
    uint64_t unknown_msgs()   const { return unknown_msgs_;   }

private:
    mavlink_message_t rxmsg_{};
    mavlink_status_t  status_{};

    uint64_t total_bytes_    = 0;
    uint64_t total_messages_ = 0;
    uint64_t parse_errors_   = 0;
    uint64_t unknown_msgs_   = 0;
};
