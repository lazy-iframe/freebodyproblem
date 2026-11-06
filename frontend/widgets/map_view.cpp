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


// frontend/widgets/map_view.cpp
//
// Slippy tile map, its imagery from the provider chosen in frontend/tile_source.hpp.
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

#include "map_view.hpp"
#include "map_view_3d.hpp"
#include "map_view_internal.hpp"
#include "../terrain.hpp"
#include "../terrain_clearance.hpp"
#include "../geodesic.hpp"
#include "../cache_dir.hpp"
#include "layout.hpp"
#include "ui_kit.hpp"
#include "vehicle_ui_state.hpp"
#include "imgui.h"

// The httplib CMake target already defines this; guard so the two definitions
// (empty here vs. 1 on the command line) do not collide.
#ifndef CPPHTTPLIB_OPENSSL_SUPPORT
#  define CPPHTTPLIB_OPENSSL_SUPPORT
#endif
#include "httplib.h"

#include "stb_image.h"

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

#include <algorithm>
#include <atomic>
#include <cmath>
#include <condition_variable>
#include <deque>
#include <fstream>
#include <limits>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#ifndef M_PI
#  define M_PI 3.14159265358979323846
#endif

// ── Tile key ──────────────────────────────────────────────────────────────────

struct TileKey {
    int x, y, z;
    bool operator==(const TileKey& o) const { return x==o.x && y==o.y && z==o.z; }
};

struct TileKeyHash {
    size_t operator()(const TileKey& k) const noexcept {
        size_t h = (size_t)(unsigned)k.z;
        h = h * 2654435761u ^ (size_t)(unsigned)k.x;
        h = h * 2654435761u ^ (size_t)(unsigned)k.y;
        return h;
    }
};

// ── Web Mercator tile math ─────────────────────────────────────────────────────

static void lat_lon_to_tile_frac(double lat, double lon, int z,
                                  double& tx, double& ty)
{
    double n = (double)(1 << z);
    tx = (lon + 180.0) / 360.0 * n;
    double r = lat * M_PI / 180.0;
    ty = (1.0 - std::log(std::tan(r) + 1.0 / std::cos(r)) / M_PI) / 2.0 * n;
}

static void tile_frac_to_lat_lon(double tx, double ty, int z,
                                   double& lat, double& lon)
{
    double n = (double)(1 << z);
    lon = tx / n * 360.0 - 180.0;
    lat = std::atan(std::sinh(M_PI * (1.0 - 2.0 * ty / n))) * 180.0 / M_PI;
}

// ── Map state ─────────────────────────────────────────────────────────────────

struct MapState {
    double center_lat  = 0.0;
    double center_lon  = 0.0;
    int    zoom        = 2;
    bool   initialized = false;
    bool   follow      = true;  // keep re-centering on vehicle until user pans
};

static MapState g_map;

static void pan_by_pixels(float dx, float dy)
{
    constexpr double TILE_PX = 256.0;
    double tx, ty;
    lat_lon_to_tile_frac(g_map.center_lat, g_map.center_lon, g_map.zoom, tx, ty);
    tx -= (double)dx / TILE_PX;
    ty -= (double)dy / TILE_PX;
    tile_frac_to_lat_lon(tx, ty, g_map.zoom, g_map.center_lat, g_map.center_lon);
    g_map.center_lat = std::max(-85.0, std::min(85.0, g_map.center_lat));
    while (g_map.center_lon >  180.0) g_map.center_lon -= 360.0;
    while (g_map.center_lon < -180.0) g_map.center_lon += 360.0;
}

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

static std::unordered_map<TileKey, GLuint, TileKeyHash> g_textures;
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
static void drain_upload_queue()
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

static void queue_tile(const TileKey& wanted)
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

// Drop queued requests for zoom levels other than keep_zoom.
// Called on the main thread whenever zoom changes.
static void flush_stale_requests(int keep_zoom)
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

static void ensure_started()
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

