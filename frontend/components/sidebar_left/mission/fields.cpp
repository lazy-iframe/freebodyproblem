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

// frontend/components/sidebar_left/mission/fields.cpp
//
// An item opened in the list: its parameters, position and height, with the
// terrain under it and the clearance of the leg arriving at it.

#include "frontend/components/sidebar_left/mission/mission_internal.hpp"
#include "frontend/source/mission_alt.hpp"
#include "frontend/source/terrain.hpp"
#include <cstdio>

namespace mission_tab {

// ── Helpers ───────────────────────────────────────────────────────────────────

void labeled_float(const char* label, float* v, const char* unit,
                   float /*speed*/)
{
    char id[64];
    snprintf(id, sizeof(id), "##lf_%p", (void*)v);

    ImGui::AlignTextToFramePadding();
    if (unit) {
        char lbl[64];
        snprintf(lbl, sizeof(lbl), "%s (%s)", label, unit);
        ImGui::TextUnformatted(lbl);
    } else {
        ImGui::TextUnformatted(label);
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputFloat(id, v, 0.f, 0.f, "%.4g");
}

void labeled_double(const char* label, double* v, const char* unit)
{
    char id[64];
    snprintf(id, sizeof(id), "##ld_%p", (void*)v);

    ImGui::AlignTextToFramePadding();
    if (unit) {
        char lbl[64];
        snprintf(lbl, sizeof(lbl), "%s (%s)", label, unit);
        ImGui::TextUnformatted(lbl);
    } else {
        ImGui::TextUnformatted(label);
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputDouble(id, v, 0.0, 0.0, "%.6f");
}

// ── Terrain ───────────────────────────────────────────────────────────────────
//
// The ground under a waypoint and how high above it the waypoint is. The
// height needs the item's altitude in AMSL, which its frame decides: absolute
// is AMSL already, relative is above home — so it needs home's own AMSL, by
// the planning rule of plan_home_amsl() — and terrain frames are height above
// ground by definition.

ItemTerrain item_terrain(const MissionItem& item, std::optional<float> home)
{
    ItemTerrain t;
    if (!mission_frame_is_global(item.frame)) return t;   // not a place on the map
    t.ground = terrain_amsl(item.lat, item.lon);
    if (!t.ground) return t;
    if (auto amsl = mission_item_amsl(item, home, t.ground)) t.agl = *amsl - *t.ground;
    else                                                    t.relative = true;
    return t;
}

// The line under Alt in an open item.
static void draw_terrain_line(const MissionItem& item, std::optional<float> home)
{
    if (!mission_frame_is_global(item.frame)) return;
    const ItemTerrain t = item_terrain(item, home);
    if (!t.ground) {
        ImGui::TextDisabled("Ground  loading...");
        return;
    }
    if (t.agl) {
        ImGui::TextDisabled("Ground  ~%.0f m AMSL  \xc2\xb7", (double)*t.ground);
        ImGui::SameLine(0, 4);
        if (*t.agl < 0.0f)
            ImGui::TextColored(col_error(), "~%.0f m BELOW TERRAIN", (double)-*t.agl);
        else
            ImGui::TextDisabled("ALT AGL TER ~%.0f", (double)*t.agl);
    } else {
        ImGui::TextDisabled("Ground  ~%.0f m AMSL", (double)*t.ground);
        if (t.relative && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
            ImGui::SetTooltip("Height above ground needs home's altitude from the vehicle");
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) && !t.relative)
        ImGui::SetTooltip("%s\nThe bare ground: no trees or buildings.", terrain_attribution().c_str());
}

// The line under that, for an item a leg arrives at: how near the ground the
// leg from the item before comes, against the clearance set in settings.
static void draw_leg_line(const MissionLeg& leg)
{
    const Clearance& c = leg.c;
    if (!c.has_min) {
        ImGui::TextDisabled("Leg from #%d  checking terrain...", leg.from);
    } else if (c.collides()) {
        ImGui::TextColored(col_error(), "Leg from #%d  HITS TERRAIN (~%.0f m into it)",
                           leg.from, (double)-c.min_m);
    } else if (c.too_close()) {
        ImGui::TextColored(col_warning(), "Leg from #%d  within ~%.0f m of terrain",
                           leg.from, (double)c.min_m);
    } else {
        ImGui::TextDisabled("Leg from #%d  clears terrain by ~%.0f m", leg.from, (double)c.min_m);
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
        ImGui::SetTooltip("Closest point at %.6f, %.6f, the path %.0f m AMSL.\n"
                          "Warns inside %.0f m (Settings \xe2\x86\x92 Terrain).",
                          c.lat, c.lon, (double)c.path_amsl, (double)terrain_clearance_m());
}

// Draw expanded parameter fields for one waypoint. `leg` is the leg arriving
// at it, if one does.
void draw_wp_fields(MissionItem& item, std::optional<float> home, const MissionLeg* leg)
{
    ImGui::Indent(12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 4.0f, 3.0f });

    const MissionCmdMeta* m = mission_cmd_meta(item.command);

    if (m) {
        if (m->p1) labeled_float(m->p1, &item.param1);
        if (m->p2) labeled_float(m->p2, &item.param2);
        if (m->p3) labeled_float(m->p3, &item.param3);
        if (m->p4) labeled_float(m->p4, &item.param4);
        if (m->show_latlon) {
            labeled_double("Lat", &item.lat);
            labeled_double("Lon", &item.lon);
        }
        if (m->show_alt || m->show_latlon) {
            labeled_float("Alt (m)", &item.alt);
        }
        if (m->show_latlon)
            draw_terrain_line(item, home);
        if (m->show_latlon && leg)
            draw_leg_line(*leg);
    } else {
        // Unknown command — show all fields generically
        labeled_float("Param1", &item.param1);
        labeled_float("Param2", &item.param2);
        labeled_float("Param3", &item.param3);
        labeled_float("Param4", &item.param4);
        labeled_double("Lat",  &item.lat);
        labeled_double("Lon",  &item.lon);
        labeled_float("Alt (m)", &item.alt);
        draw_terrain_line(item, home);
        if (leg) draw_leg_line(*leg);
    }

    // Frame selector
    const char* frames[] = { "GLOBAL_ABS", "LOCAL", "MISSION", "GLOBAL_REL", "LOCAL_ENU",
                              "GLOBAL_INT", "GLOBAL_REL_INT", "LOCAL_OFFSET",
                              "BODY_NED", "BODY_OFFSET", "GLOBAL_TERRAIN" };
    const int n_frames = (int)(sizeof(frames) / sizeof(frames[0]));
    int fi = std::min((int)item.frame, n_frames - 1);
    ImGui::TextDisabled("Frame");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1.0f);
    char frame_id[32];
    snprintf(frame_id, sizeof(frame_id), "##frame_%u", item.seq);
    if (ImGui::BeginCombo(frame_id, frames[fi])) {
        for (int f = 0; f < n_frames; ++f) {
            bool sel = (f == fi);
            if (ImGui::Selectable(frames[f], sel))
                item.frame = (uint8_t)f;
            if (sel) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::PopStyleVar();
    ImGui::Unindent(12.0f);
}

} // namespace mission_tab
