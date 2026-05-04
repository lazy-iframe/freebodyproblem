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


// frontend/main.cpp
// Dear ImGui ground-control display for freebodyproblem.
// Owns: link thread (UDP/TCP/Serial), shared state, GLFW/ImGui lifecycle.
// Each telemetry panel lives in its own file under frontend/widgets/.

// ── Platform socket abstraction ───────────────────────────────────────────────
#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <winsock2.h>
#  include <ws2tcpip.h>
   typedef int ssize_t;
#  define close_socket closesocket
static inline void init_sockets()    { WSADATA w; WSAStartup(MAKEWORD(2,2), &w); }
static inline void cleanup_sockets() { WSACleanup(); }
#else
#  include <arpa/inet.h>
#  include <netinet/in.h>
#  include <sys/socket.h>
#  include <sys/select.h>
#  include <sys/time.h>
#  include <unistd.h>
#  include <fcntl.h>
#  include <termios.h>
#  define close_socket close
static inline void init_sockets()    {}
static inline void cleanup_sockets() {}
#endif

#include <atomic>
#include <chrono>
#include <deque>
#include <mutex>
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

// Settings
#include "settings.hpp"

// Widgets
#include "widgets/theme.hpp"
#include "widgets/topbar.hpp"
#include "widgets/sidebar_left.hpp"
#include "widgets/center_view.hpp"
#include "widgets/sidebar_right.hpp"
#include "widgets/map_view.hpp"
#include "widgets/bottombar.hpp"
#include "widgets/video_player.hpp"
#include "widgets/splash_screen.hpp"
#include "mission_pick.hpp"

using Clock = std::chrono::steady_clock;

// ── Shared state ──────────────────────────────────────────────────────────────

static std::mutex          g_mtx;
static VehicleState        g_state;
static std::unordered_map<std::string, ParamEntry> g_params;
static uint32_t            g_params_generation = 0;
static std::unordered_map<uint32_t, MessageStats> g_msg_stats;
static std::vector<StatusText> g_status_texts;
static uint64_t            g_total_messages = 0;
static uint64_t            g_total_bytes    = 0;
static uint64_t            g_parse_errors   = 0;

static std::atomic<bool>       g_link_running{false};
static std::atomic<LinkStatus> g_link_status{LinkStatus::Idle};

// App console log — written from any thread (under g_mtx), read by UI
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
        std::lock_guard<std::mutex> lk(g_mtx);
        if (g_app_log.size() >= APP_LOG_MAX) g_app_log.pop_front();
        g_app_log.push_back(buf);
    }
    fprintf(stderr, "[gcs] %s\n", buf);
}

// Outbound command queue — enqueued by UI thread, flushed by link thread
static MavlinkSender  g_sender;

// ── Serial helpers (POSIX) ────────────────────────────────────────────────────

#ifndef _WIN32
static speed_t baud_to_speed(int baud)
{
    switch (baud) {
    case 9600:   return B9600;
    case 19200:  return B19200;
    case 38400:  return B38400;
    case 57600:  return B57600;
    case 115200: return B115200;
    case 230400: return B230400;
    case 460800: return B460800;
    case 921600: return B921600;
    default:     return B57600;
    }
}
#endif

// ── Link thread ───────────────────────────────────────────────────────────────

struct LinkConfig {
    ConnType type    = ConnType::UDP;
    char     host[64]   = "0.0.0.0";
    int      port       = 14550;
    char     device[64] = {};
    int      baud       = 57600;
};

static constexpr double CONNECT_TIMEOUT_S = 10.0;

