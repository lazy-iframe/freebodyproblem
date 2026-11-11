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


// frontend/components/map/map_view_3d.cpp
//
// How the 3D map is built, per frame:
//
//   World   An east/north/up frame in metres, curved down with the earth about
//           the centre (Frame::drop), centred on the point the
//           camera orbits. Web Mercator tile coordinates are scaled by the
//           cosine of that point's latitude, which is true to well under a
//           percent across the tens of kilometres in view. Heights are AMSL
//           less the ground under the centre, times the exaggeration, so the
//           numbers stay small and the exaggeration pivots on the ground.
//
//   Tiles   A quadtree over the imagery tile grid: a tile is split while the
//           camera is close to it compared with its size, so tiles shrink
//           towards the camera and the horizon is a few coarse ones. Each
//           leaf is one draw of a shared 33×33 grid, raised in the vertex
//           shader by a 33×33 height texture built from frontend/source/terrain.hpp,
//           and textured with the 2D map's own imagery tile (or the part of
//           a coarser one, while it loads). Skirts hang from every tile edge
//           so the seam between two sizes of tile shows no gap.
//
//   Frame   Drawn into an offscreen colour + depth target the size of the
//           panel, which is then an ImGui image; the overlays are ImGui
//           drawing on top, at the points the camera projects them to.

#include "frontend/components/map/map_view_3d.hpp"
#include "frontend/components/map/map3d/map3d_internal.hpp"
#include "frontend/source/terrain.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>

namespace map3d_detail {

// ── View state ────────────────────────────────────────────────────────────────

Camera                      s_cam;
const float                 EXAGS[] = { 1.0f, 1.5f, 2.0f };
static int                  s_exag  = 1;
bool                        s_textures = true;   // imagery on the terrain; off, the bare mesh
int                         s_drag  = -1;      // the button that started a drag on the view, or -1
static std::optional<float> s_ground;  // last ground height under the centre

// ── Input ─────────────────────────────────────────────────────────────────────

static void handle_input(bool hovered, ImVec2 size, Map3DCenter c)
{
    ImGuiIO& io = ImGui::GetIO();
    if (hovered) {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))  s_drag = ImGuiMouseButton_Left;
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) s_drag = ImGuiMouseButton_Right;
        if (io.MouseWheel != 0.0f)
            s_cam.dist = std::clamp(s_cam.dist * std::pow(0.8, (double)io.MouseWheel), MIN_DIST, MAX_DIST);
    }
    if (s_drag < 0) return;
    if (!ImGui::IsMouseDown(s_drag)) { s_drag = -1; return; }

    const ImVec2 d = io.MouseDelta;
    if (d.x == 0.0f && d.y == 0.0f) return;
    if (s_drag == ImGuiMouseButton_Left) {
        s_cam.yaw_deg   = std::fmod(s_cam.yaw_deg + d.x * 0.25 + 360.0, 360.0);
        s_cam.pitch_deg = std::clamp(s_cam.pitch_deg + d.y * 0.2, 5.0, 89.0);
        return;
    }
    // Pan: the ground under the pointer follows it, near enough — metres per
    // pixel at the centre's distance, stretched along the view by the tilt.
    const double mpp = 2.0 * s_cam.dist * std::tan(FOV_Y * 0.5) / std::max(1.0f, size.y);
    const double yaw = s_cam.yaw_deg * DEG;
    const double fwd = d.y * mpp / std::max(0.2, std::sin(s_cam.pitch_deg * DEG));
    const double de  = -d.x * mpp * std::cos(yaw) + fwd * std::sin(yaw);
    const double dn  =  d.x * mpp * std::sin(yaw) + fwd * std::cos(yaw);

    double u, v;
    merc(c.lat, c.lon, u, v);
    const double m = EARTH_C * std::cos(c.lat * DEG);
    unmerc(u + de / m, v - dn / m, c.lat, c.lon);
    c.follow = false;
}

} // namespace map3d_detail

using namespace map3d_detail;

// ── Public API ────────────────────────────────────────────────────────────────

bool map3d_available()
{
    return init_gl();
}

float map3d_exaggeration()       { return EXAGS[s_exag]; }
void  map3d_cycle_exaggeration() { s_exag = (s_exag + 1) % (int)(sizeof(EXAGS) / sizeof(EXAGS[0])); }
bool  map3d_textures()           { return s_textures; }
void  map3d_toggle_textures()    { s_textures = !s_textures; }
float map3d_yaw_deg()            { return (float)s_cam.yaw_deg; }

bool map3d_take_goto(double& lat, double& lon, float& rel_alt)
{
    if (!s_goto_ready) return false;
    s_goto_ready = false;
    lat = s_goto_lat; lon = s_goto_lon; rel_alt = s_goto_rel;
    return true;
}

bool map3d_picking_goto() { return s_goto.active; }

bool map3d_take_clear()
{
    const bool r = s_clear_ready;
    s_clear_ready = false;
    return r;
}

