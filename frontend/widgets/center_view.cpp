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


#include "center_view.hpp"
#include "layout.hpp"
#include "ui_kit.hpp"
#include "map_view.hpp"
#include "plugin_rail.hpp"
#include "video_player.hpp"
#include "../app_log.hpp"
#include "../../plugins/plugin_api.hpp"   // gcs_camera_* are implemented here
#include "imgui.h"
#include "stb_image_write.h"

#include <GLFW/glfw3.h>   // brings in OpenGL3 core header
#include <cstring>
#include <vector>

// The Windows SDK ships the OpenGL 1.1 headers; GL_CLAMP_TO_EDGE is 1.2.
// The runtime supports it fine — only the declaration is missing.
// (Same workaround as map_view.cpp.)
#ifndef GL_CLAMP_TO_EDGE
#  define GL_CLAMP_TO_EDGE 0x812F
#endif

// ── Module-level state ────────────────────────────────────────────────────────

static VideoPlayer s_vp;
static GLuint      s_tex_id   = 0;
static int         s_tex_w    = 0;
static int         s_tex_h    = 0;

// Config form persistent state
static char s_url_buf[512] = "rtsp://";
static int  s_proto_sel    = 0;   // 0 = RTSP, 1 = UDP

// Centre view display mode
enum class CenterMode { MapAndVideo, VideoOnly, MapOnly };
static CenterMode s_mode = CenterMode::MapAndVideo;

static constexpr float HEADER_H = 30.0f;

// ── Helpers ───────────────────────────────────────────────────────────────────

static void delete_texture()
{
    if (s_tex_id) {
        glDeleteTextures(1, &s_tex_id);
        s_tex_id = 0;
        s_tex_w  = 0;
        s_tex_h  = 0;
    }
}

