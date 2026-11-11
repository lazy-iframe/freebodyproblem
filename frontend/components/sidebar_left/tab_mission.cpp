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

// frontend/components/sidebar_left/tab_mission.cpp
//
// The MISSION tab: the editor's state and its sync with the vehicle, and the
// tab itself — map picks, PLAN / GEOFENCE, fetch and clear, EDIT, the terrain
// summary and upload. The list, item fields, add grid and fence view are in
// mission/.

#include "frontend/components/sidebar_left/mission/mission_internal.hpp"
#include "frontend/source/app_log.hpp"
#include "frontend/source/audio.hpp"
#include <cstdio>
#include <optional>
#include <vector>

namespace mission_tab {

static VehicleUiState<MissionTabState> s_state;
MissionTabState& st() { return *s_state; }

// Sync the editor from vehicle state when a fresh download completes.
// Will NOT fire after the user has pressed CLEAR, or while there are pending
// edits, to avoid clobbering unsaved work — except right after FETCH, which
// asked for exactly that.
//
// "Fresh" is a change of mission_generation, which ticks once per completed
// download. Watching has_mission fall and rise again instead missed any
// download quicker than the snapshot cadence — a short mission over SITL —
// and left the editor showing the old list.
static void maybe_sync(const VehicleState* vs)
{
    if (!vs || !vs->has_mission) return;
    if (vs->mission_generation == st().fetch_gen) return;   // already up to date
    if (!st().fetching && (st().cleared || st().has_edit)) return;

    st().edit      = vs->mission;
    st().fetch_gen = vs->mission_generation;
    st().has_edit  = true;
    st().cleared   = false;
    st().fetching  = false;
    st().open.assign(st().edit.size(), false);
    st().upload_done = false;
}

// The fence editor's counterpart: a fresh fence download replaces the plan
// unless the operator has edits or cleared it, or asked with FETCH.
static void maybe_sync_fence(const VehicleState* vs)
{
    if (!vs || !vs->has_fence) return;
    if (vs->fence_generation == st().fence_gen) return;
    if (!st().fence_fetching && (st().fence_cleared || st().fence_has_edit)) return;

    st().fence          = fence_from_items(vs->fence);
    st().fence_gen      = vs->fence_generation;
    st().fence_has_edit = true;
    st().fence_cleared  = false;
    st().fence_fetching = false;
    st().fence_open     = -1;
}

} // namespace mission_tab

using namespace mission_tab;

// ── Main draw function ────────────────────────────────────────────────────────

