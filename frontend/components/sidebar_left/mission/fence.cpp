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

// frontend/components/sidebar_left/mission/fence.cpp
//
// The GEOFENCE view: the uploaded zones as shapes drawn on the map, edited
// beside the vehicle's own fence parameters, and uploaded as one list.

#include "frontend/components/sidebar_left/mission/mission_internal.hpp"
#include "frontend/source/app_log.hpp"
#include <cmath>
#include <cstdio>

namespace mission_tab {

// ── FENCE view ────────────────────────────────────────────────────────────────

// Ground distance in metres, near enough for a fence radius: the two points
// are a click apart on the map.
static float ground_dist_m(const FenceVertex& a, const FenceVertex& b)
{
    const double k  = 111320.0;
    const double dy = (b.lat - a.lat) * k;
    const double dx = (b.lon - a.lon) * k * std::cos(a.lat * 3.14159265358979 / 180.0);
    return (float)std::sqrt(dx * dx + dy * dy);
}

// A finished shape joins the plan, opened in the list; the drawing ends.
static void add_fence_shape(MissionPickState* pick, FenceShape shape)
{
    st().fence.shapes.push_back(std::move(shape));
    st().fence_open     = (int)st().fence.shapes.size() - 1;
    st().fence_has_edit = true;
    st().fence_cleared  = false;
    pick->cancel_fence_draw();
}

void finish_fence_polygon(MissionPickState* pick)
{
    if (pick->fence_pts.size() < 3) return;
    FenceShape s;
    s.kind      = FenceShape::Kind::Polygon;
    s.inclusion = pick->fence_inclusion;
    s.pts       = pick->fence_pts;
    gcs_log("fence: added %s polygon, %d vertices",
            s.inclusion ? "inclusion" : "exclusion", (int)s.pts.size());
    add_fence_shape(pick, std::move(s));
}

// A map click while a fence shape is being drawn.
void consume_fence_pick(MissionPickState* pick)
{
    const FenceVertex at = { pick->picked_lat, pick->picked_lon };
    if (pick->fence_draw == MissionPickState::FenceDraw::Polygon) {
        if (pick->fence_close) finish_fence_polygon(pick);
        else                   pick->fence_pts.push_back(at);
    } else if (pick->fence_pts.empty()) {
        pick->fence_pts.push_back(at);                    // the centre
    } else {
        FenceShape s;
        s.kind      = FenceShape::Kind::Circle;
        s.inclusion = pick->fence_inclusion;
        s.center    = pick->fence_pts[0];
        s.radius_m  = std::max(1.0f, std::round(ground_dist_m(s.center, at)));
        gcs_log("fence: added %s circle, radius %.0f m",
                s.inclusion ? "inclusion" : "exclusion", (double)s.radius_m);
        add_fence_shape(pick, std::move(s));
    }
    pick->fence_close = false;
}

// What the vehicle's parameter-set fence is: the cylinder the map rings home
// with. Read-only here; the PARAMS tab edits it.
static void draw_param_fence_summary(const VehicleState* vs)
{
    ImGui::TextDisabled("PARAMETER FENCE");
    const FenceParams* f = vs ? &vs->fence_params : nullptr;
    if (!f || (!f->has_radius && !f->has_enable)) {
        ImGui::TextDisabled("  not reported yet");
        return;
    }
    const bool on = !f->has_enable || f->enabled;
    if (f->circle_configured()) {
        if (f->ceiling_configured())
            ImGui::Text("  Cylinder  R %.0f m, ceiling %.0f m", (double)f->radius_m,
                        (double)f->alt_max_m);
        else
            ImGui::Text("  Circle  R %.0f m", (double)f->radius_m);
    } else if (f->ceiling_configured()) {
        ImGui::Text("  Ceiling %.0f m only", (double)f->alt_max_m);
    } else {
        ImGui::TextDisabled("  No radius or ceiling set");
    }
    ImGui::SameLine(0, 8);
    if (on) ImGui::TextColored(col_ok(), "ON");
    else    ImGui::TextColored(col_warning(), "OFF");
    if (f->has_action)
        ImGui::TextDisabled("  %s %d  \xe2\x80\x94  edit in PARAMS",
                            f->px4 ? "GF_ACTION" : "FENCE_ACTION", f->action);
}

void draw_fence_view(MavlinkSender* sender, const VehicleState* vs,
                            MissionPickState* pick, bool connected,
                            uint8_t tsys, uint8_t tcomp)
{
    using FD = MissionPickState::FenceDraw;
    FencePlan& plan = st().fence;

    // ── Fetch / Clear ─────────────────────────────────────────────────────────
    const float half = (ImGui::GetContentRegionAvail().x - 4.0f) * 0.5f;
    ImGui::BeginDisabled(!connected);
    if (ui_grid_button("FETCH###fence_fetch", { half, 24.0f }) && connected) {
        sender->request_mission_list(tsys, tcomp, MAV_MISSION_TYPE_FENCE);
        st().fence_fetching = true;
        st().fence_gen      = vs->fence_generation;
        if (pick) pick->cancel_fence_draw();
        gcs_log("requesting fence list");
    }
    ImGui::EndDisabled();
    ImGui::SameLine(0, 4);
    ImGui::BeginDisabled(plan.shapes.empty() && !plan.has_return);
    if (ui_solid_button("CLEAR###fence_clear", { half, 24.0f },
                        btn_disconnect_base(), btn_disconnect_hov())) {
        // Kept as an edit, so UPLOAD can send the empty fence and clear the
        // vehicle's too; and held off from re-syncing until the next FETCH.
        plan                = FencePlan{};
        st().fence_has_edit = true;
        st().fence_cleared  = true;
        st().fence_open     = -1;
        if (pick) { pick->cancel_fence_draw(); pick->fence_edit = false; }
    }
    ImGui::EndDisabled();

    if (vs && vs->fence_count > 0 && vs->fence_received < vs->fence_count) {
        ImGui::Spacing();
        char prog[32];
        snprintf(prog, sizeof(prog), "%u / %u", vs->fence_received, vs->fence_count);
        const float frac = (float)vs->fence_received / vs->fence_count;
        ui_progress_bar(prog, frac, 10.0f);
        gcs_progress("fence", frac);
    }

    ImGui::Spacing();
    draw_param_fence_summary(vs);

    // Uploaded zones are only enforced on ArduPilot with FENCE_TYPE's polygon
    // bit, and nothing is with the fence disabled — said here, where they are
    // drawn, rather than left to be found out in the air.
    if (vs) {
        const FenceParams& f = vs->fence_params;
        if (!f.px4 && f.has_type && !(f.type & FenceParams::TYPE_POLYGON))
            ImGui::TextColored(col_warning(),
                               "FENCE_TYPE has no Polygon bit: uploaded zones are not enforced");
        if (f.has_enable && !f.enabled && !f.px4)
            ImGui::TextColored(col_warning(), "FENCE_ENABLE is 0: no fence is enforced");
        if (!vs->has_fence && connected)
            ImGui::TextDisabled("FETCH first to keep the vehicle's existing zones");
    }

    ImGui::Spacing();
    themed_sep();
    ImGui::Spacing();

    if (pick) {
        // ── EDIT ──────────────────────────────────────────────────────────────
        const bool editing = pick->fence_edit;
        ImGui::BeginDisabled(plan.shapes.empty() && !editing);
        if (ui_grid_button(editing ? "EDITING \xe2\x80\x94 DRAG FENCE POINTS ON THE MAP###fedit"
                                   : "EDIT###fedit",
                           { -1.0f, 32.0f }, editing)) {
            pick->fence_edit = !editing;
            if (pick->fence_edit) pick->cancel_fence_draw();
        }
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_AllowWhenDisabled))
            ImGui::SetTooltip("Drag a polygon vertex or a circle's centre on the map (Esc to stop)");
        ImGui::Spacing();

