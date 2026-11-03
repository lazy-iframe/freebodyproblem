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


// frontend/terrain.cpp
//
// The terrain tile store. Shaped like the map's imagery pipeline — a queue,
// a few workers each holding one persistent connection, a disk cache — but
// the result stays on the CPU as heights instead of going to the GPU, so any
// thread can read it.

#include "terrain.hpp"
#include "cache_dir.hpp"

// The httplib CMake target already defines this; guard so the two definitions
// (empty here vs. 1 on the command line) do not collide.
#ifndef CPPHTTPLIB_OPENSSL_SUPPORT
#  define CPPHTTPLIB_OPENSSL_SUPPORT
#endif
#include "httplib.h"

#include "stb_image.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>

#ifndef M_PI
#  define M_PI 3.14159265358979323846
#endif

namespace {


// Two is plenty: a mission needs a handful of tiles, and the imagery's four
// workers are already sharing the link.
constexpr int N_WORKERS = 2;

// How long a tile that failed stays failed before a lookup asks for it again.
constexpr auto RETRY_AFTER = std::chrono::seconds(30);

struct Key {
    int x, y, z;
    bool operator==(const Key& o) const { return x == o.x && y == o.y && z == o.z; }
};
struct KeyHash {
    size_t operator()(const Key& k) const noexcept {
        size_t h = (size_t)(unsigned)k.z;
        h = h * 2654435761u ^ (size_t)(unsigned)k.x;
        h = h * 2654435761u ^ (size_t)(unsigned)k.y;
        return h;
    }
};

struct Entry {
    enum class State { Pending, Ready, Failed } state = State::Pending;
    std::shared_ptr<const TerrainTile>          tile;
    std::chrono::steady_clock::time_point       failed_at{};
};

// The source in use, with the generation it was set in. Workers take a copy
// with each tile they fetch, so a tile asked for before a switch is fetched
// from the source it was asked of and then thrown away, not filed under the
// new one.
struct Source {
    std::string     url = TERRAIN_DEFAULT_URL;
    TerrainEncoding enc = TerrainEncoding::Terrarium;
    std::string     cache_dir;   // this source's folder
    uint64_t        gen = 0;
};

struct Job { Key k; Source src; };

std::mutex                              g_mtx;
std::condition_variable                 g_cv;
std::unordered_map<Key, Entry, KeyHash> g_tiles;
std::queue<Job>                         g_queue;
std::vector<std::thread>                g_workers;
Source                                  g_src;
std::string                             g_attribution = TERRAIN_DEFAULT_ATTRIBUTION;
std::atomic<uint64_t>                   g_gen{0};
bool                                    g_started = false;
std::atomic<bool>                       g_running{false};

// Each source caches in a folder of its own: the default where it always
// has, any other under a name from its URL and encoding, so heights read one
// way are never read back the other.
std::string cache_dir_for(const std::string& url, TerrainEncoding enc)
{
    const std::string base = app_cache_dir("terrain");
    if (url == TERRAIN_DEFAULT_URL && enc == TerrainEncoding::Terrarium) return base;
    uint64_t h = 1469598103934665603ull;
    for (unsigned char c : url) h = (h ^ c) * 1099511628211ull;
    h = (h ^ (unsigned)enc) * 1099511628211ull;
    char name[24];
    snprintf(name, sizeof(name), "%016llx", (unsigned long long)h);
    return base + "/" + name;
}

std::string cache_path(const Source& s, const Key& k)
{
    return s.cache_dir + "/" + std::to_string(k.z) + "/" + std::to_string(k.x)
         + "/" + std::to_string(k.y) + ".png";
}

std::vector<uint8_t> disk_load(const Source& s, const Key& k)
{
    std::ifstream f(cache_path(s, k), std::ios::binary);
    if (!f) return {};
    return {std::istreambuf_iterator<char>(f), {}};
}

void disk_save(const Source& s, const Key& k, const std::string& body)
{
    const std::string path = cache_path(s, k);
    mkdirs_for_file(path);
    std::ofstream f(path, std::ios::binary);
    if (f) f.write(body.data(), (std::streamsize)body.size());
}

float decode_px(const uint8_t* p, TerrainEncoding enc)
{
    if (enc == TerrainEncoding::TerrainRgb)
        return ((float)p[0] * 65536.0f + (float)p[1] * 256.0f + (float)p[2]) * 0.1f - 10000.0f;
    return (float)p[0] * 256.0f + (float)p[1] + (float)p[2] / 256.0f - 32768.0f;
}

// Tile image to heights. A 512 px tile is halved by averaging each 2×2 into
// one; anything but 256 or 512 square is refused — an error page or a
// truncated download must not become a flat tile kilometres underground.
std::shared_ptr<TerrainTile> decode(const Key& k, TerrainEncoding enc, const uint8_t* data, size_t sz)
{
    int w, h, ch;
    uint8_t* px = stbi_load_from_memory(data, (int)sz, &w, &h, &ch, 3);
    if (!px) return nullptr;
    std::shared_ptr<TerrainTile> t;
    if (w == h && (w == TERRAIN_TILE_PX || w == 2 * TERRAIN_TILE_PX)) {
        const int f = w / TERRAIN_TILE_PX;
        t    = std::make_shared<TerrainTile>();
        t->x = k.x; t->y = k.y; t->z = k.z;
        t->h.resize((size_t)TERRAIN_TILE_PX * TERRAIN_TILE_PX);
        for (int y = 0; y < TERRAIN_TILE_PX; ++y)
            for (int x = 0; x < TERRAIN_TILE_PX; ++x) {
                float sum = 0.0f;
                for (int dy = 0; dy < f; ++dy)
                    for (int dx = 0; dx < f; ++dx)
                        sum += decode_px(px + ((size_t)(y * f + dy) * w + (x * f + dx)) * 3, enc);
                t->h[(size_t)y * TERRAIN_TILE_PX + x] = sum / (float)(f * f);
            }
    }
    stbi_image_free(px);
    return t;
}

// Split "https://host/path/{z}/..." into host and path template.
void split_url(const std::string& url, std::string& host, std::string& path)
{
    const size_t scheme = url.find("://");
    const size_t slash  = url.find('/', scheme == std::string::npos ? 0 : scheme + 3);
    if (slash == std::string::npos) { host = url; path = "/"; return; }
    host = url.substr(0, slash);
    path = url.substr(slash);
}

std::string tile_path(std::string tmpl, const Key& k)
{
    const auto sub = [&](const char* tok, int v) {
        for (size_t p; (p = tmpl.find(tok)) != std::string::npos;)
            tmpl.replace(p, strlen(tok), std::to_string(v));
    };
    sub("{z}", k.z); sub("{x}", k.x); sub("{y}", k.y);
    return tmpl;
}

void finish(const Key& k, uint64_t gen, std::shared_ptr<const TerrainTile> tile)
{
    std::lock_guard<std::mutex> lk(g_mtx);
    if (gen != g_src.gen) return;   // asked of a source since switched away from
    Entry& e = g_tiles[k];
    if (tile) {
        e.state = Entry::State::Ready;
        e.tile  = std::move(tile);
    } else {
        e.state     = Entry::State::Failed;
        e.failed_at = std::chrono::steady_clock::now();
    }
}

void worker()
{
    // One persistent connection, rebuilt when the source's host changes.
    std::string                      cur_host;
    std::unique_ptr<httplib::Client> cli;

    while (true) {
        Job job;
        {
            std::unique_lock<std::mutex> lk(g_mtx);
            g_cv.wait(lk, [] { return !g_queue.empty() || !g_running; });
            if (!g_running) return;
            job = std::move(g_queue.front());
            g_queue.pop();
        }
        const Key&    k   = job.k;
        const Source& src = job.src;

        const std::vector<uint8_t> cached = disk_load(src, k);
        if (!cached.empty()) {
            if (auto t = decode(k, src.enc, cached.data(), cached.size())) { finish(k, src.gen, std::move(t)); continue; }
            // A corrupt cache entry: fetch it again.
        }

        std::string host, path;
        split_url(src.url, host, path);
        if (host != cur_host || !cli) {
            cur_host = host;
            cli = std::make_unique<httplib::Client>(host);
            cli->set_connection_timeout(8);
            cli->set_read_timeout(10);
            cli->set_follow_location(true);
            cli->set_default_headers({{"User-Agent", "freebodyproblem-gcs/1.0"}});
        }
        auto res = cli->Get(tile_path(path, k));
        if (!res || res->status != 200) { finish(k, src.gen, nullptr); continue; }

        auto t = decode(k, src.enc, reinterpret_cast<const uint8_t*>(res->body.data()), res->body.size());
        if (t) disk_save(src, k, res->body);   // only what decoded is worth keeping
        finish(k, src.gen, std::move(t));
    }
}

// Called with g_mtx held.
void start_locked()
{
    if (g_started) return;
    g_started = true;
    g_running = true;
    if (g_src.cache_dir.empty()) g_src.cache_dir = cache_dir_for(g_src.url, g_src.enc);
    for (int i = 0; i < N_WORKERS; ++i)
        g_workers.emplace_back(worker);
}

} // namespace