bool map3d_draw(ImDrawList* dl, ImVec2 p0, ImVec2 size, bool hovered,
                Map3DCenter center, const Map3DScene& scene)
{
    if (!init_gl() || size.x < 2.0f || size.y < 2.0f || !scene.fleet) return false;
    ++s_frame;

    // Heights from a terrain source since switched away from are not this one's.
    static uint64_t s_terrain_gen = terrain_generation();
    if (s_terrain_gen != terrain_generation()) {
        s_terrain_gen = terrain_generation();
        forget_heights();
        s_ground.reset();
    }

    const MapVehicle* active = nullptr;
    for (const MapVehicle& mv : *scene.fleet) if (mv.active) active = &mv;

    if (center.follow && active && active->has_pos) {
        center.lat = active->lat;
        center.lon = active->lon;
    }
    // A waypoint held in EDIT has the pointer: no orbit, and no zoom under it.
    const bool holding_wp = s_wp_drag.index >= 0 && ImGui::IsMouseDown(ImGuiMouseButton_Left);
    handle_input(hovered && !holding_wp, size, center);

    const ImVec2 scale = ImGui::GetIO().DisplayFramebufferScale;
    const int fb_w = std::max(1, (int)(size.x * scale.x));
    const int fb_h = std::max(1, (int)(size.y * scale.y));
    if (!ensure_fbo(fb_w, fb_h)) return false;

    // The point orbited: the active vehicle where it flies while following,
    // else the ground at the centre.
    if (auto gnd = terrain_amsl(center.lat, center.lon)) s_ground = gnd;
    Frame f;
    merc(center.lat, center.lon, f.u0, f.v0);
    f.m    = EARTH_C * std::cos(center.lat * DEG);
    f.exag = map3d_exaggeration();
    f.href = s_ground ? *s_ground : (active && active->has_alt ? active->alt_amsl : 0.0);
    f.p0   = p0;
    f.size = size;
    // Never under the ground, though: a vehicle reporting an altitude inside
    // the terrain (a wrong home, or terrain data that disagrees) would put
    // the point orbited in the rock, and no camera position sees out of that
    // — the line-of-sight lift below would carry the camera kilometres up.
    // Orbiting the surface above it keeps the view, and the vehicle's label
    // says how far down it is.
    double target_amsl = (center.follow && active && active->has_alt) ? active->alt_amsl : f.href;
    if (s_ground) target_amsl = std::max(target_amsl, (double)*s_ground);

    // Eased, not snapped: panning over rough ground, the ground under the
    // centre changes every frame, and an orbit point pinned to it would carry
    // the camera up and down with every bump it passes.
    const double dt   = std::clamp((double)ImGui::GetIO().DeltaTime, 0.0, 0.1);
    const auto   ease = [dt](double cur, double to, double tau) {
        return cur + (to - cur) * (1.0 - std::exp(-dt / tau));
    };
    static std::optional<double> s_orbit_amsl;
    s_orbit_amsl = s_orbit_amsl ? ease(*s_orbit_amsl, target_amsl, 0.25) : target_amsl;
    const V3 target = { 0.0, 0.0, (*s_orbit_amsl - f.href) * f.exag };

    // Keep the line of sight clear of the ground. Orbiting a valley, the
    // camera's place can be inside the mountain beside it, or behind a ridge
    // from what it looks at; either way the view is of rock. So the terrain
    // is read along the line from the centre out to the camera, and where it
    // crosses that line the tilt itself is raised — to the shallowest that
    // clears it, found by halving between the tilt asked for and straight
    // down.
    //
    // The tilt stays raised: the terrain only ever stops a drag towards the
    // horizon, never pulls the view back to one. A drag the other way answers
    // at once, and panning over ridges moves the view only up, never to and
    // fro.
    const double yaw       = s_cam.yaw_deg * DEG;
    const double clearance = std::max(15.0, s_cam.dist * 0.03);
    const auto   eye_at    = [&](double pitch_deg) {
        const double p = pitch_deg * DEG;
        return target + V3{ -std::sin(yaw) * std::cos(p), -std::cos(yaw) * std::cos(p), std::sin(p) } * s_cam.dist;
    };
    const auto   clear_at  = [&](double pitch_deg) {
        constexpr int LOS_SAMPLES = 24;
        const V3 e = eye_at(pitch_deg);
        for (int i = 2; i <= LOS_SAMPLES; ++i) {
            const double t  = (double)i / LOS_SAMPLES;   // 0 at the centre, 1 at the camera
            const V3     p  = target + (e - target) * t;
            const auto   gz = ground_z(f, p.x, p.y);
            if (gz && p.z < *gz + clearance * t) return false;
        }
        return true;
    };
    if (s_cam.dist < CAMERA_LOS_MAX_DIST && !clear_at(s_cam.pitch_deg)) {
        double lo = s_cam.pitch_deg, hi = 89.0;
        for (int k = 0; k < 10; ++k) {
            const double mid = 0.5 * (lo + hi);
            (clear_at(mid) ? hi : lo) = mid;
        }
        s_cam.pitch_deg = hi;
    }
    V3 eye = eye_at(s_cam.pitch_deg);

    // Never below the ground right under the camera, whatever the search
    // found: that would show the inside of the hill.
    if (const auto gz = ground_z(f, eye.x, eye.y)) eye.z = std::max(eye.z, *gz + 2.0);

    // Near plane from how close the ground really is, so a camera low over a
    // slope does not clip away the slope.
    const auto   under = ground_z(f, eye.x, eye.y);
    const double above = under ? std::max(1.0, eye.z - *under) : s_cam.dist;
    const double range = std::clamp(s_cam.dist * RANGE_K, MIN_RANGE, MAX_RANGE);
    const float  near_ = (float)std::max(1.0, std::min(above * 0.5, s_cam.dist * 0.02));
    const M4 proj = perspective(FOV_Y, size.x / size.y, near_, (float)(range * 1.2 + s_cam.dist));
    f.vp = mul(proj, look_at(eye, target, { 0.0, 0.0, 1.0 }));

    Projection shadow;
    if (scene.mission && !scene.mission->empty())
        place_bands(f, eye, mission_bands(*scene.mission, active), shadow);
    place_bands(f, eye, fence_bands(scene.fence, active, scene.mission), shadow);
    render_scene(f, eye, range, fb_w, fb_h, *scene.fleet, shadow);

    const ImVec2 p1 = { p0.x + size.x, p0.y + size.y };
    dl->AddImage((ImTextureID)(intptr_t)g.color, p0, p1, { 0, 1 }, { 1, 0 });

    dl->PushClipRect(p0, p1, true);
    draw_trails(dl, f, *scene.fleet);
    if (scene.mission && !scene.mission->empty()) draw_mission(dl, f, eye, *scene.mission, active);
    if (scene.go && scene.go->has_target) draw_goto_target(dl, f, eye, *scene.go, scene.mission, active);
    draw_vehicles(dl, f, eye, *scene.fleet);

    for (int b : { ImGuiMouseButton_Left, ImGuiMouseButton_Right })
        if (ImGui::IsMouseClicked(b)) s_press_in_view[b] = hovered;

    // A mission item being placed has the pointer; GO HERE waits until not.
    const bool placing = scene.pick && scene.pick->add_command != 0 && !scene.pick->pick_done &&
                         !scene.pick->fence_drawing();
    // EDIT, likewise. A click beside a waypoint in EDIT is most likely a grab that
    // missed, so it starts no GO HERE; the right-click's CLEAR stays.
    const bool editing = !placing && scene.pick && scene.pick->drag_edit && scene.pick->edit_mission;
    if (!editing) s_wp_drag = {};
    if (placing) {
        s_goto = {};
        place_item(dl, f, eye, target, range, hovered, *scene.pick, scene.mission, active);
    } else if (editing) {
        s_place = {};
        s_place_cmd = 0;
        s_goto = {};
        edit_waypoints(dl, f, eye, target, range, hovered, *scene.pick->edit_mission, active, center);
        if (view_click(ImGuiMouseButton_Right, hovered)) s_clear_ready = true;
    } else {
        s_place = {};
        s_place_cmd = 0;
        if (scene.go) pick_goto(dl, f, eye, target, range, hovered, *scene.go, scene.mission, active);
    }
    dl->PopClipRect();
    return true;
}

