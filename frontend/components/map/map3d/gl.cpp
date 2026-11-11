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

// frontend/components/map/map3d/gl.cpp
//
// The GL side of the 3D map: the shaders, the objects they draw from, and the
// offscreen target the frame is drawn into.

#include "frontend/components/map/map3d/map3d_internal.hpp"

#include <cstdint>
#include <cstdio>
#include <vector>

namespace map3d_detail {

// ── GL objects ────────────────────────────────────────────────────────────────

Gl g;

static const char* VS = R"(#version 330 core
layout(location = 0) in vec3 a_grid;   // column, row, 1 for a skirt vertex
uniform mat4  u_vp;
uniform vec2  u_origin;    // the tile's north-west corner, east/north metres
uniform vec2  u_size;      // its extent east and south, metres
uniform float u_exag;
uniform float u_href;
uniform float u_skirt;
uniform sampler2D u_height;
uniform vec4  u_uv;        // the imagery rectangle this tile reads
uniform vec3  u_eye;
uniform vec2  u_cell_off;    // the tile's corner, in mesh-grid cells (fraction only)
uniform float u_cell_scale;  // mesh-grid cells per vertex step
out vec2  v_uv;
out float v_dist;
out vec3  v_world;
out vec2  v_cell;
void main() {
    float h = texelFetch(u_height, ivec2(a_grid.xy), 0).r;
    vec2  f = a_grid.xy / 32.0;
    vec3  p = vec3(u_origin.x + f.x * u_size.x,
                   u_origin.y - f.y * u_size.y,
                   (h - u_href) * u_exag - a_grid.z * u_skirt);
    p.z    -= dot(p.xy, p.xy) / (2.0 * 6371000.0) * u_exag;   // Frame::drop()
    v_uv    = mix(u_uv.xy, u_uv.zw, f);
    v_dist  = distance(p, u_eye);
    v_world = p;
    v_cell  = u_cell_off + a_grid.xy * u_cell_scale;
    gl_Position = u_vp * vec4(p, 1.0);
}
)";

// With u_mesh, the bare elevation: olive, shaded by the slope's own normal
// (from the screen-space change in position, so no normals are stored), with
// the mesh grid drawn on it — one spacing across every tile, the coarsest
// tile's (see render_scene) — faded out where its cells shrink under a couple
// of pixels and the lines would only blur into the colour.
static const char* FS = R"(#version 330 core
in vec2  v_uv;
in float v_dist;
in vec3  v_world;
in vec2  v_cell;
uniform sampler2D u_image;
uniform int   u_has_image;
uniform int   u_mesh;
uniform vec3  u_mesh_fill;   // the bare mesh's surface
uniform vec3  u_mesh_line;   // and its grid
uniform vec3  u_flat;
uniform vec3  u_fog;
uniform vec2  u_fog_range;
out vec4 o_color;
void main() {
    vec3 c;
    if (u_mesh != 0) {
        vec3 n = normalize(cross(dFdx(v_world), dFdy(v_world)));
        if (n.z < 0.0) n = -n;
        float l = 0.30 + 0.70 * max(dot(n, normalize(vec3(-0.45, 0.55, 0.70))), 0.0);
        vec2  w = fwidth(v_cell);
        vec2  g = abs(fract(v_cell - 0.5) - 0.5) / max(w, vec2(1e-4));
        float line = (1.0 - min(min(g.x, g.y), 1.0)) * (1.0 - smoothstep(0.15, 0.45, max(w.x, w.y)));
        c = mix(u_mesh_fill * l, u_mesh_line, line * 0.7);
    } else {
        c = u_has_image != 0 ? texture(u_image, v_uv).rgb : u_flat;
    }
    o_color = vec4(mix(c, u_fog, smoothstep(u_fog_range.x, u_fog_range.y, v_dist)), 1.0);
}
)";

static const char* CRAFT_VS = R"(#version 330 core
layout(location = 0) in vec3 a_pos;    // body frame: x forward, y right, z down
layout(location = 1) in vec3 a_nrm;
layout(location = 2) in vec3 a_col;
layout(location = 3) in vec2 a_uv;
layout(location = 4) in float a_layer;
uniform mat4 u_vp;
uniform mat4 u_model;                  // body to world, attitude and scale in it
uniform vec3 u_eye;
out vec3  v_nrm;
out vec3  v_col;
out float v_dist;
out vec3  v_world;
out vec3  v_body;
out vec2  v_uv;
flat out float v_layer;
void main() {
    vec4 w  = u_model * vec4(a_pos, 1.0);
    v_nrm   = mat3(u_model) * a_nrm;
    v_col   = a_col;
    v_dist  = distance(w.xyz, u_eye);
    v_world = w.xyz;
    v_body  = a_pos;
    v_uv    = a_uv;
    v_layer = a_layer;
    gl_Position = u_vp * w;
}
)";

