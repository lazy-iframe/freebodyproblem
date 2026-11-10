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

#include "backend/mavlink/signing.hpp"

#include <cstring>
#include <mutex>

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
        if (signing_link_) sync_signing();

        int parsed = 0;
        mavlink_message_t msg;
        mavlink_status_t  st;

        for (size_t i = 0; i < len; ++i) {
            uint8_t result;
            if (status_.signing && status_.signature_wait > 0) {
                // A signature's bytes: the last of them checks it against the
                // streams all links share.
                std::lock_guard<std::mutex> lk(signing::streams_mutex());
                result = mavlink_frame_char_buffer(&rxmsg_, &status_, buf[i], &msg, &st);
            } else {
                result = mavlink_frame_char_buffer(&rxmsg_, &status_, buf[i], &msg, &st);
            }
            if (result == MAVLINK_FRAMING_OK) {
                // A signature that passed moves this link's clock on, and the
                // clock everything is signed with: a vehicle ahead of this ground
                // station's clock would refuse it otherwise.
                if (status_.signing && (msg.incompat_flags & MAVLINK_IFLAG_SIGNED))
                    signing::observe_timestamp(signing_.timestamp);

                // Radios make RADIO_STATUS themselves, and cannot sign it.
                if (status_.signing && !(msg.incompat_flags & MAVLINK_IFLAG_SIGNED) &&
                    signing::require_signed(signing_link_) &&
                    msg.msgid != MAVLINK_MSG_ID_RADIO_STATUS) {
                    ++unsigned_dropped_;
                    continue;
                }
                ++total_messages_;
                ++parsed;
                cb(static_cast<const mavlink_message_t&>(msg));
            } else if (result == MAVLINK_FRAMING_BAD_SIGNATURE) {
                if (!(msg.incompat_flags & MAVLINK_IFLAG_SIGNED)) {
                    ++unsigned_dropped_;
                } else if (signing_.last_status == MAVLINK_SIGNING_STATUS_OLD_TIMESTAMP) {
                    // Signed with this key, so it is the vehicle, but its clock is
                    // far behind: an autopilot with no GPS, restarted after a long
                    // while off. See take_stale_signed().
                    ++stale_signed_;
                    stale_pending_ = true;
                } else {
                    ++bad_signature_;
                }
            } else if (result == MAVLINK_FRAMING_BAD_CRC) {
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

    // Check signatures on this framer, with this connection's key. Left unset where
    // what is fed in is not live traffic.
    void set_signing_link(signing::LinkKey* link) { signing_link_ = link; }

    // Dropped for failing a signature, a replay among them: a wrong key, a forgery.
    uint64_t bad_signature()    const { return bad_signature_;    }
    // Dropped for having none, under signing::require_signed(). Both since this
    // connection's key was last set.
    uint64_t unsigned_dropped() const { return unsigned_dropped_; }

    // True once after a frame correctly signed but too old to accept. A vehicle takes
    // its clock from what it hears, so one signed frame from here puts it right; but
    // with none of its frames accepted the fleet has no vehicle to send that from, and
    // the link would stay silent both ways for good. The caller answers.
    bool take_stale_signed()
    {
        const bool r = stale_pending_;
        stale_pending_ = false;
        return r;
    }

private:
    void sync_signing()
    {
        const uint64_t gen = signing::link_generation(signing_link_);
        if (gen == signing_gen_) return;
        signing_gen_ = gen;

        // Drops under the old key are no news about this one. A key put right should
        // not leave its link showing a warning.
        bad_signature_ = unsigned_dropped_ = stale_signed_ = 0;

        status_.signing         = nullptr;
        status_.signing_streams = nullptr;
        std::memset(&signing_, 0, sizeof(signing_));

        if (signing::copy_key(signing_link_, signing_.secret_key)) {
            signing_.timestamp                = signing::next_timestamp();
            signing_.accept_unsigned_callback = &signing::accept_unverified;
            status_.signing         = &signing_;
            status_.signing_streams = signing::streams();
        }
    }

    mavlink_message_t rxmsg_{};
    mavlink_status_t  status_{};

    signing::LinkKey* signing_link_ = nullptr;
    uint64_t          signing_gen_ = 0;
    mavlink_signing_t signing_{};   // flags left 0: this one is never signed with
    uint64_t          bad_signature_    = 0;
    uint64_t          unsigned_dropped_ = 0;
    uint64_t          stale_signed_     = 0;
    bool              stale_pending_    = false;

    uint64_t total_bytes_    = 0;
    uint64_t total_messages_ = 0;
    uint64_t parse_errors_   = 0;
    uint64_t unknown_msgs_   = 0;
};