static bool draw_fallback_tile(ImDrawList* dl,
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

// ── Position trail ────────────────────────────────────────────────────────────
//
// Where each vehicle has been, kept per vehicle so switching the callsign chip
// shows that aircraft's track and not a line drawn between the two.

namespace {

struct VehicleTrack {
    std::deque<TrackPoint> pts;
};

// Thinning distance. Below this the aircraft has not moved enough to be worth a
// point — GPS noise on a vehicle standing still would otherwise fill the trail
// with a smudge the size of its own position error.
constexpr double TRACK_MIN_STEP_M = 3.0;

// Trail length. At 3 m spacing this is 18 km of ground track, past which the
// oldest point is dropped for each new one. 6000 points is ~96 KB per vehicle
// and one polyline call, so the cap is about the trail staying readable rather
// than about the memory.
constexpr size_t TRACK_MAX_POINTS = 6000;

// Metres between two nearby positions. Equirectangular rather than haversine:
// over the tens of metres this is asked about the two agree to well under the
// thinning distance, and this is run once per vehicle per frame.
double rough_metres(double lat_a, double lon_a, double lat_b, double lon_b)
{
    constexpr double M_PER_DEG = 111320.0;
    const double dlat = (lat_b - lat_a) * M_PER_DEG;
    const double dlon = (lon_b - lon_a) * M_PER_DEG *
                        std::cos(lat_a * M_PI / 180.0);
    return std::sqrt(dlat * dlat + dlon * dlon);
}

// Every vehicle's trail, not just the bound one's — the map draws the whole
// fleet, so it has to be able to reach a track it is not currently bound to.
// VehicleUiState would give only the bound vehicle, so this keeps its own map
// and registers the same forgetter it would have; the registry it registers
// into is function-local, so this file-scope constructor is safe.
struct TrackRegistry {
    std::unordered_map<VehicleId, VehicleTrack, VehicleIdHash> by_vehicle;

    TrackRegistry()
    {
        detail::ui_register_forgetter(
            [this](VehicleId id) { by_vehicle.erase(id); });
    }
};

TrackRegistry g_tracks;

} // namespace

void map_track_pump(const VehicleState& vs)
{
    if (!vs.has_global_pos)              return;
    // A fix the vehicle has not got yet reads as the origin, and a trail
    // through Null Island is worse than no trail.
    if (vs.lat == 0.0 && vs.lon == 0.0)  return;

    auto& pts = g_tracks.by_vehicle[ui_bound_vehicle()].pts;
    // Height counts as movement too: the 3D view draws the climb out of a
    // vertical takeoff, which never moves the aircraft across the ground.
    if (!pts.empty() &&
        rough_metres(pts.back().lat, pts.back().lon, vs.lat, vs.lon)
            < TRACK_MIN_STEP_M &&
        std::fabs(pts.back().alt_amsl - vs.alt_asl) < TRACK_MIN_STEP_M)
        return;

    pts.push_back({ vs.lat, vs.lon, vs.alt_asl });
    if (pts.size() > TRACK_MAX_POINTS) pts.pop_front();
}

void map_track_clear()
{
    g_tracks.by_vehicle[ui_bound_vehicle()].pts.clear();
}

const std::deque<TrackPoint>* map_track_of(VehicleId id)
{
    const auto it = g_tracks.by_vehicle.find(id);
    return it == g_tracks.by_vehicle.end() ? nullptr : &it->second.pts;
}

// ── Home symbol ───────────────────────────────────────────────────────────────
//
// A helipad: the H inside its circle, which is what the marking on the ground
// looks like from the air. Drawn as strokes rather than as the letter in a
// font, so it keeps its weight and its proportions at any size and matches the
// rest of the vector chrome on this map.

static void draw_home(ImDrawList* dl, ImVec2 c, bool active)
{
    const float A = active ? 1.0f : 0.55f;

    constexpr float R  = 11.0f;   // pad radius
    constexpr float HW = 4.5f;    // half-width of the H
    constexpr float HH = 5.5f;    // half-height of the H

    // Green rather than the aircraft's red or the target's amber: home is the
    // one mark on the map that is neither the vehicle nor an instruction to it.
    const ImU32 col = ui_col(g_theme.col_ok, A);

    dl->AddCircleFilled(c, R, ui_col(g_theme.bg_topbar, 0.60f * A));
    dl->AddCircle      (c, R, col, 0, 2.0f);

    dl->AddLine({ c.x - HW, c.y - HH }, { c.x - HW, c.y + HH }, col, 2.0f);
    dl->AddLine({ c.x + HW, c.y - HH }, { c.x + HW, c.y + HH }, col, 2.0f);
    dl->AddLine({ c.x - HW, c.y      }, { c.x + HW, c.y      }, col, 2.0f);
}

void map_draw_home_symbol(ImDrawList* dl, ImVec2 c, bool active) { draw_home(dl, c, active); }

// The aircraft's heading line and its line of sight home, so the two read as a pair.
static constexpr float INDICATOR_LINE_W = 2.5f;

// ── Vehicle symbols ───────────────────────────────────────────────────────────
//
// Top-down silhouettes, one per airframe family, or the legacy arrow. All of
// them keep the arrow's idea of colour: the same fixed red pair, port side the
// darker, so which way up the airframe is reads at a glance whatever its
// shape, and an outline in the theme's ring colour.
//
// Shapes are written in a local frame — x to starboard, y forward, in pixels
// at scale 1 — and turned onto the heading by Frame. Every filled part is
// convex, which is what ImDrawList's polygon fill wants.

static MapIconStyle g_icon_style = MapIconStyle::ByType;

void map_view_set_icon_style(MapIconStyle s) { g_icon_style = s; }


static SymbolKind symbol_kind(uint8_t mav_type, MapIconStyle style)
{
    switch (style) {
    case MapIconStyle::Legacy:      return SymbolKind::Arrow;
    case MapIconStyle::Plane:       return SymbolKind::Plane;
    case MapIconStyle::Multicopter: return SymbolKind::Multicopter;
    case MapIconStyle::Helicopter:  return SymbolKind::Helicopter;
    case MapIconStyle::ByType:      break;
    }
    switch (mav_type) {
    case MAV_TYPE_FIXED_WING:
    case MAV_TYPE_FLAPPING_WING:
    case MAV_TYPE_PARAFOIL:
    case MAV_TYPE_VTOL_TAILSITTER_DUOROTOR:
    case MAV_TYPE_VTOL_TAILSITTER_QUADROTOR:
    case MAV_TYPE_VTOL_TILTROTOR:
    case MAV_TYPE_VTOL_FIXEDROTOR:
    case MAV_TYPE_VTOL_TAILSITTER:
    case MAV_TYPE_VTOL_TILTWING:
    case MAV_TYPE_VTOL_RESERVED5:
        return SymbolKind::Plane;
    case MAV_TYPE_QUADROTOR:
    case MAV_TYPE_HEXAROTOR:
    case MAV_TYPE_OCTOROTOR:
    case MAV_TYPE_TRICOPTER:
    case MAV_TYPE_DODECAROTOR:
    case MAV_TYPE_DECAROTOR:
    case MAV_TYPE_GENERIC_MULTIROTOR:
        return SymbolKind::Multicopter;
    case MAV_TYPE_HELICOPTER:
    case MAV_TYPE_COAXIAL:
        return SymbolKind::Helicopter;
    default:
        // Rovers, boats, and an autopilot that has not said: the arrow, which
        // claims nothing about the airframe beyond which way it points.
        return SymbolKind::Arrow;
    }
}

// Local airframe coordinates → screen.
struct Frame {
    ImVec2 c;
    float  dx, dy;    // forward, as a screen-space unit vector
    float  s;         // scale
    ImVec2 operator()(float x, float y) const
    {
        // Starboard is forward turned 90° clockwise on screen: (-dy, dx).
        return { c.x + (-dy * x + dx * y) * s, c.y + (dx * x + dy * y) * s };
    }
};

// Fill and outline one convex part given in local coordinates.
static void part(ImDrawList* dl, const Frame& f, std::initializer_list<ImVec2> local,
                 ImU32 fill, ImU32 edge, float edge_w)
{
    ImVec2 pts[16];
    int n = 0;
    for (const ImVec2& p : local) pts[n++] = f(p.x, p.y);
    dl->AddConvexPolyFilled(pts, n, fill);
    dl->AddPolyline(pts, n, edge, ImDrawFlags_Closed, edge_w);
}

static float draw_plane(ImDrawList* dl, const Frame& f, ImU32 port, ImU32 stbd, ImU32 edge)
{
    // Wings and tailplane under the fuselage, so the fuselage seam is unbroken.
    part(dl, f, { { -3.0f, 9.0f }, { -25.0f, 1.0f }, { -25.0f, -4.0f }, { -3.0f, -2.0f } }, port, edge, 1.2f);
    part(dl, f, { {  3.0f, 9.0f }, {  25.0f, 1.0f }, {  25.0f, -4.0f }, {  3.0f, -2.0f } }, stbd, edge, 1.2f);
    part(dl, f, { { -2.0f, -13.0f }, { -10.0f, -17.5f }, { -10.0f, -20.5f }, { -2.0f, -19.0f } }, port, edge, 1.2f);
    part(dl, f, { {  2.0f, -13.0f }, {  10.0f, -17.5f }, {  10.0f, -20.5f }, {  2.0f, -19.0f } }, stbd, edge, 1.2f);
    // Fuselage in two halves, split down the centreline like the arrow.
    part(dl, f, { { 0.0f, 27.0f }, { 3.0f, 21.0f }, { 3.6f, 9.0f }, { 3.0f, -14.0f },
                  { 1.4f, -21.0f }, { 0.0f, -21.0f } }, stbd, edge, 1.3f);
    part(dl, f, { { 0.0f, 27.0f }, { 0.0f, -21.0f }, { -1.4f, -21.0f }, { -3.0f, -14.0f },
                  { -3.6f, 9.0f }, { -3.0f, 21.0f } }, port, edge, 1.3f);
    return 27.0f;
}

static float draw_multicopter(ImDrawList* dl, const Frame& f, ImU32 port, ImU32 stbd, ImU32 edge,
                              float A)
{
    constexpr float ARM = 15.0f, ROTOR = 8.5f;
    const ImVec2 arms[4] = { { -ARM, ARM }, { ARM, ARM }, { ARM, -ARM }, { -ARM, -ARM } };
    for (int i = 0; i < 4; ++i) {
        const bool   port_side = arms[i].x < 0.0f;
        const bool   front     = arms[i].y > 0.0f;
        const ImVec2 hub       = f(arms[i].x, arms[i].y);
        dl->AddLine(f(0.0f, 0.0f), hub, port_side ? port : stbd, 3.0f * f.s);
        // Rotor discs: washed, the front pair less so — the nose of a
        // symmetric airframe is otherwise only the outline's say-so.
        const float disc_a = (front ? 0.55f : 0.28f) * A;
        dl->AddCircleFilled(hub, ROTOR * f.s, (port_side ? port : stbd) & 0x00FFFFFF |
                                              ((ImU32)(disc_a * 255.0f) << IM_COL32_A_SHIFT));
        dl->AddCircle(hub, ROTOR * f.s, edge, 0, (front ? 1.6f : 1.0f));
    }
    // Body: a small rounded plate with a nose wedge.
    part(dl, f, { { 0.0f, 12.0f }, { 5.0f, 5.0f }, { 5.5f, -5.0f }, { 0.0f, -6.5f } }, stbd, edge, 1.2f);
    part(dl, f, { { 0.0f, 12.0f }, { 0.0f, -6.5f }, { -5.5f, -5.0f }, { -5.0f, 5.0f } }, port, edge, 1.2f);
    return ARM + ROTOR;
}

static float draw_helicopter(ImDrawList* dl, const Frame& f, ImU32 port, ImU32 stbd, ImU32 edge,
                             float A, float rotor)
{
    // Tail boom and tail rotor first, under the cabin.
    part(dl, f, { { 1.6f, -5.0f }, { 1.0f, -27.0f }, { -1.0f, -27.0f }, { -1.6f, -5.0f } }, port, edge, 1.0f);
    dl->AddLine(f(2.5f, -22.0f), f(2.5f, -29.0f), edge, 2.0f * f.s);
    part(dl, f, { { -1.0f, -24.0f }, { -6.0f, -26.0f }, { -6.0f, -27.5f }, { -1.0f, -27.0f } }, port, edge, 1.0f);
    // Cabin, split down the middle like the others.
    part(dl, f, { { 0.0f, 15.0f }, { 4.5f, 12.0f }, { 6.5f, 5.0f }, { 6.0f, -3.0f }, { 3.5f, -7.0f }, { 0.0f, -7.0f } },
         stbd, edge, 1.3f);
    part(dl, f, { { 0.0f, 15.0f }, { 0.0f, -7.0f }, { -3.5f, -7.0f }, { -6.0f, -3.0f }, { -6.5f, 5.0f }, { -4.5f, 12.0f } },
         port, edge, 1.3f);
    // Main rotor: the disc it sweeps, and two blades at `rotor` radians — the
    // caller turns them while the vehicle is armed, slowly enough to read as a
    // rotor rather than as flicker.
    constexpr float R = 23.0f;
    dl->AddCircle(f(0.0f, 0.0f), R * f.s, (edge & 0x00FFFFFF) | ((ImU32)(0.45f * A * 255.0f) << IM_COL32_A_SHIFT),
                  0, 1.0f);
    // Washed back, so the cabin under them still reads.
    const ImU32 blade = (edge & 0x00FFFFFF) | ((ImU32)(0.7f * A * 255.0f) << IM_COL32_A_SHIFT);
    const float bx = std::cos(rotor) * R, by = std::sin(rotor) * R;
    dl->AddLine(f(-bx, -by), f(bx, by), blade, 1.6f * f.s);
    dl->AddLine(f(by, -bx),  f(-by, bx), blade, 1.6f * f.s);
    dl->AddCircleFilled(f(0.0f, 0.0f), 2.2f * f.s, edge);
    return 15.0f;
}

static float draw_arrow(ImDrawList* dl, const Frame& f, ImU32 port, ImU32 stbd, ImU32 edge)
{
    // Arrow dimensions
    constexpr float BODY   = 27.0f; // distance from center to tip
    constexpr float TAIL   = 15.0f; // distance from center to tail
    constexpr float WING   = 10.5f; // half-width at wing base
    constexpr float NOTCH  = 6.0f;  // tail notch depth

    const ImVec2 tip    = f(0.0f, BODY);
    const ImVec2 tail_c = f(0.0f, -TAIL);
    const ImVec2 wL     = f(-WING, 0.0f);
    const ImVec2 wR     = f( WING, 0.0f);
    // Tail notch pulls tail center slightly toward tip
    const ImVec2 notch  = f(0.0f, -(TAIL - NOTCH));

    dl->AddTriangleFilled(tip, wL, notch, port);
    dl->AddTriangleFilled(tip, wR, notch, stbd);
    dl->AddQuad(tip, wL, notch, wR, edge, 1.5f);
    dl->AddLine(tail_c, notch, edge, 1.5f);
    return BODY;
}

// Draws the symbol and returns how far ahead of `c` its nose is, at scale 1 —
// where the heading line starts.
static float draw_symbol(ImDrawList* dl, ImVec2 c, SymbolKind kind, float heading_deg,
                         float A, float scale, float rotor)
{
    const float rad = heading_deg * (float)(M_PI / 180.0);
    const Frame f   = { c, std::sin(rad), -std::cos(rad), scale };

    // Two shades of the same red rather than two colours: the seam is what
    // says which way up the airframe is at a glance, and it has to read
    // without becoming a second marker. Fixed rather than themed, since the
    // pair only works as a pair.
    const ImU32 port = IM_COL32(150, 26, 26, (int)(235 * A));
    const ImU32 stbd = IM_COL32(206, 48, 48, (int)(235 * A));
    const ImU32 edge = ui_col(g_theme.map_vehicle_ring, A);

    switch (kind) {
    case SymbolKind::Plane:       return draw_plane(dl, f, port, stbd, edge);
    case SymbolKind::Multicopter: return draw_multicopter(dl, f, port, stbd, edge, A);
    case SymbolKind::Helicopter: {
        // Drawn 10% up: at the others' size its rotor disc read as the whole
        // symbol and the cabin inside it as too small to make out.
        constexpr float HELI_SCALE = 1.1f;
        Frame big = f;
        big.s *= HELI_SCALE;
        return draw_helicopter(dl, big, port, stbd, edge, A, rotor) * HELI_SCALE;
    }
    case SymbolKind::Arrow:       break;
    }
    return draw_arrow(dl, f, port, stbd, edge);
}

SymbolKind map_symbol_kind(uint8_t mav_type) { return symbol_kind(mav_type, g_icon_style); }

void map_draw_vehicle_symbol(ImDrawList* dl, ImVec2 c, uint8_t mav_type, MapIconStyle style,
                             float heading_deg, float alpha, float scale)
{
    // Blades parked across the cabin: a preview is of no vehicle, armed or not.
    draw_symbol(dl, c, symbol_kind(mav_type, style), heading_deg, alpha, scale,
                (float)(M_PI * 0.25));
}

// ── Aircraft symbol ───────────────────────────────────────────────────────────
//
// One vehicle: the airframe, its heading line, and the label naming it. Drawn
// for every vehicle on the map, so everything that distinguishes the one the
// panels are bound to from the rest is the `active` flag rather than a second
// copy of this code.

static void draw_aircraft(ImDrawList* dl, ImVec2 c, const MapVehicle& mv,
                          ImVec2 wp, float win_w, float win_h)
{
    // Inactive aircraft are drawn down, not out: they stay legible, but the
    // vehicle every other panel is showing is the one the eye lands on.
    const float A = mv.active ? 1.0f : 0.55f;

    // Checked in the SWARM view: a ring at full strength whatever the
    // vehicle's own dimming, so the set a batch command will reach can be read
    // off the map before the button is pressed.
    if (mv.checked)
        dl->AddCircle(c, 34.0f, map_amber_u32(), 0, 2.0f);

    if (mv.has_hdg) {
        // Heading (degrees CW from north) as a screen vector. Screen Y grows
        // downward, so north is -Y: (sin, -cos).
        const float rad = mv.heading * (float)(M_PI / 180.0);
        const float dx  = std::sin(rad);   // east component  (+x on screen)
        const float dy  = -std::cos(rad);  // north component (-y on screen)

        // Rotor angle per vehicle, advanced only while it is armed — so on
        // disarm the blades stop where they are rather than snapping to a
        // parked angle, and spin up again from there.
        static std::unordered_map<VehicleId, float, VehicleIdHash> s_rotor;
        float& rotor = s_rotor.try_emplace(mv.id, (float)(M_PI * 0.25)).first->second;
        if (mv.armed)
            rotor = std::fmod(rotor + 1.6f * ImGui::GetIO().DeltaTime, (float)(2.0 * M_PI));

        const float  nose = draw_symbol(dl, c, symbol_kind(mv.mav_type, g_icon_style),
                                        mv.heading, A, 1.0f, rotor);
        const ImVec2 tip  = { c.x + dx * nose, c.y + dy * nose };

        // Dotted heading line. For the active aircraft it runs the width of the
        // panel, which is what says where the vehicle is pointed against
        // everything else on the map. The others get a stub: three lines across
        // the whole map is not three times as much information, it is a grid
        // over the imagery.
        constexpr float DOT_LEN  = 5.0f;
        constexpr float DOT_GAP  = 5.0f;
        constexpr float STUB_LEN = 40.0f;
        constexpr float PERIOD   = DOT_LEN + DOT_GAP;

        // The span of the ray that is actually on the panel — where it comes
        // into view and where it leaves — rather than only how far it is to the
        // far edge.
        //
        // The difference matters when the aircraft is off screen, which happens
        // the moment there are two vehicles far apart: switch to one while the
        // map is still over the other, and the tip of this ray is however many
        // pixels apart those two places are. At zoom 19 that is tens of
        // millions, and asking only for the exit distance and stepping ten
        // pixels at a time meant millions of segments per frame for a line with
        // nothing of it visible. Clipped properly the span is at most the
        // panel's diagonal, whatever the aircraft is doing.
        float t_enter = 0.0f;
        float t_exit  = mv.active ? std::numeric_limits<float>::max() : STUB_LEN;
        {
            const float org[2] = { tip.x, tip.y };
            const float dir[2] = { dx, dy };
            const float lo [2] = { wp.x, wp.y };
            const float hi [2] = { wp.x + win_w, wp.y + win_h };
            for (int ax = 0; ax < 2; ++ax) {
                if (std::fabs(dir[ax]) < 1e-6f) {
                    // Parallel to this edge: either always within it or never.
                    if (org[ax] < lo[ax] || org[ax] > hi[ax]) {
                        t_exit = t_enter;   // nothing to draw
                        break;
                    }
                    continue;
                }
                float ta = (lo[ax] - org[ax]) / dir[ax];
                float tb = (hi[ax] - org[ax]) / dir[ax];
                if (ta > tb) { const float tmp = ta; ta = tb; tb = tmp; }
                if (ta > t_enter) t_enter = ta;
                if (tb < t_exit)  t_exit  = tb;
            }
        }

        // Phase pinned to the ray's own origin rather than to wherever it
        // enters the panel, so the dashes do not crawl along the line as the
        // map is panned.
        for (float t = std::floor(t_enter / PERIOD) * PERIOD;
             t < t_exit; t += PERIOD) {
            const float a0 = std::max(t, t_enter);
            const float a1 = std::min(t + DOT_LEN, t_exit);
            if (a1 <= a0) continue;
            dl->AddLine({ tip.x + dx * a0, tip.y + dy * a0 },
                        { tip.x + dx * a1, tip.y + dy * a1 },
                        ui_col(g_theme.map_vehicle_ring, A), INDICATOR_LINE_W);
        }
    } else {
        // No heading — plain dot marker
        dl->AddCircleFilled(c, 13.5f, ui_col(g_theme.map_vehicle_fill, A));
        dl->AddCircle      (c, 13.5f, ui_col(g_theme.map_vehicle_ring, A), 0, 2.0f);
    }

    // ── Label ────────────────────────────────────────────────────────────────
    // "(2) SYS 1" — Fleet's number, then what the aircraft calls itself. The
    // number is there because the sysid is not a name: two airframes fresh off
    // the bench both answer to 1, and the map would show two identical labels.
    // Under it, its height above the terrain data.
    //
    // Held above the symbol at a fixed offset rather than swung around with the
    // heading, so a turning aircraft does not drag its own label round with it.
    // Boxed, because text straight on satellite imagery is unreadable over
    // anything pale.
    map_draw_vehicle_tags(dl, c, 38.0f, mv);
}

// ── GO HERE and CLEAR menus ───────────────────────────────────────────────────
//
// A click on the map names a point — in 3D, picked in place and height — and
// the GO HERE menu is what the operator does with it. A right-click opens
// CLEAR, for what the map is drawing that the operator has finished with.
// Both are small popups at the pointer, as a context menu is. They only read
// and write GotoTargetState — the send itself belongs to the caller, which is
// the one holding a sender.

static double s_ctx_lat = 0.0;      // the point the GO HERE menu is for
static double s_ctx_lon = 0.0;
static double s_ctx_copied_at = -10.0;   // ImGui time the coordinates were last copied
static bool   s_go_press = false;        // the left press may be a GO HERE click

// Target altitude, metres above home. Seeded from the vehicle's own altitude
// the first time the menu opens — "go there, stay at this height" is the
// common ask — and remembered after that, since an operator working a series
// of targets means the same height for all of them. Negative is below home, as
// in a valley under a mountain takeoff; the terrain check in the menu is what
// speaks up if that is into the ground. The limits only catch a slip of the keyboard:
// they are past any relief on earth, so no height the 3D map can pick is cut short.
static float s_goto_alt_m     = 10.0f;
static bool  s_goto_alt_seeded = false;

static constexpr float GOTO_ALT_MIN_M = -10000.0f;
static constexpr float GOTO_ALT_MAX_M = 10000.0f;

// The vehicle every other panel is showing, from the map's fleet list.
static const MapVehicle* active_vehicle(const std::vector<MapVehicle>* fleet)
{
    if (fleet)
        for (const MapVehicle& mv : *fleet)
            if (mv.active) return &mv;
    return nullptr;
}

// Tight, as a context menu: little padding, the theme's panel behind it and
// a hairline of the accent round it.
static void push_menu_style()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,    { 8.0f, 6.0f });
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,      { 6.0f, 4.0f });
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,     { 5.0f, 2.0f });
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, g_theme.bg_panel);
    ImGui::PushStyleColor(ImGuiCol_Border,  ui_col(g_theme.accent, 0.55f));
}
static void pop_menu_style()
{
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(4);
}

