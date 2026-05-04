// Free Body Problem: A modern, simple and minimalist Ground Control Station
// Copyright (C) 2026  Azhar Tanweer

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.


// frontend/widgets/map_view.cpp
//
// Slippy tile map backed by ESRI World Imagery satellite tiles.
// Tile fetching runs on a pool of background threads; decoded pixels are
// queued for GL texture upload on the main thread.
//
// ESRI World Imagery is free to use (same source as QGroundControl satellite view).
// Attribution must be displayed:
//   Tiles © Esri — Source: Esri, Maxar, Earthstar Geographics
//   Reasonable caching (tiles are cached on disk across sessions)
//
// Performance improvements over v1:
//   - Thread pool (N_WORKERS threads), one persistent SSL connection per worker
//   - Disk tile cache to skip network round-trips on re-visit
//   - Stale-request pruning when zoom changes
//   - Parent-tile fallback rendering while tiles load

#include "map_view.hpp"
#include "layout.hpp"
#include "imgui.h"

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"

#include "stb_image.h"

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <shlobj.h>
#else
#  include <errno.h>
#  include <sys/stat.h>
#endif
#include <GL/gl.h>

#ifndef GL_CLAMP_TO_EDGE
#  define GL_CLAMP_TO_EDGE 0x812F
#endif

#include <atomic>
#include <cmath>
#include <condition_variable>
#include <fstream>
#include <mutex>
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
#ifdef _WIN32
    char buf[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, buf)))
        g_cache_dir = std::string(buf) + "\\freebodyproblem\\tiles";
    else
        g_cache_dir = ".\\tile_cache";
#else
    const char* xdg = getenv("XDG_CACHE_HOME");
    if (xdg && *xdg) {
        g_cache_dir = std::string(xdg) + "/freebodyproblem/tiles";
    } else {
        const char* home = getenv("HOME");
        g_cache_dir = home ? std::string(home) + "/.cache/freebodyproblem/tiles"
                           : "./.tile_cache";
    }
#endif
}

// Recursively create directories (best-effort).
static void mkdirs_for_file(const std::string& filepath)
{
    size_t sep = filepath.rfind('/');
#ifdef _WIN32
    if (sep == std::string::npos) sep = filepath.rfind('\\');
#endif
    if (sep == std::string::npos) return;
    std::string dir = filepath.substr(0, sep);
    for (size_t i = 1; i <= dir.size(); ++i) {
        if (i == dir.size() || dir[i] == '/' || dir[i] == '\\') {
            std::string sub = dir.substr(0, i);
#ifdef _WIN32
            CreateDirectoryA(sub.c_str(), nullptr);
#else
            mkdir(sub.c_str(), 0755);
#endif
        }
    }
}

static std::string tile_cache_path(const TileKey& k)
{
    return g_cache_dir
         + "/" + std::to_string(k.z)
         + "/" + std::to_string(k.x)
         + "/" + std::to_string(k.y) + ".jpg";
}

// Returns empty vector on cache miss.
static std::vector<uint8_t> disk_load(const TileKey& k)
{
    std::ifstream f(tile_cache_path(k), std::ios::binary);
    if (!f) return {};
    return {std::istreambuf_iterator<char>(f), {}};
}

// Best-effort write; silently ignores errors.
static void disk_save(const TileKey& k, const void* data, size_t sz)
{
    std::string path = tile_cache_path(k);
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
    int                  w = 0, h = 0;
    std::vector<uint8_t> rgba;
};

// ── Tile source config (written from UI/main thread, read by workers) ─────────

static std::mutex  g_tile_cfg_mtx;
static std::string g_tile_url         = "https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/{z}/{y}/{x}";
static std::string g_tile_attribution = "\xc2\xa9 Esri \xe2\x80\x94 Source: Esri, Maxar, Earthstar Geographics";

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

static std::atomic<bool>        g_running{false};
static std::vector<std::thread> g_fetch_threads;

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

