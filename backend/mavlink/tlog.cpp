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


#include "backend/mavlink/tlog.hpp"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <thread>

#include "frontend/source/app_log.hpp"   // gcs_log — thread-safe, see the header

namespace {

// What a log may hold in memory while the disk is not keeping up. A vehicle on
// a fast link streams some tens of kilobytes a second, so this is minutes of
// backlog: reaching it means the disk has stopped, not slowed.
constexpr size_t PENDING_MAX = 8 * 1024 * 1024;

constexpr size_t STAMP_LEN = 8;

std::string       g_dir;   // under the registry's lock
std::atomic<bool> g_enabled{false};

// Never destroyed. The writer thread is detached and outlives main(), and a
// registry torn down by static destruction would be pulled from under it.
struct Registry {
    std::mutex                       mtx;
    std::vector<std::weak_ptr<Tlog>> logs;
};
Registry& registry()
{
    static Registry* r = new Registry;
    return *r;
}

} // namespace

void Tlog::set_dir(std::string dir)
{
    std::lock_guard<std::mutex> lk(registry().mtx);
    g_dir = std::move(dir);
}
void Tlog::set_enabled(bool on) { g_enabled.store(on, std::memory_order_relaxed); }
bool Tlog::enabled()            { return g_enabled.load(std::memory_order_relaxed); }

std::shared_ptr<Tlog> Tlog::open(const std::string& stem)
{
    const std::time_t now = std::time(nullptr);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &now);
#else
    localtime_r(&now, &tm);
#endif
    char when[32];
    std::strftime(when, sizeof(when), "%Y-%m-%d_%H-%M-%S", &tm);

    Registry& r = registry();
    std::lock_guard<std::mutex> lk(r.mtx);
    std::shared_ptr<Tlog> log(new Tlog(
        (std::filesystem::path(g_dir) / (std::string(when) + "_")).string(), stem));
    static bool started = false;
    if (!started) {
        started = true;
        std::thread(&Tlog::writer_loop).detach();
    }
    r.logs.push_back(log);
    return log;
}

Tlog::~Tlog() { close(); }

// ── Any thread ────────────────────────────────────────────────────────────────

uint8_t* Tlog::record(size_t frame_len)
{
    if (pending_.size() + STAMP_LEN + frame_len > PENDING_MAX) {
        overflowed_ = true;
        return nullptr;
    }
    const uint64_t us = (uint64_t)std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    const size_t at = pending_.size();
    pending_.resize(at + STAMP_LEN + frame_len);
    uint8_t* p = pending_.data() + at;
    for (int i = 0; i < 8; ++i) p[i] = (uint8_t)(us >> (56 - 8 * i));
    return p + STAMP_LEN;
}

void Tlog::add(const uint8_t* frame, size_t len)
{
    if (!enabled() || failed_.load(std::memory_order_relaxed) || len == 0) return;
    std::lock_guard<std::mutex> lk(mtx_);
    if (uint8_t* p = record(len)) std::memcpy(p, frame, len);
}

