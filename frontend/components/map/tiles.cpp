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

// frontend/components/map/tiles.cpp
//
// The map's imagery, for the flat view and the 3D view alike: slippy tiles from the
// provider chosen in frontend/source/tile_source.hpp.
// Tile fetching runs on a pool of background threads; decoded pixels are
// queued for GL texture upload on the main thread.
//
// Each provider's terms differ, and this project has no agreement with any of them;
// see tile_source.hpp. Tiles are kept on disk across sessions where that is allowed.
//
// Performance improvements over v1:
//   - Thread pool (N_WORKERS threads), one persistent SSL connection per worker
//   - Disk tile cache to skip network round-trips on re-visit
//   - Stale-request pruning when zoom changes
//   - Parent-tile fallback rendering while tiles load

#include "frontend/components/map/map_view.hpp"
#include "frontend/components/map/map_internal.hpp"
#include "frontend/source/cache_dir.hpp"

// The httplib CMake target already defines this; guard so the two definitions
// (empty here vs. 1 on the command line) do not collide.
#ifndef CPPHTTPLIB_OPENSSL_SUPPORT
#  define CPPHTTPLIB_OPENSSL_SUPPORT
#endif
#include "httplib.h"

#include "stb_image.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <fstream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

// Windows' <GL/gl.h> needs the WINGDIAPI/APIENTRY macros from <windows.h>.
#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#endif

// macOS ships OpenGL as a framework, so the header is <OpenGL/gl.h>.
#ifdef __APPLE__
#  include <OpenGL/gl.h>
#else
#  include <GL/gl.h>
#endif

#ifndef GL_CLAMP_TO_EDGE
#  define GL_CLAMP_TO_EDGE 0x812F
#endif

#ifndef M_PI
#  define M_PI 3.14159265358979323846
#endif

