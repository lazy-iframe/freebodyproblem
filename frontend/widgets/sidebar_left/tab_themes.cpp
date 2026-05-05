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


#include "sidebar_internal.hpp"
#include "../sidebar_themes.hpp"
#include "imgui.h"

void draw_tab_themes(AppSettings* settings)
{
    if (!settings) return;

    ImGui::Spacing();
    ImGui::TextColored(accent_col(), "THEMES");
    themed_sep();
    ImGui::Spacing();

    const float avail_h = ImGui::GetContentRegionAvail().y;
    ImGui::PushStyleColor(ImGuiCol_ChildBg, bg_child_darker());
    if (ImGui::BeginChild("##themes_scroll", { -1.0f, avail_h }, false)) {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 4.0f, 4.0f });
        draw_themes_panel(*settings);
        ImGui::PopStyleVar();
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
}