// `active` and `mission` put the target in height for the terrain check: the
// target's altitude is above home, and home's AMSL comes from them by the
// planning rule of plan_home_amsl().
static void draw_goto_menu(GotoTargetState& go, const MapVehicle* active,
                           const std::vector<MissionItem>* mission)
{
    constexpr float ROW_H = 22.0f;

    push_menu_style();
    if (ImGui::BeginPopup("##map_go", ImGuiWindowFlags_AlwaysAutoResize |
                                      ImGuiWindowFlags_NoTitleBar)) {
        // The point, as a heading. Click to copy, as signed decimal degrees —
        // what other tools paste. It says COPIED for a moment so the click is
        // seen to land.
        {
            char coords[80];
            snprintf(coords, sizeof(coords), "%.6f%c  %.6f%c",
                     s_ctx_lat < 0 ? -s_ctx_lat : s_ctx_lat, s_ctx_lat < 0 ? 'S' : 'N',
                     s_ctx_lon < 0 ? -s_ctx_lon : s_ctx_lon, s_ctx_lon < 0 ? 'W' : 'E');
            const bool  copied = ImGui::GetTime() - s_ctx_copied_at < 1.2;
            const char* shown  = copied ? "COPIED" : coords;
            const ImVec2 sz = ImGui::CalcTextSize(coords);
            const ImVec2 p  = ImGui::GetCursorScreenPos();
            if (ImGui::InvisibleButton("##ctx_coords", sz)) {
                char clip[64];
                snprintf(clip, sizeof(clip), "%.7f, %.7f", s_ctx_lat, s_ctx_lon);
                ImGui::SetClipboardText(clip);
                s_ctx_copied_at = ImGui::GetTime();
            }
            const bool hov = ImGui::IsItemHovered();
            if (hov) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                ImGui::SetTooltip("Click to copy");
            }
            ImGui::GetWindowDrawList()->AddText(p, (hov || copied) ? ui_col_accent() : ui_col_value(), shown);
        }

        // The height, the only argument the target carries beyond the point.
        {
            ImGui::AlignTextToFramePadding();
            ImGui::PushStyleColor(ImGuiCol_Text, ui_col_label());
            ImGui::TextUnformatted("ALT");
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::SetNextItemWidth(64.0f);
            ImGui::InputFloat("##goto_alt", &s_goto_alt_m, 0.f, 0.f, "%.1f");
            s_goto_alt_m = std::max(GOTO_ALT_MIN_M, std::min(GOTO_ALT_MAX_M, s_goto_alt_m));
            ImGui::SameLine();
            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("m above home");

            const std::string home_dist = map_home_distance(active, s_ctx_lat, s_ctx_lon);
            if (!home_dist.empty()) {
                ImGui::PushStyleColor(ImGuiCol_Text, ui_col_label());
                ImGui::TextUnformatted(home_dist.c_str());
                ImGui::PopStyleColor();
            }
        }

        // The straight line from the vehicle to the target, against the
        // terrain and the clearance set in settings. Advice, not a lock: GO
        // stays live — the terrain data is ~30 m SRTM, and the operator may
        // know the ground better than it does.
        if (active && active->has_pos && active->has_alt) {
            std::optional<float> reported;
            if (active->has_home_alt) reported = active->home_alt_amsl;
            const std::optional<float> home =
                plan_home_amsl(reported, active->has_home ? &active->home_lat : nullptr,
                               active->has_home ? &active->home_lon : nullptr, mission);
            if (home) {
                const Clearance c = path_clearance(active->lat, active->lon, active->alt_amsl,
                                                   s_ctx_lat, s_ctx_lon, *home + s_goto_alt_m);
                char t[64];
                ImU32 col = ui_col_label();
                if (!c.has_min)          snprintf(t, sizeof(t), "Checking terrain...");
                else if (c.collides())   { snprintf(t, sizeof(t), "Path hits terrain (~%.0f m into it)", (double)-c.min_m);
                                           col = ui_col(g_theme.col_error); }
                else if (c.too_close())  { snprintf(t, sizeof(t), "Path within ~%.0f m of terrain", (double)c.min_m);
                                           col = ui_col(g_theme.col_warning); }
                else                     snprintf(t, sizeof(t), "Clears terrain by ~%.0f m", (double)c.min_m);
                ImGui::PushStyleColor(ImGuiCol_Text, col);
                ImGui::TextUnformatted(t);
                ImGui::PopStyleColor();
            }
        }

        // The vehicle discards a position target it is not in GUIDED for, and
        // says nothing about having done so. Better to refuse here and name the
        // reason than to send one into silence.
        const float w = ImGui::GetContentRegionAvail().x;
        ImGui::BeginDisabled(!go.sendable);
        if (ui_grid_button("GO", { w, ROW_H })) {
            go.req_lat   = s_ctx_lat;
            go.req_lon   = s_ctx_lon;
            go.req_alt_m = s_goto_alt_m;
            go.requested = true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndDisabled();
        if (!go.sendable) {
            ImGui::PushStyleColor(ImGuiCol_Text, ui_col(g_theme.col_error));
            ImGui::TextUnformatted(go.reason ? go.reason : "Unavailable");
            ImGui::PopStyleColor();
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Escape, false))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    pop_menu_style();
}

