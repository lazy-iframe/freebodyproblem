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


// frontend/main.cpp
// Dear ImGui ground-control display for freebodyproblem.
// Owns: link thread (UDP/TCP/Serial), shared state, GLFW/ImGui lifecycle.
// Each telemetry panel lives in its own file under frontend/widgets/.

// ── Platform socket abstraction ───────────────────────────────────────────────
// Sockets and serial ports live behind backend/link.cpp now; all this file
// still needs from the platform is the path to its own executable.
#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>   // GetModuleFileNameA / MAX_PATH
#else
#  include <unistd.h>    // readlink
#endif

#include <algorithm>
#include <atomic>
#include <chrono>
#include <deque>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <cstdarg>

// App log (bottom bar)
#include "app_log.hpp"

// ImGui + GLFW + OpenGL
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

// Backend
#include "../backend/mavlink_parser.hpp"
#include "../backend/mavlink_sender.hpp"
#include "../backend/connection.hpp"
#include "../backend/timesync.hpp"
#include "../backend/link.hpp"
#include "../backend/vehicle.hpp"
#include "../backend/fleet.hpp"

// Settings
#include "settings.hpp"

// Widgets
#include "widgets/app_icon.hpp"
#include "widgets/theme.hpp"
#include "widgets/vehicle_ui_state.hpp"
#include "widgets/ui_kit.hpp"
#include "widgets/topbar.hpp"
#include "widgets/sidebar_left.hpp"
#include "widgets/center_view.hpp"
#include "widgets/sidebar_right.hpp"
#include "widgets/map_view.hpp"
#include "widgets/bottombar.hpp"
#include "widgets/video_player.hpp"
#include "widgets/splash_screen.hpp"
#include "mission_pick.hpp"
#include "audio.hpp"
#include "../plugins/plugin_api.hpp"

using Clock = std::chrono::steady_clock;

// ── Shared state ──────────────────────────────────────────────────────────────

// Every link and every vehicle behind them, each with its own thread. What used
// to be a single link thread writing a single VehicleState now lives here; this
// file keeps only the log and whichever vehicle the operator is looking at.
static Fleet g_fleet;

// Handed to the widgets when there is no vehicle. Every panel takes a
// MavlinkSender* unconditionally and always has, so the alternative is a null
// check at several dozen call sites for a case where the answer is always "do
// nothing". Nothing ever flushes this one, so commands aimed at a vehicle that
// is not there are discarded, which is what should happen to them.
static MavlinkSender g_null_sender;

// App console log — written from any thread, read by the UI.
//
// Its own mutex, not the one guarding vehicle state. Those used to be the same
// lock, which was harmless when one link thread wrote and one UI thread read.
// With a thread per vehicle all logging at once, sharing a lock with the UI's
// per-frame snapshot copy would make the log a convoy point.
static std::mutex              g_log_mtx;
static std::deque<std::string> g_app_log;
static constexpr size_t        APP_LOG_MAX = 200;

void gcs_log(const char* fmt, ...)
{
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    {
        std::lock_guard<std::mutex> lk(g_log_mtx);
        if (g_app_log.size() >= APP_LOG_MAX) g_app_log.pop_front();
        g_app_log.push_back(buf);
    }
    fprintf(stderr, "[gcs] %s\n", buf);
}


// ── Asset resolution ──────────────────────────────────────────────────────────
//
// Fonts have to be found in three situations: from an installed package, from a
// portable directory sitting next to the binary, and from the source tree during
// development. Everything is resolved relative to the executable so the install
// prefix stays relocatable — a path baked in at build time only ever works on
// the machine that did the build.

static std::string executable_dir()
{
#ifdef _WIN32
    char buf[MAX_PATH] = {};
    const DWORD n = GetModuleFileNameA(nullptr, buf, sizeof(buf));
    if (n == 0) return ".";
    const std::string p(buf, n);
#else
    char buf[4096] = {};
    const ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (n <= 0) return ".";
    const std::string p(buf, static_cast<size_t>(n));
#endif
    const size_t slash = p.find_last_of("/\\");
    return (slash == std::string::npos) ? std::string(".") : p.substr(0, slash);
}