// The airframe texture times the vertex tint, lit from high in the south-east
// with a cool sky above and a warm bounce from the ground below, and a glint
// where the surface is glossy (the texture's alpha). Every normal is turned
// to the eye first, so a mesh's winding never matters. The navigation lights'
// layer is drawn unlit. Fogged as the terrain is, so a far aircraft sits in it.
//
// With u_xray, the second pass over an aircraft — drawn through the terrain,
// the depth test off: only what lies under the ground plane u_gplane /
// u_ggrad, and of that only a grid fixed to the airframe, every quarter of a
// body unit, in thick bright amber. The rest of the fragments are dropped,
// so what shows is the buried part of the aircraft as a wire cage.
static const char* CRAFT_FS = R"(#version 330 core
in vec3  v_nrm;
in vec3  v_col;
in float v_dist;
in vec3  v_world;
in vec3  v_body;
in vec2  v_uv;
flat in float v_layer;
uniform sampler2DArray u_tex;
uniform vec3  u_eye;
uniform vec3  u_fog;
uniform vec2  u_fog_range;
uniform float u_dim;
uniform int   u_xray;
uniform vec3  u_gplane;   // a point on the ground under the aircraft, world
uniform vec2  u_ggrad;    // the ground's slope there, z per x and per y
out vec4 o_color;
void main() {
    if (u_xray != 0) {
        float ground = u_gplane.z + dot(u_ggrad, v_world.xy - u_gplane.xy);
        if (v_world.z >= ground) discard;
        vec3  q = v_body / 0.25 + 0.37;            // off the parts' own flat faces
        vec3  w = max(fwidth(q), vec3(1e-4));
        vec3  d = abs(fract(q - 0.5) - 0.5) / w;   // pixels to the nearest line
        if (min(min(d.x, d.y), d.z) > 1.25) discard;
        o_color = vec4(1.0, 0.76, 0.22, 1.0);
        return;
    }
    vec4  tx = texture(u_tex, vec3(v_uv, v_layer));
    vec3  c;
    if (v_layer > 7.5) {   // L_LIGHT
        c = v_col * u_dim;
    } else {
        vec3  n = normalize(v_nrm);
        vec3  e = normalize(u_eye - v_world);
        if (dot(n, e) < 0.0) n = -n;
        vec3  L   = normalize(vec3(0.35, -0.45, 0.82));
        vec3  amb = mix(vec3(0.42, 0.38, 0.33), vec3(0.58, 0.64, 0.74), 0.5 + 0.5 * n.z);
        float dif = max(dot(n, L), 0.0);
        float spc = pow(max(dot(n, normalize(L + e)), 0.0), 60.0) * tx.a;
        float rim = pow(1.0 - max(dot(n, e), 0.0), 4.0) * 0.18 * tx.a;
        c = (tx.rgb * v_col * (amb * 0.62 + dif * 0.72) + vec3(0.55 * spc + rim)) * u_dim;
    }
    o_color = vec4(mix(c, u_fog, smoothstep(u_fog_range.x, u_fog_range.y, v_dist)), 1.0);
}
)";

static GLuint compile(GLenum type, const char* src)
{
    const GLuint s = gl3::CreateShader(type);
    gl3::ShaderSource(s, 1, &src, nullptr);
    gl3::CompileShader(s);
    GLint ok = 0;
    gl3::GetShaderiv(s, gl3::COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        gl3::GetShaderInfoLog(s, sizeof(log), nullptr, log);
        fprintf(stderr, "map3d: shader: %s\n", log);
        gl3::DeleteShader(s);
        return 0;
    }
    return s;
}