static void fetch_worker()
{
    // One persistent connection per worker, rebuilt whenever the tile host changes.
    std::string                      cur_host;
    std::unique_ptr<httplib::Client> cli;

    auto make_cli = [](const std::string& host) {
        auto c = std::make_unique<httplib::Client>(host);
        c->set_connection_timeout(8);
        c->set_read_timeout(10);
        c->set_follow_location(true);
        c->set_default_headers({{"User-Agent", "freebodyproblem-gcs/1.0"}});
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

        // ── Disk cache ────────────────────────────────────────────────────────
        auto cached = disk_load(key);
        if (!cached.empty()) {
            DecodedTile dt;
            dt.key = key;
            if (decode_png(cached.data(), (int)cached.size(), dt)) {
                std::lock_guard<std::mutex> ulk(g_upload_mtx);
                g_upload_queue.push(std::move(dt));
                continue;
            }
            // Corrupt cache entry — fall through to network fetch
        }

        // ── Network fetch ─────────────────────────────────────────────────────
        std::string url_tmpl;
        { std::lock_guard<std::mutex> lk(g_tile_cfg_mtx); url_tmpl = g_tile_url; }

        std::string host, path_tmpl;
        parse_tile_url(url_tmpl, host, path_tmpl);

        if (host != cur_host || !cli) {
            cur_host = host;
            cli = make_cli(cur_host);
        }

        std::string path = make_tile_path(path_tmpl, key.x, key.y, key.z);

        auto res = cli->Get(path);
        if (!res || res->status != 200) continue;

        disk_save(key, res->body.data(), res->body.size());

        DecodedTile dt;
        dt.key = key;
        const auto* body = reinterpret_cast<const uint8_t*>(res->body.data());
        if (!decode_png(body, (int)res->body.size(), dt)) continue;

        std::lock_guard<std::mutex> ulk(g_upload_mtx);
        g_upload_queue.push(std::move(dt));
    }
}