std::shared_ptr<const TerrainTile> terrain_tile(int z, int x, int y)
{
    if (z < 0 || z > TERRAIN_MAX_ZOOM) return nullptr;
    const int n = 1 << z;
    if (x < 0 || x >= n || y < 0 || y >= n) return nullptr;

    const Key k{x, y, z};
    std::lock_guard<std::mutex> lk(g_mtx);
    if (g_started && !g_running) return nullptr;   // shut down

    auto it = g_tiles.find(k);
    if (it != g_tiles.end()) {
        Entry& e = it->second;
        if (e.state == Entry::State::Ready)   return e.tile;
        if (e.state == Entry::State::Pending) return nullptr;
        if (std::chrono::steady_clock::now() - e.failed_at < RETRY_AFTER) return nullptr;
        e.state = Entry::State::Pending;
    } else {
        g_tiles.emplace(k, Entry{});
    }
    start_locked();
    g_queue.push({ k, g_src });
    g_cv.notify_one();
    return nullptr;
}

void terrain_set_source(const std::string& url, TerrainEncoding enc, const std::string& attribution)
{
    std::lock_guard<std::mutex> lk(g_mtx);
    g_attribution = attribution;
    if (url == g_src.url && enc == g_src.enc && !g_src.cache_dir.empty()) return;
    g_src.url       = url;
    g_src.enc       = enc;
    g_src.cache_dir = cache_dir_for(url, enc);
    g_src.gen       = ++g_gen;
    g_tiles.clear();
    g_queue = {};
}