static void link_thread_fn(LinkConfig cfg)
{
    init_sockets();
    g_link_status.store(LinkStatus::Connecting);

    int  fd        = -1;
    bool is_stream = false;

    sockaddr_in vehicle_addr{};
    bool        vehicle_addr_set = false;

    // ── Open transport ────────────────────────────────────────────────────────
    if (cfg.type == ConnType::UDP) {
        fd = (int)socket(PF_INET, SOCK_DGRAM, 0);
        if (fd < 0) {
            perror("socket");
            g_link_status.store(LinkStatus::Error);
            cleanup_sockets();
            return;
        }
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        inet_pton(AF_INET, cfg.host, &addr.sin_addr);
        addr.sin_port = htons((uint16_t)cfg.port);
        if (bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
            perror("bind");
            close_socket(fd);
            g_link_status.store(LinkStatus::Error);
            cleanup_sockets();
            return;
        }
        // Short poll interval so we can check g_link_running and the timeout
#ifdef _WIN32
        DWORD tv_ms = 200;
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv_ms, sizeof(tv_ms));
#else
        timeval tv{0, 200'000};
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif
        is_stream = false;

    } else if (cfg.type == ConnType::TCP) {
#ifndef _WIN32
        fd = (int)socket(PF_INET, SOCK_STREAM, 0);
        if (fd < 0) {
            perror("socket");
            g_link_status.store(LinkStatus::Error);
            cleanup_sockets();
            return;
        }
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        inet_pton(AF_INET, cfg.host, &addr.sin_addr);
        addr.sin_port = htons((uint16_t)cfg.port);

        // Non-blocking connect with 10 s timeout
        int flags = fcntl(fd, F_GETFL, 0);
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
        int rc = connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
        if (rc == -1 && errno == EINPROGRESS) {
            fd_set wfds, efds;
            FD_ZERO(&wfds); FD_SET(fd, &wfds);
            FD_ZERO(&efds); FD_SET(fd, &efds);
            timeval tv{(long)CONNECT_TIMEOUT_S, 0};
            rc = select(fd + 1, nullptr, &wfds, &efds, &tv);
            if (rc == 0) {
                gcs_log("TCP connect timeout");
                close_socket(fd);
                g_link_status.store(LinkStatus::Timeout);
                cleanup_sockets();
                return;
            }
            if (rc < 0 || FD_ISSET(fd, &efds)) {
                perror("connect");
                close_socket(fd);
                g_link_status.store(LinkStatus::Error);
                cleanup_sockets();
                return;
            }
            // Verify connection actually succeeded
            int err = 0; socklen_t elen = sizeof(err);
            getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &elen);
            if (err != 0) {
                gcs_log("TCP connect error: %s", strerror(err));
                close_socket(fd);
                g_link_status.store(LinkStatus::Error);
                cleanup_sockets();
                return;
            }
        } else if (rc != 0) {
            perror("connect");
            close_socket(fd);
            g_link_status.store(LinkStatus::Error);
            cleanup_sockets();
            return;
        }
        // Restore blocking with 200 ms receive timeout
        fcntl(fd, F_SETFL, flags);
        timeval tv{0, 200'000};
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif
        is_stream = true;

    } else { // Serial
#ifndef _WIN32
        fd = open(cfg.device, O_RDWR | O_NOCTTY);
        if (fd < 0) {
            perror("open serial");
            g_link_status.store(LinkStatus::Error);
            return;
        }
        termios tio{};
        tcgetattr(fd, &tio);
        cfmakeraw(&tio);
        const speed_t spd = baud_to_speed(cfg.baud);
        cfsetispeed(&tio, spd);
        cfsetospeed(&tio, spd);
        tio.c_cc[VTIME] = 2; // 0.2 s read timeout
        tio.c_cc[VMIN]  = 0;
        tcsetattr(fd, TCSANOW, &tio);
#endif
        is_stream = true;
    }

    // ── Receive / timeout loop ────────────────────────────────────────────────
    MavlinkParser parser;
    uint8_t buf[2048];
    bool     rates_requested    = false;
    auto     link_start         = Clock::now();
    uint32_t last_params_gen    = 0;
    uint32_t link_known_params_gen = 0;   // link-thread-local; avoids shared reads outside mutex
    auto     last_param_time    = Clock::now();
    size_t   last_status_count  = 0;      // track new STATUSTEXT arrivals for logging
    bool     logged_heartbeat   = false;
    bool     logged_fw_info     = false;
    bool     logged_mission     = false;
    VehicleState::UploadStatus last_upload_status = VehicleState::UploadStatus::Idle;

    while (g_link_running.load()) {
        ssize_t n = 0;

        if (!is_stream) {
            sockaddr_in src{};
            socklen_t   src_len = sizeof(src);
            n = recvfrom(fd, (char*)buf, sizeof(buf), 0,
                         reinterpret_cast<sockaddr*>(&src), &src_len);
            if (n > 0 && !vehicle_addr_set) {
                vehicle_addr     = src;
                vehicle_addr_set = true;
            }
        } else {
#ifndef _WIN32
            n = read(fd, buf, sizeof(buf));
#endif
            if (n > 0 && !vehicle_addr_set)
                vehicle_addr_set = true;
        }

        if (n > 0) {
            // Mark connected on first data
            if (g_link_status.load() == LinkStatus::Connecting) {
                g_link_status.store(LinkStatus::Connected);
                gcs_log("link established");
            }

            // First MAVLink message: request telemetry rates and firmware info
            if (!rates_requested) {
                rates_requested = true;
                g_sender.request_message_interval(1, 1,  30,  50000); // ATTITUDE           @ 20 Hz
                g_sender.request_message_interval(1, 1,  74, 100000); // VFR_HUD            @ 10 Hz
                g_sender.request_message_interval(1, 1,  33, 200000); // GLOBAL_POSITION_INT @  5 Hz
                g_sender.request_message_interval(1, 1,   1, 500000); // SYS_STATUS         @  2 Hz
                g_sender.request_message_interval(1, 1,  24, 500000); // GPS_RAW_INT        @  2 Hz
                g_sender.request_message_interval(1, 1, 193, 200000); // EKF_STATUS_REPORT  @  5 Hz
                g_sender.request_autopilot_capabilities(1, 1);
                gcs_log("telemetry rates configured");
            }

            parser.parse(buf, static_cast<size_t>(n));

            // Log command ACKs for user-visible commands
            for (const auto& ack : parser.pending_acks()) {
                g_sender.notify_ack(ack.command, ack.result);
                const char* cname = nullptr;
                switch (ack.command) {
                case 400: cname = "arm/disarm";   break;
                case  22: cname = "takeoff";      break;
                case  20: cname = "RTL";          break;
                case 176: cname = "mode change";  break;
                case 218: cname = "aux function"; break;
                default:  break;
                }
                if (cname) {
                    const char* result = (ack.result == 0) ? "accepted"
                                       : (ack.result == 1) ? "temp. rejected"
                                       : (ack.result == 2) ? "denied"
                                       : (ack.result == 4) ? "failed"
                                                           : "rejected";
                    gcs_log("ack: %s → %s", cname, result);
                }
            }
            parser.clear_acks();

            // Drain per-item mission requests generated by MISSION_COUNT /
            // MISSION_ITEM_INT handlers (one request triggers the next).
            if (vehicle_addr_set) {
                for (const auto& req : parser.pending_mission_reqs())
                    g_sender.request_mission_item(req.tsys, req.tcomp, req.seq);
            }
            parser.clear_mission_reqs();

            // Drain FC's MISSION_REQUEST_INT during mission upload
            if (vehicle_addr_set) {
                for (const auto& req : parser.pending_item_reqs())
                    g_sender.send_mission_item_at(req.tsys, req.tcomp, req.seq);
            }
            parser.clear_item_reqs();

            const auto& ps = parser.state();

            // Log new vehicle STATUSTEXT messages to the bottom bar
            {
                const auto& sts = parser.status_texts();
                static const char* const kSev[] = {
                    "EMRG","ALRT","CRIT","ERR","WARN","NOTE","INFO","DEBUG"
                };
                for (size_t i = last_status_count; i < sts.size(); ++i)
                    gcs_log("[vehicle/%s] %s", kSev[sts[i].severity & 7], sts[i].text);
                last_status_count = sts.size();
            }

            // Log first heartbeat
            if (ps.has_heartbeat && !logged_heartbeat) {
                logged_heartbeat = true;
                gcs_log("heartbeat: sys=%d comp=%d", (int)ps.sysid, (int)ps.compid);
            }

            // Log firmware info once received
            if (ps.has_fw_info && !logged_fw_info) {
                logged_fw_info = true;
                const char* ap = "unknown";
                if (ps.autopilot == MAV_AUTOPILOT_ARDUPILOTMEGA) {
                    switch (ps.type) {
                    case MAV_TYPE_FIXED_WING:   ap = "ArduPlane";  break;
                    case MAV_TYPE_GROUND_ROVER: ap = "ArduRover";  break;
                    case MAV_TYPE_SUBMARINE:    ap = "ArduSub";    break;
                    default:                    ap = "ArduCopter"; break;
                    }
                } else if (ps.autopilot == MAV_AUTOPILOT_PX4) {
                    ap = "PX4";
                }
                gcs_log("firmware: %s %s (%.8s)", ap, ps.fw_version, ps.fw_hash);
            }

            // Log mission completion once
            if (ps.has_mission && !logged_mission) {
                logged_mission = true;
                gcs_log("mission loaded: %u waypoints", (unsigned)ps.mission_count);
                g_sender.send_mission_ack(ps.sysid, ps.compid);
            }

            // Log upload result when status changes
            if (ps.upload_status != last_upload_status) {
                last_upload_status = ps.upload_status;
                if (ps.upload_status == VehicleState::UploadStatus::Accepted) {
                    gcs_log("mission upload accepted");
                    g_sender.clear_upload();
                } else if (ps.upload_status == VehicleState::UploadStatus::Failed) {
                    gcs_log("mission upload failed (result=%u)", (unsigned)ps.upload_ack_result);
                    g_sender.clear_upload();
                }
            }

            // Track last-param-arrival time for retransmit logic
            if (ps.params_generation != last_params_gen) {
                last_params_gen = ps.params_generation;
                last_param_time = Clock::now();
            }

            // Detect param fetch completion (link-thread-local comparison avoids shared reads)
            const bool params_just_completed =
                ps.param_count > 0 &&
                ps.params_received == ps.param_count &&
                ps.params_generation != link_known_params_gen;
            if (params_just_completed)
                link_known_params_gen = ps.params_generation;

            {
                std::lock_guard<std::mutex> lk(g_mtx);
                g_state          = ps;   // cheap: VehicleState is now all scalars
                g_msg_stats      = parser.msg_stats();
                g_total_messages = parser.total_messages();
                g_total_bytes    = parser.total_bytes();
                g_parse_errors   = parser.parse_errors();
                g_status_texts.assign(parser.status_texts().begin(),
                                      parser.status_texts().end());

                if (params_just_completed) {
                    g_params            = parser.params();
                    g_params_generation = ps.params_generation;
                }
            }

            if (params_just_completed)
                gcs_log("parameters loaded: %u params", (unsigned)ps.param_count);
        } else if (!rates_requested) {
            // No data yet — check 10 s timeout
            double elapsed = std::chrono::duration<double>(Clock::now() - link_start).count();
            if (elapsed >= CONNECT_TIMEOUT_S) {
                gcs_log("connection timeout after %.0f s", elapsed);
                g_link_status.store(LinkStatus::Timeout);
                break;
            }
        }

        // Flush outbound commands
        if (vehicle_addr_set) {
            if (!is_stream)
                g_sender.flush(fd, vehicle_addr);
            else
                g_sender.flush_stream(fd);
        }

        // Retransmit dropped PARAM_VALUE packets after a 2 s stall mid-fetch
        {
            const auto& ps = parser.state();
            if (vehicle_addr_set &&
                ps.param_count > 0 &&
                ps.params_received > 0 &&
                ps.params_received < ps.param_count)
            {
                double stall = std::chrono::duration<double>(
                                   Clock::now() - last_param_time).count();
                if (stall >= 2.0) {
                    std::unordered_set<uint16_t> have;
                    have.reserve(parser.params().size());
                    for (const auto& [key, e] : parser.params())
                        have.insert(e.index);

                    int missing = 0;
                    for (uint16_t idx = 0; idx < ps.param_count; ++idx) {
                        if (have.find(idx) == have.end()) {
                            g_sender.request_param_read(ps.sysid, ps.compid,
                                                        (int16_t)idx);
                            ++missing;
                        }
                    }
                    gcs_log("param fetch stalled at %u/%u — re-requesting %d missing",
                            (unsigned)ps.params_received, (unsigned)ps.param_count, missing);
                    last_param_time = Clock::now(); // rate-limit re-requests to once per 2 s
                }
            }
        }
    }

    close_socket(fd);
    cleanup_sockets();
}