// Upload the latest frame to the GL texture (called on UI / OpenGL thread).
static void upload_frame()
{
    std::lock_guard<std::mutex> lk(s_vp.frame_mtx);
    if (!s_vp.new_frame || s_vp.frame_w <= 0 || s_vp.frame_h <= 0)
        return;

    const int w = s_vp.frame_w;
    const int h = s_vp.frame_h;

    if (!s_tex_id || w != s_tex_w || h != s_tex_h) {
        delete_texture();
        glGenTextures(1, &s_tex_id);
        glBindTexture(GL_TEXTURE_2D, s_tex_id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        s_tex_w = w;
        s_tex_h = h;
    } else {
        glBindTexture(GL_TEXTURE_2D, s_tex_id);
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, s_vp.frame_data.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    s_vp.new_frame = false;
}

// ── Video picking ─────────────────────────────────────────────────────────────
//
// A click on the picture, or a click dragged into a box, handed to whatever
// plugins registered for it. The gesture is read over the drawn image rect
// rather than the window, so the letterbox bars are dead space and the
// coordinates a handler receives are in the frame, not on the screen.

// Below this much travel the gesture is a click, above it a box. Enough to
// absorb the shake of a firm click without swallowing a deliberate small box.
static constexpr float PICK_DRAG_MIN_PX = 5.0f;

static bool   s_pick_dragging = false;
static ImVec2 s_pick_from{};      // screen position of the press

// Screen point → frame coordinates, clamped to the picture. A drag that leaves
// the image still resolves against its edge rather than reporting nonsense.
static VideoPoint frame_point(ImVec2 pos, ImVec2 p0, ImVec2 p1)
{
    const float w = (p1.x - p0.x) > 1.0f ? (p1.x - p0.x) : 1.0f;
    const float h = (p1.y - p0.y) > 1.0f ? (p1.y - p0.y) : 1.0f;

    float u = (pos.x - p0.x) / w;
    float v = (pos.y - p0.y) / h;
    u = u < 0.0f ? 0.0f : (u > 1.0f ? 1.0f : u);
    v = v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);

    // Pixel indices, not a scaled length: the rightmost column of a 1920-wide
    // frame is 1919, and a handler using this to index the image would run off
    // the end on an exact-edge click.
    int x = (int)(u * (float)s_tex_w);
    int y = (int)(v * (float)s_tex_h);
    if (x >= s_tex_w) x = s_tex_w - 1;
    if (y >= s_tex_h) y = s_tex_h - 1;
    if (x < 0) x = 0;
    if (y < 0) y = 0;

    return VideoPoint{ u, v, x, y };
}

static void draw_video_picker(const VehicleState& vs, MavlinkSender* sender,
                              ImVec2 p0, ImVec2 p1)
{
    const bool want_click = gcs_video_has_click_handlers();
    const bool want_box   = gcs_video_has_box_handlers();
    if (!want_click && !want_box) return;   // nobody listening: leave clicks alone
    if (s_tex_w <= 0 || s_tex_h <= 0)  return;

    // The picker covers the whole picture, including the stop button drawn over
    // it. Without this the picker — submitted first — would take the press and
    // leave the stop button dead; with it, a later overlapping item wins.
    ImGui::SetNextItemAllowOverlap();
    ImGui::SetCursorScreenPos(p0);
    ImGui::InvisibleButton("##video_pick", { p1.x - p0.x, p1.y - p0.y });

    if (ImGui::IsItemActivated()) {
        s_pick_from     = ImGui::GetMousePos();
        s_pick_dragging = true;
    }

    const ImVec2 now = ImGui::GetMousePos();
    const float  dx  = now.x - s_pick_from.x;
    const float  dy  = now.y - s_pick_from.y;
    const bool   is_box = (dx < 0 ? -dx : dx) > PICK_DRAG_MIN_PX ||
                          (dy < 0 ? -dy : dy) > PICK_DRAG_MIN_PX;

    // Rubber band while the drag is live — in the rail's blue, because this is
    // the plugin system reaching onto the feed.
    if (s_pick_dragging && want_box && ImGui::IsItemActive() && is_box) {
        ImDrawList*  dl = ImGui::GetWindowDrawList();
        const ImVec2 a  = { s_pick_from.x < now.x ? s_pick_from.x : now.x,
                            s_pick_from.y < now.y ? s_pick_from.y : now.y };
        const ImVec2 b  = { s_pick_from.x < now.x ? now.x : s_pick_from.x,
                            s_pick_from.y < now.y ? now.y : s_pick_from.y };
        dl->AddRectFilled(a, b, ui_col(plugin_accent(), 0.14f));
        ui_frame(dl, a, b, ui_col(plugin_accent()));
    }

    if (!ImGui::IsItemDeactivated() || !s_pick_dragging) return;
    s_pick_dragging = false;

    PluginContext ctx = center_view_plugin_context(vs, sender);

    if (is_box) {
        if (!want_box) return;
        const VideoPoint a = frame_point(s_pick_from, p0, p1);
        const VideoPoint b = frame_point(now, p0, p1);

        // Handed over as top-left / bottom-right whichever way it was dragged;
        // a handler should not have to sort corners before using them.
        VideoBox box;
        box.min = VideoPoint{ a.u < b.u ? a.u : b.u, a.v < b.v ? a.v : b.v,
                              a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y };
        box.max = VideoPoint{ a.u < b.u ? b.u : a.u, a.v < b.v ? b.v : a.v,
                              a.x < b.x ? b.x : a.x, a.y < b.y ? b.y : a.y };
        box.frame_w = s_tex_w;
        box.frame_h = s_tex_h;
        gcs_video_dispatch_box(ctx, box);
    } else {
        if (!want_click) return;
        VideoClick click;
        click.at      = frame_point(now, p0, p1);
        click.frame_w = s_tex_w;
        click.frame_h = s_tex_h;
        gcs_video_dispatch_click(ctx, click);
    }
}

// ── draw_center_view ──────────────────────────────────────────────────────────

void draw_center_view(const VehicleState& vs, MavlinkSender* sender,
                      MissionPickState* pick)
{
    const GcsLayout l = GcsLayout::compute();

    // ── Poll GStreamer bus for errors / EOS ───────────────────────────────────
    {
        VideoState prev = s_vp.state.load(std::memory_order_acquire);
        video_player_poll(s_vp);
        VideoState cur  = s_vp.state.load(std::memory_order_acquire);
        if (prev != VideoState::Error && cur == VideoState::Error) {
            gcs_log("video error: %s", s_vp.error_msg);
            delete_texture();
        }
    }

    // ── Header bar — title + mode toggle buttons ──────────────────────────────
    // Spans the whole middle band (video + plugin rail), not just the video, so
    // the mode buttons stay pinned to the right edge whichever mode is showing.
    ImGui::SetNextWindowPos ({l.center_x, l.top}, ImGuiCond_Always);
    ImGui::SetNextWindowSize({l.band_w, HEADER_H}, ImGuiCond_Always);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, g_theme.bg_topbar);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {8.f, 0.f});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    if (ImGui::Begin("##cv_header", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse)) {

        // Title strip: what the centre column is currently showing.
        {
            ImDrawList*  dl = ImGui::GetWindowDrawList();
            const ImVec2 wp = ImGui::GetWindowPos();
            const char*  title =
                (s_mode == CenterMode::VideoOnly) ? "SENSOR FEED"
              : (s_mode == CenterMode::MapOnly)   ? "CARTOGRAPH \xe2\x80\x94 2D / NORTH-UP"
                                                  : "SENSOR FEED / CARTOGRAPH";
            ui_panel_header_at(dl, { wp.x, wp.y }, l.band_w, title, nullptr,
                               1.0f, HEADER_H);
        }

        // Mode buttons — right-aligned segmented control
        const char*  btn_labels[] = { "VIDEO##m0", "MAP##m1", "MAP + VIDEO##m2" };
        const CenterMode btn_modes[] = {
            CenterMode::VideoOnly, CenterMode::MapOnly, CenterMode::MapAndVideo };
        constexpr float BTN_W = 108.f, BTN_H = 22.f, BTN_GAP = 3.f;
        constexpr int   N = 3;
        const float     btns_total = BTN_W * N + BTN_GAP * (N - 1);
        const float     btns_x     = l.band_w - btns_total - 8.f;

        for (int i = 0; i < N; ++i) {
            ImGui::SetCursorPos({ btns_x + i * (BTN_W + BTN_GAP),
                                  (HEADER_H - BTN_H) * 0.5f });
            if (ui_tab_button(btn_labels[i], { BTN_W, BTN_H }, s_mode == btn_modes[i]))
                s_mode = btn_modes[i];
        }
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();

    // ── Compute content area (below header) ───────────────────────────────────
    const float content_top = l.top + HEADER_H;
    const float content_h   = l.total_h - HEADER_H;

    // Video pane is a true 16:9 box across the centre column; the map takes
    // whatever height is left below it.
    float vid_h = l.center_w * (9.f / 16.f);
    float map_h = content_h - vid_h;
    if (map_h < 80.f) { map_h = 80.f; vid_h = content_h - map_h; }

    if (s_mode == CenterMode::VideoOnly) { vid_h = content_h; map_h = 0.f; }
    if (s_mode == CenterMode::MapOnly)   { map_h = content_h; vid_h = 0.f; }

    // ── Video window ──────────────────────────────────────────────────────────
    if (s_mode != CenterMode::MapOnly) {
    ImGui::SetNextWindowPos ({ l.center_x, content_top }, ImGuiCond_Always);
    ImGui::SetNextWindowSize({ l.center_w, vid_h       }, ImGuiCond_Always);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, bg_video());
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,  { 0.0f, 0.0f });
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

    const ImGuiWindowFlags win_flags =
        ImGuiWindowFlags_NoTitleBar          |
        ImGuiWindowFlags_NoResize            |
        ImGuiWindowFlags_NoMove              |
        ImGuiWindowFlags_NoScrollbar         |
        ImGuiWindowFlags_NoScrollWithMouse;

    if (ImGui::Begin("##video", nullptr, win_flags)) {
        const VideoState state = s_vp.state.load(std::memory_order_acquire);

        // Feed banner — drawn last so it sits over the frame (see below).
        const ImVec2 vwp = ImGui::GetWindowPos();

        // ── Playing ───────────────────────────────────────────────────────────
        if (state == VideoState::Playing) {
            upload_frame();

            if (s_tex_id) {
                // Fit the frame inside the window while preserving aspect ratio
                // (letterbox / pillarbox — never stretch).
                const float src_aspect = (float)s_tex_w / (float)s_tex_h;
                const float win_aspect = l.center_w / vid_h;

                float draw_w, draw_h;
                if (src_aspect >= win_aspect) {
                    draw_w = l.center_w;
                    draw_h = l.center_w / src_aspect;
                } else {
                    draw_h = vid_h;
                    draw_w = vid_h * src_aspect;
                }

                const ImVec2 wp = ImGui::GetWindowPos();
                const float  ox = (l.center_w - draw_w) * 0.5f;
                const float  oy = (vid_h - draw_h) * 0.5f;
                const ImVec2 p0 = { wp.x + ox,          wp.y + oy          };
                const ImVec2 p1 = { wp.x + ox + draw_w, wp.y + oy + draw_h };
                ImGui::GetWindowDrawList()->AddImage(
                    (ImTextureID)(intptr_t)s_tex_id, p0, p1);

                // Picking is submitted before the stop button so the stop
                // button wins the overlap: ImGui hands hover to the last item
                // under the cursor, and the picker covers the whole picture.
                draw_video_picker(vs, sender, p0, p1);
            }

            // Small red stop button — upper-left corner
            ImGui::SetCursorPos({ 8.0f, UI_HEADER_H + 8.0f });
            if (ui_solid_button("##stop", { 20.0f, 20.0f },
                                btn_stop_base(), btn_stop_hov())) {
                gcs_log("video stream stopped");
                video_player_stop(s_vp);
                delete_texture();
            }

        // ── Connecting ────────────────────────────────────────────────────────
        } else if (state == VideoState::Connecting) {
            const ImVec2 wsz = ImGui::GetWindowSize();
            const char*  msg = "CONNECTING...";
            const ImVec2 tsz = ImGui::CalcTextSize(msg);
            ImGui::SetCursorPos({ (wsz.x - tsz.x) * 0.5f,
                                  (wsz.y - tsz.y) * 0.5f });
            ImGui::TextDisabled("%s", msg);

            // Stop button while connecting
            ImGui::SetCursorPos({ 8.0f, UI_HEADER_H + 8.0f });
            if (ui_solid_button("##stop", { 20.0f, 20.0f },
                                btn_stop_base(), btn_stop_hov())) {
                gcs_log("video connection cancelled");
                video_player_stop(s_vp);
            }

        // ── Idle / Error — configuration form ─────────────────────────────────
        } else {
            const ImVec2 wsz    = ImGui::GetWindowSize();
            const float  form_w = 340.0f;
            const bool   is_err = (state == VideoState::Error);

            // Estimate form height to vertically centre it
            const float form_h = 22.0f           // protocol tabs
                                + 4.0f            // spacing
                                + 22.0f           // url input
                                + 6.0f            // spacing
                                + (is_err ? 18.0f + 4.0f : 0.0f)  // error line
                                + 26.0f;          // connect button

            ImGui::SetCursorPos({ (wsz.x - form_w) * 0.5f,
                                  (wsz.y - form_h) * 0.5f });
            ImGui::BeginGroup();

            // ── Protocol selector ─────────────────────────────────────────────
            const char* const proto_labels[] = { "RTSP", "UDP" };
            const char* const proto_prefixes[] = { "rtsp://", "udp://" };
            for (int i = 0; i < 2; ++i) {
                if (i > 0) ImGui::SameLine(0, 4);
                if (ui_tab_button(proto_labels[i], { 80.0f, 24.0f }, s_proto_sel == i)) {
                    s_proto_sel = i;
                    // Replace only if the current url matches the old prefix
                    const char* old_pfx = proto_prefixes[1 - i];
                    if (std::strncmp(s_url_buf, old_pfx, std::strlen(old_pfx)) == 0)
                        std::snprintf(s_url_buf, sizeof(s_url_buf),
                                      "%s%s", proto_prefixes[i],
                                      s_url_buf + std::strlen(old_pfx));
                }
            }

            // ── URL input ─────────────────────────────────────────────────────
            ImGui::Spacing();
            ImGui::SetNextItemWidth(form_w);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, FRAME_ROUNDING_SM);
            ImGui::InputText("##video_url", s_url_buf, sizeof(s_url_buf));
            ImGui::PopStyleVar();

            // ── Error message ─────────────────────────────────────────────────
            if (is_err) {
                ImGui::Spacing();
                ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + form_w);
                ImGui::TextColored(col_error_alt(),
                                   "ERROR: %s", s_vp.error_msg);
                ImGui::PopTextWrapPos();
            }

            // ── Connect button ────────────────────────────────────────────────
            ImGui::Spacing();
            if (ui_grid_button("CONNECT##video", { form_w, 26.0f })) {
                std::strncpy(s_vp.url, s_url_buf, sizeof(s_vp.url) - 1);
                video_player_start(s_vp);
                gcs_log("video: connecting to %s", s_vp.url);
            }

            ImGui::EndGroup();
        }

        // ── Feed banner ───────────────────────────────────────────────────────
        // Semi-transparent strip over the top of the frame: source on the left,
        // pipeline state on the right, with a live REC pip while streaming.
        {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            const char* state_s = (state == VideoState::Playing)    ? "LIVE"
                                : (state == VideoState::Connecting) ? "ACQUIRING"
                                : (state == VideoState::Error)      ? "FAULT"
                                                                    : "STANDBY";
            char meta[128];
            snprintf(meta, sizeof(meta), "%s \xc2\xb7 %s",
                     s_vp.url[0] ? s_vp.url : "NO SOURCE", state_s);
            ui_panel_header_at(dl, vwp, l.center_w, "EO / IR FEED \xe2\x80\x94 FWD",
                               meta, 0.82f);

            // Blinking REC pip while the pipeline is running.
            if (state == VideoState::Playing && fmodf((float)ImGui::GetTime(), 1.4f) < 0.9f) {
                ImFont*     fm  = g_font_micro ? g_font_micro : ImGui::GetFont();
                const float mw  = ui_tracked_width(fm, UI_SZ_MICRO, meta);
                const ImVec2 c  = { vwp.x + l.center_w - mw - 20.0f,
                                    vwp.y + UI_HEADER_H * 0.5f };
                dl->AddCircleFilled(c, 3.0f, ui_col(g_theme.col_error));
            }
        }
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
    } // end if (s_mode != MapOnly)

    // ── Plugin rail ───────────────────────────────────────────────────────────
    // Full height of the centre band, outside the mode switch: the buttons are
    // vehicle and payload controls, and stay reachable in every centre mode.
    draw_plugin_rail(vs, sender, l.plugin_x, content_top, l.plugin_w, content_h);

    // ── Map ───────────────────────────────────────────────────────────────────
    if (s_mode != CenterMode::VideoOnly) {
        const std::vector<MissionItem>* map_mission =
            (pick && pick->edit_mission) ? pick->edit_mission
            : (vs.has_mission           ? &vs.mission : nullptr);

        draw_map_view(vs.lat, vs.lon, vs.has_global_pos,
                      (float)vs.heading, vs.has_vfr,
                      l.center_x, content_top + vid_h,
                      l.center_w, map_h,
                      map_mission, pick,
                      vs.alt_rel, vs.groundspeed);
    }
}