// Neither is a command — they only stop the map drawing something the
// operator has finished with. Each is greyed out when there is nothing to
// clear.
static void draw_clear_menu(GotoTargetState& go)
{
    push_menu_style();
    if (ImGui::BeginPopup("##map_clear", ImGuiWindowFlags_NoTitleBar)) {
        if (ImGui::MenuItem("Clear target", nullptr, false, go.has_target))
            go.has_target = false;
        if (ImGui::MenuItem("Clear track", nullptr, false,
                            !g_tracks.by_vehicle[ui_bound_vehicle()].pts.empty()))
            map_track_clear();
        ImGui::EndPopup();
    }
    pop_menu_style();
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

// The waypoint being dragged in the mission tab's EDIT, as an index into
// MissionPickState::edit_mission; -1 when none. Set on the press, followed
// while the button is held, dropped on release.
static int s_drag_wp = -1;

// The fence point being dragged in the FENCE view's EDIT: a shape in
// MissionPickState::edit_fence and a vertex of it, -1 for a circle's centre.
// shape < 0 when none.
struct FenceDrag { int shape = -1; int vertex = -1; };
static FenceDrag s_drag_fence;

// ── Geofence drawing ──────────────────────────────────────────────────────────

static ImU32 fence_col(bool inclusion, float alpha)
{
    return ui_col(inclusion ? g_theme.col_ok : g_theme.col_error, alpha);
}

// Metres on the ground to pixels on the map, at a latitude: the screen
// distance of a point that far north. Mercator is conformal, so one direction
// serves for a circle's radius.
template <class Project>
static float metres_to_px(const Project& project, double lat, double lon, float m)
{
    const ImVec2 a = project(lat, lon);
    const ImVec2 b = project(lat + (double)m / 111320.0, lon);
    return std::fabs(a.y - b.y);
}

// One uploaded fence zone: a translucent fill so overlapping zones still read,
// an outline in the zone's colour, and its number at the middle. Inclusion
// zones are green, exclusion zones red.
template <class Project>
static void draw_fence_shape(ImDrawList* dl, const Project& project,
                             const FenceShape& s, int number, bool dragging_this)
{
    const ImU32 fill = fence_col(s.inclusion, 0.16f);
    const ImU32 line = fence_col(s.inclusion, 0.95f);
    ImVec2 label_at;

    if (s.kind == FenceShape::Kind::Polygon) {
        if (s.pts.size() < 2) return;
        static std::vector<ImVec2> scr;   // UI thread only
        scr.clear();
        float cx = 0.0f, cy = 0.0f;
        for (const FenceVertex& v : s.pts) {
            scr.push_back(project(v.lat, v.lon));
            cx += scr.back().x; cy += scr.back().y;
        }
        if (scr.size() >= 3)
            dl->AddConcavePolyFilled(scr.data(), (int)scr.size(), fill);
        dl->AddPolyline(scr.data(), (int)scr.size(), line,
                        ImDrawFlags_Closed, dragging_this ? 3.0f : 2.0f);
        for (const ImVec2& p : scr) dl->AddCircleFilled(p, 3.5f, line);
        label_at = { cx / scr.size(), cy / scr.size() };
    } else {
        const ImVec2 c = project(s.center.lat, s.center.lon);
        const float  r = metres_to_px(project, s.center.lat, s.center.lon, s.radius_m);
        dl->AddCircleFilled(c, r, fill, 64);
        dl->AddCircle(c, r, line, 64, dragging_this ? 3.0f : 2.0f);
        dl->AddCircleFilled(c, 3.5f, line);
        label_at = c;
    }

    char lbl[24];
    snprintf(lbl, sizeof(lbl), "F%d %s", number, s.inclusion ? "IN" : "EX");
    ImFont*      fm = g_font_micro ? g_font_micro : ImGui::GetFont();
    const float  tw = ui_tracked_width(fm, UI_SZ_MICRO, lbl);
    const ImVec2 tp = { label_at.x - tw * 0.5f, label_at.y + 6.0f };
    dl->AddRectFilled({ tp.x - 3.0f, tp.y - 1.0f }, { tp.x + tw + 3.0f, tp.y + UI_SZ_MICRO + 1.0f },
                      IM_COL32(0, 0, 0, 150), 2.0f);
    ui_tracked_text(dl, fm, UI_SZ_MICRO, tp, line, lbl);
}

// The parameter-set fence: a ring around home of FENCE_RADIUS (PX4:
// GF_MAX_HOR_DIST), amber, labelled with its radius and ceiling. Not drawn
// when the fence is off.
template <class Project>
static void draw_fence_cylinder(ImDrawList* dl, const Project& project,
                                const MapVehicle& mv)
{
    const FenceParams& f = mv.fence;
    if (!mv.has_home || !f.circle_configured() || f.disabled()) return;

    const float  alpha = mv.active ? 1.0f : 0.5f;
    const ImU32  col   = ui_col(g_theme.col_warning, 0.9f * alpha);
    const ImVec2 c     = project(mv.home_lat, mv.home_lon);
    const float  r     = metres_to_px(project, mv.home_lat, mv.home_lon, f.radius_m);

    dl->AddCircle(c, r, col, 96, 2.5f);

    char lbl[64];
    if (f.ceiling_configured())
        snprintf(lbl, sizeof(lbl), "FENCE %.0f M  \xe2\x89\xa4 %.0f M",
                 (double)f.radius_m, (double)f.alt_max_m);
    else
        snprintf(lbl, sizeof(lbl), "FENCE %.0f M", (double)f.radius_m);
    ImFont*      fm = g_font_micro ? g_font_micro : ImGui::GetFont();
    const float  tw = ui_tracked_width(fm, UI_SZ_MICRO, lbl);
    const ImVec2 tp = { c.x - tw * 0.5f, c.y - r - UI_SZ_MICRO - 6.0f };
    dl->AddRectFilled({ tp.x - 3.0f, tp.y - 1.0f }, { tp.x + tw + 3.0f, tp.y + UI_SZ_MICRO + 1.0f },
                      IM_COL32(0, 0, 0, 150), 2.0f);
    ui_tracked_text(dl, fm, UI_SZ_MICRO, tp, col, lbl);
}

// ── Overlay chrome shared by the flat map and the 3D view ─────────────────────

constexpr float OVL_M   = 8.0f;    // from the map's edges
constexpr float AC_H    = 55.0f;   // the A/C block's height
constexpr float BTN_W   = 72.0f;   // the control column, under the A/C block
constexpr float BTN_H   = 30.0f;
constexpr float BTN_GAP = 6.0f;

// The 3D view in place of the flat map. Off while placing or dragging
// anything, which only the flat map does.
static bool s_3d = false;

// The control column, top left under the A/C block, one button a row:
// CENTER, 3D, and in 3D the exaggeration and the imagery switch. Fixed in
// place — turning 3D on adds rows below and moves nothing. r[2i..2i+1] is
// row i's rectangle.
static void overlay_button_rects(ImVec2 wp, ImVec2 r[8])
{
    for (int i = 0; i < 4; ++i) {
        r[2 * i]     = { wp.x + OVL_M, wp.y + OVL_M + AC_H + BTN_GAP + i * (BTN_H + BTN_GAP) };
        r[2 * i + 1] = { r[2 * i].x + BTN_W, r[2 * i].y + BTN_H };
    }
}

static bool over_overlay_buttons(ImVec2 wp)
{
    ImVec2 r[8];
    overlay_button_rects(wp, r);
    const int rows = s_3d ? 4 : 2;
    for (int i = 0; i < rows; ++i)
        if (ImGui::IsMouseHoveringRect(r[2 * i], r[2 * i + 1], false)) return true;
    return false;
}

// CENTER, lit while the map is following, and 3D under it; in 3D, the
// vertical exaggeration under that, pressed to step through its values,
// and TEX, lit while imagery covers the terrain.
static void draw_overlay_buttons(ImVec2 wp, bool has_pos, double lat, double lon)
{
    ImVec2 r[8];
    overlay_button_rects(wp, r);

    // Without a position there is nothing to centre on.
    ImGui::SetCursorScreenPos(r[0]);
    ImGui::BeginDisabled(!has_pos);
    if (ui_tab_button("CENTER##map_center", { BTN_W, BTN_H }, g_map.follow) &&
        has_pos) {
        g_map.follow     = true;
        g_map.center_lat = lat;
        g_map.center_lon = lon;
    }
    ImGui::EndDisabled();

    const bool avail = map3d_available();
    ImGui::SetCursorScreenPos(r[2]);
    ImGui::BeginDisabled(!avail);
    if (ui_tab_button("3D##map_3d", { BTN_W, BTN_H }, s_3d)) s_3d = !s_3d;
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_AllowWhenDisabled))
        ImGui::SetTooltip(avail ? "Terrain in 3D: drag to orbit, right-drag to pan, wheel to zoom"
                                : "3D needs OpenGL 3.3, which this driver does not offer");
    if (s_3d) {
        char ex[32];
        snprintf(ex, sizeof(ex), "\xc3\x97%g##map_exag", (double)map3d_exaggeration());
        ImGui::SetCursorScreenPos(r[4]);
        if (ui_tab_button(ex, { BTN_W, BTN_H }, false)) map3d_cycle_exaggeration();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
            ImGui::SetTooltip("Vertical exaggeration: heights drawn this many times their size");

        ImGui::SetCursorScreenPos(r[6]);
        if (ui_tab_button("TEX##map_tex", { BTN_W, BTN_H }, map3d_textures())) map3d_toggle_textures();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
            ImGui::SetTooltip("Imagery on the terrain; off shows the bare elevation mesh");
    }
}

// The active vehicle's height above the terrain under it: its own AMSL
// altitude less the terrain data's ground there — so it is the GCS's word on
// the ground, not the vehicle's, and disagrees with the vehicle's own
// "AGL" (above home) wherever home and the ground there differ. nullopt
// without a position and altitude, or while the terrain tile loads.
static std::optional<float> active_terrain_agl(const std::vector<MapVehicle>* fleet)
{
    if (!fleet) return std::nullopt;
    for (const MapVehicle& mv : *fleet) {
        if (!mv.active || !mv.has_pos || !mv.has_alt) continue;
        if (auto g = terrain_amsl(mv.lat, mv.lon)) return mv.alt_amsl - *g;
    }
    return std::nullopt;
}

static float s_ac_right = 0.0f;   // see map_ac_block_right()

float map_ac_block_right() { return s_ac_right; }

// `text` split into lines no wider than `wrap_w`, as the radio tab's notice
// splits its own: the longest run of whole words that fits, a word too long
// for a line broken by character, no line opening or closing on a space.
static std::vector<std::pair<const char*, const char*>> wrap_lines(const char* text, float wrap_w)
{
    std::vector<std::pair<const char*, const char*>> lines;
    for (const char* p = text; *p; ) {
        while (*p == ' ') ++p;
        if (!*p) break;
        const char* fit = nullptr;
        for (const char* q = p; ; ) {
            while (*q && *q != ' ') ++q;
            if (ImGui::CalcTextSize(p, q).x > wrap_w) break;
            fit = q;
            while (*q == ' ') ++q;
            if (!*q) break;
        }
        if (!fit) {
            fit = p + 1;
            while (*fit && *fit != ' ' && ImGui::CalcTextSize(p, fit + 1).x <= wrap_w) ++fit;
        }
        lines.push_back({ p, fit });
        p = fit;
    }
    return lines;
}

constexpr float TAG_PAD_X = 5.0f, TAG_PAD_Y = 3.0f;

ImVec2 map_tag_size(const char* text)
{
    const ImVec2 ts = ImGui::CalcTextSize(text);
    return { ts.x + TAG_PAD_X * 2.0f, ts.y + TAG_PAD_Y * 2.0f };
}

// The tags over the map are drawn at this opacity, box and text alike, so the
// imagery under them shows through a little.
constexpr float TAG_OPACITY = 0.8f;

void map_draw_tag(ImDrawList* dl, ImVec2 left_mid, const char* text, ImVec4 col, float alpha)
{
    alpha *= TAG_OPACITY;
    const ImVec2 sz  = map_tag_size(text);
    const ImVec2 a   = { left_mid.x, left_mid.y - sz.y * 0.5f };
    const ImVec2 b   = { a.x + sz.x, a.y + sz.y };
    const float  rnd = ImGui::GetStyle().ChildRounding;
    dl->AddRectFilled(a, b, ui_col(g_theme.bg_topbar, alpha), rnd);
    dl->AddRectFilled(a, b, ImGui::GetColorU32({ col.x, col.y, col.z, 0.14f * alpha }), rnd);
    dl->AddRect      (a, b, ImGui::GetColorU32({ col.x, col.y, col.z, alpha }), rnd, 0, FRAME_BORDER_NORMAL);
    dl->AddText({ a.x + TAG_PAD_X, a.y + TAG_PAD_Y }, ImGui::GetColorU32({ col.x, col.y, col.z, alpha }), text);
}

std::string map_home_distance(const MapVehicle* mv, double lat, double lon)
{
    if (!mv || !mv->has_home) return {};
    const double d = geodesic_distance_m(mv->home_lat, mv->home_lon, lat, lon);
    char t[32];
    if (d < 1000.0) snprintf(t, sizeof(t), "HOME %.0f m", d);
    else            snprintf(t, sizeof(t), "HOME %.2f km", d / 1000.0);
    return t;
}