void draw_tab_mission(MavlinkSender* sender, const VehicleState* vs,
                      MissionPickState* pick)
{
    const bool    connected = (vs && vs->has_heartbeat);
    const uint8_t tsys      = connected ? vs->sysid  : 1;
    const uint8_t tcomp     = connected ? vs->compid : 1;

    maybe_sync(vs);
    maybe_sync_fence(vs);

    // ── Publish live edit pointers so map_view can draw current edits ─────────
    if (pick) {
        pick->edit_mission = st().has_edit ? &st().edit : nullptr;
        pick->edit_fence   = st().fence_has_edit ? &st().fence : nullptr;
    }

    // ── Consume a completed map pick ──────────────────────────────────────────
    if (pick && pick->pick_done && pick->fence_drawing()) {
        consume_fence_pick(pick);
        pick->pick_done = false;
    } else if (pick && pick->pick_done) {
        if (pick->add_command != 0) {
            std::optional<float> alt;
            if (pick->has_picked_alt) alt = pick->picked_alt;
            append_item(pick->add_command, pick->picked_lat, pick->picked_lon, alt);
            if (alt)
                gcs_log("mission: added %s at %.6f, %.6f, %.0f m",
                        mission_cmd_name(pick->add_command), pick->picked_lat, pick->picked_lon, (double)*alt);
            else
                gcs_log("mission: added %s at %.6f, %.6f",
                        mission_cmd_name(pick->add_command), pick->picked_lat, pick->picked_lon);
        }
        pick->pick_done      = false;
        pick->has_picked_alt = false;
        pick->add_command    = 0;
    }

    // ── Escape: cancel a pending pick, else leave EDIT ────────────────────────
    if (pick && ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        if (pick->picking()) {
            pick->add_command = 0;
            pick->cancel_fence_draw();
        } else {
            pick->drag_edit  = false;
            pick->fence_edit = false;
        }
    }
    // Enter closes a polygon being drawn, once it has three vertices.
    if (pick && pick->fence_draw == MissionPickState::FenceDraw::Polygon &&
        (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)))
        finish_fence_polygon(pick);

    ImGui::Spacing();
    ImGui::TextColored(accent_col(), "MISSION");
    if (!connected) {
        ImGui::SameLine(0, 6);
        ImGui::TextColored(col_no_link_muted(), "(no link)");
    }
    themed_sep();
    ImGui::Spacing();

    // ── MISSION / FENCE ───────────────────────────────────────────────────────
    // Two lists the vehicle keeps apart, edited apart. Switching drops whatever
    // the map was armed for in the other.
    {
        const float w2 = (ImGui::GetContentRegionAvail().x - 4.0f) * 0.5f;
        bool to_fence = st().fence_view;
        if (ui_tab_button("PLAN###v_plan", { w2, 26.0f }, !st().fence_view)) to_fence = false;
        ImGui::SameLine(0, 4);
        if (ui_tab_button("GEOFENCE###v_fence", { w2, 26.0f }, st().fence_view)) to_fence = true;
        if (to_fence != st().fence_view) {
            st().fence_view = to_fence;
            if (pick) {
                pick->add_command = 0;  pick->pick_done  = false;
                pick->drag_edit   = false; pick->fence_edit = false;
                pick->cancel_fence_draw();
            }
        }
        ImGui::Spacing();
    }
    if (st().fence_view) {
        draw_fence_view(sender, vs, pick, connected, tsys, tcomp);
        return;
    }

    // ── Fetch / Clear row ─────────────────────────────────────────────────────
    const float btn_w = (ImGui::GetContentRegionAvail().x - 4.0f) * 0.5f;

    ImGui::BeginDisabled(!connected);
    if (ui_grid_button("FETCH", { btn_w, 24.0f }) && connected) {
        sender->request_mission_list(tsys, tcomp);
        // Only a download newer than this press may replace the editor — not
        // one that finished earlier and was held off by unsaved edits.
        st().fetching          = true;
        st().fetch_gen         = vs->mission_generation;
        if (pick) { pick->add_command = 0; pick->pick_done = false; }
        gcs_log("requesting mission list");
    }
    ImGui::EndDisabled();

    ImGui::SameLine(0, 4);

    ImGui::BeginDisabled(!st().has_edit);
    if (ui_solid_button("CLEAR", { btn_w, 24.0f },
                        btn_disconnect_base(), btn_disconnect_hov()) && st().has_edit) {
        st().edit.clear();
        st().open.clear();
        st().has_edit    = false;
        st().cleared     = true;   // block auto-re-sync until user presses FETCH
        st().upload_done = false;
        if (pick) {
            pick->add_command = 0; pick->pick_done = false;
            pick->drag_edit = false; pick->edit_mission = nullptr;
        }
    }
    ImGui::EndDisabled();

    // ── Download progress ─────────────────────────────────────────────────────
    if (vs && vs->mission_count > 0 && vs->mission_received < vs->mission_count) {
        ImGui::Spacing();
        char prog_lbl[32];
        snprintf(prog_lbl, sizeof(prog_lbl), "%u / %u", vs->mission_received, vs->mission_count);
        const float frac = (float)vs->mission_received / vs->mission_count;
        ui_progress_bar(prog_lbl, frac, 10.0f);
        // Its own channel, so a mission download during a parameter fetch keeps
        // its own tempo instead of the two of them beating against each other.
        gcs_progress("mission", frac);
    }

    ImGui::Spacing();
    themed_sep();
    ImGui::Spacing();

    // ── EDIT + add grid ───────────────────────────────────────────────────────
    // EDIT hands the map's left button to the waypoints: press one and drag it.
    // Off, the map pans as always. Lit on the active plate while on.
    if (pick) {
        const bool editing = pick->drag_edit;
        ImGui::BeginDisabled(st().edit.empty() && !editing);
        if (ui_grid_button(editing ? "EDITING \xe2\x80\x94 DRAG WAYPOINTS ON THE MAP###edit"
                                   : "EDIT###edit",
                           { -1.0f, 32.0f }, editing)) {
            pick->drag_edit = !editing;
            if (pick->drag_edit) pick->add_command = 0;
        }
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_AllowWhenDisabled))
            ImGui::SetTooltip("Drag any waypoint on the map to move it.\n"
                              "In 3D, drag its vertical line to set its height.\n"
                              "Esc to stop.");
        ImGui::Spacing();
    }
    draw_add_grid(pick);

    ImGui::Spacing();
    ImGui::TextDisabled("%d items", (int)st().edit.size());

    // ── Terrain clearance ─────────────────────────────────────────────────────
    // Every leg against the ground, the worst named here; the items they
    // arrive at are marked in the list below.
    const std::optional<float> plan_home =
        plan_home_amsl(home_amsl(vs),
                       vs && vs->has_home ? &vs->home_lat : nullptr,
                       vs && vs->has_home ? &vs->home_lon : nullptr, &st().edit);
    const std::vector<MissionLeg> legs = mission_legs(st().edit, plan_home);
    std::vector<const MissionLeg*> leg_to(st().edit.size(), nullptr);
    const MissionLeg* worst = nullptr;
    int n_bad = 0;
    bool checking = false;
    for (const MissionLeg& lg : legs) {
        leg_to[lg.to] = &lg;
        checking = checking || !lg.c.complete;
        if (!lg.c.too_close()) continue;
        ++n_bad;
        if (!worst || lg.c.min_m < worst->c.min_m) worst = &lg;
    }
    if (worst) {
        ImGui::SameLine(0, 10);
        char more[24] = "";
        if (n_bad > 1) snprintf(more, sizeof(more), "  (+%d more)", n_bad - 1);
        if (worst->c.collides())
            ImGui::TextColored(col_error(), "! #%d\xe2\x86\x92#%d HITS TERRAIN%s",
                               worst->from, worst->to, more);
        else
            ImGui::TextColored(col_warning(), "! #%d\xe2\x86\x92#%d within ~%.0f m of terrain%s",
                               worst->from, worst->to, (double)worst->c.min_m, more);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
            ImGui::SetTooltip("%d leg%s closer to the terrain than %.0f m (Settings \xe2\x86\x92 Terrain)",
                              n_bad, n_bad == 1 ? "" : "s", (double)terrain_clearance_m());
    } else if (checking) {
        ImGui::SameLine(0, 10);
        ImGui::TextDisabled("checking terrain...");
    } else if (!legs.empty()) {
        ImGui::SameLine(0, 10);
        ImGui::TextDisabled("clears terrain by %.0f m+", (double)terrain_clearance_m());
    }
    ImGui::Spacing();

    draw_waypoint_list(plan_home, leg_to);

    // ── Upload ────────────────────────────────────────────────────────────────
    ImGui::Spacing();
    ImGui::BeginDisabled(!connected || st().edit.empty());
    if (ui_grid_button("UPLOAD MISSION", { -1.0f, 26.0f }) && connected && !st().edit.empty()) {
        sender->start_upload(tsys, tcomp, mission_for_wire(*vs, st().edit));
        st().upload_done = false;
        gcs_log("uploading %d waypoints", (int)st().edit.size());
    }
    ImGui::EndDisabled();

    // Upload status badge
    if (vs) {
        using US = VehicleState::UploadStatus;
        if (vs->upload_status == US::InProgress) {
            ImGui::SameLine(0, 6);
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "uploading...");
        } else if (vs->upload_status == US::Accepted) {
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "Upload accepted");
        } else if (vs->upload_status == US::Failed) {
            char fbuf[40];
            snprintf(fbuf, sizeof(fbuf), "Upload FAILED (err %u)", (unsigned)vs->upload_ack_result);
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", fbuf);
        }
    }
}
