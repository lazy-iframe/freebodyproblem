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

// frontend/components/map/map2d/menus.cpp
//
// The GO HERE and CLEAR menus the map opens at the pointer, from either view.

#include "frontend/components/map/map2d/map2d_internal.hpp"
#include "frontend/source/coord_format.hpp"
#include "frontend/source/terrain_clearance.hpp"
#include <algorithm>
#include <cstdio>
#include <optional>
#include <string>

using namespace map_detail;

namespace map2d_detail {

// ── GO HERE and CLEAR menus ───────────────────────────────────────────────────
//
// A click on the map names a point — in 3D, picked in place and height — and
// the GO HERE menu is what the operator does with it. A right-click opens
// CLEAR, for what the map is drawing that the operator has finished with.
// Both are small popups at the pointer, as a context menu is. They only read
// and write GotoTargetState — the send itself belongs to the caller, which is
// the one holding a sender.

double s_ctx_lat = 0.0;      // the point the GO HERE menu is for
double s_ctx_lon = 0.0;
static double s_ctx_copied_at = -10.0;   // ImGui time the coordinates were last copied

// Target altitude, metres above home. Seeded from the vehicle's own altitude
// the first time the menu opens — "go there, stay at this height" is the
// common ask — and remembered after that, since an operator working a series
// of targets means the same height for all of them. Negative is below home, as
// in a valley under a mountain takeoff; the terrain check in the menu is what
// speaks up if that is into the ground. The limits only catch a slip of the keyboard:
// they are past any relief on earth, so no height the 3D map can pick is cut short.
float s_goto_alt_m     = 10.0f;
bool  s_goto_alt_seeded = false;

// Tight, as a context menu: little padding, the theme's panel behind it and
// a hairline of the accent round it.
static void push_menu_style()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,    { 8.0f, 6.0f });
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,      { 6.0f, 4.0f });
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,     { 5.0f, 2.0f });
    ui_push_popup_frame();
}
static void pop_menu_style()
{
    ui_pop_popup_frame();
    ImGui::PopStyleVar(3);
}

// `active` and `mission` put the target in height for the terrain check: the
// target's altitude is above home, and home's AMSL comes from them by the
// planning rule of plan_home_amsl().
void draw_goto_menu(GotoTargetState& go, const MapVehicle* active,
                    const std::vector<MissionItem>* mission)
{
    constexpr float ROW_H = 22.0f;

    push_menu_style();
    if (ImGui::BeginPopup("##map_go", ImGuiWindowFlags_AlwaysAutoResize |
                                      ImGuiWindowFlags_NoTitleBar)) {
        // The point, as a heading. Click to copy, as signed decimal degrees —
        // what other tools paste. It says COPIED for a moment so the click is
        // seen to land.
        {
            const std::string coords = format_lat_lon(s_ctx_lat, s_ctx_lon, 6, "  ");
            const bool  copied = ImGui::GetTime() - s_ctx_copied_at < 1.2;
            const char* shown  = copied ? "COPIED" : coords.c_str();
            const ImVec2 sz = ImGui::CalcTextSize(coords.c_str());
            const ImVec2 p  = ImGui::GetCursorScreenPos();
            if (ImGui::InvisibleButton("##ctx_coords", sz)) {
                char clip[64];
                snprintf(clip, sizeof(clip), "%.7f, %.7f", s_ctx_lat, s_ctx_lon);
                ImGui::SetClipboardText(clip);
                s_ctx_copied_at = ImGui::GetTime();
            }
            const bool hov = ImGui::IsItemHovered();
            if (hov) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                ImGui::SetTooltip("Click to copy");
            }
            ImGui::GetWindowDrawList()->AddText(p, (hov || copied) ? ui_col_accent() : ui_col_value(), shown);
        }

        // The height, the only argument the target carries beyond the point.
        {
            ImGui::AlignTextToFramePadding();
            ImGui::PushStyleColor(ImGuiCol_Text, ui_col_label());
            ImGui::TextUnformatted("ALT");
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::SetNextItemWidth(64.0f);
            ImGui::InputFloat("##goto_alt", &s_goto_alt_m, 0.f, 0.f, "%.1f");
            s_goto_alt_m = std::max(GOTO_ALT_MIN_M, std::min(GOTO_ALT_MAX_M, s_goto_alt_m));
            ImGui::SameLine();
            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("m above home");

            const std::string home_dist = map_home_distance(active, s_ctx_lat, s_ctx_lon);
            if (!home_dist.empty()) {
                ImGui::PushStyleColor(ImGuiCol_Text, ui_col_label());
                ImGui::TextUnformatted(home_dist.c_str());
                ImGui::PopStyleColor();
            }
        }

        // The straight line from the vehicle to the target, against the
        // terrain and the clearance set in settings. Advice, not a lock: GO
        // stays live — the terrain data is ~30 m SRTM, and the operator may
        // know the ground better than it does.
        if (active && active->has_pos && active->has_alt) {
            const std::optional<float> home = plan_home_amsl(active, mission);
            if (home) {
                const Clearance c = path_clearance(active->lat, active->lon, active->alt_amsl,
                                                   s_ctx_lat, s_ctx_lon, *home + s_goto_alt_m);
                char t[64];
                ImU32 col = ui_col_label();
                if (!c.has_min)          snprintf(t, sizeof(t), "Checking terrain...");
                else if (c.collides())   { snprintf(t, sizeof(t), "Path hits terrain (~%.0f m into it)", (double)-c.min_m);
                                           col = ui_col(g_theme.col_error); }
                else if (c.too_close())  { snprintf(t, sizeof(t), "Path within ~%.0f m of terrain", (double)c.min_m);
                                           col = ui_col(g_theme.col_warning); }
                else                     snprintf(t, sizeof(t), "Clears terrain by ~%.0f m", (double)c.min_m);
                ImGui::PushStyleColor(ImGuiCol_Text, col);
                ImGui::TextUnformatted(t);
                ImGui::PopStyleColor();
            }
        }

        // The vehicle discards a position target it is not in GUIDED for, and
        // says nothing about having done so. Better to refuse here and name the
        // reason than to send one into silence.
        const float w = ImGui::GetContentRegionAvail().x;
        ImGui::BeginDisabled(!go.sendable);
        if (ui_grid_button("GO", { w, ROW_H })) {
            go.req_lat   = s_ctx_lat;
            go.req_lon   = s_ctx_lon;
            go.req_alt_m = s_goto_alt_m;
            go.requested = true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndDisabled();
        if (!go.sendable) {
            ImGui::PushStyleColor(ImGuiCol_Text, ui_col(g_theme.col_error));
            ImGui::TextUnformatted(go.reason ? go.reason : "Unavailable");
            ImGui::PopStyleColor();
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Escape, false))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    pop_menu_style();
}

// Neither is a command — they only stop the map drawing something the
// operator has finished with. Each is greyed out when there is nothing to
// clear.
void draw_clear_menu(GotoTargetState& go)
{
    push_menu_style();
    if (ImGui::BeginPopup("##map_clear", ImGuiWindowFlags_NoTitleBar)) {
        if (ImGui::MenuItem("Clear target", nullptr, false, go.has_target))
            go.has_target = false;
        if (ImGui::MenuItem("Clear track", nullptr, false,
                            !g_tracks.by_vehicle[ui_bound_vehicle()].pts.empty()))
            map_track_clear();
        ImGui::EndPopup();
    }
    pop_menu_style();
}

} // namespace map2d_detail