// ── Link management ───────────────────────────────────────────────────────────

static std::thread g_link_thread;

static void start_link(const LinkConfig& cfg)
{
    if (g_link_thread.joinable()) {
        g_link_running = false;
        g_link_thread.join();
    }

    // Clear stale telemetry from the previous link
    {
        std::lock_guard<std::mutex> lk(g_mtx);
        g_state             = VehicleState{};
        g_params.clear();
        g_params_generation = 0;
        g_msg_stats.clear();
        g_status_texts.clear();
        g_total_messages    = 0;
        g_total_bytes       = 0;
        g_parse_errors      = 0;
    }

    g_link_running = true;
    g_link_status.store(LinkStatus::Idle);  // thread sets Connecting immediately
    g_link_thread  = std::thread(link_thread_fn, cfg);
}

// ── Frame render ──────────────────────────────────────────────────────────────

static ConnectionRequest g_conn_req{};
static AppSettings       g_settings{};
static bool              g_close_req = false;
static MissionPickState  g_mission_pick{};

// ── Splash screen ─────────────────────────────────────────────────────────────
static ImFont* g_font_splash_title = nullptr;
static bool    g_splash_done       = false;
static double  g_splash_start      = -1.0;

static void render_ui()
{
    g_conn_req.requested  = false;
    g_conn_req.disconnect = false;
    g_close_req           = false;

    VehicleState vs;
    std::unordered_map<std::string, ParamEntry> params;
    std::unordered_map<uint32_t, MessageStats> stats;
    std::vector<StatusText> status_texts;
    std::deque<std::string> app_log;
    uint64_t total_msg, total_bytes, errors;
    {
        std::lock_guard<std::mutex> lk(g_mtx);
        vs           = g_state;
        params       = g_params;
        stats        = g_msg_stats;
        status_texts = g_status_texts;
        app_log      = g_app_log;
        total_msg    = g_total_messages;
        total_bytes  = g_total_bytes;
        errors       = g_parse_errors;
    }

    draw_topbar(vs, stats, total_msg, total_bytes, errors, &g_sender,
                g_link_status.load(), &g_close_req);
    draw_sidebar_left(&g_sender, &vs, &g_conn_req, g_link_status.load(), &params, &g_settings,
                      &stats, total_msg, total_bytes, errors,
                      &g_mission_pick);
    draw_center_view(vs, &g_mission_pick);
    draw_sidebar_right(vs, status_texts, &g_sender, &g_settings);
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

    GLFWwindow* window = glfwCreateWindow(1280, 720, "freebodyproblem", nullptr, nullptr);
    if (!window) {
        fprintf(stderr, "glfwCreateWindow failed\n");
        glfwTerminate();
        return 1;
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
        if (g_settings.active_theme == THEME_DRACULA_NAME) {
            g_theme = dracula_theme_vars();
        } else {
            auto it = g_settings.themes.find(g_settings.active_theme);
            if (it != g_settings.themes.end())
                g_theme = it->second;
            // else: "Retro Amber" or unknown — ThemeVars defaults apply
        }
    }
    apply_global_theme(ImGui::GetStyle());

#ifdef GCS_FONT_PATH
    ImGui::GetIO().Fonts->AddFontFromFileTTF(GCS_FONT_PATH, 15.0f);
    g_font_splash_title = ImGui::GetIO().Fonts->AddFontFromFileTTF(GCS_FONT_PATH, 38.0f);
#endif
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

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
            if (g_link_thread.joinable()) {
                g_link_running = false;
                g_link_thread.join();
            }
            {
                std::lock_guard<std::mutex> lk(g_mtx);
                g_state             = VehicleState{};
                g_params.clear();
                g_params_generation = 0;
                g_msg_stats.clear();
                g_status_texts.clear();
                g_total_messages    = 0;
                g_total_bytes       = 0;
                g_parse_errors      = 0;
            }
            g_link_status.store(LinkStatus::Idle);
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
            start_link(cfg);
        }

        ImGui::Render();
        int fb_w, fb_h;
        glfwGetFramebufferSize(window, &fb_w, &fb_h);
        glViewport(0, 0, fb_w, fb_h);
        glClearColor(0.04f, 0.02f, 0.00f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    if (g_link_thread.joinable()) {
        g_link_running = false;
        g_link_thread.join();
    }

    settings_save(g_settings);

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