// The part of the line a to b inside the rectangle, as fractions along it. False if
// none of it is.
static bool clip_to_rect(ImVec2 a, ImVec2 b, ImVec2 r0, ImVec2 r1, double& t0, double& t1)
{
    t0 = 0.0;
    t1 = 1.0;
    const auto axis = [&](double p, double d, double lo, double hi) {
        if (std::fabs(d) < 1e-6) return p >= lo && p <= hi;
        const double u = (lo - p) / d, v = (hi - p) / d;
        t0 = std::max(t0, std::min(u, v));
        t1 = std::min(t1, std::max(u, v));
        return t0 <= t1;
    };
    return axis(a.x, (double)b.x - a.x, r0.x, r1.x) && axis(a.y, (double)b.y - a.y, r0.y, r1.y);
}

void map_dotted_line(ImDrawList* dl, ImVec2 a, ImVec2 b, ImU32 col, float w)
{
    // Only the part in view, and the dots always this far apart, whatever the width, so
    // a warning's dots land on the path's. One end of a line can be very far off: cut
    // at the camera plane in 3D, an end behind the camera lands millions of pixels away.
    constexpr double ON = 3.0, PERIOD = 8.0;
    const double dx = (double)b.x - a.x, dy = (double)b.y - a.y;
    const double len = std::sqrt(dx * dx + dy * dy);
    if (len < 1.0) return;

    double t0, t1;
    if (!clip_to_rect(a, b, dl->GetClipRectMin(), dl->GetClipRectMax(), t0, t1)) return;

    // Counted from a's end, so a dot stays put as the panel's edge passes over the line.
    const double last = t1 * len;
    for (double s = std::floor(t0 * len / PERIOD) * PERIOD; s < last; s += PERIOD) {
        const double u = std::max(s, 0.0), v = std::min(s + ON, len);
        dl->AddLine({ (float)(a.x + dx * u / len), (float)(a.y + dy * u / len) },
                    { (float)(a.x + dx * v / len), (float)(a.y + dy * v / len) }, col, w);
    }
}

void map_draw_los(ImDrawList* dl, ImVec2 home, ImVec2 vehicle, const MapVehicle& mv)
{
    constexpr float MIN_PX = 40.0f;   // parked on its pad, a label would only cover it
    constexpr float HOME_MAST_M = 2.0f;   // off the ground: an operator or an antenna
    const float dx = vehicle.x - home.x, dy = vehicle.y - home.y;
    if (!mv.has_pos || !mv.has_alt || !mv.has_home || dx * dx + dy * dy < MIN_PX * MIN_PX) return;

    // Each end stood on the terrain data at least. Home is on the ground and the data
    // is tens of metres out in places, so otherwise the line can begin inside it, or
    // end inside it under an aircraft that has landed, and always read as blocked.
    const std::optional<float> gh = terrain_amsl(mv.home_lat, mv.home_lon);
    const std::optional<float> gv = terrain_amsl(mv.lat, mv.lon);
    bool blocked = false, known = false;
    if (gh && gv) {
        const float h0 = std::max(mv.has_home_alt ? mv.home_alt_amsl : *gh, *gh) + HOME_MAST_M;
        const float h1 = std::max(mv.alt_amsl, *gv);
        const Clearance c = line_of_sight_clearance(mv.home_lat, mv.home_lon, h0, mv.lat, mv.lon, h1);
        blocked = c.collides();
        known   = blocked || c.complete;   // one hill is enough, the rest still loading or not
    }

    constexpr ImVec4 CYAN = { 0.0f, 0.9f, 1.0f, 1.0f };
    const ImVec4 col = blocked ? g_theme.col_error : CYAN;
    map_dotted_line(dl, home, vehicle, ui_col(col, 0.9f), INDICATOR_LINE_W);

    const char*  text = blocked ? "NO LOS" : known ? "LOS" : "LOS ?";
    const ImVec2 ts   = map_tag_size(text);

    // At the middle of the part in view: with one end off the panel the true middle
    // may be too, and the line's whole message with it. Nothing if none of it is in view.
    double t0, t1;
    if (!clip_to_rect(home, vehicle, dl->GetClipRectMin(), dl->GetClipRectMax(), t0, t1)) return;
    const float tm = (float)(0.5 * (t0 + t1));
    map_draw_tag(dl, { home.x + dx * tm - ts.x * 0.5f, home.y + dy * tm }, text, col, 1.0f);
}

void map_draw_tag(ImDrawList* dl, ImVec2 left_mid, const char* text)
{
    map_draw_tag(dl, left_mid, text, col_ok());
}

void map_draw_vehicle_tags(ImDrawList* dl, ImVec2 centre, float above, const MapVehicle& mv)
{
    if (mv.number == 0 && mv.sysid == 0) return;
    const float A = mv.active ? 1.0f : 0.55f;

    // The GCS's word on the ground, not the vehicle's: its AMSL less the
    // terrain data there, so it differs from the vehicle's own "AGL" (above
    // home) wherever home and the ground under it differ.
    char t[80] = "";
    float agl = 0.0f;
    const std::string home = mv.has_pos ? map_home_distance(&mv, mv.lat, mv.lon) : std::string();
    if (mv.has_pos && mv.has_alt)
        if (const std::optional<float> g = terrain_amsl(mv.lat, mv.lon)) {
            agl = mv.alt_amsl - *g;
            if (agl < 0.0f) snprintf(t, sizeof(t), "BELOW TERRAIN ~%.0f m", (double)-agl);
            else            snprintf(t, sizeof(t), "ALT AGL TER ~%.0f m", (double)agl);
        }
    if (!home.empty()) {
        const size_t len = strlen(t);
        snprintf(t + len, sizeof(t) - len, "%s%s", len ? "  \xc2\xb7  " : "", home.c_str());
    }

    // Stacked upward from the vehicle: the height nearest it, the name on top.
    float bottom = centre.y - above;
    if (*t) {
        const ImVec2 ts = map_tag_size(t);
        map_draw_tag(dl, { centre.x - ts.x * 0.5f, bottom - ts.y * 0.5f }, t,
                     agl < 0.0f ? g_theme.col_error : col_ok(), A);
        bottom -= ts.y + 2.0f;
    }
    char lbl[32];
    snprintf(lbl, sizeof(lbl), "(%u) SYS %u", (unsigned)mv.number, (unsigned)mv.sysid);
    const ImVec2 ls = map_tag_size(lbl);
    map_draw_tag(dl, { centre.x - ls.x * 0.5f, bottom - ls.y * 0.5f }, lbl, col_ok(), A);
}

void map_draw_instruction(ImVec2 p0, ImVec2 size, const char* text)
{
    constexpr float PAD_X = 6.0f, PAD_Y = 5.0f;
    ImDrawList*  dl  = ImGui::GetForegroundDrawList();
    const ImVec4 ok  = col_ok();

    // At most half the map's width: the text wrapped to that and centred line
    // by line, the box only as wide as its widest line and as tall as the
    // lines it took.
    const float max_w = std::floor(size.x * 0.5f);
    const auto  lines = wrap_lines(text, max_w - PAD_X * 2.0f);
    float       text_w = 0.0f;
    for (const auto& ln : lines) text_w = std::max(text_w, ImGui::CalcTextSize(ln.first, ln.second).x);
    const float bw    = std::min(max_w, std::ceil(text_w) + PAD_X * 2.0f);
    const float lh    = ImGui::GetTextLineHeight();
    const float bh    = lines.size() * lh + PAD_Y * 2.0f;
    const float x     = std::max(p0.x + (size.x - bw) * 0.5f, s_ac_right + 8.0f);
    const float y     = p0.y + OVL_M;
    const float rnd   = ImGui::GetStyle().ChildRounding;

    // The notice's tint is over a panel; here it is over imagery, so a dark
    // backing first, or the tint would vanish into whatever is under it.
    dl->AddRectFilled({ x, y }, { x + bw, y + bh }, ui_col(g_theme.bg_topbar, 0.92f), rnd);
    dl->AddRectFilled({ x, y }, { x + bw, y + bh }, ImGui::GetColorU32({ ok.x, ok.y, ok.z, 0.14f }), rnd);
    dl->AddRect      ({ x, y }, { x + bw, y + bh }, ImGui::GetColorU32(ok), rnd, 0, FRAME_BORDER_NORMAL);
    for (size_t i = 0; i < lines.size(); ++i) {
        const float lw = ImGui::CalcTextSize(lines[i].first, lines[i].second).x;
        dl->AddText({ x + (bw - lw) * 0.5f, y + PAD_Y + i * lh }, ImGui::GetColorU32(ok),
                    lines[i].first, lines[i].second);
    }
}

// The A/C state block, top left. Its third line is height above the terrain
// data, `ter_agl`; red, and saying so, when the vehicle is below it. Then how far it
// is from home.
static void draw_ac_block(ImDrawList* dl, ImVec2 wp, bool has_pos,
                          double lat, double lon, float heading, bool has_hdg,
                          float alt_rel, float gs, std::optional<float> ter_agl,
                          const std::string& home_dist)
{
    ImFont*     fm  = g_font_micro ? g_font_micro : ImGui::GetFont();
    const ImU32 lbl = ui_col_label();
    const ImU32 val = ui_col_value();

    char l1[64], l2[64];
    if (has_pos)
        snprintf(l1, sizeof(l1), "A/C  %.5f%c %.5f%c",
                 lat < 0 ? -lat : lat, lat < 0 ? 'S' : 'N',
                 lon < 0 ? -lon : lon, lon < 0 ? 'W' : 'E');
    else
        snprintf(l1, sizeof(l1), "A/C  NO POSITION");
    snprintf(l2, sizeof(l2), "HDG %s  ALT %.0f AGL  GS %.0f",
             has_hdg ? "" : "---", (double)alt_rel, (double)gs);
    if (has_hdg)
        snprintf(l2, sizeof(l2), "HDG %03d  ALT %.0f AGL  GS %.0f",
                 (int)heading, (double)alt_rel, (double)gs);

    char l3[96];
    const bool below = ter_agl && *ter_agl < 0.0f;
    if (!ter_agl)    snprintf(l3, sizeof(l3), "ALT AGL TER ---");
    else if (below)  snprintf(l3, sizeof(l3), "BELOW TERRAIN ~%.0f m", (double)-*ter_agl);
    else             snprintf(l3, sizeof(l3), "ALT AGL TER ~%.0f", (double)*ter_agl);
    if (!home_dist.empty()) {
        const size_t len = strlen(l3);
        snprintf(l3 + len, sizeof(l3) - len, "  %s", home_dist.c_str());
    }

    const float w1 = ui_tracked_width(fm, UI_SZ_MICRO, l1);
    const float w2 = ui_tracked_width(fm, UI_SZ_MICRO, l2);
    const float w3 = ui_tracked_width(fm, UI_SZ_MICRO, l3);
    const float bw = std::max({ w1, w2, w3 }) + 16.0f;

    const ImVec2 b0 = { wp.x + OVL_M, wp.y + OVL_M };
    const ImVec2 b1 = { b0.x + std::max(bw, BTN_W), b0.y + AC_H };
    s_ac_right = b1.x;
    dl->AddRectFilled(b0, b1, ui_col(g_theme.bg_topbar, 0.78f));
    ui_frame(dl, b0, b1, ui_col(g_theme.separator, 0.7f));
    ui_tracked_text(dl, fm, UI_SZ_MICRO, { b0.x + 8.0f, b0.y + 8.0f },
                    has_pos ? val : lbl, l1);
    ui_tracked_text(dl, fm, UI_SZ_MICRO, { b0.x + 8.0f, b0.y + 23.0f },
                    lbl, l2);
    ui_tracked_text(dl, fm, UI_SZ_MICRO, { b0.x + 8.0f, b0.y + 38.0f },
                    below ? ui_col(g_theme.col_error) : ter_agl ? val : lbl, l3);
}

