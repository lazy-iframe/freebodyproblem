// Free Body Problem: A modern, simple and minimalist Ground Control Station
// Copyright (C) 2026  Azhar Tanweer
// Contact: azhar.tanweer404@gmail.com

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


#include "center_view.hpp"
#include "layout.hpp"
#include "map_view.hpp"
#include "video_player.hpp"
#include "../app_log.hpp"
#include "imgui.h"

#include <GLFW/glfw3.h>   // brings in OpenGL3 core header
#include <cstring>

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

static constexpr float HEADER_H = 28.0f;

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

// ── draw_center_view ──────────────────────────────────────────────────────────

void draw_center_view(const VehicleState& vs, MissionPickState* pick)
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
    ImGui::SetNextWindowPos ({l.center_x, l.top}, ImGuiCond_Always);
    ImGui::SetNextWindowSize({l.center_w, HEADER_H}, ImGuiCond_Always);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, g_theme.bg_topbar);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {8.f, 0.f});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    if (ImGui::Begin("##cv_header", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse)) {

        // Mode buttons — right-aligned
        const char*  btn_labels[] = { "VIDEO ONLY", "MAP ONLY", "MAP + VIDEO" };
        const CenterMode btn_modes[] = {
            CenterMode::VideoOnly, CenterMode::MapOnly, CenterMode::MapAndVideo };
        constexpr float BTN_W = 82.f, BTN_H = 20.f, BTN_GAP = 4.f;
        constexpr int   N = 3;
        const float     btns_total = BTN_W * N + BTN_GAP * (N - 1);
        const float     btns_x     = l.center_w - btns_total - 8.f;

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, FRAME_ROUNDING_SM);
        for (int i = 0; i < N; ++i) {
            const bool active = (s_mode == btn_modes[i]);
            ImGui::SameLine(btns_x + i * (BTN_W + BTN_GAP));
            ImGui::SetCursorPosY((HEADER_H - BTN_H) * 0.5f);
            if (active) {
                ImGui::PushStyleColor(ImGuiCol_Button,        btn_tab_active_base());
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, btn_tab_active_hov());
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  btn_tab_active_base());
            } else {
                push_flash_colors(CmdFlashState::Normal);
            }
            if (ImGui::Button(btn_labels[i], {BTN_W, BTN_H}))
                s_mode = btn_modes[i];
            ImGui::PopStyleColor(3);
        }
        ImGui::PopStyleVar(); // FrameRounding
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();

    // ── Compute content area (below header) ───────────────────────────────────
    const float content_top = l.top + HEADER_H;
    const float content_h   = l.total_h - HEADER_H;

    // Re-derive video/map split from the reduced content height
    float vid_h = l.center_w * (9.f / 20.f);
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
            }

            // Small red stop button — upper-left corner
            ImGui::SetCursorPos({ 8.0f, 8.0f });
            ImGui::PushStyleColor(ImGuiCol_Button,        btn_stop_base());
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, btn_stop_hov());
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  btn_stop_act());
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, FRAME_ROUNDING_SM);
            if (ImGui::Button("##stop", { 20.0f, 20.0f })) {
                gcs_log("video stream stopped");
                video_player_stop(s_vp);
                delete_texture();
            }
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(3);

        // ── Connecting ────────────────────────────────────────────────────────
        } else if (state == VideoState::Connecting) {
            const ImVec2 wsz = ImGui::GetWindowSize();
            const char*  msg = "CONNECTING...";
            const ImVec2 tsz = ImGui::CalcTextSize(msg);
            ImGui::SetCursorPos({ (wsz.x - tsz.x) * 0.5f,
                                  (wsz.y - tsz.y) * 0.5f });
            ImGui::TextDisabled("%s", msg);

            // Stop button while connecting
            ImGui::SetCursorPos({ 8.0f, 8.0f });
            ImGui::PushStyleColor(ImGuiCol_Button,        btn_stop_base());
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, btn_stop_hov());
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  btn_stop_act());
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, FRAME_ROUNDING_SM);
            if (ImGui::Button("##stop", { 20.0f, 20.0f })) {
                gcs_log("video connection cancelled");
                video_player_stop(s_vp);
            }
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(3);

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
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, FRAME_ROUNDING_SM);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, FRAME_BORDER_NORMAL);
            ImGui::PushStyleColor(ImGuiCol_Border, col_border_form());
            for (int i = 0; i < 2; ++i) {
                if (i > 0) ImGui::SameLine(0, 4);
                const bool active = (s_proto_sel == i);
                if (active) {
                    ImGui::PushStyleColor(ImGuiCol_Button,        btn_tab_active_base());
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, btn_tab_active_hov());
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  btn_tab_active_base());
                    ImGui::PushStyleColor(ImGuiCol_Text,          { 1.0f, 1.0f, 1.0f, 1.0f });
                } else {
                    push_flash_colors(CmdFlashState::Normal);
                }
                if (ImGui::Button(proto_labels[i], { 70.0f, 22.0f })) {
                    s_proto_sel = i;
                    // Replace only if the current url matches the old prefix
                    const char* old_pfx = proto_prefixes[1 - i];
                    if (std::strncmp(s_url_buf, old_pfx, std::strlen(old_pfx)) == 0)
                        std::snprintf(s_url_buf, sizeof(s_url_buf),
                                      "%s%s", proto_prefixes[i],
                                      s_url_buf + std::strlen(old_pfx));
                }
                ImGui::PopStyleColor(active ? 4 : 3);
            }
            ImGui::PopStyleColor(); // Border
            ImGui::PopStyleVar(2);  // FrameRounding, FrameBorderSize

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
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, FRAME_ROUNDING_SM);
            push_flash_colors(CmdFlashState::Normal);
            if (ImGui::Button("CONNECT##video", { form_w, 26.0f })) {
                std::strncpy(s_vp.url, s_url_buf, sizeof(s_vp.url) - 1);
                video_player_start(s_vp);
                gcs_log("video: connecting to %s", s_vp.url);
            }
            ImGui::PopStyleColor(3);
            ImGui::PopStyleVar();

            ImGui::EndGroup();
        }
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
    } // end if (s_mode != MapOnly)

    // ── Map ───────────────────────────────────────────────────────────────────
    if (s_mode != CenterMode::VideoOnly) {
        const std::vector<MissionItem>* map_mission =
            (pick && pick->edit_mission) ? pick->edit_mission
            : (vs.has_mission           ? &vs.mission : nullptr);

        draw_map_view(vs.lat, vs.lon, vs.has_global_pos,
                      (float)vs.heading, vs.has_vfr,
                      l.center_x, content_top + vid_h,
                      l.center_w, map_h,
                      map_mission, pick);
    }
}

// ── center_view_shutdown ──────────────────────────────────────────────────────
// Called from main() before the OpenGL context is destroyed.

void center_view_shutdown()
{
    video_player_stop(s_vp);
    delete_texture();
}
