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


#include "backend/mavlink/signing.hpp"

#include <nlohmann/json.hpp>
#include <openssl/rand.h>
#include <openssl/sha.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>

#ifndef _WIN32
#include <sys/stat.h>
#endif

using json = nlohmann::json;

namespace signing {

struct LinkKey {
    std::string           name;
    uint8_t               id = 1;
    uint8_t               key[32] = {};     // under g_mtx
    bool                  has_key = false;  // under g_mtx
    std::atomic<bool>     active{false};
    std::atomic<bool>     require{false};
    std::atomic<uint64_t> generation{1};
};

namespace {

constexpr uint64_t EPOCH_2015_S = 1420070400;

std::mutex             g_mtx;           // the links, the salt and the path
std::string            g_path;
uint32_t               g_salt    = 0;
std::map<std::string, std::unique_ptr<LinkKey>> g_links;
std::map<std::string, std::array<uint8_t, 32>>  g_saved;
std::atomic<uint64_t>  g_generation{1};
std::atomic<uint64_t>  g_last_ts{0};

std::mutex                g_streams_mtx;
mavlink_signing_streams_t g_streams{};

uint64_t clock_now()
{
    const auto s = std::chrono::duration_cast<std::chrono::microseconds>(
                       std::chrono::system_clock::now().time_since_epoch()).count();
    const uint64_t us = s > 0 ? (uint64_t)s : 0;
    const uint64_t offset = EPOCH_2015_S * 1000000ull;
    return us > offset ? (us - offset) / 10 : 0;
}

std::string to_hex(const uint8_t* p, size_t n)
{
    static const char D[] = "0123456789abcdef";
    std::string s;
    s.reserve(n * 2);
    for (size_t i = 0; i < n; ++i) { s += D[p[i] >> 4]; s += D[p[i] & 15]; }
    return s;
}

bool from_hex(const std::string& s, uint8_t* out, size_t n)
{
    if (s.size() != n * 2) return false;
    const auto v = [](char c) {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        return -1;
    };
    for (size_t i = 0; i < n; ++i) {
        const int a = v(s[i * 2]), b = v(s[i * 2 + 1]);
        if (a < 0 || b < 0) return false;
        out[i] = (uint8_t)(a << 4 | b);
    }
    return true;
}

// With g_mtx held. Whole or not at all: a crash in the middle of this must not lose
// the key, or leave it where another user can read it.
void save_locked()
{
    if (g_path.empty()) return;
    json j;
    j["salt"]      = g_salt;
    j["timestamp"] = g_last_ts.load();
    j["links"]     = json::object();
    for (const auto& [name, l] : g_links) {
        if (!l->has_key && !l->require) continue;
        json e;
        e["require_signed"] = l->require.load();
        if (l->has_key) e["key"] = to_hex(l->key, sizeof(l->key));
        j["links"][name] = e;
    }
    j["saved_keys"] = json::object();
    for (const auto& [name, k] : g_saved) j["saved_keys"][name] = to_hex(k.data(), k.size());

    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(g_path).parent_path(), ec);

    const std::string tmp = g_path + ".tmp";
    {
        std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
#ifndef _WIN32
        chmod(tmp.c_str(), 0600);   // before anything secret is in it
#endif
        if (!f) return;
        f << j.dump();
        if (!f.good()) return;
    }
    std::filesystem::rename(tmp, g_path, ec);
    if (ec) std::filesystem::remove(tmp, ec);
}

// With g_mtx held.
LinkKey* link_locked(const std::string& name)
{
    auto& l = g_links[name];
    if (!l) {
        l = std::make_unique<LinkKey>();
        l->name = name;
        uint32_t h = 2166136261u ^ g_salt;
        for (const unsigned char c : name) { h ^= c; h *= 16777619u; }
        h ^= h >> 16;
        l->id = (uint8_t)((h ^ (h >> 8)) & 0xFF);
    }
    return l.get();
}

// With g_mtx held.
std::string fingerprint_of(const uint8_t* key)
{
    unsigned char h[SHA256_DIGEST_LENGTH];
    SHA256(key, 32, h);
    return to_hex(h, 4);
}

void reset_streams()
{
    std::lock_guard<std::mutex> lk(g_streams_mtx);
    std::memset(&g_streams, 0, sizeof(g_streams));
}

} // namespace