static GLuint link(const char* vs_src, const char* fs_src)
{
    const GLuint vs = compile(gl3::VERTEX_SHADER, vs_src);
    const GLuint fs = compile(gl3::FRAGMENT_SHADER, fs_src);
    if (!vs || !fs) return 0;
    const GLuint prog = gl3::CreateProgram();
    gl3::AttachShader(prog, vs);
    gl3::AttachShader(prog, fs);
    gl3::LinkProgram(prog);
    gl3::DeleteShader(vs);
    gl3::DeleteShader(fs);
    GLint linked = 0;
    gl3::GetProgramiv(prog, gl3::LINK_STATUS, &linked);
    if (!linked) {
        char log[1024];
        gl3::GetProgramInfoLog(prog, sizeof(log), nullptr, log);
        fprintf(stderr, "map3d: link: %s\n", log);
        gl3::DeleteProgram(prog);
        return 0;
    }
    return prog;
}

static const char* FLAT_VS = R"(#version 330 core
layout(location = 0) in vec3 a_pos;
uniform mat4 u_vp;
void main() { gl_Position = u_vp * vec4(a_pos, 1.0); }
)";
static const char* FLAT_FS = R"(#version 330 core
uniform vec4 u_col;
out vec4 o_color;
void main() { o_color = u_col; }
)";

static bool init_flat()
{
    g.flat_prog = link(FLAT_VS, FLAT_FS);
    if (!g.flat_prog) return false;
    g.f_vp  = gl3::GetUniformLocation(g.flat_prog, "u_vp");
    g.f_col = gl3::GetUniformLocation(g.flat_prog, "u_col");
    gl3::GenVertexArrays(1, &g.flat_vao);
    gl3::BindVertexArray(g.flat_vao);
    gl3::GenBuffers(1, &g.flat_vbo);
    gl3::BindBuffer(gl3::ARRAY_BUFFER, g.flat_vbo);
    gl3::VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    gl3::EnableVertexAttribArray(0);
    gl3::BindVertexArray(0);
    return true;
}

static bool init_craft()
{
    g.craft_prog = link(CRAFT_VS, CRAFT_FS);
    if (!g.craft_prog) return false;
#define U(field, name) g.field = gl3::GetUniformLocation(g.craft_prog, name)
    U(c_vp, "u_vp"); U(c_model, "u_model"); U(c_eye, "u_eye");
    U(c_fog, "u_fog"); U(c_fog_range, "u_fog_range"); U(c_dim, "u_dim");
    U(c_xray, "u_xray"); U(c_gplane, "u_gplane"); U(c_ggrad, "u_ggrad");
    U(c_tex, "u_tex");
#undef U
    g.craft_tex = make_airframe_texture();
    void (*const build[4])(std::vector<CV>&) = { build_arrow, build_plane, build_multicopter, build_helicopter };
    for (int k = 0; k < 4; ++k) {
        std::vector<CV> v;
        build[k](v);
        g.craft[k] = upload(v);
    }
    std::vector<CV> rotor, tail;
    build_heli_rotor(rotor);
    build_heli_tail_rotor(tail);
    g.heli_rotor = upload(rotor);
    g.heli_tail  = upload(tail);
    std::vector<CV> prop;
    build_plane_prop(prop);
    g.plane_prop = upload(prop);
    std::vector<CV> qf, qr;
    build_quad_prop(qf, FRONT_PROP);
    build_quad_prop(qr, REAR);
    g.quad_prop_front = upload(qf);
    g.quad_prop_rear  = upload(qr);
    return true;
}