namespace map_detail {

// ── Disk cache ────────────────────────────────────────────────────────────────

static std::string g_cache_dir;

static void init_cache_dir()
{
    g_cache_dir = app_cache_dir("tiles");
}

// `subdir` is the provider's own folder under the cache.
static std::string tile_cache_path(const std::string& subdir, const TileKey& k)
{
    return g_cache_dir + (subdir.empty() ? "" : "/" + subdir)
         + "/" + std::to_string(k.z)
         + "/" + std::to_string(k.x)
         + "/" + std::to_string(k.y) + ".jpg";
}

// Returns empty vector on cache miss.
static std::vector<uint8_t> disk_load(const std::string& subdir, const TileKey& k)
{
    std::ifstream f(tile_cache_path(subdir, k), std::ios::binary);
    if (!f) return {};
    return {std::istreambuf_iterator<char>(f), {}};
}

// Best-effort write; silently ignores errors.
static void disk_save(const std::string& subdir, const TileKey& k, const void* data, size_t sz)
{
    std::string path = tile_cache_path(subdir, k);
    mkdirs_for_file(path);
    std::ofstream f(path, std::ios::binary);
    if (f) f.write(static_cast<const char*>(data), sz);
}

// ── Tile cache (main thread only) ─────────────────────────────────────────────

std::unordered_map<TileKey, GLuint, TileKeyHash> g_textures;
static std::unordered_set<TileKey, TileKeyHash>          g_pending;

// ── Fetch / upload pipeline ───────────────────────────────────────────────────

struct DecodedTile {
    TileKey              key;
    uint64_t             source_gen = 0;   // the source it was fetched from; see below
    int                  w = 0, h = 0;
    std::vector<uint8_t> rgba;
};

// ── Tile source (written from the main thread, read by workers) ───────────────

static std::mutex       g_tile_cfg_mtx;
static TileSourceConfig g_tile_cfg;
static std::string      g_tile_status;        // for the operator; never holds a key

// Bumped on every change of source, so a tile on its way when the source changed
// is not shown as the new one's.
static std::atomic<uint64_t> g_tile_gen{1};

// Past this zoom the source has no tiles: its last one is fetched and stretched.
static std::atomic<int> g_tile_max_zoom{TILE_DEFAULT_MAX_ZOOM};

// How often one tile may be asked for again after a refusal, a failure to connect or a
// server error. Then it is left alone until the source is applied again: a provider
// that is refusing must not be sent the same requests over and over, which is how an
// app earns a block. Under g_tile_cfg_mtx.
static constexpr int MAX_TILE_ATTEMPTS = 3;
static std::unordered_map<TileKey, int, TileKeyHash> g_tile_attempts;

// Tiles of this source that stayed blank however often they were asked for.
static std::atomic<int> g_tile_blanks{0};

static TileKey capped_tile(TileKey k)
{
    const int over = k.z - g_tile_max_zoom.load();
    if (over > 0) { k.x >>= over; k.y >>= over; k.z -= over; }
    return k;
}

// Google's session and its live credit, all under g_tile_cfg_mtx.
static std::string                           g_google_session;
static int64_t                               g_google_expiry = 0;
static std::string                           g_google_copyright;
static std::chrono::steady_clock::time_point g_google_retry_at{};
static std::chrono::steady_clock::time_point g_google_credit_at{};

// One worker at a time asks for a session; the rest wait for its answer.
static std::mutex g_google_session_mtx;

static void set_tile_status(const std::string& s)
{
    std::lock_guard<std::mutex> lk(g_tile_cfg_mtx);
    g_tile_status = s;
}

// Split a full URL template into host (with scheme) and path template.
// e.g. "https://host/path/{z}/{y}/{x}" → host="https://host", path="/path/{z}/{y}/{x}"
static void parse_tile_url(const std::string& url, std::string& out_host, std::string& out_path)
{
    size_t scheme_end = url.find("://");
    if (scheme_end == std::string::npos) { out_host = ""; out_path = url; return; }
    size_t slash = url.find('/', scheme_end + 3);
    if (slash == std::string::npos) { out_host = url; out_path = "/"; return; }
    out_host = url.substr(0, slash);
    out_path = url.substr(slash);
}

static std::string make_tile_path(const std::string& tmpl, int x, int y, int z)
{
    std::string r = tmpl;
    auto sub = [&](const std::string& tok, int v) {
        size_t p;
        while ((p = r.find(tok)) != std::string::npos)
            r.replace(p, tok.size(), std::to_string(v));
    };
    sub("{z}", z); sub("{x}", x); sub("{y}", y);
    return r;
}

// Number of parallel fetch threads (each holds one persistent connection).
static constexpr int N_WORKERS = 4;

static std::mutex              g_fetch_mtx;
static std::condition_variable g_fetch_cv;
static std::queue<TileKey>     g_fetch_queue;

static std::mutex              g_upload_mtx;
static std::queue<DecodedTile> g_upload_queue;
static std::vector<TileKey>    g_retry_tiles;   // under g_upload_mtx; asked for again when next wanted

static std::atomic<bool>        g_running{false};
static std::vector<std::thread> g_fetch_threads;

// The overlay over the base, where it is not transparent. Same size or not at all.
static void blend_overlay(DecodedTile& base, const DecodedTile& over)
{
    if (base.w != over.w || base.h != over.h) return;
    const size_t n = (size_t)base.w * base.h * 4;
    if (base.rgba.size() != n || over.rgba.size() != n) return;
    for (size_t i = 0; i < n; i += 4) {
        const unsigned a = over.rgba[i + 3];
        for (size_t c = 0; c < 3; ++c)
            base.rgba[i + c] = (uint8_t)((over.rgba[i + c] * a + base.rgba[i + c] * (255 - a)) / 255);
    }
}

// All transparent, or all black: GIBS's empty tile, in PNG and JPEG.
static bool tile_is_blank(const DecodedTile& t)
{
    bool transparent = true, black = true;
    for (size_t i = 0; i + 3 < t.rgba.size() && (transparent || black); i += 4) {
        if (t.rgba[i + 3] != 0) transparent = false;
        if (t.rgba[i] > 2 || t.rgba[i + 1] > 2 || t.rgba[i + 2] > 2) black = false;
    }
    return transparent || black;
}

static bool decode_png(const uint8_t* data, int sz, DecodedTile& out)
{
    int w, h, ch;
    uint8_t* px = stbi_load_from_memory(data, sz, &w, &h, &ch, 4);
    if (!px) return false;
    out.w = w;
    out.h = h;
    out.rgba.assign(px, px + w * h * 4);
    stbi_image_free(px);
    return true;
}

// The session to fetch Google's tiles with, made or renewed if need be. Empty if
// there is none to be had, with the reason in the status, and no second try for a
// while: a bad key must not be sent to Google on every frame.
static std::string google_session(const std::string& key, TileKind kind)
{
    using Clock = std::chrono::steady_clock;
    std::lock_guard<std::mutex> one_at_a_time(g_google_session_mtx);
    {
        std::lock_guard<std::mutex> lk(g_tile_cfg_mtx);
        // Renewed a day early: "two weeks", the documents say, "but this might change".
        const int64_t now = std::chrono::duration_cast<std::chrono::seconds>(
                                std::chrono::system_clock::now().time_since_epoch()).count();
        if (!g_google_session.empty() && (g_google_expiry == 0 || now < g_google_expiry - 86400))
            return g_google_session;
        if (Clock::now() < g_google_retry_at) return {};
    }

    httplib::Client c(TILE_GOOGLE_HOST);
    c.set_connection_timeout(8);
    c.set_read_timeout(10);
    c.set_default_headers({{"User-Agent", tile_user_agent(TILE_GOOGLE_HOST)}});
    const auto res = c.Post(tile_google_session_path(key), tile_google_session_body(kind), "application/json");

    std::string session;
    int64_t     expiry = 0;
    const bool  ok = res && res->status == 200 && tile_google_parse_session(res->body, session, expiry);

    std::lock_guard<std::mutex> lk(g_tile_cfg_mtx);
    if (g_tile_cfg.provider != TileProvider::Google || g_tile_cfg.google_key != key) return {};
    if (!ok) {
        g_tile_status = res ? "Google refused the session (HTTP " + std::to_string(res->status) + ")"
                            : std::string("Google could not be reached");
        g_google_retry_at = Clock::now() + std::chrono::seconds(30);
        return {};
    }
    g_google_session = session;
    g_google_expiry  = expiry;
    return session;
}

// Google's credit depends on where the imagery is. Asked for around the tile just
// fetched, which is near enough to the view in either the 2D or the 3D map, and no
// oftener than this.
static void refresh_google_credit(httplib::Client& c, const std::string& session,
                                  const std::string& key, const TileKey& t)
{
    using Clock = std::chrono::steady_clock;
    {
        std::lock_guard<std::mutex> lk(g_tile_cfg_mtx);
        if (Clock::now() < g_google_credit_at) return;
        g_google_credit_at = Clock::now() + std::chrono::seconds(5);
    }
    constexpr int SPAN = 3;   // tiles either side
    double north, south, east, west;
    tile_frac_to_lat_lon(t.x - SPAN,     t.y - SPAN,     t.z, north, west);
    tile_frac_to_lat_lon(t.x + SPAN + 1, t.y + SPAN + 1, t.z, south, east);

    const auto res = c.Get(tile_google_viewport_path(session, key, t.z, north, south, east, west));
    if (!res || res->status != 200) return;
    const std::string credit = tile_google_parse_copyright(res->body);
    if (credit.empty()) return;
    std::lock_guard<std::mutex> lk(g_tile_cfg_mtx);
    g_google_copyright = credit;
}

// Each worker keeps to one of a provider's numbered servers, so it can hold one
// persistent connection and the load still spreads across them.
static std::string with_server_number(std::string url, int server)
{
    for (size_t p; (p = url.find("{s}")) != std::string::npos;)
        url.replace(p, 3, std::to_string(server));
    return url;
}

static void fetch_worker()
{
    static std::atomic<int> s_next_worker{0};
    const int server = s_next_worker.fetch_add(1) % 4;

    // One persistent connection per worker, rebuilt whenever the tile host changes.
    std::string                      cur_host, overlay_host;
    std::unique_ptr<httplib::Client> cli, overlay_cli;

    auto make_cli = [](const std::string& host) {
        auto c = std::make_unique<httplib::Client>(host);
        c->set_connection_timeout(8);
        c->set_read_timeout(10);
        c->set_follow_location(true);
        c->set_default_headers({{"User-Agent", tile_user_agent(host)}});
        return c;
    };

    while (true) {
        TileKey key;
        {
            std::unique_lock<std::mutex> lk(g_fetch_mtx);
            g_fetch_cv.wait(lk, [] {
                return !g_fetch_queue.empty() || !g_running;
            });
            if (!g_running) break;
            key = g_fetch_queue.front();
            g_fetch_queue.pop();
        }

        TileSourceConfig source;
        uint64_t         gen;
        {
            std::lock_guard<std::mutex> lk(g_tile_cfg_mtx);
            source = g_tile_cfg;
            gen    = g_tile_gen.load();
        }

        // Given back to be asked for again, a limited number of times.
        const auto give_back = [&] {
            bool again;
            {
                std::lock_guard<std::mutex> lk(g_tile_cfg_mtx);
                again = gen == g_tile_gen.load() && ++g_tile_attempts[key] < MAX_TILE_ATTEMPTS;
            }
            if (!again) return;
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            std::lock_guard<std::mutex> ulk(g_upload_mtx);
            g_retry_tiles.push_back(key);
        };

        // Nothing to send until the settings change, and changing them starts over.
        if (const char* problem = tile_source_problem(source)) {
            set_tile_status(problem);
            continue;
        }

        std::string cache_subdir;
        const bool  use_disk = tile_source_cache_dir(source, cache_subdir);

        // Hybrid's second layer, blended onto a base tile however that was come by.
        // Without it the base still shows: streets missing beats no map.
        const std::string overlay_tmpl = tile_source_overlay_template(source);
        const auto add_overlay = [&](DecodedTile& dt) {
            if (overlay_tmpl.empty()) return;
            const std::string dir = tile_source_overlay_cache_dir(source);
            DecodedTile over;
            auto bytes = disk_load(dir, key);
            if (!bytes.empty() && decode_png(bytes.data(), (int)bytes.size(), over)) {
                blend_overlay(dt, over);
                return;
            }
            std::string h, path_t;
            parse_tile_url(overlay_tmpl, h, path_t);
            if (h != overlay_host || !overlay_cli) { overlay_host = h; overlay_cli = make_cli(h); }
            auto res = overlay_cli->Get(make_tile_path(path_t, key.x, key.y, key.z));
            if (!res || res->status != 200) return;
            const auto* b = reinterpret_cast<const uint8_t*>(res->body.data());
            if (!decode_png(b, (int)res->body.size(), over)) return;
            disk_save(dir, key, res->body.data(), res->body.size());
            blend_overlay(dt, over);
        };

        // ── Disk cache ────────────────────────────────────────────────────────
        if (use_disk) {
            auto cached = disk_load(cache_subdir, key);
            if (!cached.empty()) {
                DecodedTile dt;
                dt.key        = key;
                dt.source_gen = gen;
                if (decode_png(cached.data(), (int)cached.size(), dt)) {
                    add_overlay(dt);
                    std::lock_guard<std::mutex> ulk(g_upload_mtx);
                    g_upload_queue.push(std::move(dt));
                    continue;
                }
                // Corrupt cache entry — fall through to network fetch
            }
        }

        // ── Network fetch ─────────────────────────────────────────────────────
        std::string session;
        if (tile_source_uses_google_api(source)) {
            session = google_session(source.google_key, source.kind);
            if (session.empty()) { give_back(); continue; }
        }

        std::string host, path_tmpl;
        parse_tile_url(with_server_number(tile_source_template(source, session), server),
                       host, path_tmpl);

        if (host != cur_host || !cli) {
            cur_host = host;
            cli = make_cli(cur_host);
        }

        std::string path = make_tile_path(path_tmpl, key.x, key.y, key.z);

        auto res = cli->Get(path);

        // GIBS sometimes answers a good request with a blank tile, and the real one
        // on asking again: measured, the same URL gave 107,797 bytes, then 334. A tile
        // with truly nothing in it costs these extra requests.
        const bool gibs = source.provider == TileProvider::NasaGibs;
        const auto blank_reply = [&] {
            DecodedTile probe;
            return res && res->status == 200 &&
                   decode_png(reinterpret_cast<const uint8_t*>(res->body.data()), (int)res->body.size(), probe) &&
                   tile_is_blank(probe);
        };
        for (int retry = 0; gibs && retry < 3 && blank_reply(); ++retry) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            res = cli->Get(path);
        }

        if (!res) {
            set_tile_status("Map source could not be reached");
            give_back();
            continue;
        }
        if (res->status == 401 || res->status == 403 || res->status == 498 || res->status == 499) {
            // A refusal is about the key or the session, not this tile. Esri answers
            // a bad token with 498 or 499.
            set_tile_status("Map source refused the request (HTTP " + std::to_string(res->status) + ")");
            if (tile_source_uses_google_api(source)) {
                std::lock_guard<std::mutex> lk(g_tile_cfg_mtx);
                g_google_session.clear();
                g_google_retry_at = std::chrono::steady_clock::now() + std::chrono::seconds(30);
            }
            give_back();
            continue;
        }
        if (gibs && (res->status == 400 || res->status == 404)) {
            // Too far back or in the future for this layer; the rest of the date's
            // tiles would only say the same.
            set_tile_status("NASA GIBS has no data for that date");
            continue;
        }
        if (res->status >= 500) {
            give_back();
            continue;
        }
        if (res->status != 200) continue;

        if (gen != g_tile_gen.load()) continue;   // a different source now

        DecodedTile dt;
        dt.key        = key;
        dt.source_gen = gen;
        const auto* body = reinterpret_cast<const uint8_t*>(res->body.data());
        if (!decode_png(body, (int)res->body.size(), dt)) continue;

        // After the decode: a 200 that is not an image, like an error or block page,
        // must not be kept as a tile, and nor must a blank that may yet have imagery.
        const bool blank = gibs && tile_is_blank(dt);
        if (blank) ++g_tile_blanks;
        if (use_disk && !blank) disk_save(cache_subdir, key, res->body.data(), res->body.size());

        add_overlay(dt);

        set_tile_status({});
        if (tile_source_uses_google_api(source))
            refresh_google_credit(*cli, session, source.google_key, key);

        std::lock_guard<std::mutex> ulk(g_upload_mtx);
        g_upload_queue.push(std::move(dt));
    }
}