// Upload pending tiles to the GPU.  Must run on the GL thread.
static void drain_upload_queue()
{
    std::lock_guard<std::mutex> lk(g_upload_mtx);
    while (!g_upload_queue.empty()) {
        DecodedTile& dt = g_upload_queue.front();

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

static void queue_tile(const TileKey& key)
{
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
static void flush_stale_requests(int keep_zoom)
{
    std::lock_guard<std::mutex> lk(g_fetch_mtx);
    std::queue<TileKey> keep;
    while (!g_fetch_queue.empty()) {
        TileKey k = g_fetch_queue.front();
        g_fetch_queue.pop();
        if (k.z == keep_zoom) {
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
    for (int dz = 1; dz <= 3 && (z - dz) >= 0; ++dz) {
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

// ── Public API ────────────────────────────────────────────────────────────────

void map_view_set_tile_source(const std::string& url, const std::string& attribution)
{
    std::lock_guard<std::mutex> lk(g_tile_cfg_mtx);
    g_tile_url         = url;
    g_tile_attribution = attribution;
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

void draw_map_view(double lat, double lon, bool has_pos,
                   float heading, bool has_hdg,
                   float win_x, float win_y, float win_w, float win_h,
                   const std::vector<MissionItem>* mission,
                   MissionPickState* pick)
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
        ImGuiWindowFlags_NoScrollWithMouse;

    if (!ImGui::Begin("##map", nullptr, FLAGS)) {
        ImGui::End();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();
        return;
    }

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2      wp = ImGui::GetWindowPos();

    // ── Keep centered on vehicle until user pans ───────────────────────────────
    if (has_pos && (!g_map.initialized || g_map.follow)) {
        g_map.center_lat = lat;
        g_map.center_lon = lon;
        if (!g_map.initialized) {
            g_map.zoom        = 15;
            g_map.initialized = true;
        }
    }

    // ── Input ─────────────────────────────────────────────────────────────────
    const bool pick_active = pick && pick->active_index >= 0;

    if (ImGui::IsWindowHovered()) {
        if (pick_active) {
            // Show a hand cursor so the user knows the map is clickable
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        } else {
            // Pan: left-drag (suppressed during pick mode)
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
        if (pick_active && ImGui::IsWindowHovered() &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            const ImVec2 mouse = ImGui::GetMousePos();
            double tx = ((double)(mouse.x - wp.x) + tl_px) / TILE_PX;
            double ty = ((double)(mouse.y - wp.y) + tl_py) / TILE_PX;
            tile_frac_to_lat_lon(tx, ty, g_map.zoom,
                                 pick->picked_lat, pick->picked_lon);
            pick->pick_done = true;
            // active_index intentionally left set; tab_mission clears it after consuming
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

        // ── Vehicle marker ────────────────────────────────────────────────────
        if (has_pos) {
            double vx, vy;
            lat_lon_to_tile_frac(lat, lon, g_map.zoom, vx, vy);
            const float sx = wp.x + (float)(vx * TILE_PX - tl_px);
            const float sy = wp.y + (float)(vy * TILE_PX - tl_py);

            if (has_hdg) {
                // Convert heading (degrees CW from north) to screen angle.
                // Screen Y grows downward, so north = -Y → angle = hdg - 90° in
                // standard math terms, but we use (sin, -cos) directly.
                const float rad   = heading * (float)(M_PI / 180.0);
                const float dx    = std::sin(rad);   // east component  (+x on screen)
                const float dy    = -std::cos(rad);  // north component (-y on screen)

                // Arrow dimensions
                constexpr float BODY   = 18.0f; // distance from center to tip
                constexpr float TAIL   = 10.0f; // distance from center to tail
                constexpr float WING   = 7.0f;  // half-width at wing base
                constexpr float NOTCH  = 4.0f;  // tail notch depth

                // Key points (all relative to marker center)
                const ImVec2 tip   = { sx + dx * BODY,  sy + dy * BODY  };
                const ImVec2 tail_c= { sx - dx * TAIL,  sy - dy * TAIL  }; // center tail
                // Wing points perpendicular to heading
                const ImVec2 wL    = { sx - dy * WING,  sy + dx * WING  };
                const ImVec2 wR    = { sx + dy * WING,  sy - dx * WING  };
                // Tail notch pulls tail center slightly toward tip
                const ImVec2 notch = { sx - dx * (TAIL - NOTCH), sy - dy * (TAIL - NOTCH) };

                // Filled arrow body: tip → wingL → notch → wingR
                dl->AddQuadFilled(tip, wL, notch, wR, map_vehicle_fill());
                // Outline
                dl->AddQuad(tip, wL, notch, wR, map_vehicle_ring(), 1.5f);
                // Tail segment centre → notch
                dl->AddLine(tail_c, notch, map_vehicle_ring(), 1.5f);

                // Dotted heading line from tip outward
                constexpr float DOT_LEN   = 5.0f;
                constexpr float DOT_GAP   = 5.0f;
                constexpr float LINE_DIST = 80.0f; // total extent from tip
                for (float t = 0.0f; t < LINE_DIST; t += DOT_LEN + DOT_GAP) {
                    const ImVec2 p0 = { tip.x + dx * t,
                                        tip.y + dy * t };
                    const ImVec2 p1 = { tip.x + dx * std::min(t + DOT_LEN, LINE_DIST),
                                        tip.y + dy * std::min(t + DOT_LEN, LINE_DIST) };
                    dl->AddLine(p0, p1, map_vehicle_ring(), 1.5f);
                }
            } else {
                // No heading — plain dot marker
                dl->AddCircleFilled({sx, sy}, 9.0f, map_vehicle_fill());
                dl->AddCircle      ({sx, sy}, 9.0f, map_vehicle_ring(), 0, 2.0f);
            }
        }

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
            for (const auto& wpi : *display_mission) {
                if (wpi.lat == 0.0 && wpi.lon == 0.0) continue;
                const ImVec2 sp = to_screen(wpi.lat, wpi.lon);
                // Yellow ring when this is the waypoint being picked
                const bool being_picked = pick_active && ((int)wpi.seq == pick->active_index);
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
            }
        }

        // ── Pick-mode hint banner ─────────────────────────────────────────────
        if (pick_active) {
            char hint[56];
            snprintf(hint, sizeof(hint),
                     "  Click to place WP #%d  —  Esc to cancel  ",
                     pick->active_index);
            const ImVec2 hsz = ImGui::CalcTextSize(hint);
            const float  hx  = wp.x + (win_w - hsz.x) * 0.5f;
            const float  hy  = wp.y + 8.0f;
            dl->AddRectFilled({ hx,          hy          },
                              { hx + hsz.x,  hy + hsz.y  },
                              IM_COL32(0, 0, 0, 180), 4.0f);
            dl->AddText({ hx, hy }, IM_COL32(255, 230, 60, 255), hint);
        }

        // ── Tile source attribution ───────────────────────────────────────────
        std::string attr_str;
        { std::lock_guard<std::mutex> lk(g_tile_cfg_mtx); attr_str = g_tile_attribution; }
        const char* attr = attr_str.c_str();
        ImVec2 asz = ImGui::CalcTextSize(attr);
        float  ax  = wp.x + win_w - asz.x - 6.0f;
        float  ay  = wp.y + win_h - asz.y - 4.0f;
        dl->AddRectFilled({ax - 4.0f, ay - 2.0f},
                           {ax + asz.x + 4.0f, ay + asz.y + 2.0f},
                           map_attr_bg());
        dl->AddText({ax, ay}, map_attr_text(), attr);
    }

    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
}
