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

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <mavlink/ardupilotmega/mavlink.h>

// Tlog — one vehicle's telemetry log, in the .tlog format Mission Planner,
// MAVProxy, QGroundControl and pymavlink all read.
//
// The format is the wire with a clock beside it: each record is eight bytes of
// big-endian microseconds since the Unix epoch, then one MAVLink frame exactly
// as it crossed the link — both directions, signature included where there is
// one. No header, no index, no footer, so a log cut short by a crash is whole
// up to its last record.
//
// add() is the only part on a hot path, and it is a clock read and a copy of
// the frame into memory under a lock nothing else holds for longer than that.
// The disk is touched by one background thread, shared by every log, that wakes
// once a second: a link thread never waits on a write, and a slow disk costs
// log records rather than telemetry.
class Tlog {
public:
    // Where logs go, and whether any are kept. Both may change at any time. A
    // new folder is where logs opened from then on go, and one already open
    // stays where it is; the switch applies to every log at once, and one that
    // is already open simply has a gap in it.
    static void set_dir(std::string dir);
    static void set_enabled(bool on);
    static bool enabled();

    // `stem` names the vehicle within the file name: "v1_sys1_alpha" gives
    // 2026-11-12_16-05-33_v1_sys1_alpha.tlog. No file exists until there is
    // something to put in it.
    static std::shared_ptr<Tlog> open(const std::string& stem);

    // The vehicle has been given another name: the file takes it, wherever it
    // has got to, and goes on from where it was.
    void rename(const std::string& stem);

    // A frame received, and a frame sent as the transport took it. Any thread.
    void add(const mavlink_message_t& msg);
    void add(const uint8_t* frame, size_t len);

    // Writes what is left and closes the file. The owner calls this when the
    // vehicle goes, so the tail of a log does not depend on the writer thread
    // getting one more turn before the process exits.
    void close();

    ~Tlog();
    Tlog(const Tlog&)            = delete;
    Tlog& operator=(const Tlog&) = delete;

private:
    Tlog(std::string prefix, const std::string& stem)
        : prefix_(std::move(prefix)), path_(prefix_ + stem + ".tlog") {}

    // Room for one record at the end of pending_, its timestamp filled in.
    // Null when the record is not to be kept. Called with mtx_ held.
    uint8_t* record(size_t frame_len);
    void     flush(bool closing);   // writer thread, and close()
    static void writer_loop();

    const std::string prefix_;   // folder and start time: all of the path but the stem

    std::mutex           mtx_;       // pending_
    std::vector<uint8_t> pending_;   // records not yet on disk
    bool                 overflowed_ = false;

    std::mutex           io_mtx_;    // everything below
    std::string          path_;
    std::vector<uint8_t> writing_;   // swapped with pending_, so neither reallocates
    std::FILE*           file_   = nullptr;
    bool                 closed_ = false;
    std::atomic<bool>    failed_{false};
};