// ── center_view_shutdown ──────────────────────────────────────────────────────
// Called from main() before the OpenGL context is destroyed.

void center_view_shutdown()
{
    video_player_stop(s_vp);
    delete_texture();
}

// ── Plugin context ────────────────────────────────────────────────────────────

PluginContext center_view_plugin_context(const VehicleState& vs,
                                         MavlinkSender* sender)
{
    const bool playing =
        s_vp.state.load(std::memory_order_acquire) == VideoState::Playing;

    PluginContext ctx{};
    ctx.vehicle   = &vs;
    ctx.sender    = sender;
    ctx.connected = vs.has_heartbeat;
    ctx.tsys      = ctx.connected ? vs.sysid  : 1;
    ctx.tcomp     = ctx.connected ? vs.compid : 1;
    // Frame size comes from the uploaded texture rather than s_vp, which would
    // need the frame lock for a number the caller only reads.
    ctx.camera    = PluginCamera{ playing, s_vp.url,
                                  playing ? s_tex_w : 0,
                                  playing ? s_tex_h : 0 };
    ctx.slot      = -1;
    return ctx;
}

// ── Camera API exposed to plugins (plugins/plugin_api.hpp) ────────────────────
//
// UI-thread only, which is where plugins run: they touch the GStreamer pipeline
// and the GL texture, neither of which is safe from another thread.