// Upload pending tiles to the GPU.  Must run on the GL thread.
void drain_upload_queue()
{
    std::lock_guard<std::mutex> lk(g_upload_mtx);
    for (const TileKey& k : g_retry_tiles) g_pending.erase(k);
    g_retry_tiles.clear();

    while (!g_upload_queue.empty()) {
        DecodedTile& dt = g_upload_queue.front();
        if (dt.source_gen != g_tile_gen.load()) {
            g_upload_queue.pop();
            continue;
        }

        GLuint tex = 0;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                     dt.w, dt.h, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                     dt.rgba.data());
        glBindTexture(GL_TEXTURE_2D, 0);

        g_textures[dt.key] = tex;
        g_pending.erase(dt.key);
        g_upload_queue.pop();
    }
}

void queue_tile(const TileKey& wanted)
{
    const TileKey key = capped_tile(wanted);
    if (g_textures.count(key) || g_pending.count(key)) return;
    g_pending.insert(key);
    {
        std::lock_guard<std::mutex> lk(g_fetch_mtx);
        g_fetch_queue.push(key);
    }
    g_fetch_cv.notify_one();
}

// Drop queued requests for zoom levels other than keep_zoom.
// Called on the main thread whenever zoom changes.
void flush_stale_requests(int keep_zoom)
{
    std::lock_guard<std::mutex> lk(g_fetch_mtx);
    std::queue<TileKey> keep;
    while (!g_fetch_queue.empty()) {
        TileKey k = g_fetch_queue.front();
        g_fetch_queue.pop();
        if (k.z == keep_zoom || k.z == g_tile_max_zoom.load()) {
            keep.push(k);
        } else {
            // Not going to be fetched — remove from pending so it can be re-queued
            g_pending.erase(k);
        }
    }
    g_fetch_queue = std::move(keep);
}