        // ── Shape ─────────────────────────────────────────────────────────────
        // Only the shape is chosen here. A new zone is an inclusion zone; the
        // list switches any zone to exclusion once it exists.
        pick->fence_inclusion = true;
        const float  gap = 4.0f;
        const float  w2  = (ImGui::GetContentRegionAvail().x - gap) * 0.5f;
        const ImVec2 sz  = { w2, 26.0f };

        const bool poly_armed = pick->fence_draw == FD::Polygon;
        const bool circ_armed = pick->fence_draw == FD::Circle;
        char poly_lbl[48];
        if (!poly_armed)                       snprintf(poly_lbl, sizeof(poly_lbl), "POLYGON###fpoly");
        else if (pick->fence_pts.size() >= 3)  snprintf(poly_lbl, sizeof(poly_lbl), "CLOSE (%d PTS)###fpoly",
                                                        (int)pick->fence_pts.size());
        else                                   snprintf(poly_lbl, sizeof(poly_lbl), "LOC? (%d PTS)###fpoly",
                                                        (int)pick->fence_pts.size());
        if (ui_grid_button(poly_lbl, sz, poly_armed)) {
            if (!poly_armed) {
                pick->cancel_fence_draw();
                pick->fence_draw  = FD::Polygon;
                pick->add_command = 0;
                pick->drag_edit   = false;
                pick->fence_edit  = false;
            } else if (pick->fence_pts.size() >= 3) {
                finish_fence_polygon(pick);
            } else {
                pick->cancel_fence_draw();
            }
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
            ImGui::SetTooltip(poly_armed
                ? "Click the map for each vertex; click the first vertex, press Enter or this button to close"
                : "Draw a polygon zone on the map");
        ImGui::SameLine(0, gap);
        const char* circ_lbl = !circ_armed ? "CIRCLE###fcirc"
                             : pick->fence_pts.empty() ? "CENTRE?###fcirc" : "RADIUS?###fcirc";
        if (ui_grid_button(circ_lbl, sz, circ_armed)) {
            if (circ_armed) {
                pick->cancel_fence_draw();
            } else {
                pick->cancel_fence_draw();
                pick->fence_draw  = FD::Circle;
                pick->add_command = 0;
                pick->drag_edit   = false;
                pick->fence_edit  = false;
            }
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
            ImGui::SetTooltip("Click the map for the centre, then again for the radius");
    }

    ImGui::Spacing();
    ImGui::TextDisabled("%d zones", (int)plan.shapes.size());
    ImGui::Spacing();

    // ── Zone list ─────────────────────────────────────────────────────────────
    const float list_h = std::max(80.0f, ImGui::GetContentRegionAvail().y - 64.0f);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, bg_child_dark());
    if (ImGui::BeginChild("##fence_list", { -1.0f, list_h }, false)) {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 4.0f, 2.0f });
        if (plan.shapes.empty()) {
            const char*  msg = "NO FENCE ZONES";
            const ImVec2 ts  = ImGui::CalcTextSize(msg);
            const ImVec2 av  = ImGui::GetContentRegionAvail();
            ImGui::SetCursorPos({ (av.x - ts.x) * 0.5f, (av.y - ts.y) * 0.5f });
            ImGui::TextDisabled("%s", msg);
        }

        int to_delete = -1;
        for (int i = 0; i < (int)plan.shapes.size(); ++i) {
            FenceShape& z = plan.shapes[i];
            ImGui::PushID(i);

            const float  del_w  = 18.0f;
            const ImVec2 del_sz = { del_w, ImGui::GetFontSize() + 2.0f };
            const float  row_w  = ImGui::GetContentRegionAvail().x - del_w - 4.0f;

            char hdr[64];
            if (z.kind == FenceShape::Kind::Polygon)
                snprintf(hdr, sizeof(hdr), "F%d  POLYGON  %s  %d PTS", i + 1,
                         z.inclusion ? "IN" : "EX", (int)z.pts.size());
            else
                snprintf(hdr, sizeof(hdr), "F%d  CIRCLE  %s  %.0f M", i + 1,
                         z.inclusion ? "IN" : "EX", (double)z.radius_m);

            const bool open = st().fence_open == i;
            ImGui::PushStyleColor(ImGuiCol_Text, z.inclusion ? col_ok() : col_error());
            if (ImGui::Selectable(hdr, open, ImGuiSelectableFlags_AllowOverlap,
                                  { row_w, del_sz.y }))
                st().fence_open = open ? -1 : i;
            ImGui::PopStyleColor();
            ImGui::SameLine(0, 4);
            if (ui_solid_button("x", del_sz, btn_disconnect_base(), btn_disconnect_hov()))
                to_delete = i;

            if (open) {
                ImGui::Indent(12.0f);
                const float w2 = (ImGui::GetContentRegionAvail().x - 4.0f) * 0.5f;
                if (ui_tab_button("INCLUSION###zin", { w2, 22.0f }, z.inclusion))
                    z.inclusion = true;
                ImGui::SameLine(0, 4);
                if (ui_tab_button("EXCLUSION###zex", { w2, 22.0f }, !z.inclusion))
                    z.inclusion = false;

                if (z.kind == FenceShape::Kind::Circle) {
                    labeled_float("Radius", &z.radius_m, "m");
                    z.radius_m = std::max(1.0f, z.radius_m);
                    ImGui::TextDisabled("Lat"); ImGui::SameLine(60.0f);
                    ImGui::SetNextItemWidth(-1.0f);
                    ImGui::InputDouble("##clat", &z.center.lat, 0.0, 0.0, "%.7f");
                    ImGui::TextDisabled("Lon"); ImGui::SameLine(60.0f);
                    ImGui::SetNextItemWidth(-1.0f);
                    ImGui::InputDouble("##clon", &z.center.lon, 0.0, 0.0, "%.7f");
                } else {
                    // Each vertex as numbers, for a fence copied off a plan;
                    // EDIT drags them on the map. A polygon keeps three.
                    int drop = -1;
                    for (int v = 0; v < (int)z.pts.size(); ++v) {
                        ImGui::PushID(v);
                        ImGui::TextDisabled("%d", v + 1);
                        ImGui::SameLine(24.0f);
                        const float fw = (ImGui::GetContentRegionAvail().x - del_w - 8.0f) * 0.5f;
                        ImGui::SetNextItemWidth(fw);
                        ImGui::InputDouble("##vlat", &z.pts[v].lat, 0.0, 0.0, "%.6f");
                        ImGui::SameLine(0, 4);
                        ImGui::SetNextItemWidth(fw);
                        ImGui::InputDouble("##vlon", &z.pts[v].lon, 0.0, 0.0, "%.6f");
                        ImGui::SameLine(0, 4);
                        ImGui::BeginDisabled(z.pts.size() <= 3);
                        if (ui_solid_button("x", del_sz, btn_disconnect_base(), btn_disconnect_hov()))
                            drop = v;
                        ImGui::EndDisabled();
                        ImGui::PopID();
                    }
                    if (drop >= 0) z.pts.erase(z.pts.begin() + drop);
                }
                ImGui::Unindent(12.0f);
            }
            ImGui::Separator();
            ImGui::PopID();
        }
        if (to_delete >= 0) {
            plan.shapes.erase(plan.shapes.begin() + to_delete);
            st().fence_open     = -1;
            st().fence_has_edit = true;
        }
        ImGui::PopStyleVar();
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();

    // ── Upload ────────────────────────────────────────────────────────────────
    // An empty plan uploads too, on purpose: it is how the vehicle's fence is
    // cleared, and the button says so.
    ImGui::Spacing();
    const bool empty = plan.shapes.empty();
    ImGui::BeginDisabled(!connected || !st().fence_has_edit);
    if (ui_grid_button(empty ? "UPLOAD EMPTY FENCE (CLEARS VEHICLE'S)###fup" : "UPLOAD FENCE###fup",
                       { -1.0f, 26.0f }) && connected) {
        const std::vector<MissionItem> items = fence_to_items(plan);
        sender->start_upload(tsys, tcomp, items, MAV_MISSION_TYPE_FENCE);
        gcs_log("uploading fence: %d zones, %d items", (int)plan.shapes.size(), (int)items.size());
    }
    ImGui::EndDisabled();

    if (vs) {
        using US = VehicleState::UploadStatus;
        if (vs->fence_upload_status == US::InProgress) {
            ImGui::TextColored(col_warning(), "uploading...");
        } else if (vs->fence_upload_status == US::Accepted) {
            ImGui::TextColored(col_ok(), "Fence upload accepted");
        } else if (vs->fence_upload_status == US::Failed) {
            ImGui::TextColored(col_error(), "Fence upload FAILED (err %u)",
                               (unsigned)vs->fence_upload_ack_result);
        }
    }
}

} // namespace mission_tab
