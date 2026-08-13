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


#include "sidebar_left.hpp"
#include "sidebar_themes.hpp"
#include "layout.hpp"
#include "ui_kit.hpp"
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

    // Tab identity, so the dispatch below never drifts when the grid changes.
    enum class Tab { Connection, Console, Flight, Mavlink, Params, Mission, Settings };
    static Tab selected = Tab::Connection;

    // ── Station header + tab grid (2-column) ──────────────────────────────────
    ui_panel_header("CONTROL STATION");

    {
        struct TabDef { const char* label; Tab id; };
        static const TabDef tabs[] = {
            { "CONNECTION", Tab::Connection }, { "CONSOLE", Tab::Console  },
            { "FLIGHT",     Tab::Flight     }, { "MAVLINK", Tab::Mavlink  },
            { "PARAMS",     Tab::Params     }, { "MISSION", Tab::Mission  },
            { "SETTINGS",   Tab::Settings   },
        };
        constexpr int TAB_COUNT = (int)(sizeof(tabs) / sizeof(tabs[0]));

        const float btn_w = (ImGui::GetContentRegionAvail().x - 2.0f) * 0.5f;

        for (int i = 0; i < TAB_COUNT; ++i) {
            if (i % 2 == 1) ImGui::SameLine(0, 2);
            if (ui_tab_button(tabs[i].label, { btn_w, 28.0f }, selected == tabs[i].id))
                selected = tabs[i].id;
        }
    }

    // ── Dispatch to per-tab panels ────────────────────────────────────────────
    switch (selected) {
    case Tab::Connection: draw_tab_connection(vs, conn_out, link_status, settings); break;
    case Tab::Flight:     draw_tab_flight(sender, vs);                              break;
    case Tab::Mavlink:    draw_tab_mavlink(sender, vs, msg_stats, total_messages,
                                           total_bytes, parse_errors);              break;
    case Tab::Params:     draw_tab_params(sender, vs, params);                      break;
    case Tab::Mission:    draw_tab_mission(sender, vs, pick);                       break;
    case Tab::Settings:   draw_tab_themes(settings);                                break;
    case Tab::Console:    break;   // no panel yet — reserved
    }

    ImGui::End();
    ImGui::PopStyleVar(4);
    ImGui::PopStyleColor();
}