void ensure_started()
{
    if (g_running.exchange(true)) return;   // already running
    init_cache_dir();
    g_fetch_threads.reserve(N_WORKERS);
    for (int i = 0; i < N_WORKERS; ++i)
        g_fetch_threads.emplace_back(fetch_worker);
}

// ── Parent-tile fallback rendering ────────────────────────────────────────────
// While tile (z, tx, ty) is still loading, render the matching sub-region of
// a lower-zoom ancestor if one is already in the texture cache.  This gives
// instant (blurry) coverage while the sharper tile arrives.

bool draw_fallback_tile(ImDrawList* dl,
                         int tx, int ty, int z,
                         float sx, float sy)
{
    constexpr int TILE_PX = 256;
    // As far back as a source with few zooms can need.
    for (int dz = 1; dz <= 12 && (z - dz) >= 0; ++dz) {
        int pz  = z - dz;
        int px  = tx >> dz;
        int py  = ty >> dz;
        auto it = g_textures.find({px, py, pz});
        if (it == g_textures.end()) continue;

        // UV coordinates of this tile within the ancestor
        int   cells = 1 << dz;
        float u0 = (float)(tx - (px << dz))       / cells;
        float v0 = (float)(ty - (py << dz))       / cells;
        float u1 = (float)(tx - (px << dz) + 1)   / cells;
        float v1 = (float)(ty - (py << dz) + 1)   / cells;

        dl->AddImage((ImTextureID)(intptr_t)it->second,
                     {sx,           sy          },
                     {sx + TILE_PX, sy + TILE_PX},
                     {u0, v0}, {u1, v1});
        return true;
    }
    return false;
}

} // namespace map_detail