bool init_gl()
{
    if (g.tried) return g.ok;
    g.tried = true;
    if (!gl3::load()) { fprintf(stderr, "map3d: OpenGL 3.3 functions missing\n"); return false; }

    g.prog = link(VS, FS);
    if (!g.prog || !init_craft() || !init_flat()) return false;
#define U(name) g.name = gl3::GetUniformLocation(g.prog, #name)
    U(u_vp); U(u_origin); U(u_size); U(u_exag); U(u_href); U(u_skirt); U(u_height);
    U(u_image); U(u_uv); U(u_eye); U(u_has_image); U(u_flat); U(u_fog); U(u_fog_range); U(u_mesh);
    U(u_cell_off); U(u_cell_scale); U(u_mesh_fill); U(u_mesh_line);
#undef U

    // The shared tile: the grid, then a copy of each edge's vertices marked
    // as skirt, which the shader drops below the surface.
    std::vector<float> verts;
    for (int j = 0; j < GV; ++j)
        for (int i = 0; i < GV; ++i) verts.insert(verts.end(), { (float)i, (float)j, 0.0f });
    const auto grid_at = [](int i, int j) { return (uint16_t)(j * GV + i); };

    std::vector<uint16_t> idx;
    for (int j = 0; j < GRID; ++j)
        for (int i = 0; i < GRID; ++i) {
            const uint16_t a = grid_at(i, j), b = grid_at(i + 1, j),
                           c = grid_at(i, j + 1), d = grid_at(i + 1, j + 1);
            idx.insert(idx.end(), { a, c, b, b, c, d });
        }
    // Edges as runs of grid vertices: north, south, west, east.
    for (int e = 0; e < 4; ++e) {
        const uint16_t base = (uint16_t)(verts.size() / 3);
        for (int k = 0; k < GV; ++k) {
            const int i = e == 2 ? 0 : e == 3 ? GRID : k;
            const int j = e == 0 ? 0 : e == 1 ? GRID : k;
            verts.insert(verts.end(), { (float)i, (float)j, 1.0f });
        }
        for (int k = 0; k < GRID; ++k) {
            const int i0 = e == 2 ? 0 : e == 3 ? GRID : k, i1 = e >= 2 ? i0 : k + 1;
            const int j0 = e == 0 ? 0 : e == 1 ? GRID : k, j1 = e < 2 ? j0 : k + 1;
            const uint16_t a = grid_at(i0, j0), b = grid_at(i1, j1);
            const uint16_t sa = (uint16_t)(base + k), sb = (uint16_t)(base + k + 1);
            idx.insert(idx.end(), { a, b, sb, a, sb, sa });
        }
    }
    g.n_idx = (GLsizei)idx.size();

    gl3::GenVertexArrays(1, &g.vao);
    gl3::BindVertexArray(g.vao);
    gl3::GenBuffers(1, &g.vbo);
    gl3::BindBuffer(gl3::ARRAY_BUFFER, g.vbo);
    gl3::BufferData(gl3::ARRAY_BUFFER, (gl3::Sizeiptr)(verts.size() * sizeof(float)),
                    verts.data(), gl3::STATIC_DRAW);
    gl3::VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    gl3::EnableVertexAttribArray(0);
    gl3::GenBuffers(1, &g.ibo);
    gl3::BindBuffer(gl3::ELEMENT_ARRAY_BUFFER, g.ibo);
    gl3::BufferData(gl3::ELEMENT_ARRAY_BUFFER, (gl3::Sizeiptr)(idx.size() * sizeof(uint16_t)),
                    idx.data(), gl3::STATIC_DRAW);
    gl3::BindVertexArray(0);

    g.ok = true;
    return true;
}

// The offscreen target, remade when the panel changes size.
bool ensure_fbo(int w, int h)
{
    if (g.fbo && w == g.fb_w && h == g.fb_h) return true;
    if (g.fbo) {
        gl3::DeleteFramebuffers(1, &g.fbo);
        glDeleteTextures(1, &g.color);
        gl3::DeleteRenderbuffers(1, &g.depth);
        g.fbo = g.color = g.depth = 0;
    }
    glGenTextures(1, &g.color);
    glBindTexture(GL_TEXTURE_2D, g.color);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);

    gl3::GenRenderbuffers(1, &g.depth);
    gl3::BindRenderbuffer(gl3::RENDERBUFFER, g.depth);
    gl3::RenderbufferStorage(gl3::RENDERBUFFER, gl3::DEPTH_COMPONENT24, w, h);
    gl3::BindRenderbuffer(gl3::RENDERBUFFER, 0);

    gl3::GenFramebuffers(1, &g.fbo);
    gl3::BindFramebuffer(gl3::FRAMEBUFFER, g.fbo);
    gl3::FramebufferTexture2D(gl3::FRAMEBUFFER, gl3::COLOR_ATTACHMENT0, GL_TEXTURE_2D, g.color, 0);
    gl3::FramebufferRenderbuffer(gl3::FRAMEBUFFER, gl3::DEPTH_ATTACHMENT, gl3::RENDERBUFFER, g.depth);
    const bool ok = gl3::CheckFramebufferStatus(gl3::FRAMEBUFFER) == gl3::FRAMEBUFFER_COMPLETE;
    gl3::BindFramebuffer(gl3::FRAMEBUFFER, 0);
    g.fb_w = w; g.fb_h = h;
    return ok;
}

} // namespace map3d_detail
