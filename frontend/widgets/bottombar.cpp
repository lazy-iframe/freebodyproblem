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


#include "bottombar.hpp"
#include "layout.hpp"
#include "imgui.h"

void draw_bottombar(const std::deque<std::string>& log)
{
    const ImGuiIO& io = ImGui::GetIO();

    ImGui::SetNextWindowPos ({ 0.0f, io.DisplaySize.y - BOTTOMBAR_H }, ImGuiCond_Always);
    ImGui::SetNextWindowSize({ io.DisplaySize.x, BOTTOMBAR_H },        ImGuiCond_Always);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, bg_bottombar());
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 8.0f, 4.0f });

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar          |
        ImGuiWindowFlags_NoResize            |
        ImGuiWindowFlags_NoMove              |
        ImGuiWindowFlags_NoScrollbar         |
        ImGuiWindowFlags_NoScrollWithMouse   |
        ImGuiWindowFlags_NoBringToFrontOnFocus;

    if (ImGui::Begin("##bottombar", nullptr, flags)) {
        // Separator line at top edge
        ImDrawList*  dl = ImGui::GetWindowDrawList();
        const ImVec2 wp = ImGui::GetWindowPos();
        dl->AddLine(wp, { wp.x + io.DisplaySize.x, wp.y }, col32_panel_border(), PANEL_BORDER_THICKNESS);

        // Scrollable log
        if (ImGui::BeginChild("##console_scroll", { 0.0f, 0.0f }, false,
                              ImGuiWindowFlags_HorizontalScrollbar))
        {
            for (const auto& entry : log)
                ImGui::TextColored(col_log(), "%s", entry.c_str());

            // Auto-scroll only when the user is already at the bottom
            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                ImGui::SetScrollHereY(1.0f);
        }
        ImGui::EndChild();
    }
    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}