using namespace map_detail;

void map_tile_queue_retain(const std::vector<MapTileId>& keep)
{
    std::unordered_set<TileKey, TileKeyHash> wanted;
    for (const MapTileId& t : keep) wanted.insert(capped_tile({ t.x, t.y, t.z }));

    std::lock_guard<std::mutex> lk(g_fetch_mtx);
    std::queue<TileKey> kept;
    while (!g_fetch_queue.empty()) {
        const TileKey k = g_fetch_queue.front();
        g_fetch_queue.pop();
        if (wanted.count(k)) kept.push(k);
        else                 g_pending.erase(k);   // free to be asked for again
    }
    g_fetch_queue = std::move(kept);
}

unsigned map_tile_texture(int z, int x, int y, bool queue)
{
    const TileKey key{ x, y, z };
    const auto it = g_textures.find(key);
    if (it != g_textures.end()) return it->second;
    if (queue) queue_tile(key);
    return 0;
}

// ── Public API ────────────────────────────────────────────────────────────────

std::string map_tile_attribution()
{
    std::lock_guard<std::mutex> lk(g_tile_cfg_mtx);
    // A map gone blank should say why where the operator is looking, and it should
    // not credit a source it is not showing.
    if (!g_tile_status.empty()) return "Map unavailable: " + g_tile_status;
    std::string line = tile_source_attribution(g_tile_cfg);
    if (g_tile_cfg.provider == TileProvider::Google && !g_google_copyright.empty())
        line += " \xc2\xb7 " + g_google_copyright;
    if (g_tile_cfg.provider == TileProvider::NasaGibs && g_tile_blanks.load() > 0)
        line += " \xc2\xb7 No imagery in some tiles";
    return line;
}