// First readable candidate wins; empty string when the asset is nowhere.
static std::string find_asset(const std::string& relative,
                              const char*        build_time_path)
{
    const std::string exe = executable_dir();
    std::vector<std::string> candidates = {
        exe + "/../share/freebodyproblem/" + relative,  // installed prefix
        exe + "/" + relative,                           // portable, side-by-side
    };
    if (build_time_path && *build_time_path)
        candidates.emplace_back(build_time_path);       // source tree, dev build

    for (const std::string& c : candidates) {
        std::ifstream f(c, std::ios::binary);
        if (f.good()) return c;
    }
    return {};
}

// ── Frame render ──────────────────────────────────────────────────────────────

static ConnectionRequest g_conn_req{};
static AppSettings       g_settings{};
static bool              g_close_req = false;
static MissionPickState  g_mission_pick{};

// Every vehicle the map draws, rebuilt each frame from the per-vehicle pump
// loop below. File scope only so the vector keeps its capacity across frames.
static std::vector<MapVehicle> g_map_fleet;

// ── Splash screen ─────────────────────────────────────────────────────────────
static ImFont* g_font_splash_title = nullptr;
static bool    g_splash_done       = false;
static double  g_splash_start      = -1.0;

static void render_ui()
{
    g_conn_req.requested          = false;
    g_conn_req.disconnect         = false;
    g_conn_req.disconnect_link_id = 0;
    g_close_req           = false;

    // The vehicle on screen. Held for the whole frame so that a link dropping on
    // another thread cannot destroy it midway through drawing it.
    std::shared_ptr<Vehicle> veh = g_fleet.active();

    VehicleSnapshot snap;
    if (veh) veh->snapshot(snap);

    // Bound to the names the panels below already use. An absent vehicle leaves
    // these default-constructed, which is exactly the disconnected screen.
    VehicleState& vs                = snap.state;
    auto&         params            = snap.params;
    auto&         stats             = snap.msg_stats;
    auto&         status_texts      = snap.status_texts;
    const uint64_t total_msg        = snap.total_messages;
    const uint64_t total_bytes      = snap.total_bytes;
    const uint64_t errors           = snap.parse_errors;

    MavlinkSender* sender = veh ? &veh->sender() : &g_null_sender;

    std::deque<std::string> app_log;
    {
        std::lock_guard<std::mutex> lk(g_log_mtx);
        app_log = g_app_log;
    }

    const LinkStatus link_status = g_fleet.display_status();

    // Armed cue, counted in HEARTBEATs rather than seconds of wall clock. At
    // the 1 Hz ArduPilot sends them, every fifth beat is a beep about every
    // five seconds — but it is the vehicle's pulse being counted, so it carries
    // what a clock cannot: beeping that stops while the aircraft is still armed
    // means the link went, not that the vehicle disarmed.
    //
    // The count resets while disarmed, so arming beeps on its own heartbeat
    // instead of up to five beats later.
    {
        constexpr uint32_t ARMED_BEEP_EVERY = 5;   // heartbeats

        static uint32_t prev_beat   = 0;
        static uint32_t armed_beats = 0;

        if (vs.heartbeat_count != prev_beat) {
            prev_beat = vs.heartbeat_count;
            if (!vs.armed) {
                armed_beats = 0;
            } else {
                if (armed_beats % ARMED_BEEP_EVERY == 0)
                    gcs_tone(GcsTone::Armed);
                ++armed_beats;
            }
        }
    }

    // Link transitions are watched here rather than at the six places the link
    // thread sets a status: one edge, one tone, whatever the cause.
    {
        static LinkStatus prev_link = LinkStatus::Idle;
        const LinkStatus  now_link  = link_status;
        if (now_link != prev_link) {
            if (now_link == LinkStatus::Connected)
                gcs_tone(GcsTone::Success);
            else if (now_link == LinkStatus::Error ||
                     now_link == LinkStatus::Timeout)
                gcs_tone(GcsTone::Failure);
            prev_link = now_link;
        }
    }

    // Fullscreen feed hides both sidebars; the topbar stays, so link state,
    // arming and the annunciators never go away. Read before the centre view
    // draws, so a toggle takes effect on the next frame — the video window is
    // submitted after the sidebars and simply covers them in the meantime.
    const bool video_full = center_view_video_fullscreen();

    // Map fullscreen keeps the left sidebar and takes the right one's width,
    // giving the attitude and the event log back as an overlay in the map's
    // own corner. Read here for the same reason as the feed's above: a toggle
    // takes effect on the next frame, not halfway through this one.
    const bool map_full = center_view_map_fullscreen();

    // Before any panel draws: a calibration sweep records from the live stream,
    // not from whether its tab happens to be visible — and now, not from whether
    // its aircraft is the one on screen. Every vehicle is pumped, so a compass
    // calibration on one keeps advancing while the operator watches another.
    {
        const double now_s = ImGui::GetTime();
        g_map_fleet.clear();
        for (const auto& v : g_fleet.vehicles()) {
            VehicleSnapshot vsnap;
            v->snapshot(vsnap);
            ui_bind_vehicle(v->id());
            rc_tab_pump(&vsnap.state);
            sensors_tab_pump(&vsnap.state, vsnap.status_texts, now_s);
            map_track_pump(vsnap.state);

            // The map draws the whole fleet, and this loop is already holding
            // every vehicle's snapshot — so its list is built here rather than
            // by snapshotting all of them a second time further down.
            MapVehicle mv;
            mv.id      = v->id();
            mv.number  = v->number();
            mv.sysid   = v->sysid();   // from the id, so it is set before the first snapshot
            mv.lat     = vsnap.state.lat;
            mv.lon     = vsnap.state.lon;
            mv.has_pos = vsnap.state.has_global_pos;
            mv.heading = (float)vsnap.state.heading;
            mv.has_hdg = vsnap.state.has_vfr;
            mv.active   = veh && (v->id() == veh->id());
            mv.home_lat = vsnap.state.home_lat;
            mv.home_lon = vsnap.state.home_lon;
            mv.has_home = vsnap.state.has_home;
            g_map_fleet.push_back(mv);
        }

        // Fleet::vehicles() comes out of a hash map, so the order changes as
        // vehicles come and go. Sorting by the display number keeps overlapping
        // labels stacked the same way from frame to frame.
        std::sort(g_map_fleet.begin(), g_map_fleet.end(),
                  [](const MapVehicle& a, const MapVehicle& b) {
                      return a.number < b.number;
                  });
    }

    // Forget the panel state of vehicles that have gone, so a link that is
    // disconnected and reopened does not inherit the last airframe's half-run
    // calibration or its staged RC edits.
    {
        static std::vector<VehicleId> known;
        std::vector<VehicleId> now_ids;
        for (const auto& v : g_fleet.vehicles()) now_ids.push_back(v->id());

        for (const VehicleId& old_id : known) {
            if (std::find(now_ids.begin(), now_ids.end(), old_id) == now_ids.end())
                ui_forget_vehicle(old_id);
        }
        known.swap(now_ids);
    }

    // Everything drawn from here on reads the vehicle on screen. Panels that
    // keep per-vehicle state look this up rather than holding a static of their
    // own — see widgets/vehicle_ui_state.hpp.
    ui_bind_vehicle(veh ? veh->id() : VehicleId{});

    // The callsign chip doubles as the vehicle switcher, so the topbar needs the
    // fleet as well as the vehicle it is drawing.
    const std::vector<VehicleChip> chips  = g_fleet.chips();
    VehicleId                      picked{};

    draw_topbar(vs, stats, total_msg, total_bytes, errors, sender,
                link_status, &g_close_req, chips, g_fleet.active_id(), &picked);
    if (picked.valid()) g_fleet.set_active(picked);

    if (!video_full)
        draw_sidebar_left(sender, &vs, &g_conn_req, link_status, &params, &g_settings,
                          &stats, total_msg, total_bytes, errors,
                          g_fleet.links(), &g_mission_pick);
    draw_center_view(vs, sender, &g_mission_pick, &g_map_fleet);
    if (!video_full && !map_full)
        draw_sidebar_right(vs, status_texts, sender, &g_settings);

    // After the centre view, which is what puts it over the map.
    if (map_full)
        draw_map_overlay(vs, status_texts);
}