void gcs_camera_start(const char* url)
{
    if (!url || !*url) {
        gcs_log("camera: empty url");
        return;
    }
    if (s_vp.state.load(std::memory_order_acquire) != VideoState::Idle) {
        video_player_stop(s_vp);
        delete_texture();
    }
    std::strncpy(s_vp.url, url, sizeof(s_vp.url) - 1);
    s_vp.url[sizeof(s_vp.url) - 1] = '\0';
    video_player_start(s_vp);
    gcs_log("camera: connecting to %s", s_vp.url);
}

void gcs_camera_stop()
{
    video_player_stop(s_vp);
    delete_texture();
}

bool gcs_camera_snapshot(const char* path)
{
    if (!path || !*path) {
        gcs_log("snapshot: no path");
        return false;
    }

    // Copy under the lock and encode outside it — PNG compression takes long
    // enough that holding frame_mtx would stall the decode thread.
    std::vector<uint8_t> rgb;
    int w = 0, h = 0;
    {
        std::lock_guard<std::mutex> lk(s_vp.frame_mtx);
        if (s_vp.frame_w <= 0 || s_vp.frame_h <= 0 || s_vp.frame_data.empty()) {
            gcs_log("snapshot: no frame");
            return false;
        }
        w   = s_vp.frame_w;
        h   = s_vp.frame_h;
        rgb = s_vp.frame_data;
    }

    if (!stbi_write_png(path, w, h, 3, rgb.data(), w * 3)) {
        gcs_log("snapshot: write failed (%s)", path);
        return false;
    }
    return true;
}
