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