void init(const std::string& path)
{
    std::lock_guard<std::mutex> lk(g_mtx);
    g_path = path;
    bool ok = false;
    json links;
    {
        std::ifstream f(path, std::ios::binary);
        if (f) {
            try {
                const json j = json::parse(f);
                g_salt    = j.value("salt", (uint32_t)0);
                g_last_ts = j.value("timestamp", (uint64_t)0);
                if (j.contains("links") && j["links"].is_object()) links = j["links"];
                if (j.contains("saved_keys") && j["saved_keys"].is_object())
                    for (auto it = j["saved_keys"].begin(); it != j["saved_keys"].end(); ++it) {
                        std::array<uint8_t, 32> k{};
                        if (it.value().is_string() &&
                            from_hex(it.value().get<std::string>(), k.data(), k.size()))
                            g_saved[it.key()] = k;
                    }
                ok = true;
            } catch (const std::exception&) {
                // Unreadable: start with no keys rather than refuse to start. The
                // operator sees that there are none, and any vehicle signing says so.
            }
        }
    }
    if (!ok || g_salt == 0) {
        if (RAND_bytes(reinterpret_cast<unsigned char*>(&g_salt), sizeof(g_salt)) != 1) g_salt = 1;
        save_locked();
    }
    if (links.is_object()) {
        for (auto it = links.begin(); it != links.end(); ++it) {
            if (!it.value().is_object()) continue;
            LinkKey* l = link_locked(it.key());
            l->require = it.value().value("require_signed", false);
            const std::string hex = it.value().value("key", std::string());
            l->has_key = !hex.empty() && from_hex(hex, l->key, sizeof(l->key));
            l->active  = l->has_key;
            ++l->generation;
        }
    }
    ++g_generation;
}

void tick()
{
    using Clock = std::chrono::steady_clock;
    static Clock::time_point s_saved = Clock::now();
    if (Clock::now() - s_saved < std::chrono::seconds(60)) return;
    s_saved = Clock::now();
    if (g_last_ts.load(std::memory_order_relaxed) != 0) save_now();
}

void save_now()
{
    std::lock_guard<std::mutex> lk(g_mtx);
    save_locked();
}

LinkKey* for_link(const std::string& link_name)
{
    std::lock_guard<std::mutex> lk(g_mtx);
    return link_locked(link_name);
}

bool set_passphrase(LinkKey* link, const std::string& passphrase)
{
    if (!link || passphrase.empty()) return false;
    {
        std::lock_guard<std::mutex> lk(g_mtx);
        SHA256(reinterpret_cast<const unsigned char*>(passphrase.data()), passphrase.size(), link->key);
        link->has_key = true;
        link->active  = true;
        save_locked();
    }
    // What was heard under the old key says nothing about the new one.
    reset_streams();
    ++link->generation;
    ++g_generation;
    return true;
}

void forget_key(LinkKey* link)
{
    if (!link) return;
    {
        std::lock_guard<std::mutex> lk(g_mtx);
        std::memset(link->key, 0, sizeof(link->key));
        link->has_key = false;
        link->active  = false;
        link->require = false;   // with no key to check by, it would drop everything
        save_locked();
    }
    reset_streams();
    ++link->generation;
    ++g_generation;
}

bool active(const LinkKey* link) { return link && link->active.load(std::memory_order_relaxed); }

std::string fingerprint(LinkKey* link)
{
    if (!link) return {};
    std::lock_guard<std::mutex> lk(g_mtx);
    if (!link->has_key) return {};
    return fingerprint_of(link->key);
}

std::vector<SavedKey> saved_keys()
{
    std::lock_guard<std::mutex> lk(g_mtx);
    std::vector<SavedKey> out;
    out.reserve(g_saved.size());
    for (const auto& [name, k] : g_saved) out.push_back({ name, fingerprint_of(k.data()) });
    return out;
}

bool save_key(LinkKey* link, const std::string& name)
{
    if (!link || name.empty()) return false;
    std::lock_guard<std::mutex> lk(g_mtx);
    if (!link->has_key) return false;
    std::array<uint8_t, 32> k;
    std::memcpy(k.data(), link->key, k.size());
    g_saved[name] = k;
    save_locked();
    return true;
}

bool has_saved_key(const std::string& name)
{
    std::lock_guard<std::mutex> lk(g_mtx);
    return g_saved.count(name) != 0;
}

void delete_saved_key(const std::string& name)
{
    std::lock_guard<std::mutex> lk(g_mtx);
    if (g_saved.erase(name)) save_locked();
}

bool use_saved_key(LinkKey* link, const std::string& name)
{
    if (!link) return false;
    {
        std::lock_guard<std::mutex> lk(g_mtx);
        const auto it = g_saved.find(name);
        if (it == g_saved.end()) return false;
        std::memcpy(link->key, it->second.data(), sizeof(link->key));
        link->has_key = true;
        link->active  = true;
        save_locked();
    }
    // As with a new passphrase: nothing heard under the old key counts for this one.
    reset_streams();
    ++link->generation;
    ++g_generation;
    return true;
}