// ── main ──────────────────────────────────────────────────────────────────────

int main()
{
    video_player_init();

    if (!glfwInit()) {
        fprintf(stderr, "glfwInit failed\n");
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // How a desktop environment ties this window back to its .desktop file, and
    // so to the installed icon. It matters most on Wayland, which has no
    // protocol for a client to hand over icon pixels at all: there the
    // compositor shows whatever the app id resolves to and nothing else, so an
    // app id that matches StartupWMClass in the desktop entry is the icon.
#ifdef GLFW_WAYLAND_APP_ID
    glfwWindowHintString(GLFW_WAYLAND_APP_ID,   "freebodyproblem");
#endif
#ifdef GLFW_X11_CLASS_NAME
    glfwWindowHintString(GLFW_X11_CLASS_NAME,    "freebodyproblem");
    glfwWindowHintString(GLFW_X11_INSTANCE_NAME, "freebodyproblem");
#endif

    GLFWwindow* window = glfwCreateWindow(1280, 720, "freebodyproblem", nullptr, nullptr);
    if (!window) {
        fprintf(stderr, "glfwCreateWindow failed\n");
        glfwTerminate();
        return 1;
    }

    {
        // Resolved the same way the fonts are, so this works from an install
        // prefix, from a portable directory, and from the build tree.
        std::vector<std::string> icons;
        for (int i = 0; i < APP_ICON_SIZE_COUNT; ++i) {
            char rel[64], built[512];
            snprintf(rel,   sizeof(rel),   "icons/freebodyproblem-%d.png",
                     APP_ICON_SIZES[i]);
            snprintf(built, sizeof(built), "%s/freebodyproblem-%d.png",
                     GCS_ICON_DIR, APP_ICON_SIZES[i]);
            std::string p = find_asset(rel, built);
            if (!p.empty()) icons.push_back(std::move(p));
        }
        app_set_window_icon(window, icons);
    }

    glfwMakeContextCurrent(window);
    glfwMaximizeWindow(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    // Load persisted settings and apply the saved theme
    settings_load(g_settings);
    map_view_set_tile_source(g_settings.tile_url, g_settings.tile_attribution);
    {
        if (g_settings.active_theme == THEME_MATRIX_NAME) {
            g_theme = matrix_theme_vars();
        } else if (g_settings.active_theme == THEME_AMBER_NAME) {
            g_theme = retro_amber_theme_vars();
        } else {
            auto it = g_settings.themes.find(g_settings.active_theme);
            if (it != g_settings.themes.end()) {
                g_theme = it->second;
            } else {
                // "Tactical", or a name that no longer exists — a built-in
                // theme dropped from the build stays behind in settings.json.
                // The defaults apply either way, so the name is put back to
                // match them: leaving it would label the selector with a theme
                // that is not the one on screen.
                g_theme = ThemeVars{};
                g_settings.active_theme = THEME_BUILTIN_NAME;
            }
        }
    }
    apply_global_theme(ImGui::GetStyle());

#ifdef GCS_FONT_PATH
    // Monospace throughout — columns of telemetry stay aligned frame to frame.
    // Latin-1 plus the punctuation the panels use: en/em dash, middot, arrows.
    static const ImWchar kGlyphRanges[] = {
        0x0020, 0x00FF,   // basic Latin + Latin-1 supplement (°, ·, ×)
        0x2010, 0x2027,   // dashes, quotes, ellipsis
        0x2190, 0x2199,   // arrows
        0,
    };
    ImFontAtlas* atlas = ImGui::GetIO().Fonts;

    const std::string mono = find_asset("fonts/GeistMono-Regular.ttf", GCS_FONT_PATH);
    if (!mono.empty()) {
        g_font_ui           = atlas->AddFontFromFileTTF(mono.c_str(), UI_SZ_BODY,  nullptr, kGlyphRanges);
        g_font_micro        = atlas->AddFontFromFileTTF(mono.c_str(), UI_SZ_MICRO, nullptr, kGlyphRanges);
        g_font_value        = atlas->AddFontFromFileTTF(mono.c_str(), 26.0f,       nullptr, kGlyphRanges);
        g_font_splash_title = atlas->AddFontFromFileTTF(mono.c_str(), 38.0f,       nullptr, kGlyphRanges);
    }
    if (!g_font_ui) {   // font file missing — fall back to the bundled Roboto
        const std::string alt = find_asset("fonts/Roboto-Medium.ttf", GCS_FONT_FALLBACK_PATH);
        if (!alt.empty()) {
            g_font_ui           = atlas->AddFontFromFileTTF(alt.c_str(), UI_SZ_BODY);
            g_font_micro        = g_font_ui;
            g_font_value        = atlas->AddFontFromFileTTF(alt.c_str(), 26.0f);
            g_font_splash_title = atlas->AddFontFromFileTTF(alt.c_str(), 38.0f);
        }
    }
    // Both missing: leave FontDefault null so ImGui uses its built-in font
    // rather than rendering nothing.
    if (g_font_ui) ImGui::GetIO().FontDefault = g_font_ui;
#endif
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Audio comes up after settings, so a saved mute is in force before the
    // first tone can play. No device is not a failure — audio_init() says so
    // once in the log and every later call is a no-op.
    audio_set_enabled(g_settings.audio_enabled);
    audio_set_volume (g_settings.audio_volume);
    audio_init();

    // User plugins get their startup pass here: last thing before the first
    // frame, so a hook may rename its button, open a device or start a worker
    // with the video and settings stack already up behind it.
    gcs_plugins_init();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        render_ui();

        if (!g_splash_done) {
            if (g_splash_start < 0.0)
                g_splash_start = glfwGetTime();
            float elapsed = (float)(glfwGetTime() - g_splash_start);

            // Any key press or mouse click dismisses the splash early
            ImGuiIO& io = ImGui::GetIO();
            if (io.MouseClicked[0] || io.MouseClicked[1] || io.MouseClicked[2])
                g_splash_done = true;
            if (!g_splash_done) {
                for (int k = ImGuiKey_NamedKey_BEGIN; k < ImGuiKey_NamedKey_END; ++k) {
                    if (ImGui::IsKeyPressed((ImGuiKey)k, false)) {
                        g_splash_done = true;
                        break;
                    }
                }
            }

            if (!g_splash_done)
                g_splash_done = draw_splash_screen(elapsed, g_font_splash_title);
        }

        if (g_close_req)
            glfwSetWindowShouldClose(window, GLFW_TRUE);

        if (g_conn_req.disconnect) {
            // The CONNECTION tab names the link to drop, since several are live
            // at once. Zero is the catch-all for anything that still asks for
            // "disconnect" without saying which.
            if (g_conn_req.disconnect_link_id != 0)
                g_fleet.disconnect(g_conn_req.disconnect_link_id);
            else
                g_fleet.disconnect_all();
            gcs_log("disconnected");
        } else if (g_conn_req.requested) {
            LinkConfig cfg;
            cfg.type = g_conn_req.type;
            cfg.port = g_conn_req.port;
            cfg.baud = g_conn_req.baud;
            strncpy(cfg.host,   g_conn_req.host,   sizeof(cfg.host)   - 1);
            strncpy(cfg.device, g_conn_req.device, sizeof(cfg.device) - 1);
            gcs_log("connecting: type=%d  host=%s  port=%d  dev=%s  baud=%d",
                    (int)cfg.type, cfg.host, cfg.port, cfg.device, cfg.baud);
            g_fleet.connect(cfg);
        }

        ImGui::Render();
        int fb_w, fb_h;
        glfwGetFramebufferSize(window, &fb_w, &fb_h);
        glViewport(0, 0, fb_w, fb_h);
        glClearColor(g_theme.bg_panel.x, g_theme.bg_panel.y, g_theme.bg_panel.z, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // Links first, then the vehicles they were feeding: once no link thread is
    // running, nothing can push into a vehicle's inbox or reach for its sender.
    g_fleet.disconnect_all();

    settings_save(g_settings);

    audio_shutdown();
    center_view_shutdown();   // stop video pipeline + delete GL texture
    map_view_shutdown();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    video_player_shutdown();
    return 0;
}