// North, bottom-right above the attribution, `lift` px higher when that runs
// to a second line; `rot_deg` turns it for a view that does not have north up.
static void draw_north_pointer(ImDrawList* dl, ImVec2 wp, float win_w, float win_h,
                               float rot_deg, float lift = 0.0f)
{
    ImFont*     fm  = g_font_micro ? g_font_micro : ImGui::GetFont();
    const float nx  = wp.x + win_w - 22.0f;
    const float ny  = wp.y + win_h - 40.0f - lift;
    const float r   = rot_deg * (float)(M_PI / 180.0);
    const float cs  = std::cos(r), sn = std::sin(r);
    const auto  at  = [&](float x, float y) { return ImVec2{ nx + x * cs - y * sn, ny + x * sn + y * cs }; };
    dl->AddTriangleFilled(at(0.0f, -10.0f), at(-5.0f, 4.0f), at(5.0f, 4.0f), map_amber_u32());
    ui_tracked_text(dl, fm, UI_SZ_MICRO, { nx - 3.0f, ny + 6.0f }, ui_col_label(), "N");
}

// The imagery's attribution, bottom-right, with `extra` on a line above it.
static void draw_attribution(ImDrawList* dl, ImVec2 wp, float win_w, float win_h, const char* extra)
{
    float bottom = wp.y + win_h - 4.0f;

    // Wrapped to the window, never clipped: several providers require their credit
    // in full, and a narrow map or a hybrid's two credits would otherwise cut it.
    // Built bottom line first, as they are drawn from the bottom up.
    const float max_w = std::max(60.0f, win_w - 16.0f);
    std::vector<std::string> lines;
    const auto add_wrapped = [&](const std::string& text) {
        std::vector<std::string> wrapped;
        std::string cur;
        size_t start = 0;
        while (start <= text.size()) {
            const size_t sp   = text.find(' ', start);
            const std::string word = text.substr(start, sp == std::string::npos ? sp : sp - start);
            const std::string next = cur.empty() ? word : cur + " " + word;
            if (!cur.empty() && ImGui::CalcTextSize(next.c_str()).x > max_w) {
                wrapped.push_back(cur);
                cur = word;
            } else {
                cur = next;
            }
            if (sp == std::string::npos) break;
            start = sp + 1;
        }
        if (!cur.empty()) wrapped.push_back(cur);
        lines.insert(lines.end(), wrapped.rbegin(), wrapped.rend());
    };

    // A provider's separate credits go on lines of their own, the first on top.
    const std::string attr_str = map_tile_attribution();
    const std::string sep = " \xc2\xb7 ";
    std::vector<std::string> credits;
    for (size_t pos = 0;;) {
        const size_t hit = attr_str.find(sep, pos);
        credits.push_back(attr_str.substr(pos, hit == std::string::npos ? hit : hit - pos));
        if (hit == std::string::npos) break;
        pos = hit + sep.size();
    }
    for (auto it = credits.rbegin(); it != credits.rend(); ++it) add_wrapped(*it);
    if (extra) add_wrapped(extra);

    for (const std::string& text : lines) {
        const char* line = text.c_str();
        const ImVec2 asz = ImGui::CalcTextSize(line);
        const float  ax  = wp.x + win_w - asz.x - 6.0f;
        const float  ay  = bottom - asz.y;
        dl->AddRectFilled({ax - 4.0f, ay - 2.0f},
                           {ax + asz.x + 4.0f, ay + asz.y + 2.0f},
                           map_attr_bg());
        dl->AddText({ax, ay}, map_attr_text(), line);
        bottom = ay - 6.0f;
    }
}