std::string terrain_attribution()
{
    std::lock_guard<std::mutex> lk(g_mtx);
    return g_attribution;
}

uint64_t terrain_generation() { return g_gen.load(); }

std::optional<float> terrain_amsl(double lat, double lon)
{
    if (!std::isfinite(lat) || !std::isfinite(lon)) return std::nullopt;
    lat = std::clamp(lat, -85.0511, 85.0511);
    lon = std::clamp(lon, -180.0, 180.0);

    // Web Mercator: the point's position in tiles at the lookup zoom.
    const int    z  = TERRAIN_LOOKUP_ZOOM;
    const double n  = (double)(1 << z);
    const double r  = lat * M_PI / 180.0;
    const double tx = (lon + 180.0) / 360.0 * n;
    const double ty = (1.0 - std::log(std::tan(r) + 1.0 / std::cos(r)) / M_PI) / 2.0 * n;

    const int x = std::clamp((int)std::floor(tx), 0, (int)n - 1);
    const int y = std::clamp((int)std::floor(ty), 0, (int)n - 1);
    auto tile = terrain_tile(z, x, y);
    if (!tile) return std::nullopt;

    // Pixel space, with each sample at its pixel's centre. Within half a pixel
    // of the tile's edge the nearest edge pixel stands in for the neighbour
    // tile's — a few metres of slope at most, not worth a second fetch.
    const double fx = std::clamp((tx - x) * TERRAIN_TILE_PX - 0.5, 0.0, (double)TERRAIN_TILE_PX - 1);
    const double fy = std::clamp((ty - y) * TERRAIN_TILE_PX - 0.5, 0.0, (double)TERRAIN_TILE_PX - 1);
    const int    x0 = (int)fx, y0 = (int)fy;
    const int    x1 = std::min(x0 + 1, TERRAIN_TILE_PX - 1);
    const int    y1 = std::min(y0 + 1, TERRAIN_TILE_PX - 1);
    const float  ax = (float)(fx - x0), ay = (float)(fy - y0);

    const float top = tile->at(x0, y0) * (1 - ax) + tile->at(x1, y0) * ax;
    const float bot = tile->at(x0, y1) * (1 - ax) + tile->at(x1, y1) * ax;
    return top * (1 - ay) + bot * ay;
}

void terrain_shutdown()
{
    {
        std::lock_guard<std::mutex> lk(g_mtx);
        if (!g_started || !g_running) return;
        g_running = false;
    }
    g_cv.notify_all();
    for (auto& t : g_workers)
        if (t.joinable()) t.join();
    g_workers.clear();
}
