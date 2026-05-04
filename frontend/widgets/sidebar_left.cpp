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


#include "sidebar_left.hpp"
#include "sidebar_themes.hpp"
#include "layout.hpp"
#include "imgui.h"
#include "sidebar_left/sidebar_internal.hpp"

void draw_sidebar_left(MavlinkSender* sender, const VehicleState* vs,
                       ConnectionRequest* conn_out, LinkStatus link_status,
                       const std::unordered_map<std::string, ParamEntry>* params,
                       AppSettings* settings,
                       const std::unordered_map<uint32_t, MessageStats>* msg_stats,
                       uint64_t total_messages,
                       uint64_t total_bytes,
                       uint64_t parse_errors,
                       MissionPickState* pick)
{
    const GcsLayout l = GcsLayout::compute();

    ImGui::SetNextWindowPos ({ l.left_x, l.top     }, ImGuiCond_Always);
    ImGui::SetNextWindowSize({ l.left_w, l.total_h }, ImGuiCond_Always);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, bg_sidebar_left());
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,  { 6.0f, 8.0f });
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,    { 4.0f, 4.0f });
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,  FRAME_ROUNDING_MD);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar          |
        ImGuiWindowFlags_NoResize            |
        ImGuiWindowFlags_NoMove              |
        ImGuiWindowFlags_NoScrollbar         |
        ImGuiWindowFlags_NoScrollWithMouse;

    if (!ImGui::Begin("##sidebar_left", nullptr, flags)) {
        ImGui::End();
        ImGui::PopStyleVar(4);
        ImGui::PopStyleColor();
        return;
    }

    // Right-edge border line (seam between sidebar and center panel)
    {
        ImDrawList*  dl = ImGui::GetWindowDrawList();
        const ImVec2 wp = ImGui::GetWindowPos();
        dl->AddLine({ wp.x + l.left_w, wp.y }, { wp.x + l.left_w, wp.y + l.total_h },
                    col32_panel_border(), PANEL_BORDER_THICKNESS);
    }

    static int selected = 0;

    // ── Tab grid (2-column) ───────────────────────────────────────────────────
    {
        static const char* const tabs[] = {
            "CONNECTION", "SENSORS",
            "FLIGHT",     "MAVLINK",
            "PARAMS",     "RADIO",
            "MISSION",    "ESC",
            "CONSOLE",    "PAYLOAD",
            "SETTINGS",   "GIMBAL",
        };
        constexpr int TAB_COUNT = (int)(sizeof(tabs) / sizeof(tabs[0]));

        const float btn_w = (ImGui::GetContentRegionAvail().x - 2.0f) * 0.5f;

        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, FRAME_BORDER_NORMAL);
        ImGui::PushStyleColor(ImGuiCol_Border, col_border_tab());

        for (int i = 0; i < TAB_COUNT; ++i) {
            if (i % 2 == 1) ImGui::SameLine(0, 2);
            const bool active = (selected == i);
            if (active) {
                ImGui::PushStyleColor(ImGuiCol_Button,        btn_tab_active_base());
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, btn_tab_active_hov());
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  btn_tab_active_base());
                ImGui::PushStyleColor(ImGuiCol_Text,          { 1.0f, 1.0f, 1.0f, 1.0f });
            } else {
                push_flash_colors(CmdFlashState::Normal);
            }
            if (ImGui::Button(tabs[i], { btn_w, 26.0f }))
                selected = i;
            ImGui::PopStyleColor(active ? 4 : 3);
        }

        ImGui::PopStyleColor(); // Border
        ImGui::PopStyleVar();   // FrameBorderSize
    }

    // ── Dispatch to per-tab panels ────────────────────────────────────────────
    if (selected == 0)  draw_tab_connection(vs, conn_out, link_status, settings);
    if (selected == 2)  draw_tab_flight(sender, vs);
    if (selected == 3)  draw_tab_mavlink(sender, vs, msg_stats, total_messages, total_bytes, parse_errors);
    if (selected == 4)  draw_tab_params(sender, vs, params);
    if (selected == 6)  draw_tab_mission(sender, vs, pick);
    if (selected == 10) draw_tab_themes(settings);

    ImGui::End();
    ImGui::PopStyleVar(4);
    ImGui::PopStyleColor();
}