std::string saved_name(LinkKey* link)
{
    if (!link) return {};
    std::lock_guard<std::mutex> lk(g_mtx);
    if (!link->has_key) return {};
    for (const auto& [name, k] : g_saved)
        if (std::memcmp(k.data(), link->key, k.size()) == 0) return name;
    return {};
}

std::string generate_passphrase()
{
    unsigned char r[32];
    if (RAND_bytes(r, sizeof(r)) != 1) return {};
    return to_hex(r, sizeof(r));
}

bool require_signed(const LinkKey* link) { return link && link->require.load(std::memory_order_relaxed); }

void set_require_signed(LinkKey* link, bool on)
{
    if (!link) return;
    link->require = on;
    save_now();
}

uint64_t next_timestamp()
{
    const uint64_t now = clock_now();
    uint64_t last = g_last_ts.load(std::memory_order_relaxed);
    uint64_t next;
    do {
        next = std::max(now, last + 1);
    } while (!g_last_ts.compare_exchange_weak(last, next, std::memory_order_relaxed));
    return next;
}

void observe_timestamp(uint64_t ts)
{
    uint64_t last = g_last_ts.load(std::memory_order_relaxed);
    while (ts > last && !g_last_ts.compare_exchange_weak(last, ts, std::memory_order_relaxed)) {}
}

uint8_t link_id(const LinkKey* link) { return link ? link->id : 0; }

bool sign_frame(LinkKey* link, const std::vector<uint8_t>& frame, std::vector<uint8_t>& out)
{
    if (!active(link)) return false;
    if (frame.size() < MAVLINK_NUM_NON_PAYLOAD_BYTES || frame[0] != MAVLINK_STX) return false;
    if (frame[2] & MAVLINK_IFLAG_SIGNED) return false;

    const size_t  header_len  = MAVLINK_NUM_HEADER_BYTES;
    const uint8_t payload_len = frame[1];
    if (frame.size() != header_len + payload_len + MAVLINK_NUM_CHECKSUM_BYTES) return false;

    const uint32_t msgid = frame[7] | (uint32_t)frame[8] << 8 | (uint32_t)frame[9] << 16;
    const mavlink_msg_entry_t* entry = mavlink_get_msg_entry(msgid);
    if (!entry) return false;

    mavlink_signing_t s{};
    s.flags     = MAVLINK_SIGNING_FLAG_SIGN_OUTGOING;
    s.link_id   = link->id;
    s.timestamp = next_timestamp();
    {
        std::lock_guard<std::mutex> lk(g_mtx);
        if (!link->has_key) return false;
        std::memcpy(s.secret_key, link->key, sizeof(link->key));
    }

    out.assign(frame.begin(), frame.end());
    out[2] |= MAVLINK_IFLAG_SIGNED;

    // The flag is part of the header, so the checksum has to be worked out again.
    uint16_t crc = crc_calculate(&out[1], (uint16_t)(header_len - 1));
    crc_accumulate_buffer(&crc, reinterpret_cast<const char*>(&out[header_len]), payload_len);
    crc_accumulate(entry->crc_extra, &crc);
    out[header_len + payload_len]     = (uint8_t)(crc & 0xFF);
    out[header_len + payload_len + 1] = (uint8_t)(crc >> 8);

    uint8_t sig[MAVLINK_SIGNATURE_BLOCK_LEN];
    mavlink_sign_packet(&s, sig, out.data(), (uint8_t)header_len, &out[header_len], payload_len,
                        &out[header_len + payload_len]);
    out.insert(out.end(), sig, sig + sizeof(sig));

    std::memset(s.secret_key, 0, sizeof(s.secret_key));
    return true;
}

bool copy_key(LinkKey* link, uint8_t out[32])
{
    if (!link) return false;
    std::lock_guard<std::mutex> lk(g_mtx);
    if (!link->has_key) return false;
    std::memcpy(out, link->key, sizeof(link->key));
    return true;
}

uint64_t generation() { return g_generation.load(std::memory_order_relaxed); }

uint64_t link_generation(const LinkKey* link)
{
    return link ? link->generation.load(std::memory_order_relaxed) : 0;
}

mavlink_signing_streams_t* streams()       { return &g_streams; }
std::mutex&                streams_mutex() { return g_streams_mtx; }

bool accept_unverified(const mavlink_status_t* status, uint32_t)
{
    // The library asks this of a frame with no signature and of one whose signature
    // has failed, in the one callback. The second is never let through, or a forger
    // need only sign with anything: the parser is waiting on the signature only then.
    return status->parse_state != MAVLINK_PARSE_STATE_SIGNATURE_WAIT &&
           status->parse_state != MAVLINK_PARSE_STATE_SIGNATURE_WAIT_BAD_CRC;
}

} // namespace signing
