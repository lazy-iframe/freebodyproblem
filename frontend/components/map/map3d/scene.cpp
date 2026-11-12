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

// frontend/components/map/map3d/scene.cpp
//
// The terrain and what stands on it: the quadtree of tiles in view, their
// imagery, the bands cast onto the ground, and the frame drawn into the target.

#include "frontend/components/map/map3d/map3d_internal.hpp"
#include "frontend/source/theme.hpp"
#include "frontend/source/terrain.hpp"

#include <algorithm>
#include <cmath>
#include <optional>
#include <vector>

namespace map3d_detail {

// The ground's world height under a world point, once its terrain is loaded.
std::optional<double> ground_z(const Frame& f, double ex, double ey)
{
    double lat, lon;
    geo_unmercator(f.u0 + ex / f.m, f.v0 - ey / f.m, lat, lon);
    if (auto g = terrain_amsl(lat, lon)) return (*g - f.href) * f.exag - f.drop(ex, ey);
    return std::nullopt;
}

struct Leaf { Key k; };

// Choose the tiles to draw: a quadtree from tiles about the view range across,
// split while the camera is near, dropping tiles past the range or outside
// the frustum.
static void select_tiles(const Frame& f, V3 eye, double range, std::vector<Leaf>& leaves)
{
    const double size0 = f.m;   // world metres across a z = 0 tile
    int zr = (int)std::floor(std::log2(size0 / range));
    zr = std::clamp(zr, 1, ZMAX);
    const int n = 1 << zr;
    const double span = range / size0 * n;   // range in tiles at zr
    const int x0 = std::max(0, (int)std::floor(f.u0 * n - span)), x1 = std::min(n - 1, (int)std::floor(f.u0 * n + span));
    const int y0 = std::max(0, (int)std::floor(f.v0 * n - span)), y1 = std::min(n - 1, (int)std::floor(f.v0 * n + span));

    const int z_finest = std::clamp((int)std::floor(std::log2(size0 / (s_cam.dist * FINEST_TILE_K))), zr, ZMAX);

    std::vector<Key> stack;
    for (int y = y0; y <= y1; ++y)
        for (int x = x0; x <= x1; ++x) stack.push_back({ x, y, zr });

    while (!stack.empty() && (int)leaves.size() < MAX_LEAVES) {
        const Key k = stack.back();
        stack.pop_back();
        const double s  = size0 / (double)(1 << k.z);
        const double e0 = (k.x * s / size0 - f.u0) * f.m;       // west edge, east metres
        const double n0 = -(k.y * s / size0 - f.v0) * f.m;      // north edge, north metres
        const double e1 = e0 + s, n1 = n0 - s;

        // Past the range: nothing here the fog has not already hidden.
        const double cx = std::clamp(0.0, e0, e1), cy = std::clamp(0.0, n1, n0);
        if (std::hypot(cx, cy) > range) continue;

        // Height span: the tile's own, once known; else generous.
        double hz0 = -800.0, hz1 = 3000.0;
        if (auto it = s_heights.find(k); it != s_heights.end() && it->second.tex) {
            hz0 = (it->second.hmin - f.href) * f.exag - 50.0;
            hz1 = (it->second.hmax - f.href) * f.exag + 50.0;
        }
        // Less the curve: most at its farthest corner, least at its nearest point.
        hz0 -= f.drop(std::max(std::fabs(e0), std::fabs(e1)), std::max(std::fabs(n0), std::fabs(n1)));
        hz1 -= f.drop(cx, cy);

        // Frustum: all eight corners beyond one clip plane.
        int out[6] = {};
        for (int c = 0; c < 8; ++c) {
            const V4 p = xform(f.vp, { (c & 1) ? e1 : e0, (c & 2) ? n1 : n0, (c & 4) ? hz1 : hz0 });
            out[0] += p.x < -p.w; out[1] += p.x > p.w;
            out[2] += p.y < -p.w; out[3] += p.y > p.w;
            out[4] += p.z < -p.w; out[5] += p.z > p.w;
        }
        if (*std::max_element(out, out + 6) == 8) continue;

        // Split while the camera is close for the tile's size.
        const double nx = std::clamp(eye.x, e0, e1), ny = std::clamp(eye.y, n1, n0);
        const double nz = std::clamp(eye.z, hz0, hz1);
        const double d  = std::sqrt((eye.x - nx) * (eye.x - nx) + (eye.y - ny) * (eye.y - ny) +
                                    (eye.z - nz) * (eye.z - nz));
        if (k.z < z_finest && d < SPLIT_K * s) {
            for (int c = 0; c < 4; ++c)
                stack.push_back({ k.x * 2 + (c & 1), k.y * 2 + (c >> 1), k.z + 1 });
            continue;
        }
        leaves.push_back({ k });
    }
}

// The imagery for a tile: its own texture, else the matching part of the
// nearest coarser one already loaded. 0 when there is none yet; the tile is
// then in `wanted`, for the request queue to keep.
static GLuint imagery_for(const Key& k, float uv[4], std::vector<MapTileId>& wanted)
{
    uv[0] = 0; uv[1] = 0; uv[2] = 1; uv[3] = 1;
    if (GLuint t = map_tile_texture(k.z, k.x, k.y, true)) return t;
    wanted.push_back({ k.z, k.x, k.y });
    for (int dz = 1; dz <= 12 && k.z - dz >= 0; ++dz) {
        GLuint t = map_tile_texture(k.z - dz, k.x >> dz, k.y >> dz, false);
        if (!t) continue;
        const float cells = (float)(1 << dz);
        uv[0] = (float)(k.x - ((k.x >> dz) << dz)) / cells;
        uv[1] = (float)(k.y - ((k.y >> dz) << dz)) / cells;
        uv[2] = uv[0] + 1.0f / cells;
        uv[3] = uv[1] + 1.0f / cells;
        return t;
    }
    return 0;
}

static void push_v(std::vector<float>& v, V3 p) { v.insert(v.end(), { (float)p.x, (float)p.y, (float)p.z }); }

// A band from `bot` (along the ground) up to `top`, one sample further on: the
// quad between the last pair and this, and the edge segments it adds.
void band_step(TintLayer& L, V3 bot0, V3 top0, V3 bot, V3 top, bool top_edge)
{
    push_v(L.fill, top0); push_v(L.fill, bot0); push_v(L.fill, bot);
    push_v(L.fill, top0); push_v(L.fill, bot);  push_v(L.fill, top);
    push_v(L.lines, bot0); push_v(L.lines, bot);
    if (top_edge) { push_v(L.lines, top0); push_v(L.lines, top); }
}

static void render_projection(const Frame& f, const Projection& pr)
{
    if (pr.layers.empty()) return;
    gl3::UseProgram(g.flat_prog);
    gl3::UniformMatrix4fv(g.f_vp, 1, GL_FALSE, f.vp.m);
    gl3::BindVertexArray(g.flat_vao);
    gl3::BindBuffer(gl3::ARRAY_BUFFER, g.flat_vbo);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);   // seen through each other, hidden by the ground
    const auto draw = [&](const std::vector<float>& v, GLenum mode, float r, float gg, float b, float a) {
        if (v.empty()) return;
        gl3::BufferData(gl3::ARRAY_BUFFER, (gl3::Sizeiptr)(v.size() * sizeof(float)), v.data(), gl3::DYNAMIC_DRAW);
        gl3::Uniform4f(g.f_col, r, gg, b, a);
        glDrawArrays(mode, 0, (GLsizei)(v.size() / 3));
    };
    for (const TintLayer& L : pr.layers) {
        draw(L.fill,  GL_TRIANGLES, L.r, L.g, L.b, 0.16f);
        draw(L.lines, GL_LINES,     L.r, L.g, L.b, 0.95f);
    }
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void render_scene(const Frame& f, V3 eye, double range, int fb_w, int fb_h,
                  const std::vector<MapVehicle>& fleet, const Projection& proj)
{
    std::vector<Leaf> leaves;
    select_tiles(f, eye, range, leaves);

    // Nearest first, so the tiles under the camera are the first fetched and
    // the first given heights from this frame's budget.
    const auto centre_dist = [&](const Key& k) {
        const double n = (double)(1 << k.z);
        const double e = ((k.x + 0.5) / n - f.u0) * f.m, no = -((k.y + 0.5) / n - f.v0) * f.m;
        return (e - eye.x) * (e - eye.x) + (no - eye.y) * (no - eye.y);
    };
    std::sort(leaves.begin(), leaves.end(),
              [&](const Leaf& a, const Leaf& b) { return centre_dist(a.k) < centre_dist(b.k); });
    static std::vector<MapTileId> wanted;
    wanted.clear();

    // GL state ImGui's renderer and the rest of the frame expect back.
    GLint prev_fbo = 0, prev_prog = 0, prev_vao = 0, prev_active = 0, prev_vp[4];
    glGetIntegerv(gl3::FRAMEBUFFER_BINDING, &prev_fbo);
    glGetIntegerv(gl3::CURRENT_PROGRAM, &prev_prog);
    glGetIntegerv(gl3::VERTEX_ARRAY_BINDING, &prev_vao);
    glGetIntegerv(gl3::ACTIVE_TEXTURE, &prev_active);
    glGetIntegerv(GL_VIEWPORT, prev_vp);
    const GLboolean prev_scissor = glIsEnabled(GL_SCISSOR_TEST);
    const GLboolean prev_blend   = glIsEnabled(GL_BLEND);
    const GLboolean prev_cull    = glIsEnabled(GL_CULL_FACE);
    const GLboolean prev_depth   = glIsEnabled(GL_DEPTH_TEST);

    gl3::BindFramebuffer(gl3::FRAMEBUFFER, g.fbo);
    glViewport(0, 0, fb_w, fb_h);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    const ImVec4 fog = g_theme.bg_map;
    const ImVec4 flat = g_theme.bg_child_dark;
    glClearColor(fog.x, fog.y, fog.z, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    gl3::UseProgram(g.prog);
    gl3::BindVertexArray(g.vao);
    gl3::UniformMatrix4fv(g.u_vp, 1, GL_FALSE, f.vp.m);
    gl3::Uniform1f(g.u_exag, (float)f.exag);
    gl3::Uniform1f(g.u_href, (float)f.href);
    gl3::Uniform3f(g.u_eye, (float)eye.x, (float)eye.y, (float)eye.z);
    gl3::Uniform3f(g.u_fog, fog.x, fog.y, fog.z);
    gl3::Uniform3f(g.u_flat, flat.x, flat.y, flat.z);
    gl3::Uniform2f(g.u_fog_range, (float)(range * 0.45), (float)range);
    gl3::Uniform1i(g.u_image, 0);
    gl3::Uniform1i(g.u_height, 1);
    gl3::Uniform1i(g.u_mesh, s_textures ? 0 : 1);
    {
        // The bare mesh in the theme's label colour: the grid in it, the
        // surface a darker shade — olive on Tactical, amber on Retro Amber,
        // green on Matrix. A light theme's labels are dark greys, made for a
        // pale panel; it keeps Tactical's olive.
        const ImVec4 l = map_theme_is_light() ? ImVec4{ 0.706f, 0.776f, 0.561f, 1.0f } : g_theme.col_log;
        gl3::Uniform3f(g.u_mesh_line, l.x, l.y, l.z);
        gl3::Uniform3f(g.u_mesh_fill, l.x * 0.6f, l.y * 0.6f, l.z * 0.6f);
    }

    Budget budget;

    // The bare mesh's grid, at one spacing for every tile: the cells of the
    // coarsest tile drawn. A finer tile is an exact subdivision of a coarser
    // one, so these lines lie on vertex rows of every tile and run straight
    // across the seams between sizes of tile, where each tile's own cells
    // would change density at every seam.
    int mesh_z = ZMAX;
    for (const Leaf& l : leaves) mesh_z = std::min(mesh_z, l.k.z);
    for (const Leaf& l : leaves) {
        const Heights* h = heights_for(l.k, budget);
        if (!h) continue;
        float uv[4] = { 0, 0, 1, 1 };
        const GLuint img = s_textures ? imagery_for(l.k, uv, wanted) : 0;

        const double s  = f.m / (double)(1 << l.k.z);
        const double e0 = (l.k.x / (double)(1 << l.k.z) - f.u0) * f.m;
        const double n0 = -(l.k.y / (double)(1 << l.k.z) - f.v0) * f.m;
        gl3::Uniform2f(g.u_origin, (float)e0, (float)n0);
        gl3::Uniform2f(g.u_size, (float)s, (float)s);
        gl3::Uniform1f(g.u_skirt, (float)(s * 0.03 + 20.0));
        gl3::Uniform4f(g.u_uv, uv[0], uv[1], uv[2], uv[3]);
        {
            // Only the fraction of the corner's cell position matters to the
            // lines; taking it in double keeps it exact at any zoom.
            const double per = std::ldexp((double)GRID, mesh_z - l.k.z);   // coarse cells per tile
            gl3::Uniform2f(g.u_cell_off, (float)std::fmod(l.k.x * per, 1.0), (float)std::fmod(l.k.y * per, 1.0));
            gl3::Uniform1f(g.u_cell_scale, (float)(per / GRID));
        }
        gl3::Uniform1i(g.u_has_image, img ? 1 : 0);

        gl3::ActiveTexture(gl3::TEXTURE0 + 1);
        glBindTexture(GL_TEXTURE_2D, h->tex);
        gl3::ActiveTexture(gl3::TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, img);
        glDrawElements(GL_TRIANGLES, g.n_idx, GL_UNSIGNED_SHORT, nullptr);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    evict_heights();
    map_tile_queue_retain(wanted);

    // The aircraft in the same depth pass, so a ridge between the camera and
    // an aircraft hides it as it would the ground behind.
    render_craft(f, eye, range, fleet);

    // Last, being see-through: the mission's shadow on the ground.
    render_projection(f, proj);

    gl3::BindVertexArray((GLuint)prev_vao);
    gl3::UseProgram((GLuint)prev_prog);
    gl3::ActiveTexture((GLenum)prev_active);
    gl3::BindFramebuffer(gl3::FRAMEBUFFER, (GLuint)prev_fbo);
    glViewport(prev_vp[0], prev_vp[1], prev_vp[2], prev_vp[3]);
    if (prev_scissor) glEnable(GL_SCISSOR_TEST);
    if (prev_blend)   glEnable(GL_BLEND);
    if (prev_cull)    glEnable(GL_CULL_FACE);
    if (!prev_depth)  glDisable(GL_DEPTH_TEST);
}

} // namespace map3d_detail