std::string map_view_tile_status()
{
    std::lock_guard<std::mutex> lk(g_tile_cfg_mtx);
    return g_tile_status;
}

void map_view_set_tile_source(const TileSourceConfig& source)
{
    {
        // Applying the same source again is how an operator asks to try again, so
        // this does not return early when nothing changed.
        std::lock_guard<std::mutex> lk(g_tile_cfg_mtx);
        g_tile_attempts.clear();
        g_tile_cfg = source;
        g_tile_status.clear();
        g_google_session.clear();
        g_google_expiry = 0;
        g_google_copyright.clear();
        g_google_retry_at = {};
        g_google_credit_at = {};
    }
    ++g_tile_gen;
    g_tile_max_zoom = tile_source_max_zoom(source);
    g_tile_blanks   = 0;

    // Nothing of the old source stays: not on screen, not on the way.
    {
        std::lock_guard<std::mutex> lk(g_fetch_mtx);
        g_fetch_queue = {};
    }
    {
        std::lock_guard<std::mutex> lk(g_upload_mtx);
        g_upload_queue = {};
        g_retry_tiles.clear();
    }
    for (auto& [k, tex] : g_textures) glDeleteTextures(1, &tex);
    g_textures.clear();
    g_pending.clear();
}

void map_view_shutdown()
{
    if (!g_running.exchange(false)) return;
    g_fetch_cv.notify_all();
    for (auto& t : g_fetch_threads)
        if (t.joinable()) t.join();
    g_fetch_threads.clear();

    for (auto& [k, tex] : g_textures)
        glDeleteTextures(1, &tex);
    g_textures.clear();
    g_pending.clear();
}
