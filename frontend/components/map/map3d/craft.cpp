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

// frontend/components/map/map3d/craft.cpp
//
// The aircraft in 3D: each vehicle's pose from its attitude, and its airframe
// drawn there.

#include "frontend/components/map/map3d/map3d_internal.hpp"
#include "frontend/source/theme.hpp"
#include "frontend/source/terrain.hpp"

#include <algorithm>
#include <cmath>
#include <optional>
#include <unordered_map>
#include <vector>

namespace map3d_detail {

// ── Aircraft pose ─────────────────────────────────────────────────────────────

// The body axes — forward, right, down — in the world frame, from ATTITUDE's
// roll, pitch and yaw (degrees). The rotation is the usual aerospace one,
// yaw then pitch then roll, worked in north/east/down and turned into the
// world's east/north/up.
static Basis body_axes(double roll_deg, double pitch_deg, double yaw_deg)
{
    const double cr = std::cos(roll_deg * DEG),  sr = std::sin(roll_deg * DEG);
    const double cp = std::cos(pitch_deg * DEG), sp = std::sin(pitch_deg * DEG);
    const double cy = std::cos(yaw_deg * DEG),   sy = std::sin(yaw_deg * DEG);
    const double R[3][3] = {   // body to NED
        { cy * cp, cy * sp * sr - sy * cr, cy * sp * cr + sy * sr },
        { sy * cp, sy * sp * sr + cy * cr, sy * sp * cr - cy * sr },
        { -sp,     cp * sr,                cp * cr                },
    };
    const auto col = [&](int j) { return V3{ R[1][j], R[0][j], -R[2][j] }; };   // NED to ENU
    return { col(0), col(1), col(2) };
}

constexpr double CRAFT_PX = 30.0;   // screen pixels per body unit

// How large each airframe is drawn against that. The helicopter and the
// plane are drawn alike — the two are about the same length and span — and
// the quadcopter a fifth larger than the arrow, all three so their parts read.
static double craft_size(SymbolKind kind)
{
    switch (kind) {
    case SymbolKind::Helicopter:
    case SymbolKind::Plane:       return 1.215;
    case SymbolKind::Multicopter: return 1.2;
    default:                      return 1.0;
    }
}

Pose vehicle_pose(const Frame& f, V3 eye, const MapVehicle& mv)
{
    Pose ps;
    ps.ground = terrain_amsl(mv.lat, mv.lon);
    const double amsl = mv.has_alt ? mv.alt_amsl : ps.ground ? *ps.ground : f.href;
    ps.p = f.world(mv.lat, mv.lon, amsl);
    const double yaw = mv.has_att ? mv.yaw : mv.has_hdg ? mv.heading : 0.0;
    ps.b = mv.has_att ? body_axes(mv.roll, mv.pitch, yaw) : body_axes(0.0, 0.0, yaw);
    const V3 d = ps.p - eye;
    ps.s = craft_size(map_symbol_kind(mv.mav_type)) *
           CRAFT_PX * 2.0 * std::sqrt(dot(d, d)) * std::tan(FOV_Y * 0.5) / std::max(1.0f, f.size.y);
    return ps;
}

// Body frame to world, column-major: the axes scaled by s, then the position.
static M4 model_matrix(const Basis& b, double s, V3 p)
{
    M4 m{};
    const V3 c[3] = { b.x * s, b.y * s, b.z * s };
    for (int i = 0; i < 3; ++i) {
        m.m[i * 4 + 0] = (float)c[i].x;
        m.m[i * 4 + 1] = (float)c[i].y;
        m.m[i * 4 + 2] = (float)c[i].z;
    }
    m.m[12] = (float)p.x; m.m[13] = (float)p.y; m.m[14] = (float)p.z; m.m[15] = 1.0f;
    return m;
}

// Each helicopter's rotor angle, turned only while it is armed — as in 2D, the
// blades stop where they are on disarm and spin up from there.
struct RotorAngles { float main = 0.0f, tail = 0.0f; };
static std::unordered_map<VehicleId, RotorAngles, VehicleIdHash> s_rotor;

// The ground under an aircraft as a plane: its height under the centre and
// its slope there, from samples a model's reach either side — enough to tell
// which parts of the model are in the hill. nullopt while the terrain loads.
struct GroundPlane { double x, y, z, gx, gy, zmax; };
static std::optional<GroundPlane> ground_plane(const Frame& f, V3 p, double reach)
{
    const auto z0 = ground_z(f, p.x, p.y);
    if (!z0) return std::nullopt;
    GroundPlane gp{ p.x, p.y, *z0, 0.0, 0.0, *z0 };
    const auto xp = ground_z(f, p.x + reach, p.y), xm = ground_z(f, p.x - reach, p.y);
    const auto yp = ground_z(f, p.x, p.y + reach), ym = ground_z(f, p.x, p.y - reach);
    if (xp && xm) gp.gx = (*xp - *xm) / (2.0 * reach);
    if (yp && ym) gp.gy = (*yp - *ym) / (2.0 * reach);
    for (const auto& z : { xp, xm, yp, ym }) if (z) gp.zmax = std::max(gp.zmax, *z);
    return gp;
}

void render_craft(const Frame& f, V3 eye, double range, const std::vector<MapVehicle>& fleet)
{
    gl3::UseProgram(g.craft_prog);
    gl3::UniformMatrix4fv(g.c_vp, 1, GL_FALSE, f.vp.m);
    gl3::Uniform3f(g.c_eye, (float)eye.x, (float)eye.y, (float)eye.z);
    const ImVec4 fog = g_theme.bg_map;
    gl3::Uniform3f(g.c_fog, fog.x, fog.y, fog.z);
    gl3::Uniform2f(g.c_fog_range, (float)(range * 0.45), (float)range);
    // On a unit of its own, clear of the terrain's two.
    gl3::ActiveTexture(gl3::TEXTURE0 + 2);
    glBindTexture(gl3::TEXTURE_2D_ARRAY, g.craft_tex);
    gl3::Uniform1i(g.c_tex, 2);

    const float dt = ImGui::GetIO().DeltaTime;
    struct Part { const Mesh* mesh; M4 m; };
    std::vector<Part> parts;
    for (const MapVehicle& mv : fleet) {
        if (!mv.has_pos) continue;
        const Pose       ps   = vehicle_pose(f, eye, mv);
        const SymbolKind kind = map_symbol_kind(mv.mav_type);

        // The airframe and every part that turns, each with its own matrix,
        // so the aircraft can be drawn twice: as it is, then its buried part.
        parts.clear();
        parts.push_back({ &g.craft[(int)kind], model_matrix(ps.b, ps.s, ps.p) });
        // A part that turns, at `hub` (body units) with its own axes.
        const auto add = [&](const Mesh& mesh, const Basis& b, V3 hub) {
            const V3 at = ps.p + (ps.b.x * hub.x + ps.b.y * hub.y + ps.b.z * hub.z) * ps.s;
            parts.push_back({ &mesh, model_matrix(b, ps.s, at) });
        };
        // Rotors and propellers turn only while armed. Each keeps its own
        // angle, so none jumps when another wraps.
        RotorAngles& ang = s_rotor.try_emplace(mv.id).first->second;

        if (kind == SymbolKind::Multicopter) {
            // Diagonal pairs turn opposite ways, as on the aircraft: front
            // right and rear left one way, front left and rear right the other.
            if (mv.armed) ang.main = std::fmod(ang.main + 14.0f * dt, (float)(2.0 * M_PI));
            for (int i = 0; i < 4; ++i) {
                const double x = (i & 1) ? QUAD_ARM : -QUAD_ARM, y = (i & 2) ? QUAD_ARM : -QUAD_ARM;
                const double a = ((x > 0) == (y > 0) ? 1.0 : -1.0) * ang.main + i * 0.7;
                const double c = std::cos(a), sn = std::sin(a);
                add(x > 0 ? g.quad_prop_front : g.quad_prop_rear,
                    { ps.b.x * c + ps.b.y * sn, ps.b.y * c - ps.b.x * sn, ps.b.z }, { x, y, QUAD_PROP_Z });
            }
        }

        if (kind == SymbolKind::Plane) {
            if (mv.armed) ang.main = std::fmod(ang.main + 14.0f * dt, (float)(2.0 * M_PI));
            const double cp = std::cos(ang.main), sp = std::sin(ang.main);
            add(g.plane_prop, { ps.b.x, ps.b.y * cp + ps.b.z * sp, ps.b.z * cp - ps.b.y * sp }, PLANE_PROP_HUB);
        }

        if (kind == SymbolKind::Helicopter) {
            // The tail about 4.6 times as fast as the main, near enough the
            // real ratio.
            if (mv.armed) {
                ang.main = std::fmod(ang.main + 9.0f * dt, (float)(2.0 * M_PI));
                ang.tail = std::fmod(ang.tail + 9.0f * 4.6f * dt, (float)(2.0 * M_PI));
            }
            const double cm = std::cos(ang.main), sm = std::sin(ang.main);
            add(g.heli_rotor, { ps.b.x * cm + ps.b.y * sm, ps.b.y * cm - ps.b.x * sm, ps.b.z }, HELI_HUB);
            const double ct = std::cos(ang.tail), st = std::sin(ang.tail);
            add(g.heli_tail, { ps.b.x * ct - ps.b.z * st, ps.b.y, ps.b.x * st + ps.b.z * ct }, HELI_TAIL_HUB);
        }

        const auto draw_parts = [&] {
            for (const Part& pt : parts) {
                gl3::UniformMatrix4fv(g.c_model, 1, GL_FALSE, pt.m.m);
                gl3::BindVertexArray(pt.mesh->vao);
                glDrawArrays(GL_TRIANGLES, 0, pt.mesh->n);
            }
        };

        // As it is. Inactive aircraft drawn down, as on the 2D map.
        gl3::Uniform1i(g.c_xray, 0);
        gl3::Uniform1f(g.c_dim, mv.active ? 1.0f : 0.6f);
        draw_parts();

        // Its part inside the terrain, through the terrain, as an amber cage.
        // Skipped when the whole model is plainly above the ground: its reach
        // is about one and a half body units from the centre, rotors included.
        const double reach = 1.6 * ps.s;
        const auto   gp    = ground_plane(f, ps.p, reach);
        if (gp && ps.p.z - reach < gp->zmax) {
            gl3::Uniform1i(g.c_xray, 1);
            gl3::Uniform3f(g.c_gplane, (float)gp->x, (float)gp->y, (float)gp->z);
            gl3::Uniform2f(g.c_ggrad, (float)gp->gx, (float)gp->gy);
            glDisable(GL_DEPTH_TEST);
            draw_parts();
            glEnable(GL_DEPTH_TEST);
        }
    }
    gl3::Uniform1i(g.c_xray, 0);
    glBindTexture(gl3::TEXTURE_2D_ARRAY, 0);
    gl3::ActiveTexture(gl3::TEXTURE0);
}

} // namespace map3d_detail