void map3d_shutdown()
{
    for (auto& [k, h] : s_heights)
        if (h.tex) glDeleteTextures(1, &h.tex);
    s_heights.clear();
    if (!g.ok) return;
    if (g.fbo) {
        gl3::DeleteFramebuffers(1, &g.fbo);
        glDeleteTextures(1, &g.color);
        gl3::DeleteRenderbuffers(1, &g.depth);
    }
    gl3::DeleteBuffers(1, &g.vbo);
    gl3::DeleteBuffers(1, &g.ibo);
    gl3::DeleteVertexArrays(1, &g.vao);
    gl3::DeleteProgram(g.prog);
    for (Mesh* m : { &g.craft[0], &g.craft[1], &g.craft[2], &g.craft[3], &g.heli_rotor, &g.heli_tail, &g.plane_prop,
                     &g.quad_prop_front, &g.quad_prop_rear }) {
        gl3::DeleteBuffers(1, &m->vbo);
        gl3::DeleteVertexArrays(1, &m->vao);
    }
    gl3::DeleteProgram(g.craft_prog);
    glDeleteTextures(1, &g.craft_tex);
    gl3::DeleteBuffers(1, &g.flat_vbo);
    gl3::DeleteVertexArrays(1, &g.flat_vao);
    gl3::DeleteProgram(g.flat_prog);
    g = Gl{};
}