// The frame is put back together by hand rather than with
// mavlink_msg_to_send_buffer(), which trims trailing zeros from a MAVLink 2
// payload. A sender is supposed to have trimmed already, but one that has not
// computed its checksum — and its signature — over the bytes it did send, and
// trimming them here would log a frame that no longer checks.
void Tlog::add(const mavlink_message_t& msg)
{
    if (!enabled() || failed_.load(std::memory_order_relaxed)) return;

    const bool   v1     = msg.magic == MAVLINK_STX_MAVLINK1;
    const bool   sig    = !v1 && (msg.incompat_flags & MAVLINK_IFLAG_SIGNED);
    const size_t head   = v1 ? 6 : 10;
    const size_t len    = head + msg.len + 2 + (sig ? MAVLINK_SIGNATURE_BLOCK_LEN : 0);

    std::lock_guard<std::mutex> lk(mtx_);
    uint8_t* p = record(len);
    if (!p) return;

    p[0] = msg.magic;
    p[1] = msg.len;
    if (v1) {
        p[2] = msg.seq;
        p[3] = msg.sysid;
        p[4] = msg.compid;
        p[5] = (uint8_t)msg.msgid;
    } else {
        p[2] = msg.incompat_flags;
        p[3] = msg.compat_flags;
        p[4] = msg.seq;
        p[5] = msg.sysid;
        p[6] = msg.compid;
        p[7] = (uint8_t)msg.msgid;
        p[8] = (uint8_t)(msg.msgid >> 8);
        p[9] = (uint8_t)(msg.msgid >> 16);
    }
    p += head;
    std::memcpy(p, _MAV_PAYLOAD(&msg), msg.len);
    p += msg.len;
    p[0] = (uint8_t)msg.checksum;
    p[1] = (uint8_t)(msg.checksum >> 8);
    if (sig) std::memcpy(p + 2, msg.signature, MAVLINK_SIGNATURE_BLOCK_LEN);
}

// ── Writer thread ─────────────────────────────────────────────────────────────

void Tlog::close() { flush(true); }

// Closed, moved and left for the next flush to open again: a file that is open
// cannot be renamed on Windows, and appending is how it was being written anyway.
void Tlog::rename(const std::string& stem)
{
    std::lock_guard<std::mutex> io(io_mtx_);
    const std::string to = prefix_ + stem + ".tlog";
    if (closed_ || to == path_) return;
    if (file_) std::fclose(file_);
    file_ = nullptr;

    std::error_code ec;
    if (std::filesystem::exists(path_, ec)) {
        std::filesystem::rename(path_, to, ec);
        if (ec) {
            gcs_log("telemetry log keeps its old name: %s", path_.c_str());
            return;
        }
    }
    path_ = to;
}

void Tlog::flush(bool closing)
{
    std::lock_guard<std::mutex> io(io_mtx_);
    if (closed_) return;

    bool overflowed = false;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        pending_.swap(writing_);
        std::swap(overflowed, overflowed_);
    }
    if (overflowed)
        gcs_log("telemetry log is not keeping up, records dropped: %s", path_.c_str());

    if (!writing_.empty() && !failed_.load(std::memory_order_relaxed)) {
        if (!file_) {
            std::error_code ec;
            std::filesystem::create_directories(
                std::filesystem::path(path_).parent_path(), ec);
            file_ = std::fopen(path_.c_str(), "ab");
            // Records arrive here a second's worth at a time, so stdio's own
            // buffer would only be a second copy of each of them.
            if (file_) std::setvbuf(file_, nullptr, _IONBF, 0);
        }
        if (!file_ ||
            std::fwrite(writing_.data(), 1, writing_.size(), file_) != writing_.size()) {
            // Said once, and add() stops collecting: a full disk is not going
            // to be less full a second from now.
            failed_.store(true, std::memory_order_relaxed);
            gcs_log("telemetry log stopped, cannot write %s", path_.c_str());
        }
    }
    writing_.clear();

    if (closing || failed_.load(std::memory_order_relaxed)) {
        if (file_) std::fclose(file_);
        file_   = nullptr;
        closed_ = closing;
    }
}

void Tlog::writer_loop()
{
    Registry& r = registry();
    std::vector<std::shared_ptr<Tlog>> live;
    for (;;) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        {
            // Only the list is copied under the lock. A vehicle is discovered
            // under the fleet's lock, which the UI takes every frame, and must
            // not find this one held across a disk write.
            std::lock_guard<std::mutex> lk(r.mtx);
            r.logs.erase(std::remove_if(r.logs.begin(), r.logs.end(),
                                        [&](const std::weak_ptr<Tlog>& w) {
                                            auto s = w.lock();
                                            if (!s) return true;
                                            live.push_back(std::move(s));
                                            return false;
                                        }),
                         r.logs.end());
        }
        for (auto& log : live) log->flush(false);
        live.clear();
    }
}