void draw_map_view(double lat, double lon, bool has_pos,
                   float heading, bool has_hdg,
                   float win_x, float win_y, float win_w, float win_h,
                   const std::vector<MissionItem>* mission,
                   MissionPickState* pick,
                   float alt_rel, float gs,
                   GotoTargetState* go,
                   const std::vector<MapVehicle>* fleet,
                   const std::vector<MissionItem>* fence)
{
    ensure_started();
    drain_upload_queue();

    // ── Window setup ──────────────────────────────────────────────────────────
    ImGui::SetNextWindowPos ({win_x, win_y}, ImGuiCond_Always);
    ImGui::SetNextWindowSize({win_w, win_h}, ImGuiCond_Always);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, bg_map());
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,  {0.0f, 0.0f});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

    constexpr ImGuiWindowFlags FLAGS =
        ImGuiWindowFlags_NoTitleBar       | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove           | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        // Panning the map must not raise it above the plugin rail floating over
        // its right edge — see the same flag on the video window.
        ImGuiWindowFlags_NoBringToFrontOnFocus;

    if (!ImGui::Begin("##map", nullptr, FLAGS)) {
        ImGui::End();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();
        return;
    }

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2      wp = ImGui::GetWindowPos();

    // The control row, top right — CENTER puts the map back on the active
    // vehicle and keeps it there, which a drag gives up. Its rects are fixed
    // so the input below can leave them alone: a press on a button must not
    // also pan, pick or open GO HERE.
    const bool map_hovered = ImGui::IsWindowHovered() && !over_overlay_buttons(wp);

    // A mission item can be placed and moved in 3D, ground then height. Fences are
    // flat-map work; drawing or editing one leaves 3D.
    if (pick && (pick->fence_drawing() || pick->fence_edit))
        s_3d = false;

    // ── Keep centered on vehicle until user pans ───────────────────────────────
    if (has_pos && (!g_map.initialized || g_map.follow)) {
        g_map.center_lat = lat;
        g_map.center_lon = lon;
        if (!g_map.initialized) {
            g_map.zoom        = 15;
            g_map.initialized = true;
        }
    }

    // ── 3D ────────────────────────────────────────────────────────────────────
    // The same window, inputs and overlay chrome, with the 3D view where the
    // tiles would be. Should it fail to draw, the flat map takes over.
    if (s_3d && g_map.initialized) {
        std::vector<MapVehicle> solo;
        const std::vector<MapVehicle>* draw_list = fleet;
        if (!draw_list || draw_list->empty()) {
            MapVehicle one;
            one.id      = ui_bound_vehicle();
            one.lat     = lat;
            one.lon     = lon;
            one.has_pos = has_pos;
            one.heading = heading;
            one.has_hdg = has_hdg;
            one.active  = true;
            solo.push_back(one);
            draw_list = &solo;
        }
        Map3DScene scene;
        scene.fleet   = draw_list;
        scene.mission = (pick && pick->edit_mission) ? pick->edit_mission : mission;
        scene.pick    = pick;
        scene.go      = go;
        FencePlan downloaded;
        scene.fence = (pick && pick->edit_fence) ? pick->edit_fence : nullptr;
        if (!scene.fence && fence && !fence->empty()) {
            downloaded  = fence_from_items(*fence);
            scene.fence = &downloaded;
        }
        if (map3d_draw(dl, wp, { win_w, win_h }, map_hovered,
                       { g_map.center_lat, g_map.center_lon, g_map.follow }, scene)) {
            draw_overlay_buttons(wp, has_pos, lat, lon);
            draw_ac_block(dl, wp, has_pos, lat, lon, heading, has_hdg, alt_rel, gs,
                          active_terrain_agl(fleet),
                          has_pos ? map_home_distance(active_vehicle(fleet), lat, lon) : std::string());
            draw_north_pointer(dl, wp, win_w, win_h, -map3d_yaw_deg(),
                               ImGui::GetTextLineHeight() + 6.0f);
            draw_attribution(dl, wp, win_w, win_h, terrain_attribution().c_str());

            // GO HERE, picked in 3D place and height: the same menu as a 2D
            // click, at that point, its altitude filled in. A right-click
            // outside the pick: the same CLEAR menu.
            double glat, glon;
            float  galt;
            if (go && map3d_take_goto(glat, glon, galt)) {
                s_ctx_lat         = glat;
                s_ctx_lon         = glon;
                s_goto_alt_m      = std::clamp(galt, GOTO_ALT_MIN_M, GOTO_ALT_MAX_M);
                s_goto_alt_seeded = true;
                ImGui::OpenPopup("##map_go");
            }
            if (go && map3d_take_clear()) ImGui::OpenPopup("##map_clear");
            if (go) {
                draw_goto_menu(*go, active_vehicle(draw_list), scene.mission);
                draw_clear_menu(*go);
            }
            ImGui::End();
            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor();
            return;
        }
        s_3d = false;
    }

    // ── Input ─────────────────────────────────────────────────────────────────
    const bool pick_active = pick && pick->picking();

    // EDIT ends with the mode, or with the mission it was editing.
    std::vector<MissionItem>* drag_mission =
        (pick && pick->drag_edit && !pick_active) ? pick->edit_mission : nullptr;
    if (!drag_mission || s_drag_wp >= (int)drag_mission->size())
        s_drag_wp = -1;

    // The FENCE view's EDIT, likewise; never while a shape is being drawn.
    FencePlan* drag_fence =
        (pick && pick->fence_edit && !pick_active) ? pick->edit_fence : nullptr;
    if (!drag_fence || s_drag_fence.shape >= (int)drag_fence->shapes.size())
        s_drag_fence = {};

    if (map_hovered) {
        if (pick_active) {
            // Show a hand cursor so the user knows the map is clickable
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        } else if (s_drag_wp < 0 && s_drag_fence.shape < 0) {
            // Pan: left-drag (suppressed during pick mode, and while a
            // waypoint is being dragged — the gesture is the waypoint's)
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                ImVec2 d = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left, 0.0f);
                ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
                if (d.x != 0.0f || d.y != 0.0f) {
                    pan_by_pixels(d.x, d.y);
                    g_map.follow = false;
                }
            }
        }
        // Zoom: scroll wheel (always available)
        float wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0.0f) {
            int new_zoom = std::max(1, std::min(19, g_map.zoom + (int)wheel));
            if (new_zoom != g_map.zoom) {
                g_map.zoom = new_zoom;
                flush_stale_requests(new_zoom);
            }
        }
    }

    // ── Tile grid ─────────────────────────────────────────────────────────────
    constexpr int TILE_PX = 256;

    if (!g_map.initialized) {
        const char* msg = "Waiting for GPS\xe2\x80\xa6";
        ImVec2 tsz = ImGui::CalcTextSize(msg);
        ImGui::SetCursorPos({(win_w - tsz.x) * 0.5f, (win_h - tsz.y) * 0.5f});
        ImGui::TextDisabled("%s", msg);
    } else {
        double cx, cy;
        lat_lon_to_tile_frac(g_map.center_lat, g_map.center_lon, g_map.zoom, cx, cy);

        double tl_px = cx * TILE_PX - win_w * 0.5;
        double tl_py = cy * TILE_PX - win_h * 0.5;

        // ── Pick-mode click → lat/lon ─────────────────────────────────────────
        if (pick_active && map_hovered &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            const ImVec2 mouse = ImGui::GetMousePos();
            double tx = ((double)(mouse.x - wp.x) + tl_px) / TILE_PX;
            double ty = ((double)(mouse.y - wp.y) + tl_py) / TILE_PX;
            tile_frac_to_lat_lon(tx, ty, g_map.zoom,
                                 pick->picked_lat, pick->picked_lon);
            pick->pick_done = true;

            // A polygon closes on a click at its first vertex.
            pick->fence_close = false;
            if (pick->fence_draw == MissionPickState::FenceDraw::Polygon &&
                pick->fence_pts.size() >= 3) {
                double fx, fy;
                lat_lon_to_tile_frac(pick->fence_pts[0].lat, pick->fence_pts[0].lon,
                                     g_map.zoom, fx, fy);
                const float dx = wp.x + (float)(fx * TILE_PX - tl_px) - mouse.x;
                const float dy = wp.y + (float)(fy * TILE_PX - tl_py) - mouse.y;
                pick->fence_close = dx * dx + dy * dy <= 12.0f * 12.0f;
            }
            // add_command intentionally left set; tab_mission clears it
            // after consuming
        }

        // ── EDIT: drag a waypoint ─────────────────────────────────────────────
        // Picked up by a press within a marker's reach, the nearest if several,
        // and moved with the pointer until the button comes up. Written into
        // the editor's own vector, so the list and the line follow the drag.
        if (drag_mission) {
            auto screen_of = [&](double wlat, double wlon) -> ImVec2 {
                double vx, vy;
                lat_lon_to_tile_frac(wlat, wlon, g_map.zoom, vx, vy);
                return { wp.x + (float)(vx * TILE_PX - tl_px),
                         wp.y + (float)(vy * TILE_PX - tl_py) };
            };
            const ImVec2 mouse = ImGui::GetMousePos();

            int   under  = -1;
            float best_d = 12.0f * 12.0f;   // marker radius plus a little slack
            for (int i = 0; i < (int)drag_mission->size(); ++i) {
                const MissionItem& it = (*drag_mission)[i];
                if (it.lat == 0.0 && it.lon == 0.0) continue;
                const ImVec2 sp = screen_of(it.lat, it.lon);
                const float  dx = sp.x - mouse.x, dy = sp.y - mouse.y;
                if (dx * dx + dy * dy <= best_d) { best_d = dx * dx + dy * dy; under = i; }
            }

            if (s_drag_wp < 0 && map_hovered && under >= 0) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    s_drag_wp = under;
                    g_map.follow = false;   // the map must hold still under the drag
                }
            }
            if (s_drag_wp >= 0) {
                if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
                    MissionItem& it = (*drag_mission)[s_drag_wp];
                    const double mx = ((double)(mouse.x - wp.x) + tl_px) / TILE_PX;
                    const double my = ((double)(mouse.y - wp.y) + tl_py) / TILE_PX;
                    tile_frac_to_lat_lon(mx, my, g_map.zoom, it.lat, it.lon);
                } else {
                    s_drag_wp = -1;
                }
            }
        }

        // ── FENCE EDIT: drag a vertex or a circle's centre ────────────────────
        // As EDIT does for waypoints: the nearest point within reach is picked
        // up on the press and follows the pointer until the button comes up.
        if (drag_fence) {
            auto screen_of = [&](double flat, double flon) -> ImVec2 {
                double vx, vy;
                lat_lon_to_tile_frac(flat, flon, g_map.zoom, vx, vy);
                return { wp.x + (float)(vx * TILE_PX - tl_px),
                         wp.y + (float)(vy * TILE_PX - tl_py) };
            };
            const ImVec2 mouse = ImGui::GetMousePos();

            FenceDrag under;
            float best_d = 10.0f * 10.0f;
            auto consider = [&](int si, int vi, double flat, double flon) {
                const ImVec2 sp = screen_of(flat, flon);
                const float  d  = (sp.x - mouse.x) * (sp.x - mouse.x) +
                                  (sp.y - mouse.y) * (sp.y - mouse.y);
                if (d <= best_d) { best_d = d; under = { si, vi }; }
            };
            for (int si = 0; si < (int)drag_fence->shapes.size(); ++si) {
                const FenceShape& fs = drag_fence->shapes[si];
                if (fs.kind == FenceShape::Kind::Polygon)
                    for (int vi = 0; vi < (int)fs.pts.size(); ++vi)
                        consider(si, vi, fs.pts[vi].lat, fs.pts[vi].lon);
                else
                    consider(si, -1, fs.center.lat, fs.center.lon);
            }

            if (s_drag_fence.shape < 0 && map_hovered && under.shape >= 0) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    s_drag_fence = under;
                    g_map.follow = false;
                }
            }
            if (s_drag_fence.shape >= 0) {
                if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
                    FenceShape& fs = drag_fence->shapes[s_drag_fence.shape];
                    FenceVertex* v = nullptr;
                    if (fs.kind == FenceShape::Kind::Circle)            v = &fs.center;
                    else if (s_drag_fence.vertex < (int)fs.pts.size()) v = &fs.pts[s_drag_fence.vertex];
                    if (v) {
                        const double mx = ((double)(mouse.x - wp.x) + tl_px) / TILE_PX;
                        const double my = ((double)(mouse.y - wp.y) + tl_py) / TILE_PX;
                        tile_frac_to_lat_lon(mx, my, g_map.zoom, v->lat, v->lon);
                    }
                } else {
                    s_drag_fence = {};
                }
            }
        }

        // ── Click → GO HERE, right-click → CLEAR ──────────────────────────────
        // A click is a press and release that barely moved, so a drag still
        // pans. Not while picking a waypoint or a fence point — the mouse
        // belongs to that gesture, and a target offered mid-pick is one the
        // operator did not ask for — nor on a press that picked up a waypoint
        // or a fence point to drag.
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            s_go_press = map_hovered && !pick_active && s_drag_wp < 0 && s_drag_fence.shape < 0;
        if (go && s_go_press && map_hovered && !pick_active &&
            ImGui::IsMouseReleased(ImGuiMouseButton_Left) &&
            ImGui::GetIO().MouseDragMaxDistanceSqr[ImGuiMouseButton_Left] < 16.0f) {
            s_go_press = false;
            const ImVec2 mouse = ImGui::GetMousePos();
            double gx = ((double)(mouse.x - wp.x) + tl_px) / TILE_PX;
            double gy = ((double)(mouse.y - wp.y) + tl_py) / TILE_PX;
            tile_frac_to_lat_lon(gx, gy, g_map.zoom, s_ctx_lat, s_ctx_lon);
            if (!s_goto_alt_seeded) {
                // Not at home's own height, where it may be standing on the ground.
                // Well below home is a real height, and is kept.
                const float here = std::round(go->current_alt_m);
                s_goto_alt_m = std::clamp(std::fabs(here) < 2.0f ? 2.0f : here,
                                          GOTO_ALT_MIN_M, GOTO_ALT_MAX_M);
                s_goto_alt_seeded = true;
            }
            ImGui::OpenPopup("##map_go");
        }
        if (go && !pick_active && map_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            ImGui::OpenPopup("##map_clear");
        if (go) {
            draw_goto_menu(*go, active_vehicle(fleet),
                           (pick && pick->edit_mission) ? pick->edit_mission : mission);
            draw_clear_menu(*go);
        }

        int tx0 = (int)std::floor(tl_px / TILE_PX);
        int ty0 = (int)std::floor(tl_py / TILE_PX);
        int tx1 = (int)std::ceil ((tl_px + win_w) / TILE_PX);
        int ty1 = (int)std::ceil ((tl_py + win_h) / TILE_PX);
        int max_tile = (1 << g_map.zoom) - 1;

        for (int ty = ty0; ty <= ty1; ++ty) {
            for (int tx = tx0; tx <= tx1; ++tx) {
                if (tx < 0 || ty < 0 || tx > max_tile || ty > max_tile) continue;

                float sx = wp.x + (float)(tx * TILE_PX - tl_px);
                float sy = wp.y + (float)(ty * TILE_PX - tl_py);

                TileKey key{tx, ty, g_map.zoom};
                auto it = g_textures.find(key);
                if (it != g_textures.end()) {
                    dl->AddImage((ImTextureID)(intptr_t)it->second,
                                 {sx,           sy          },
                                 {sx + TILE_PX, sy + TILE_PX});
                } else {
                    // Show a zoomed-out ancestor while the real tile loads
                    if (!draw_fallback_tile(dl, tx, ty, g_map.zoom, sx, sy)) {
                        dl->AddRectFilled({sx, sy},
                                          {sx + TILE_PX, sy + TILE_PX},
                                          map_tile_placeholder());
                    }
                    queue_tile(key);
                }
            }
        }

        // ── Fleet ─────────────────────────────────────────────────────────────
        //
        // Every vehicle the caller handed over. A caller with no fleet becomes a
        // fleet of one built from the scalar arguments, so there is one drawing
        // path rather than two that have to be kept in agreement.
        std::vector<MapVehicle> solo;
        const std::vector<MapVehicle>* draw_list = fleet;
        if (!draw_list || draw_list->empty()) {
            MapVehicle one;
            one.id      = ui_bound_vehicle();   // so it finds its own trail
            one.lat     = lat;
            one.lon     = lon;
            one.has_pos = has_pos;
            one.heading = heading;
            one.has_hdg = has_hdg;
            one.active  = true;
            solo.push_back(one);
            draw_list = &solo;
        }

        const auto project = [&](double plat, double plon) -> ImVec2 {
            double px, py;
            lat_lon_to_tile_frac(plat, plon, g_map.zoom, px, py);
            return { wp.x + (float)(px * TILE_PX - tl_px),
                     wp.y + (float)(py * TILE_PX - tl_py) };
        };

        // Inactive vehicles first, the active one last, so the aircraft every
        // other panel is showing is never buried under another's trail or
        // symbol. Both passes run over the same list, so a vehicle's trail and
        // its symbol keep the same relative order.
        const auto in_draw_order = [&](auto&& fn) {
            for (const MapVehicle& mv : *draw_list) if (!mv.active) fn(mv);
            for (const MapVehicle& mv : *draw_list) if ( mv.active) fn(mv);
        };

        // ── Geofences ─────────────────────────────────────────────────────────
        // Lowest of everything drawn over the tiles: they are areas, and the
        // trails, the aircraft and the mission are all read against them.
        //
        // Uploaded zones: the fence editor's plan while it has one, else the
        // active vehicle's fence as last downloaded, unless the fence is off.
        {
            FencePlan downloaded;
            const FencePlan* zones = (pick && pick->edit_fence) ? pick->edit_fence : nullptr;
            if (!zones && fence && !fence->empty()) {
                downloaded = fence_from_items(*fence);
                zones      = &downloaded;
            }
            if (zones)
                for (int si = 0; si < (int)zones->shapes.size(); ++si)
                    draw_fence_shape(dl, project, zones->shapes[si], si + 1,
                                     drag_fence && si == s_drag_fence.shape);
        }
        // The parameter-set cylinder, every vehicle's around its own home.
        in_draw_order([&](const MapVehicle& mv) { draw_fence_cylinder(dl, project, mv); });

        // ── Position trails ───────────────────────────────────────────────────
        // Under the aircraft and under the mission, so neither is obscured by
        // where something has already been.
        in_draw_order([&](const MapVehicle& mv) {
            const auto it = g_tracks.by_vehicle.find(mv.id);
            if (it == g_tracks.by_vehicle.end()) return;
            const std::deque<TrackPoint>& pts = it->second.pts;
            if (pts.size() < 2) return;

            // Projected and thinned again, this time in pixels: at low zoom a
            // kilometre of track collapses into a few pixels, and handing ImGui
            // six thousand points to draw inside one of them costs the whole
            // frame for nothing. Points outside the panel are kept — dropping
            // them would break the segments that cross it.
            static std::vector<ImVec2> scr;   // reused; UI thread only
            scr.clear();
            scr.reserve(pts.size());
            for (const TrackPoint& tp : pts) {
                const ImVec2 p = project(tp.lat, tp.lon);
                if (!scr.empty()) {
                    const ImVec2& q = scr.back();
                    if (std::fabs(p.x - q.x) < 2.0f &&
                        std::fabs(p.y - q.y) < 2.0f) continue;
                }
                scr.push_back(p);
            }
            // The aircraft's own position closes its trail, so the line reaches
            // the symbol instead of stopping a sample short of it.
            if (mv.has_pos) scr.push_back(project(mv.lat, mv.lon));
            if (scr.size() < 2) return;

            dl->AddPolyline(scr.data(), (int)scr.size(),
                            ui_col(g_theme.map_track, mv.active ? 1.0f : 0.40f),
                            ImDrawFlags_None, mv.active ? 3.0f : 2.0f);
        });

        // ── Home ──────────────────────────────────────────────────────────────
        // Under the aircraft: a vehicle sitting on its own home must not be
        // hidden by the pad it is standing on.
        in_draw_order([&](const MapVehicle& mv) {
            if (!mv.has_home) return;
            draw_home(dl, project(mv.home_lat, mv.home_lon), mv.active);
        });

        // ── Line of sight ─────────────────────────────────────────────────────
        // For the vehicle every other panel is showing; one per aircraft would
        // lay a web over the imagery.
        in_draw_order([&](const MapVehicle& mv) {
            if (mv.active && mv.has_pos && mv.has_home)
                map_draw_los(dl, project(mv.home_lat, mv.home_lon), project(mv.lat, mv.lon), mv);
        });

        // ── Aircraft ──────────────────────────────────────────────────────────
        in_draw_order([&](const MapVehicle& mv) {
            if (!mv.has_pos) return;
            draw_aircraft(dl, project(mv.lat, mv.lon), mv,
                          wp, win_w, win_h);
        });

        // ── Mission waypoint overlay ──────────────────────────────────────────
        // Prefer the live edit vector so the map reflects unsaved edits.
        const std::vector<MissionItem>* display_mission =
            (pick && pick->edit_mission) ? pick->edit_mission : mission;

        if (display_mission && !display_mission->empty()) {
            auto to_screen = [&](double wlat, double wlon) -> ImVec2 {
                double vx, vy;
                lat_lon_to_tile_frac(wlat, wlon, g_map.zoom, vx, vy);
                return { wp.x + (float)(vx * TILE_PX - tl_px),
                         wp.y + (float)(vy * TILE_PX - tl_py) };
            };

            // Connecting polyline
            for (size_t i = 1; i < display_mission->size(); ++i) {
                const auto& a = (*display_mission)[i - 1];
                const auto& b = (*display_mission)[i];
                if (a.lat == 0.0 && a.lon == 0.0) continue;
                if (b.lat == 0.0 && b.lon == 0.0) continue;
                dl->AddLine(to_screen(a.lat, a.lon),
                            to_screen(b.lat, b.lon),
                            IM_COL32(255, 200, 50, 180), 1.5f);
            }

            // Waypoint circles with sequence number labels
            constexpr float R = 8.0f;
            for (size_t wi = 0; wi < display_mission->size(); ++wi) {
                const MissionItem& wpi = (*display_mission)[wi];
                if (wpi.lat == 0.0 && wpi.lon == 0.0) continue;
                const ImVec2 sp = to_screen(wpi.lat, wpi.lon);
                // Yellow ring on the waypoint being dragged
                const bool being_picked = drag_mission && (int)wi == s_drag_wp;
                const ImU32 fill = being_picked
                    ? IM_COL32(255, 240,  0, 240)    // yellow  — awaiting map click
                    : wpi.is_current
                    ? IM_COL32(255, 100, 50, 220)    // orange  — active (current) WP
                    : IM_COL32( 50, 150, 255, 200);  // blue    — normal WP
                const float radius = being_picked ? R + 2.0f : R;
                dl->AddCircleFilled(sp, radius, fill);
                dl->AddCircle(sp, radius, IM_COL32(255, 255, 255, 220), 0, 1.5f);
                char seq_str[8];
                snprintf(seq_str, sizeof(seq_str), "%u", (unsigned)wpi.seq);
                const ImVec2 tsz = ImGui::CalcTextSize(seq_str);
                dl->AddText({ sp.x - tsz.x * 0.5f, sp.y - tsz.y * 0.5f },
                            IM_COL32(255, 255, 255, 255), seq_str);

                // Where it is going, while it is being dragged.
                if (drag_mission && (int)wi == s_drag_wp) {
                    char ll[48];
                    snprintf(ll, sizeof(ll), "%.6f, %.6f", wpi.lat, wpi.lon);
                    const ImVec2 lsz = ImGui::CalcTextSize(ll);
                    const ImVec2 lp  = { sp.x + R + 6.0f, sp.y - lsz.y * 0.5f };
                    dl->AddRectFilled({ lp.x - 3.0f, lp.y - 1.0f },
                                      { lp.x + lsz.x + 3.0f, lp.y + lsz.y + 1.0f },
                                      IM_COL32(0, 0, 0, 180), 3.0f);
                    dl->AddText(lp, IM_COL32(255, 230, 60, 255), ll);
                }
            }
        }

        // ── Fence shape being drawn ───────────────────────────────────────────
        // The vertices so far, joined, with a rubber band to the pointer; a
        // circle's centre with the ring it would have if the next click set
        // its radius. The first vertex rings once a polygon can be closed on it.
        if (pick && pick->fence_drawing()) {
            const ImU32  col   = fence_col(pick->fence_inclusion, 0.95f);
            const ImVec2 mouse = ImGui::GetMousePos();
            const auto&  pts   = pick->fence_pts;
            if (pick->fence_draw == MissionPickState::FenceDraw::Polygon) {
                static std::vector<ImVec2> scr;
                scr.clear();
                for (const FenceVertex& v : pts) scr.push_back(project(v.lat, v.lon));
                if (scr.size() >= 2)
                    dl->AddPolyline(scr.data(), (int)scr.size(), col, ImDrawFlags_None, 2.0f);
                if (!scr.empty() && map_hovered)
                    dl->AddLine(scr.back(), mouse, fence_col(pick->fence_inclusion, 0.6f), 1.5f);
                for (const ImVec2& p : scr) dl->AddCircleFilled(p, 4.0f, col);
                if (scr.size() >= 3) {
                    const float dx = scr[0].x - mouse.x, dy = scr[0].y - mouse.y;
                    const bool  on_first = dx * dx + dy * dy <= 12.0f * 12.0f;
                    dl->AddCircle(scr[0], on_first ? 10.0f : 7.0f, col, 0, 2.0f);
                }
            } else if (!pts.empty()) {
                const ImVec2 c = project(pts[0].lat, pts[0].lon);
                dl->AddCircleFilled(c, 4.0f, col);
                if (map_hovered) {
                    const float r = std::sqrt((mouse.x - c.x) * (mouse.x - c.x) +
                                              (mouse.y - c.y) * (mouse.y - c.y));
                    dl->AddCircleFilled(c, r, fence_col(pick->fence_inclusion, 0.12f), 64);
                    dl->AddCircle(c, r, col, 64, 2.0f);
                    dl->AddLine(c, mouse, fence_col(pick->fence_inclusion, 0.6f), 1.5f);
                }
            }
        }

        // ── Guided target marker ──────────────────────────────────────────────
        // Where the vehicle was last told to go, with the altitude it was told
        // to hold. A ringed cross rather than a filled disc, so it reads as a
        // destination and never as one more waypoint.
        if (go && go->has_target) {
            double gx, gy;
            lat_lon_to_tile_frac(go->target_lat, go->target_lon, g_map.zoom, gx, gy);
            const ImVec2 tp = { wp.x + (float)(gx * TILE_PX - tl_px),
                                wp.y + (float)(gy * TILE_PX - tl_py) };
            constexpr float R = 10.0f;
            const ImU32 col = map_amber_u32();
            dl->AddCircle(tp, R,        col, 0, 2.0f);
            dl->AddCircle(tp, R * 0.35f, col, 0, 2.0f);
            dl->AddLine({ tp.x - R - 5.0f, tp.y }, { tp.x - R + 2.0f, tp.y }, col, 2.0f);
            dl->AddLine({ tp.x + R - 2.0f, tp.y }, { tp.x + R + 5.0f, tp.y }, col, 2.0f);
            dl->AddLine({ tp.x, tp.y - R - 5.0f }, { tp.x, tp.y - R + 2.0f }, col, 2.0f);
            dl->AddLine({ tp.x, tp.y + R - 2.0f }, { tp.x, tp.y + R + 5.0f }, col, 2.0f);

            char tlbl[32];
            snprintf(tlbl, sizeof(tlbl), "GOTO  %.0f M", (double)go->target_alt_m);
            map_draw_tag(dl, { tp.x + R + 8.0f, tp.y }, tlbl);

            // A line from the aircraft to the target: at a glance it says both
            // which vehicle the target belongs to and how far it has to run.
            if (has_pos) {
                double vx2, vy2;
                lat_lon_to_tile_frac(lat, lon, g_map.zoom, vx2, vy2);
                dl->AddLine({ wp.x + (float)(vx2 * TILE_PX - tl_px),
                              wp.y + (float)(vy2 * TILE_PX - tl_py) },
                            tp, map_amber_u32(0.45f), 1.5f);
            }
        }

        // ── Pick-mode / EDIT hint banner ──────────────────────────────────────
        if (pick_active || drag_mission || drag_fence) {
            char hint[112];
            using FD = MissionPickState::FenceDraw;
            if (pick_active && pick->fence_draw == FD::Polygon)
                snprintf(hint, sizeof(hint), pick->fence_pts.size() < 3
                         ? "  Click to add vertex %d  \xe2\x80\x94  Esc to cancel  "
                         : "  Click to add vertex %d, or the first one to close  \xe2\x80\x94  Esc to cancel  ",
                         (int)pick->fence_pts.size() + 1);
            else if (pick_active && pick->fence_draw == FD::Circle)
                snprintf(hint, sizeof(hint), pick->fence_pts.empty()
                         ? "  Click to place the circle's centre  \xe2\x80\x94  Esc to cancel  "
                         : "  Click to set the radius  \xe2\x80\x94  Esc to cancel  ");
            else if (pick_active)
                snprintf(hint, sizeof(hint),
                         "  Click to place %s  \xe2\x80\x94  Esc to cancel  ",
                         pick->add_label ? pick->add_label : "item");
            else if (drag_fence)
                snprintf(hint, sizeof(hint),
                         "  EDIT  \xe2\x80\x94  drag a fence vertex or circle centre  ");
            else
                snprintf(hint, sizeof(hint),
                         "  EDIT  \xe2\x80\x94  drag a waypoint to move it  ");
            map_draw_instruction(wp, { win_w, win_h }, hint);
        }

        // ── Tactical overlay: A/C block, scale bar, north pointer ─────────────
        {
            ImFont*     fm  = g_font_micro ? g_font_micro : ImGui::GetFont();
            const ImU32 lbl = ui_col_label();

            draw_overlay_buttons(wp, has_pos, lat, lon);
            draw_ac_block(dl, wp, has_pos, lat, lon, heading, has_hdg, alt_rel, gs,
                          active_terrain_agl(fleet),
                          has_pos ? map_home_distance(active_vehicle(fleet), lat, lon) : std::string());

            // Scale bar, bottom-left: 120 px of ground truth at this latitude.
            {
                constexpr float BAR_PX = 120.0f;
                const double m_per_px = 156543.03392 *
                                        std::cos(g_map.center_lat * M_PI / 180.0) /
                                        (double)(1 << g_map.zoom);
                const double span = m_per_px * BAR_PX;
                char scale_s[48];
                if (span >= 1000.0) snprintf(scale_s, sizeof(scale_s), "%.1f km  \xc2\xb7  Z%d", span / 1000.0, g_map.zoom);
                else                snprintf(scale_s, sizeof(scale_s), "%.0f m  \xc2\xb7  Z%d",  span,          g_map.zoom);

                const float sx = wp.x + 10.0f;
                const float sy = wp.y + win_h - 16.0f;
                dl->AddLine({ sx, sy }, { sx + BAR_PX, sy }, lbl, 1.5f);
                dl->AddLine({ sx, sy - 4.0f }, { sx, sy + 4.0f }, lbl, 1.5f);
                dl->AddLine({ sx + BAR_PX, sy - 4.0f }, { sx + BAR_PX, sy + 4.0f }, lbl, 1.5f);
                ui_tracked_text(dl, fm, UI_SZ_MICRO, { sx, sy - 17.0f }, lbl, scale_s);
            }

            draw_north_pointer(dl, wp, win_w, win_h, 0.0f);
        }

        draw_attribution(dl, wp, win_w, win_h, nullptr);
    }

    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
}
