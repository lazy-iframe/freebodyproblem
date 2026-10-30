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


// frontend/widgets/map_view_3d.cpp
//
// How the 3D map is built, per frame:
//
//   World   A flat east/north/up frame in metres, centred on the point the
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
//           shader by a 33×33 height texture built from frontend/terrain.hpp,
//           and textured with the 2D map's own imagery tile (or the part of
//           a coarser one, while it loads). Skirts hang from every tile edge
//           so the seam between two sizes of tile shows no gap.
//
//   Frame   Drawn into an offscreen colour + depth target the size of the
//           panel, which is then an ImGui image; the overlays are ImGui
//           drawing on top, at the points the camera projects them to.

#include "map_view_3d.hpp"
#include "map_view_internal.hpp"
#include "gl3.hpp"
#include "theme.hpp"
#include "ui_kit.hpp"
#include "../mission_alt.hpp"
#include "../terrain.hpp"
#include "../terrain_clearance.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <optional>
#include <unordered_map>
#include <vector>

#ifndef M_PI
#  define M_PI 3.14159265358979323846
#endif

namespace {

constexpr double EARTH_C   = 40075016.686;   // Web Mercator world width, metres
constexpr double DEG       = M_PI / 180.0;
constexpr int    GRID      = 32;             // cells per tile edge
constexpr int    GV        = GRID + 1;       // vertices per tile edge
constexpr int    ZMAX      = 18;             // deepest imagery tile drawn
constexpr int    TERRAIN_Z = 12;             // deepest terrain read; SRTM has no more
constexpr double SPLIT_K   = 2.0;            // split while distance < SPLIT_K × tile size
constexpr int    MAX_LEAVES = 250;

// How far out the camera may go, and how far it then sees. The work is all in
// the range: tiles fetched, heights built, draws made. Five kilometres out is
// a whole mission area in view; past that the terrain is a texture, and the
// 2D map shows it better and cheaper.
constexpr double MIN_DIST   = 30.0;
constexpr double MAX_DIST   = 5000.0;
constexpr double RANGE_K    = 8.0;           // range, in camera distances
constexpr double MIN_RANGE  = 3000.0;
constexpr double MAX_RANGE  = 40000.0;
constexpr int    BUILDS_PER_FRAME = 24;      // height textures made per frame, at most
constexpr int    TRIES_PER_FRAME  = 96;      // terrain reads attempted per frame, at most
constexpr float  FOV_Y     = 45.0f * (float)DEG;

// ── Small linear algebra ──────────────────────────────────────────────────────

struct V3 { double x, y, z; };
V3     operator+(V3 a, V3 b)     { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
V3     operator-(V3 a, V3 b)     { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
V3     operator*(V3 a, double s) { return { a.x * s, a.y * s, a.z * s }; }
double dot(V3 a, V3 b)           { return a.x * b.x + a.y * b.y + a.z * b.z; }
V3     cross(V3 a, V3 b)         { return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x }; }
V3     norm(V3 a)                { const double l = std::sqrt(dot(a, a)); return l > 0 ? a * (1.0 / l) : a; }

// Column-major, as GL takes it: m[col * 4 + row].
struct M4 { float m[16]; };

M4 mul(const M4& a, const M4& b)
{
    M4 r{};
    for (int c = 0; c < 4; ++c)
        for (int rr = 0; rr < 4; ++rr) {
            float s = 0;
            for (int k = 0; k < 4; ++k) s += a.m[k * 4 + rr] * b.m[c * 4 + k];
            r.m[c * 4 + rr] = s;
        }
    return r;
}

M4 perspective(float fovy, float aspect, float n, float f)
{
    const float t = 1.0f / std::tan(fovy * 0.5f);
    M4 r{};
    r.m[0]  = t / aspect;
    r.m[5]  = t;
    r.m[10] = (f + n) / (n - f);
    r.m[11] = -1.0f;
    r.m[14] = 2.0f * f * n / (n - f);
    return r;
}

M4 look_at(V3 eye, V3 at, V3 up)
{
    const V3 f = norm(at - eye);
    const V3 s = norm(cross(f, up));
    const V3 u = cross(s, f);
    M4 r{};
    r.m[0] = (float)s.x;  r.m[4] = (float)s.y;  r.m[8]  = (float)s.z;
    r.m[1] = (float)u.x;  r.m[5] = (float)u.y;  r.m[9]  = (float)u.z;
    r.m[2] = (float)-f.x; r.m[6] = (float)-f.y; r.m[10] = (float)-f.z;
    r.m[12] = (float)-dot(s, eye);
    r.m[13] = (float)-dot(u, eye);
    r.m[14] = (float) dot(f, eye);
    r.m[15] = 1.0f;
    return r;
}

struct V4 { double x, y, z, w; };
V4 xform(const M4& m, V3 p)
{
    return { m.m[0] * p.x + m.m[4] * p.y + m.m[8]  * p.z + m.m[12],
             m.m[1] * p.x + m.m[5] * p.y + m.m[9]  * p.z + m.m[13],
             m.m[2] * p.x + m.m[6] * p.y + m.m[10] * p.z + m.m[14],
             m.m[3] * p.x + m.m[7] * p.y + m.m[11] * p.z + m.m[15] };
}

// ── Web Mercator ──────────────────────────────────────────────────────────────

// A point as a fraction of the world, 0..1 east from the antimeridian and
// south from the top.
void merc(double lat, double lon, double& u, double& v)
{
    lat = std::clamp(lat, -85.0511, 85.0511);
    const double r = lat * DEG;
    u = (lon + 180.0) / 360.0;
    v = (1.0 - std::log(std::tan(r) + 1.0 / std::cos(r)) / M_PI) / 2.0;
}

void unmerc(double u, double v, double& lat, double& lon)
{
    lon = u * 360.0 - 180.0;
    lat = std::atan(std::sinh(M_PI * (1.0 - 2.0 * v))) / DEG;
}

// ── View state ────────────────────────────────────────────────────────────────

struct Camera {
    double yaw_deg   = 0.0;     // where it looks, clockwise from north
    double pitch_deg = 35.0;    // down from the horizon
    double dist      = 1500.0;  // metres from the point it orbits
};

Camera       s_cam;
const float  EXAGS[] = { 1.0f, 1.5f, 2.0f };
int          s_exag  = 1;
bool         s_textures = true;   // imagery on the terrain; off, the bare mesh
int          s_drag  = -1;      // the button that started a drag on the view, or -1
std::optional<float> s_ground;  // last ground height under the centre

// ── GL objects ────────────────────────────────────────────────────────────────

struct Mesh { GLuint vao = 0, vbo = 0; GLsizei n = 0; };

struct Gl {
    bool   tried = false, ok = false;
    GLuint prog = 0, vao = 0, vbo = 0, ibo = 0;

    // Airframes: one body mesh per SymbolKind, and the helicopter's main
    // rotor apart from its body, so it can turn.
    GLuint craft_prog = 0;
    Mesh   craft[4], heli_rotor, heli_tail, plane_prop, quad_prop_front, quad_prop_rear;

    // Flat colour, for the mission's ground track and the curtain under it:
    // world positions streamed each frame.
    GLuint flat_prog = 0, flat_vao = 0, flat_vbo = 0;
    GLint  f_vp, f_col;
    GLint  c_vp, c_model, c_eye, c_fog, c_fog_range, c_dim, c_xray, c_gplane, c_ggrad,
           c_outline, c_px_per_dist, c_outline_col;

    GLsizei n_idx = 0;
    GLuint fbo = 0, color = 0, depth = 0;
    int    fb_w = 0, fb_h = 0;
    GLint  u_vp, u_origin, u_size, u_exag, u_href, u_skirt, u_height, u_image,
           u_uv, u_eye, u_has_image, u_flat, u_fog, u_fog_range, u_mesh,
           u_cell_off, u_cell_scale, u_mesh_fill, u_mesh_line;
};
Gl g;

const char* VS = R"(#version 330 core
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
const char* FS = R"(#version 330 core
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
        // The imagery a little dimmed, so the aircraft, the path and the
        // marks over it stand out from it.
        c = u_has_image != 0 ? texture(u_image, v_uv).rgb * 0.78 : u_flat;
    }
    o_color = vec4(mix(c, u_fog, smoothstep(u_fog_range.x, u_fog_range.y, v_dist)), 1.0);
}
)";

const char* CRAFT_VS = R"(#version 330 core
layout(location = 0) in vec3 a_pos;    // body frame: x forward, y right, z down
layout(location = 1) in vec3 a_nrm;
layout(location = 2) in vec3 a_col;
uniform mat4 u_vp;
uniform mat4 u_model;                  // body to world, attitude and scale in it
uniform vec3 u_eye;
uniform float u_outline;               // px to push the surface out by; 0 off
uniform float u_px_per_dist;           // world size of a pixel, per unit of distance
out vec3  v_nrm;
out vec3  v_col;
out float v_dist;
out vec3  v_world;
out vec3  v_body;
void main() {
    vec4 w  = u_model * vec4(a_pos, 1.0);
    // The glow passes: the surface pushed out along its normal by a fixed
    // number of pixels, whatever the distance.
    if (u_outline > 0.0)
        w.xyz += normalize(mat3(u_model) * a_nrm) * u_outline * u_px_per_dist * distance(w.xyz, u_eye);
    v_nrm   = mat3(u_model) * a_nrm;
    v_col   = a_col;
    v_dist  = distance(w.xyz, u_eye);
    v_world = w.xyz;
    v_body  = a_pos;
    gl_Position = u_vp * w;
}
)";

// Lit from high in the south-east, both faces alike, so a mesh's winding
// never matters; fogged as the terrain is, so a far aircraft sits in it.
//
// With u_xray, the second pass over an aircraft — drawn through the terrain,
// the depth test off: only what lies under the ground plane u_gplane /
// u_ggrad, and of that only a grid fixed to the airframe, every quarter of a
// body unit, in thick bright amber. The rest of the fragments are dropped,
// so what shows is the buried part of the aircraft as a wire cage.
const char* CRAFT_FS = R"(#version 330 core
in vec3  v_nrm;
in vec3  v_col;
in float v_dist;
in vec3  v_world;
in vec3  v_body;
uniform vec3  u_fog;
uniform vec2  u_fog_range;
uniform float u_dim;
uniform int   u_xray;
uniform vec3  u_gplane;   // a point on the ground under the aircraft, world
uniform vec2  u_ggrad;    // the ground's slope there, z per x and per y
uniform float u_outline;
uniform vec4  u_outline_col;
out vec4 o_color;
void main() {
    if (u_outline > 0.0) { o_color = u_outline_col; return; }
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
    float l = 0.45 + 0.55 * abs(dot(normalize(v_nrm), normalize(vec3(0.35, -0.45, 0.82))));
    vec3  c = v_col * l * u_dim;
    o_color = vec4(mix(c, u_fog, smoothstep(u_fog_range.x, u_fog_range.y, v_dist)), 1.0);
}
)";

GLuint compile(GLenum type, const char* src)
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

GLuint link(const char* vs_src, const char* fs_src)
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

// ── Airframe meshes ───────────────────────────────────────────────────────────
//
// Built from boxes, ellipsoids and tubes in the body frame, about a unit
// across, in red — the colour that stood out best over the imagery of all
// that were tried, a step brighter than the 2D symbols': the darker on the
// port side, the lighter to starboard.

struct CV  { float p[3], n[3], c[3]; };
struct Col { float r, g, b; };

constexpr Col PORT  = { 196 / 255.f,  34 / 255.f,  34 / 255.f };
constexpr Col STBD  = { 245 / 255.f,  72 / 255.f,  60 / 255.f };
constexpr Col FRONT = { 236 / 255.f, 185 / 255.f,  78 / 255.f };
constexpr Col DARK  = {  58 / 255.f,  58 / 255.f,  58 / 255.f };
constexpr Col REAR  = {  40 / 255.f, 110 / 255.f, 230 / 255.f };   // a quad's rear propellers
constexpr Col FRONT_PROP = { 60 / 255.f, 200 / 255.f, 90 / 255.f };  // and its front ones

void add_tri(std::vector<CV>& v, V3 a, V3 b, V3 c, Col col)
{
    const V3 n = norm(cross(b - a, c - a));
    for (const V3& p : { a, b, c })
        v.push_back({ { (float)p.x, (float)p.y, (float)p.z },
                      { (float)n.x, (float)n.y, (float)n.z }, { col.r, col.g, col.b } });
}

void add_quad(std::vector<CV>& v, V3 a, V3 b, V3 c, V3 d, Col col)
{
    add_tri(v, a, b, c, col);
    add_tri(v, a, c, d, col);
}

// A box about `c` with half-extents `h`, turned `yaw` radians about body z.
void add_box(std::vector<CV>& v, V3 c, V3 h, double yaw, Col col)
{
    const double cs = std::cos(yaw), sn = std::sin(yaw);
    V3 k[8];
    for (int i = 0; i < 8; ++i) {
        const double x = (i & 1) ? h.x : -h.x, y = (i & 2) ? h.y : -h.y, z = (i & 4) ? h.z : -h.z;
        k[i] = { c.x + x * cs - y * sn, c.y + x * sn + y * cs, c.z + z };
    }
    add_quad(v, k[0], k[1], k[3], k[2], col);   // -z
    add_quad(v, k[4], k[5], k[7], k[6], col);   // +z
    add_quad(v, k[0], k[1], k[5], k[4], col);   // -y
    add_quad(v, k[2], k[3], k[7], k[6], col);   // +y
    add_quad(v, k[0], k[2], k[6], k[4], col);   // -x
    add_quad(v, k[1], k[3], k[7], k[5], col);   // +x
}

// One vertex with its own normal: curved surfaces are shaded smooth, the
// normal following the surface rather than the facet.
void add_vertex(std::vector<CV>& v, V3 p, V3 n, Col col)
{
    v.push_back({ { (float)p.x, (float)p.y, (float)p.z },
                  { (float)n.x, (float)n.y, (float)n.z }, { col.r, col.g, col.b } });
}

// An ellipsoid about `c` with radii `r`, smooth. Its port half (y < 0 of its
// centre) takes `port`, the rest `stbd` — the two reds of the 2D symbols.
void add_ellipsoid(std::vector<CV>& v, V3 c, V3 r, Col port, Col stbd)
{
    constexpr int SLICES = 20, STACKS = 12;
    const auto pt = [&](int i, int j) {
        const double th = M_PI * j / STACKS, ph = 2 * M_PI * i / SLICES;
        return V3{ std::cos(th), std::sin(th) * std::cos(ph), std::sin(th) * std::sin(ph) };
    };
    const auto put = [&](V3 u) {
        const V3 p = { c.x + r.x * u.x, c.y + r.y * u.y, c.z + r.z * u.z };
        const V3 n = norm({ u.x / r.x, u.y / r.y, u.z / r.z });   // the ellipsoid's own normal
        add_vertex(v, p, n, u.y < 0 ? port : stbd);
    };
    for (int j = 0; j < STACKS; ++j)
        for (int i = 0; i < SLICES; ++i) {
            const V3 a = pt(i, j), b = pt(i + 1, j), cc = pt(i + 1, j + 1), d = pt(i, j + 1);
            put(a); put(b); put(cc);
            put(a); put(cc); put(d);
        }
}

// A tube from `a` to `b`, radius `ra` at one end and `rb` at the other — a
// tapered boom, a strut, a skid — smooth round, with flat caps. Its port side
// (y below the axis) takes `port`, the rest `stbd`.
void add_tube(std::vector<CV>& v, V3 a, V3 b, double ra, double rb, Col port, Col stbd)
{
    constexpr int SEG = 14;
    const V3 ax = norm(b - a);
    const V3 up = std::fabs(ax.z) < 0.9 ? V3{ 0, 0, 1 } : V3{ 1, 0, 0 };
    const V3 u  = norm(cross(ax, up)), w = cross(ax, u);
    const double len = std::sqrt(dot(b - a, b - a));
    for (int i = 0; i < SEG; ++i) {
        const double t0 = 2 * M_PI * i / SEG, t1 = 2 * M_PI * (i + 1) / SEG;
        const V3 d0 = u * std::cos(t0) + w * std::sin(t0), d1 = u * std::cos(t1) + w * std::sin(t1);
        // A cone's normal leans along the axis by the taper.
        const V3 n0 = norm(d0 + ax * ((ra - rb) / len)), n1 = norm(d1 + ax * ((ra - rb) / len));
        const Col c0 = d0.y < -1e-9 ? port : stbd, c1 = d1.y < -1e-9 ? port : stbd;
        add_vertex(v, a + d0 * ra, n0, c0); add_vertex(v, a + d1 * ra, n1, c1); add_vertex(v, b + d1 * rb, n1, c1);
        add_vertex(v, a + d0 * ra, n0, c0); add_vertex(v, b + d1 * rb, n1, c1); add_vertex(v, b + d0 * rb, n0, c0);
        add_tri(v, a, a + d1 * ra, a + d0 * ra, c0);
        add_tri(v, b, b + d0 * rb, b + d1 * rb, c0);
    }
}

void add_tube(std::vector<CV>& v, V3 a, V3 b, double ra, double rb, Col col)
{
    add_tube(v, a, b, ra, rb, col, col);
}

// A flat four-cornered panel — a wing, a tailplane, a fin — given its corners
// around one face, and thickened by `t` on both sides of it along `n`.
void add_slab(std::vector<CV>& v, V3 p0, V3 p1, V3 p2, V3 p3, V3 n, double t, Col col)
{
    const V3 o = n * t;
    const V3 a[4] = { p0 + o, p1 + o, p2 + o, p3 + o };
    const V3 b[4] = { p0 - o, p1 - o, p2 - o, p3 - o };
    add_quad(v, a[0], a[1], a[2], a[3], col);
    add_quad(v, b[3], b[2], b[1], b[0], col);
    for (int i = 0; i < 4; ++i) {
        const int j = (i + 1) % 4;
        add_quad(v, a[i], b[i], b[j], a[j], col);
    }
}

// An X quadcopter: a rounded body, four arms out on the diagonals, a motor at
// the end of each. The propellers are meshes of their own, turned as they are
// drawn — green the front pair, blue the rear, so which way it faces reads
// from any side.
constexpr double QUAD_ARM = 0.72;   // centre to motor, along each axis
void build_multicopter(std::vector<CV>& v)
{
    // The body half as large again as it was, the arms and propellers where
    // they were: the same span, a heavier centre. It stays clear of the
    // propeller discs, whose inner edges come no nearer than 0.66 of centre.
    add_ellipsoid(v, { 0.03, 0, 0.0 }, { 0.45, 0.30, 0.13 }, PORT, STBD);           // body
    add_ellipsoid(v, { 0.06, 0, -0.11 }, { 0.26, 0.17, 0.075 }, PORT, STBD);        // top shell
    for (int i = 0; i < 4; ++i) {
        const double x = (i & 1) ? QUAD_ARM : -QUAD_ARM, y = (i & 2) ? QUAD_ARM : -QUAD_ARM;
        const Col    c = y < 0 ? PORT : STBD;
        add_tube(v, { x * 0.3, y * 0.3, 0.0 }, { x, y, 0.0 }, 0.04, 0.03, c);      // arm
        add_tube(v, { x, y, 0.05 }, { x, y, -0.07 }, 0.055, 0.05, DARK);            // motor
        add_tube(v, { x, y, 0.05 }, { x, y, 0.16 }, 0.012, 0.012, DARK);            // leg
    }
}

// A propeller about its hub: two blades and the cap, in `col`. Drawn at each
// motor's top, turned about the body's z.
constexpr double QUAD_PROP_Z = -0.085;
void build_quad_prop(std::vector<CV>& v, Col col)
{
    add_tube(v, { 0, 0, 0.012 }, { 0, 0, -0.02 }, 0.03, 0.02, col);
    add_box(v, { 0, 0, 0 }, { 0.36, 0.045, 0.006 }, 0.0, col);
}

// A light single: a round fuselage running back into a tail cone, a glass
// canopy, straight tapered wings set with a little dihedral, a tapered
// tailplane, a swept fin, and a spinner on the nose. The propeller is a mesh
// of its own, turned as it is drawn. Port surfaces in the darker red.
void build_plane(std::vector<CV>& v)
{
    add_ellipsoid(v, { 0.22, 0, 0.0 },  { 0.58, 0.13, 0.14 }, PORT, STBD);              // fuselage
    add_tube(v, { -0.18, 0, -0.01 }, { -1.08, 0, -0.06 }, 0.125, 0.035, PORT, STBD);    // tail cone
    add_ellipsoid(v, { 0.30, 0, -0.11 }, { 0.22, 0.085, 0.075 }, DARK, DARK);           // canopy
    add_tube(v, { 0.78, 0, 0.0 }, { 0.90, 0, 0.0 }, 0.065, 0.012, DARK);                // spinner

    // Wings: root chord at the fuselage, a shorter chord at the tip, the tip
    // a little higher (z is down) — the dihedral.
    for (int side : { -1, 1 }) {
        const Col c = side < 0 ? PORT : STBD;
        add_slab(v, { 0.38, 0.10 * side, 0.02 },  { 0.30, 1.18 * side, -0.05 },
                    { 0.06, 1.18 * side, -0.05 }, { -0.06, 0.10 * side, 0.02 },
                 { 0, 0, 1 }, 0.018, c);                                                  // wing
        add_slab(v, { -0.80, 0.04 * side, -0.05 }, { -0.88, 0.44 * side, -0.05 },
                    { -1.00, 0.44 * side, -0.05 }, { -1.04, 0.04 * side, -0.05 },
                 { 0, 0, 1 }, 0.012, c);                                                  // tailplane
    }
    add_slab(v, { -0.78, 0, -0.08 }, { -0.98, 0, -0.40 },
                { -1.10, 0, -0.40 }, { -1.08, 0, -0.08 }, { 0, 1, 0 }, 0.012, STBD);     // fin
}

// The propeller about its hub: two blades. Drawn at PLANE_PROP_HUB, turned
// about the body's x.
const V3 PLANE_PROP_HUB = { 0.86, 0, 0.0 };
void build_plane_prop(std::vector<CV>& v)
{
    add_box(v, { 0, 0, 0 }, { 0.012, 0.32, 0.035 }, 0.0, DARK);
}

// A light helicopter: a round cabin with its canopy forward, a boom tapering
// back to a fin and a stabiliser, skids on struts, and the mast. Cabin and
// canopy alike in the reds, port darker. Both rotors are meshes of their
// own, turned as they are drawn.
void build_helicopter(std::vector<CV>& v)
{
    add_ellipsoid(v, { 0.12, 0, 0.0 },   { 0.56, 0.27, 0.26 }, PORT, STBD);    // cabin
    add_ellipsoid(v, { 0.44, 0, -0.04 }, { 0.24, 0.21, 0.18 }, PORT, STBD);    // canopy
    add_tube(v, { -0.30, 0, -0.06 }, { -1.36, 0, -0.14 }, 0.10, 0.035, PORT);   // tail boom
    add_box (v, { -1.06, 0, -0.12 }, { 0.06, 0.20, 0.012 }, 0.0, STBD);        // stabiliser
    add_box (v, { -1.34, 0, -0.27 }, { 0.09, 0.012, 0.15 }, 0.0, PORT);        // fin
    add_tube(v, { 0.12, 0, -0.22 },  { 0.12, 0, -0.36 }, 0.035, 0.03, DARK);   // mast
    for (int side : { -1, 1 }) {
        add_tube(v, { -0.38, 0.27 * side, 0.34 }, { 0.62, 0.27 * side, 0.34 }, 0.024, 0.024, DARK);   // skid
        add_tube(v, { 0.62, 0.27 * side, 0.34 },  { 0.70, 0.27 * side, 0.28 }, 0.024, 0.024, DARK);   // its upturned toe
        for (double x : { -0.12, 0.38 })
            add_tube(v, { x, 0.13 * side, 0.20 }, { x, 0.27 * side, 0.34 }, 0.018, 0.018, DARK);       // struts
    }
}

// The main rotor about its hub: four blades and the hub. Drawn at HELI_HUB,
// turned about the body's z by the rotor angle.
const V3 HELI_HUB = { 0.12, 0, -0.37 };
void build_heli_rotor(std::vector<CV>& v)
{
    add_tube(v, { 0, 0, 0.02 }, { 0, 0, -0.02 }, 0.06, 0.06, DARK);
    for (double yaw : { 0.0, M_PI / 2 })
        add_box(v, { 0, 0, 0 }, { 1.15, 0.045, 0.01 }, yaw, DARK);
}

// The tail rotor about its hub: two blades and the hub, on the fin's right.
// Drawn at HELI_TAIL_HUB, turned about the body's y.
const V3 HELI_TAIL_HUB = { -1.34, 0.04, -0.27 };
void build_heli_tail_rotor(std::vector<CV>& v)
{
    add_tube(v, { 0, -0.015, 0 }, { 0, 0.015, 0 }, 0.03, 0.03, DARK);
    add_box(v, { 0, 0, 0 }, { 0.20, 0.008, 0.03 }, 0.0, DARK);
}

// The 2D map's arrow, given thickness: a nose and two tail tips about a notch.
void build_arrow(std::vector<CV>& v)
{
    const double H = 0.08;
    const V3 nose = { 1.0, 0, 0 }, left = { -0.7, -0.6, 0 }, right = { -0.7, 0.6, 0 }, notch = { -0.35, 0, 0 };
    const auto up = [&](V3 p) { return V3{ p.x, p.y, p.z - H }; };
    const auto dn = [&](V3 p) { return V3{ p.x, p.y, p.z + H }; };
    add_tri(v, up(nose), up(notch), up(left), PORT);
    add_tri(v, up(nose), up(right), up(notch), STBD);
    add_tri(v, dn(nose), dn(left), dn(notch), PORT);
    add_tri(v, dn(nose), dn(notch), dn(right), STBD);
    add_quad(v, up(nose), up(left), dn(left), dn(nose), PORT);
    add_quad(v, up(left), up(notch), dn(notch), dn(left), PORT);
    add_quad(v, up(notch), up(right), dn(right), dn(notch), STBD);
    add_quad(v, up(right), up(nose), dn(nose), dn(right), STBD);
}

Mesh upload(const std::vector<CV>& v)
{
    Mesh m;
    m.n = (GLsizei)v.size();
    gl3::GenVertexArrays(1, &m.vao);
    gl3::BindVertexArray(m.vao);
    gl3::GenBuffers(1, &m.vbo);
    gl3::BindBuffer(gl3::ARRAY_BUFFER, m.vbo);
    gl3::BufferData(gl3::ARRAY_BUFFER, (gl3::Sizeiptr)(v.size() * sizeof(CV)), v.data(), gl3::STATIC_DRAW);
    gl3::VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(CV), (const void*)offsetof(CV, p));
    gl3::VertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(CV), (const void*)offsetof(CV, n));
    gl3::VertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(CV), (const void*)offsetof(CV, c));
    gl3::EnableVertexAttribArray(0);
    gl3::EnableVertexAttribArray(1);
    gl3::EnableVertexAttribArray(2);
    gl3::BindVertexArray(0);
    return m;
}

const char* FLAT_VS = R"(#version 330 core
layout(location = 0) in vec3 a_pos;
uniform mat4 u_vp;
void main() { gl_Position = u_vp * vec4(a_pos, 1.0); }
)";
const char* FLAT_FS = R"(#version 330 core
uniform vec4 u_col;
out vec4 o_color;
void main() { o_color = u_col; }
)";

bool init_flat()
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

bool init_craft()
{
    g.craft_prog = link(CRAFT_VS, CRAFT_FS);
    if (!g.craft_prog) return false;
#define U(field, name) g.field = gl3::GetUniformLocation(g.craft_prog, name)
    U(c_vp, "u_vp"); U(c_model, "u_model"); U(c_eye, "u_eye");
    U(c_fog, "u_fog"); U(c_fog_range, "u_fog_range"); U(c_dim, "u_dim");
    U(c_xray, "u_xray"); U(c_gplane, "u_gplane"); U(c_ggrad, "u_ggrad");
    U(c_outline, "u_outline"); U(c_px_per_dist, "u_px_per_dist"); U(c_outline_col, "u_outline_col");
#undef U
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

// ── Height textures ───────────────────────────────────────────────────────────

struct Key {
    int x, y, z;
    bool operator==(const Key& o) const { return x == o.x && y == o.y && z == o.z; }
};
struct KeyHash {
    size_t operator()(const Key& k) const noexcept {
        size_t h = (size_t)(unsigned)k.z;
        h = h * 2654435761u ^ (size_t)(unsigned)k.x;
        h = h * 2654435761u ^ (size_t)(unsigned)k.y;
        return h;
    }
};

struct Heights {
    GLuint   tex  = 0;
    int      tz   = -1;      // the terrain zoom it was read from
    float    hmin = 0, hmax = 0;
    uint64_t used = 0;       // frame last drawn
};
std::unordered_map<Key, Heights, KeyHash> s_heights;
uint64_t s_frame = 0;

// The tile's 33×33 vertex heights from terrain tiles at zoom tz, or false if
// any terrain tile it touches is not loaded yet (asking for those that are
// not). Each vertex reads the terrain at its own position, interpolated
// between pixel centres, so a vertex on the edge two tiles share comes out
// the same from both and they meet without a crack.
bool sample_heights(const Key& k, int tz, std::vector<float>& out)
{
    const double scale = std::ldexp(1.0, tz - k.z) * TERRAIN_TILE_PX;   // tz pixels per z tile
    const double gx0 = k.x * scale - 0.5, gy0 = k.y * scale - 0.5;
    const int    npx = TERRAIN_TILE_PX << tz;                           // pixels across the world

    const int tx0 = std::max(0, (int)std::floor(gx0) / TERRAIN_TILE_PX);
    const int ty0 = std::max(0, (int)std::floor(gy0) / TERRAIN_TILE_PX);
    const int tx1 = std::min((1 << tz) - 1, ((int)std::floor(gx0 + scale) + 1) / TERRAIN_TILE_PX);
    const int ty1 = std::min((1 << tz) - 1, ((int)std::floor(gy0 + scale) + 1) / TERRAIN_TILE_PX);
    const int tw  = tx1 - tx0 + 1;

    std::vector<std::shared_ptr<const TerrainTile>> tiles((size_t)tw * (ty1 - ty0 + 1));
    bool ready = true;
    for (int ty = ty0; ty <= ty1; ++ty)
        for (int tx = tx0; tx <= tx1; ++tx) {
            auto t = terrain_tile(tz, tx, ty);
            ready = ready && t;
            tiles[(size_t)(ty - ty0) * tw + (tx - tx0)] = std::move(t);
        }
    if (!ready) return false;

    const auto px = [&](int ix, int iy) {
        ix = std::clamp(ix, 0, npx - 1);
        iy = std::clamp(iy, 0, npx - 1);
        const int tx = std::clamp(ix / TERRAIN_TILE_PX, tx0, tx1);
        const int ty = std::clamp(iy / TERRAIN_TILE_PX, ty0, ty1);
        const TerrainTile& t = *tiles[(size_t)(ty - ty0) * tw + (tx - tx0)];
        return t.at(std::clamp(ix - tx * TERRAIN_TILE_PX, 0, TERRAIN_TILE_PX - 1),
                    std::clamp(iy - ty * TERRAIN_TILE_PX, 0, TERRAIN_TILE_PX - 1));
    };

    out.resize((size_t)GV * GV);
    for (int j = 0; j < GV; ++j)
        for (int i = 0; i < GV; ++i) {
            const double fx = gx0 + scale * i / GRID, fy = gy0 + scale * j / GRID;
            const int    x0 = (int)std::floor(fx),   y0 = (int)std::floor(fy);
            const float  ax = (float)(fx - x0),      ay = (float)(fy - y0);
            const float  top = px(x0, y0)     * (1 - ax) + px(x0 + 1, y0)     * ax;
            const float  bot = px(x0, y0 + 1) * (1 - ax) + px(x0 + 1, y0 + 1) * ax;
            out[(size_t)j * GV + i] = top * (1 - ay) + bot * ay;
        }
    return true;
}

// The tile's heights, made or improved within this frame's budget. Terrain at
// the tile's own zoom (capped at TERRAIN_Z) is asked for; until it arrives a
// coarser terrain zoom already loaded stands in, so ground appears at once
// and sharpens as the tiles come.
// Both caps matter while tiles load: a read that fails still takes locks, and
// a few hundred tiles each trying seven zooms every frame would add up.
struct Budget { int builds = BUILDS_PER_FRAME, tries = TRIES_PER_FRAME; };

const Heights* heights_for(const Key& k, Budget& budget)
{
    const int want = std::min(k.z, TERRAIN_Z);
    Heights&  h    = s_heights[k];
    h.used = s_frame;
    if (h.tz == want || budget.builds <= 0) return h.tex ? &h : nullptr;

    static std::vector<float> buf;
    int got = -1;
    for (int tz = want; tz >= std::max(0, want - 6) && tz > h.tz && budget.tries > 0; --tz) {
        --budget.tries;
        if (sample_heights(k, tz, buf)) { got = tz; break; }
    }
    if (got < 0) return h.tex ? &h : nullptr;
    --budget.builds;

    if (!h.tex) glGenTextures(1, &h.tex);
    glBindTexture(GL_TEXTURE_2D, h.tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, gl3::CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, gl3::CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, gl3::R32F, GV, GV, 0, gl3::RED, GL_FLOAT, buf.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    h.tz   = got;
    h.hmin = *std::min_element(buf.begin(), buf.end());
    h.hmax = *std::max_element(buf.begin(), buf.end());
    return &h;
}

// Drop every height texture, when the terrain source changes under them.
void forget_heights()
{
    for (auto& [k, h] : s_heights)
        if (h.tex) glDeleteTextures(1, &h.tex);
    s_heights.clear();
}

// Drop height textures no tile has drawn for a while.
void evict_heights()
{
    if (s_frame % 120 != 0) return;
    for (auto it = s_heights.begin(); it != s_heights.end();) {
        if (s_frame - it->second.used > 600) {
            if (it->second.tex) glDeleteTextures(1, &it->second.tex);
            it = s_heights.erase(it);
        } else {
            ++it;
        }
    }
}

// ── The frame ─────────────────────────────────────────────────────────────────

// Everything one frame's drawing needs to place a point.
struct Frame {
    double u0, v0;       // the centre, as a Mercator fraction
    double m;            // metres per unit of Mercator fraction at the centre
    double href;         // ground AMSL under the centre
    double exag;
    M4     vp;
    ImVec2 p0, size;

    V3 world(double lat, double lon, double amsl) const
    {
        double u, v;
        merc(lat, lon, u, v);
        return { (u - u0) * m, -(v - v0) * m, (amsl - href) * exag };
    }

    // Screen position of a world point, or false behind the camera.
    bool screen(V3 p, ImVec2& out) const
    {
        const V4 c = xform(vp, p);
        if (c.w < 1e-3) return false;
        out = { p0.x + (float)((c.x / c.w * 0.5 + 0.5) * size.x),
                p0.y + (float)((0.5 - c.y / c.w * 0.5) * size.y) };
        return true;
    }

    // A segment's on-screen part, cut at the camera plane.
    bool segment(V3 a, V3 b, ImVec2& sa, ImVec2& sb) const
    {
        const V4 ca = xform(vp, a), cb = xform(vp, b);
        constexpr double W = 1e-2;
        if (ca.w < W && cb.w < W) return false;
        if (ca.w < W) a = a + (b - a) * ((W - ca.w) / (cb.w - ca.w));
        if (cb.w < W) b = b + (a - b) * ((W - cb.w) / (ca.w - cb.w));
        return screen(a, sa) && screen(b, sb);
    }
};

// ── Aircraft pose ─────────────────────────────────────────────────────────────

// The body axes — forward, right, down — in the world frame, from ATTITUDE's
// roll, pitch and yaw (degrees). The rotation is the usual aerospace one,
// yaw then pitch then roll, worked in north/east/down and turned into the
// world's east/north/up.
struct Basis { V3 x, y, z; };

Basis body_axes(double roll_deg, double pitch_deg, double yaw_deg)
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

// Where an aircraft is drawn and how: its position, its axes, and the scale
// that keeps it the same size on screen at any distance, as the 2D symbol is.
struct Pose {
    V3                   p;
    Basis                b;
    double               s;
    std::optional<float> ground;   // terrain AMSL under it
};

constexpr double CRAFT_PX = 30.0;   // screen pixels per body unit

// How large each airframe is drawn against that. The helicopter and the
// plane are drawn alike — the two are about the same length and span — and
// the quadcopter a fifth larger than the arrow, all three so their parts read.
double craft_size(SymbolKind kind)
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
M4 model_matrix(const Basis& b, double s, V3 p)
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
std::unordered_map<VehicleId, RotorAngles, VehicleIdHash> s_rotor;

std::optional<double> ground_z(const Frame& f, double ex, double ey);

// The ground under an aircraft as a plane: its height under the centre and
// its slope there, from samples a model's reach either side — enough to tell
// which parts of the model are in the hill. nullopt while the terrain loads.
struct GroundPlane { double x, y, z, gx, gy, zmax; };
std::optional<GroundPlane> ground_plane(const Frame& f, V3 p, double reach)
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

        // A thin amber glow round it, so it stands off the imagery: the model
        // pushed out a few pixels and drawn in amber behind itself — a soft
        // wide shell, then a firmer narrow one — writing no depth, so the
        // aircraft drawn next covers all but the rim. The terrain still hides
        // it, as it hides the aircraft.
        {
            const ImVec4 am = map_amber();
            const float  A  = mv.active ? 1.0f : 0.55f;
            gl3::Uniform1i(g.c_xray, 0);
            gl3::Uniform1f(g.c_px_per_dist, (float)(2.0 * std::tan(FOV_Y * 0.5) / std::max(1.0f, f.size.y)));
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE);
            gl3::Uniform1f(g.c_outline, 3.5f);
            gl3::Uniform4f(g.c_outline_col, am.x, am.y, am.z, 0.28f * A);
            draw_parts();
            gl3::Uniform1f(g.c_outline, 1.6f);
            gl3::Uniform4f(g.c_outline_col, am.x, am.y, am.z, 0.85f * A);
            draw_parts();
            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);
            gl3::Uniform1f(g.c_outline, 0.0f);
        }

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
}

// The ground's world height under a world point, once its terrain is loaded.
std::optional<double> ground_z(const Frame& f, double ex, double ey)
{
    double lat, lon;
    unmerc(f.u0 + ex / f.m, f.v0 - ey / f.m, lat, lon);
    if (auto g = terrain_amsl(lat, lon)) return (*g - f.href) * f.exag;
    return std::nullopt;
}

struct Leaf { Key k; };

// Choose the tiles to draw: a quadtree from tiles about the view range across,
// split while the camera is near, dropping tiles past the range or outside
// the frustum.
void select_tiles(const Frame& f, V3 eye, double range, std::vector<Leaf>& leaves)
{
    const double size0 = f.m;   // world metres across a z = 0 tile
    int zr = (int)std::floor(std::log2(size0 / range));
    zr = std::clamp(zr, 1, ZMAX);
    const int n = 1 << zr;
    const double span = range / size0 * n;   // range in tiles at zr
    const int x0 = std::max(0, (int)std::floor(f.u0 * n - span)), x1 = std::min(n - 1, (int)std::floor(f.u0 * n + span));
    const int y0 = std::max(0, (int)std::floor(f.v0 * n - span)), y1 = std::min(n - 1, (int)std::floor(f.v0 * n + span));

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
        if (k.z < ZMAX && d < SPLIT_K * s) {
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
GLuint imagery_for(const Key& k, float uv[4], std::vector<MapTileId>& wanted)
{
    uv[0] = 0; uv[1] = 0; uv[2] = 1; uv[3] = 1;
    if (GLuint t = map_tile_texture(k.z, k.x, k.y, true)) return t;
    wanted.push_back({ k.z, k.x, k.y });
    for (int dz = 1; dz <= 6 && k.z - dz >= 0; ++dz) {
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

// Upright see-through bands standing on the terrain, each layer in a tint of
// its own: the mission's flight path cast onto the ground (blue), and the
// geofence walls (green inclusion, red exclusion). `fill` the triangles of
// the band, `lines` its edges as segments; world positions, three floats a
// vertex.
struct TintLayer {
    std::vector<float> fill, lines;
    float r, g, b;      // the tint; the fill faint, the edges strong
};
struct Projection { std::vector<TintLayer> layers; };

void push_v(std::vector<float>& v, V3 p) { v.insert(v.end(), { (float)p.x, (float)p.y, (float)p.z }); }

// A band from `bot` (along the ground) up to `top`, one sample further on: the
// quad between the last pair and this, and the edge segments it adds.
void band_step(TintLayer& L, V3 bot0, V3 top0, V3 bot, V3 top, bool top_edge)
{
    push_v(L.fill, top0); push_v(L.fill, bot0); push_v(L.fill, bot);
    push_v(L.fill, top0); push_v(L.fill, bot);  push_v(L.fill, top);
    push_v(L.lines, bot0); push_v(L.lines, bot);
    if (top_edge) { push_v(L.lines, top0); push_v(L.lines, top); }
}

void render_projection(const Frame& f, const Projection& pr)
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

// ── Overlays ──────────────────────────────────────────────────────────────────

// Lines dropped to the terrain are the map's amber, every one of them —
// vehicle, waypoint, or the one being placed.

// A line from a point down to the ground under it, and a small dotted ring
// lying on the ground where it lands — drawn in the ground's plane, so in
// perspective it is an ellipse, and kept a few pixels across at any distance.
void drop_line(ImDrawList* dl, const Frame& f, V3 eye, V3 top, double ground_amsl, float alpha = 1.0f)
{
    const ImU32 col  = map_amber_u32(0.92f * alpha);
    const V3    foot = { top.x, top.y, (ground_amsl - f.href) * f.exag };
    ImVec2 a, b;
    if (f.segment(top, foot, a, b)) dl->AddLine(a, b, col, 1.5f);

    const V3     d = foot - eye;
    const double r = 7.0 * 2.0 * std::sqrt(dot(d, d)) * std::tan(FOV_Y * 0.5) / std::max(1.0f, f.size.y);
    constexpr int SEG = 16;   // drawn every other one: dotted
    for (int i = 0; i < SEG; i += 2) {
        const double a0 = 2 * M_PI * i / SEG, a1 = 2 * M_PI * (i + 1) / SEG;
        ImVec2 p, q;
        if (f.segment({ foot.x + r * std::cos(a0), foot.y + r * std::sin(a0), foot.z },
                      { foot.x + r * std::cos(a1), foot.y + r * std::sin(a1), foot.z }, p, q))
            dl->AddLine(p, q, col, 1.5f);
    }
}

void draw_trails(ImDrawList* dl, const Frame& f, const std::vector<MapVehicle>& fleet)
{
    static std::vector<ImVec2> run;
    for (const MapVehicle& mv : fleet) {
        const auto* pts = map_track_of(mv.id);
        if (!pts || pts->size() < 2) continue;
        const ImU32 col = ui_col(g_theme.map_track, mv.active ? 1.0f : 0.40f);
        const float w   = mv.active ? 2.5f : 1.5f;
        run.clear();
        const auto flush = [&] {
            if (run.size() >= 2) dl->AddPolyline(run.data(), (int)run.size(), col, ImDrawFlags_None, w);
            run.clear();
        };
        for (const TrackPoint& tp : *pts) {
            ImVec2 sp;
            if (!f.screen(f.world(tp.lat, tp.lon, tp.alt_amsl), sp)) { flush(); continue; }
            if (!run.empty() && std::fabs(sp.x - run.back().x) < 2.0f &&
                std::fabs(sp.y - run.back().y) < 2.0f) continue;
            run.push_back(sp);
        }
        flush();
    }
}

// Home's AMSL for the relative frames: the vehicle's, else the ground under
// its home, else under the first placed item — a plan made with no vehicle
// connected still stands on the ground it starts from.
std::optional<float> plan_home_amsl(const MapVehicle* active, const std::vector<MissionItem>* mission)
{
    std::optional<float> reported;
    if (active && active->has_home_alt) reported = active->home_alt_amsl;
    const bool pos = active && active->has_home;
    return plan_home_amsl(reported, pos ? &active->home_lat : nullptr,
                          pos ? &active->home_lon : nullptr, mission);
}

// A path checked against the terrain: drawn over in amber where it comes
// nearer the ground than the clearance set in settings, red where it goes
// into it, with a mark at its closest point saying by how much. Nothing for
// a path that clears.
void draw_clearance(ImDrawList* dl, const Frame& f, V3 a, V3 b, const Clearance& c)
{
    if (!c.too_close()) return;
    const ImU32 col = c.collides() ? ui_col(g_theme.col_error) : ui_col(g_theme.col_warning);
    ImVec2 sa, sb;
    if (f.segment(a, b, sa, sb)) dl->AddLine(sa, sb, col, 3.5f);
    ImVec2 sp;
    if (!f.screen(f.world(c.lat, c.lon, c.path_amsl), sp)) return;
    dl->AddCircleFilled(sp, 5.0f, col);
    dl->AddCircle(sp, 5.0f, IM_COL32(0, 0, 0, 200), 0, 1.5f);
    char t[40];
    if (c.collides()) snprintf(t, sizeof(t), "HITS TERRAIN %.0f m", (double)-c.min_m);
    else              snprintf(t, sizeof(t), "%.0f m TO TERRAIN", (double)c.min_m);
    const ImVec2 ts = ImGui::CalcTextSize(t);
    const ImVec2 tp = { sp.x + 8.0f, sp.y - ts.y - 4.0f };
    dl->AddRectFilled({ tp.x - 3, tp.y - 1 }, { tp.x + ts.x + 3, tp.y + ts.y + 1 }, IM_COL32(0, 0, 0, 180), 3.0f);
    dl->AddText(tp, col, t);
}

void draw_mission(ImDrawList* dl, const Frame& f, V3 eye, const std::vector<MissionItem>& mission,
                  const MapVehicle* active)
{
    const std::optional<float> home = plan_home_amsl(active, &mission);

    struct Placed { V3 p; ImVec2 sp; bool on_screen; float ground; bool below; uint16_t seq; bool current; };
    std::vector<Placed> placed;
    for (const MissionItem& it : mission) {
        if (it.lat == 0.0 && it.lon == 0.0) continue;
        if (!mission_frame_is_global(it.frame)) continue;
        const std::optional<float> ground = terrain_amsl(it.lat, it.lon);
        if (!ground) continue;
        const float amsl = mission_item_amsl(it, home, ground).value_or(*ground);
        Placed pl{ f.world(it.lat, it.lon, amsl), {}, false, *ground, amsl < *ground, it.seq, it.is_current };
        pl.on_screen = f.screen(pl.p, pl.sp);
        placed.push_back(pl);
    }

    const ImU32 path = IM_COL32(255, 200, 50, 200);
    for (size_t i = 1; i < placed.size(); ++i) {
        ImVec2 a, b;
        if (f.segment(placed[i - 1].p, placed[i].p, a, b)) dl->AddLine(a, b, path, 2.0f);
    }
    for (const Placed& pl : placed)
        drop_line(dl, f, eye, pl.p, pl.ground, 0.8f);

    // Legs too near the ground, over the plain path.
    const auto world_of = [&](const MissionItem& it) -> std::optional<V3> {
        const auto g = terrain_amsl(it.lat, it.lon);
        const auto h = mission_item_amsl(it, home, g);
        if (!h) return std::nullopt;
        return f.world(it.lat, it.lon, *h);
    };
    for (const MissionLeg& lg : mission_legs(mission, home)) {
        const auto a = world_of(mission[lg.from]), b = world_of(mission[lg.to]);
        if (a && b) draw_clearance(dl, f, *a, *b, lg.c);
    }

    constexpr float R = 8.0f;
    for (const Placed& pl : placed) {
        if (!pl.on_screen) continue;
        const ImU32 fill = pl.below   ? ui_col(g_theme.col_error, 0.95f)
                         : pl.current ? IM_COL32(255, 100, 50, 220)
                                      : IM_COL32( 50, 150, 255, 200);
        dl->AddCircleFilled(pl.sp, R, fill);
        dl->AddCircle(pl.sp, R, IM_COL32(255, 255, 255, 220), 0, 1.5f);
        char s[8];
        snprintf(s, sizeof(s), "%u", (unsigned)pl.seq);
        const ImVec2 ts = ImGui::CalcTextSize(s);
        dl->AddText({ pl.sp.x - ts.x * 0.5f, pl.sp.y - ts.y * 0.5f }, IM_COL32(255, 255, 255, 255), s);
    }
}

// The forward axis as a gizmo: X, in amber — the airframes are red, and a
// red axis was lost against them — reaching past the nose and lettered at its
// end. Pitch and heading read off it from any angle, and it shows through
// terrain, which the mesh does not.
void draw_gizmo(ImDrawList* dl, const Frame& f, const Pose& ps, float alpha)
{
    constexpr double L = 1.9;   // body units
    const ImU32 col = map_amber_u32(alpha);
    ImVec2 a, b;
    if (!f.segment(ps.p, ps.p + ps.b.x * (L * ps.s), a, b)) return;
    dl->AddLine(a, b, col, 2.0f);
    dl->AddCircleFilled(b, 2.5f, col);
    dl->AddText({ b.x + 3.0f, b.y - 7.0f }, col, "X");
}

void draw_vehicles(ImDrawList* dl, const Frame& f, V3 eye, const std::vector<MapVehicle>& fleet)
{
    const auto one = [&](const MapVehicle& mv) {
        const float A = mv.active ? 1.0f : 0.55f;
        if (mv.has_home) {
            const std::optional<float> g = terrain_amsl(mv.home_lat, mv.home_lon);
            const double h = g ? *g : mv.has_home_alt ? mv.home_alt_amsl : f.href;
            ImVec2 sp;
            if (f.screen(f.world(mv.home_lat, mv.home_lon, h), sp))
                map_draw_home_symbol(dl, sp, mv.active);
        }
        if (!mv.has_pos) return;
        const Pose ps = vehicle_pose(f, eye, mv);
        if (ps.ground) drop_line(dl, f, eye, ps.p, *ps.ground, A);
        if (mv.has_att) draw_gizmo(dl, f, ps, A);
        ImVec2 sp;
        if (!f.screen(ps.p, sp)) return;
        if (mv.checked)   // in the SWARM view's command set, as the 2D ring says
            dl->AddCircle(sp, 30.0f, map_amber_u32(), 0, 2.0f);
        char lbl[32];
        snprintf(lbl, sizeof(lbl), "(%u) SYS %u", mv.number, (unsigned)mv.sysid);
        const ImVec2 ts = ImGui::CalcTextSize(lbl);
        dl->AddText({ sp.x - ts.x * 0.5f, sp.y + 30.0f }, map_amber_u32(A), lbl);

        // Under the name, its height above the terrain data — red, and saying
        // so, when the aircraft is inside it: a vehicle under the ground is
        // otherwise only its gizmo and this label, the mesh hidden in the hill.
        if (mv.has_alt && ps.ground) {
            const float agl = mv.alt_amsl - *ps.ground;
            char t[40];
            if (agl < 0.0f) snprintf(t, sizeof(t), "BELOW TERRAIN %.0f m", (double)-agl);
            else            snprintf(t, sizeof(t), "%.0f m AGL", (double)agl);
            const ImVec2 tsz = ImGui::CalcTextSize(t);
            const ImVec2 tp  = { sp.x - tsz.x * 0.5f, sp.y + 30.0f + ts.y + 2.0f };
            if (agl < 0.0f)
                dl->AddRectFilled({ tp.x - 3, tp.y - 1 }, { tp.x + tsz.x + 3, tp.y + tsz.y + 1 },
                                  IM_COL32(0, 0, 0, 170), 3.0f);
            dl->AddText(tp, agl < 0.0f ? ui_col(g_theme.col_error, A) : map_amber_u32(0.8f * A), t);
        }
    };
    for (const MapVehicle& mv : fleet) if (!mv.active) one(mv);
    for (const MapVehicle& mv : fleet) if ( mv.active) one(mv);
}

// ── Placing a mission item ────────────────────────────────────────────────────
//
// Two clicks, as the mission tab's LOC? asks for one. The first is on the
// ground: the new waypoint floats above the pointer at the height it would
// get, with its line dropped to where the pointer is, so what is being chosen
// is the spot under it. The second sets the height: the waypoint stays over
// that spot and rides its vertical line to wherever the pointer is nearest.
// A click is a press and release that barely moved — a drag still orbits, in
// either step, so the view can be turned to judge the height.

constexpr float DEFAULT_REL_ALT = 20.0f;   // as the mission tab's DEFAULT_ALT_M

// One 3D point being picked: where on the ground, then how high over it.
struct PointPick {
    bool   active    = false;
    bool   alt_phase = false;
    double lat = 0, lon = 0;      // fixed by the ground click
    double ground    = 0;         // AMSL under it
    double amsl      = 0;         // the point's height, AMSL
};

// A point by position and height AMSL.
struct Geo { double lat, lon, amsl; };

// What differs between the pickers: the button that clicks, what the banner
// calls the point, how its marker looks, where it starts and what its faint
// leg comes from.
struct PickStyle {
    ImGuiMouseButton  button;
    const char*       what;         // "the new WAYPOINT"
    const char*       tag;          // drawn in the marker
    ImU32             fill, text;
    double            start_rel;    // height above home the preview starts at
    std::optional<Geo> from;        // a faint leg from here to the point, checked against the terrain
    bool              tagged = false;   // its readout in the instruction's green box
};

PointPick s_place;                  // the mission tab's LOC?, left button
uint16_t  s_place_cmd = 0;          // the command it is for
PointPick s_goto;                   // GO HERE, right button

// The GO HERE point once its height is clicked, until map_view takes it.
bool   s_goto_ready = false;
double s_goto_lat = 0, s_goto_lon = 0;
float  s_goto_rel = 0;

bool s_press_in_view[2] = {};       // the left / right press began over the view

// A click of `b` on the view: pressed over it and released barely moved — so
// a drag with the same button still orbits or pans.
bool view_click(ImGuiMouseButton b, bool hovered)
{
    return hovered && s_press_in_view[b] && ImGui::IsMouseReleased(b) &&
           ImGui::GetIO().MouseDragMaxDistanceSqr[b] < 16.0f;
}

// A ray from the camera through the pointer.
V3 pointer_ray(const Frame& f, V3 eye, V3 target, ImVec2 mouse)
{
    const V3     fwd = norm(target - eye);
    const V3     rt  = norm(cross(fwd, { 0.0, 0.0, 1.0 }));
    const V3     up  = cross(rt, fwd);
    const double th  = std::tan(FOV_Y * 0.5);
    const double nx  = (mouse.x - f.p0.x) / f.size.x * 2.0 - 1.0;
    const double ny  = 1.0 - (mouse.y - f.p0.y) / f.size.y * 2.0;
    return norm(fwd + rt * (nx * th * f.size.x / f.size.y) + up * (ny * th));
}

// Where the ray first meets the ground: marched out in steps that grow with
// distance, then narrowed by halving the last step.
std::optional<V3> ray_ground(const Frame& f, V3 o, V3 d, double max_t)
{
    const auto under = [&](double t) {
        const V3   p  = o + d * t;
        const auto gz = ground_z(f, p.x, p.y);
        return gz && p.z <= *gz;
    };
    double prev = 0.0, t = 1.0;
    for (int i = 0; i < 800 && t < max_t; ++i) {
        if (under(t)) {
            double lo = prev, hi = t;
            for (int k = 0; k < 14; ++k) {
                const double mid = 0.5 * (lo + hi);
                (under(mid) ? hi : lo) = mid;
            }
            return o + d * hi;
        }
        prev = t;
        t   += std::max(2.0, t * 0.015);
    }
    return std::nullopt;
}

// The height on the vertical line through (x, y) nearest the ray, or nothing
// when the ray runs along the line (looking straight down) or meets it
// behind the camera. Closest points of two lines.
std::optional<double> vertical_z_nearest(V3 o, V3 d, double x, double y)
{
    const V3     w0  = { x - o.x, y - o.y, -o.z };
    const double b   = d.z, e = dot(d, w0);
    const double den = 1.0 - b * b;
    if (den < 0.02) return std::nullopt;
    if ((e - b * w0.z) / den <= 0.0) return std::nullopt;
    return (b * e - w0.z) / den;
}

// The relative altitude the new item starts at: the last placed item's
// height above home, as the mission tab would carry it on; else the default.
double starting_rel_alt(const std::vector<MissionItem>* mission, std::optional<float> home)
{
    if (mission && home)
        for (auto it = mission->rbegin(); it != mission->rend(); ++it) {
            if (it->lat == 0.0 && it->lon == 0.0) continue;
            if (!mission_frame_is_global(it->frame)) continue;
            if (auto a = mission_item_amsl(*it, home, terrain_amsl(it->lat, it->lon)))
                return *a - *home;
        }
    return DEFAULT_REL_ALT;
}

// The instruction for the step being picked: the same box, in the same
// place, as the 2D map's.
void draw_banner(const Frame& f, const char* text)
{
    map_draw_instruction(f.p0, f.size, text);
}

// Advance a pick by this frame's pointer and draw it: the point, its line to
// the ground, the readout, the banner. True on the click that sets the
// height; the point is then in `pp` for the caller to take.
bool run_point_pick(PointPick& pp, ImDrawList* dl, const Frame& f, V3 eye, V3 target, double range,
                    bool hovered, std::optional<float> home, const PickStyle& st)
{
    const V3   ray   = pointer_ray(f, eye, target, ImGui::GetIO().MousePos);
    const bool click = view_click(st.button, hovered);
    bool steep = false;

    if (!pp.alt_phase) {
        if (!hovered) return false;
        const auto hit = ray_ground(f, eye, ray, range);
        if (!hit) return false;
        unmerc(f.u0 + hit->x / f.m, f.v0 - hit->y / f.m, pp.lat, pp.lon);
        pp.ground = hit->z / f.exag + f.href;
        // Where it would start, but never in the hill: over rising ground the
        // line has to reach down to the spot chosen.
        pp.amsl   = std::max((home ? *home : pp.ground) + st.start_rel, pp.ground + DEFAULT_REL_ALT);
        if (click) pp.alt_phase = true;
    } else {
        const V3 foot = f.world(pp.lat, pp.lon, pp.ground);
        if (hovered) {
            if (auto z = vertical_z_nearest(eye, ray, foot.x, foot.y))
                pp.amsl = std::clamp(*z / f.exag + f.href, pp.ground, pp.ground + 3000.0);
            else
                steep = true;
        }
        if (click) return true;
    }

    const V3 top = f.world(pp.lat, pp.lon, pp.amsl);
    ImVec2 a, b;
    std::optional<Clearance> leg;
    if (st.from) {
        const V3 fw = f.world(st.from->lat, st.from->lon, st.from->amsl);
        if (f.segment(fw, top, a, b)) dl->AddLine(a, b, (st.fill & 0x00FFFFFFu) | 0x78000000u, 1.5f);
        leg = path_clearance(st.from->lat, st.from->lon, st.from->amsl, pp.lat, pp.lon, pp.amsl);
        draw_clearance(dl, f, fw, top, *leg);
    }
    drop_line(dl, f, eye, top, pp.ground);
    ImVec2 sp;
    if (f.screen(top, sp)) {
        const ImVec2 ts = ImGui::CalcTextSize(st.tag);
        if (st.tagged) {
            // The marker as the readout beside it: dark, tinted green, a green
            // ring, green letters.
            const ImVec4 ok = col_ok();
            const float  r  = std::max(9.0f, ts.x * 0.5f + 3.0f);
            dl->AddCircleFilled(sp, r, ui_col(g_theme.bg_topbar, 0.92f));
            dl->AddCircleFilled(sp, r, ImGui::GetColorU32({ ok.x, ok.y, ok.z, 0.14f }));
            dl->AddCircle(sp, r, ImGui::GetColorU32(ok), 0, 1.5f);
            dl->AddText({ sp.x - ts.x * 0.5f, sp.y - ts.y * 0.5f }, ImGui::GetColorU32(ok), st.tag);
        } else {
            dl->AddCircleFilled(sp, 9.0f, st.fill);
            dl->AddCircle(sp, 9.0f, IM_COL32(255, 255, 255, 220), 0, 1.5f);
            dl->AddText({ sp.x - ts.x * 0.5f, sp.y - ts.y * 0.5f }, st.text, st.tag);
        }

        char ro[48];
        const double base = home ? *home : pp.ground;
        snprintf(ro, sizeof(ro), "%.0f m  \xc2\xb7  %.0f AGL", pp.amsl - base, pp.amsl - pp.ground);
        if (st.tagged) {
            map_draw_tag(dl, { sp.x + 14.0f, sp.y }, ro);
        } else {
            const ImVec2 rs = ImGui::CalcTextSize(ro);
            const ImVec2 rp = { sp.x + 14.0f, sp.y - rs.y * 0.5f };
            dl->AddRectFilled({ rp.x - 3, rp.y - 1 }, { rp.x + rs.x + 3, rp.y + rs.y + 1 }, IM_COL32(0, 0, 0, 180), 3.0f);
            dl->AddText(rp, IM_COL32(255, 230, 60, 255), ro);
        }
    }

    const char* btn = st.button == ImGuiMouseButton_Right ? "Right-click" : "Click";
    char hint[128];
    if (!pp.alt_phase)
        snprintf(hint, sizeof(hint), "  %s the ground under %s  \xe2\x80\x94  Esc to cancel  ", btn, st.what);
    else if (steep)
        snprintf(hint, sizeof(hint), "  Tilt the view to set the height  \xe2\x80\x94  Esc to cancel  ");
    else
        snprintf(hint, sizeof(hint), "  Move up or down for its height, %s to set  \xe2\x80\x94  Esc to cancel  ",
                 st.button == ImGuiMouseButton_Right ? "right-click" : "click");
    draw_banner(f, hint);
    return false;
}

// The mission tab's LOC?, in 3D: the new item's place and height.
void place_item(ImDrawList* dl, const Frame& f, V3 eye, V3 target, double range, bool hovered,
                MissionPickState& pick, const std::vector<MissionItem>* mission,
                const MapVehicle* active)
{
    if (s_place_cmd != pick.add_command) {
        s_place     = {};
        s_place_cmd = pick.add_command;
    }
    const std::optional<float> home = plan_home_amsl(active, mission);

    // The faint leg comes from the last placed item.
    std::optional<Geo> from;
    if (mission && home)
        for (auto it = mission->rbegin(); it != mission->rend(); ++it) {
            if (it->lat == 0.0 && it->lon == 0.0) continue;
            if (!mission_frame_is_global(it->frame)) continue;
            if (const auto g = terrain_amsl(it->lat, it->lon))
                from = Geo{ it->lat, it->lon, mission_item_amsl(*it, home, g).value_or(*g) };
            break;
        }

    char what[48], tag[8];
    snprintf(what, sizeof(what), "the new %s", pick.add_label ? pick.add_label : "item");
    snprintf(tag, sizeof(tag), "%d", mission ? (int)mission->size() : 0);
    const PickStyle st{ ImGuiMouseButton_Left, what, tag, IM_COL32(255, 240, 0, 240),
                        IM_COL32(0, 0, 0, 255), starting_rel_alt(mission, home), from };

    if (run_point_pick(s_place, dl, f, eye, target, range, hovered, home, st)) {
        const double base = home ? *home : s_place.ground;
        pick.picked_lat     = s_place.lat;
        pick.picked_lon     = s_place.lon;
        pick.picked_alt     = (float)std::round(s_place.amsl - base);
        pick.has_picked_alt = true;
        pick.pick_done      = true;
        s_place = {};
    }
}

// GO HERE, in 3D: a right-click starts it, a right-click on the ground fixes
// where, a right-click sets the height — and the map opens its GO HERE menu
// there, the height filled in above home. Starts at the vehicle's own height,
// as the 2D menu does: "go there, stay at this altitude".
void pick_goto(ImDrawList* dl, const Frame& f, V3 eye, V3 target, double range, bool hovered,
               const GotoTargetState& go, const std::vector<MissionItem>* mission,
               const MapVehicle* active)
{
    if (!s_goto.active) {
        if (view_click(ImGuiMouseButton_Right, hovered)) {
            s_goto = {};
            s_goto.active = true;   // the click that starts it places nothing
        }
        return;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) { s_goto = {}; return; }

    const std::optional<float> home = plan_home_amsl(active, mission);
    std::optional<Geo> from;
    if (active && active->has_pos && active->has_alt)
        from = Geo{ active->lat, active->lon, active->alt_amsl };
    const PickStyle st{ ImGuiMouseButton_Right, "the GO HERE target", "GO", map_amber_u32(),
                        IM_COL32(0, 0, 0, 255), go.current_alt_m, from, true };

    if (run_point_pick(s_goto, dl, f, eye, target, range, hovered, home, st)) {
        const double base = home ? *home : s_goto.ground;
        s_goto_lat   = s_goto.lat;
        s_goto_lon   = s_goto.lon;
        s_goto_rel   = (float)std::round(s_goto.amsl - base);
        s_goto_ready = true;
        s_goto = {};
    }
}

// The standing GO HERE target, as on the 2D map: a ringed cross at the height
// it was sent with, its line to the ground, and a line from the vehicle.
void draw_goto_target(ImDrawList* dl, const Frame& f, V3 eye, const GotoTargetState& go,
                      const std::vector<MissionItem>* mission, const MapVehicle* active)
{
    const std::optional<float> ground = terrain_amsl(go.target_lat, go.target_lon);
    const std::optional<float> home   = plan_home_amsl(active, mission);
    if (!ground) return;
    const V3 p = f.world(go.target_lat, go.target_lon, (home ? *home : *ground) + go.target_alt_m);
    const ImU32 col = map_amber_u32();
    ImVec2 a, b;
    if (active && active->has_pos && active->has_alt &&
        f.segment(f.world(active->lat, active->lon, active->alt_amsl), p, a, b))
        dl->AddLine(a, b, map_amber_u32(0.5f), 1.5f);
    drop_line(dl, f, eye, p, *ground);
    ImVec2 tp;
    if (!f.screen(p, tp)) return;
    constexpr float R = 10.0f;
    dl->AddCircle(tp, R,        col, 0, 2.0f);
    dl->AddCircle(tp, R * 0.35f, col, 0, 2.0f);
    dl->AddLine({ tp.x - R - 5.0f, tp.y }, { tp.x - R + 2.0f, tp.y }, col, 2.0f);
    dl->AddLine({ tp.x + R - 2.0f, tp.y }, { tp.x + R + 5.0f, tp.y }, col, 2.0f);
    dl->AddLine({ tp.x, tp.y - R - 5.0f }, { tp.x, tp.y - R + 2.0f }, col, 2.0f);
    dl->AddLine({ tp.x, tp.y + R - 2.0f }, { tp.x, tp.y + R + 5.0f }, col, 2.0f);
    char t[32];
    snprintf(t, sizeof(t), "GOTO  %.0f M", (double)go.target_alt_m);
    map_draw_tag(dl, { tp.x + R + 8.0f, tp.y }, t);
}

// ── Mission ground projection ─────────────────────────────────────────────────
//
// The planned path cast straight down onto the terrain: a line along the
// ground under every leg, and the upright band between it and the path tinted
// a faint blue, so how high each leg runs over the ground — and where the
// ground rises to meet it — reads from any angle. Sampled along each leg at
// the terrain's own spacing, the way the clearance check reads it.

void build_projection(const Frame& f, V3 eye, const std::vector<MissionItem>& mission,
                      const MapVehicle* active, Projection& out)
{
    const std::optional<float> home = plan_home_amsl(active, &mission);
    const auto terrain_frame = [](uint8_t fr) {
        return fr == MAV_FRAME_GLOBAL_TERRAIN_ALT || fr == MAV_FRAME_GLOBAL_TERRAIN_ALT_INT;
    };
    TintLayer& L = out.layers.emplace_back();
    L.r = 0.40f; L.g = 0.65f; L.b = 1.00f;   // blue
    constexpr double STEP_M = 25.0;
    constexpr int    MAX_SAMPLES = 120;

    int prev = -1;
    for (int i = 0; i < (int)mission.size(); ++i) {
        const MissionItem& it = mission[i];
        if (it.lat == 0.0 && it.lon == 0.0) continue;
        if (!mission_frame_is_global(it.frame)) continue;
        if (prev < 0) { prev = i; continue; }
        const MissionItem& a = mission[prev];
        prev = i;

        const auto ga = terrain_amsl(a.lat, a.lon), gb = terrain_amsl(it.lat, it.lon);
        const auto ha = mission_item_amsl(a, home, ga), hb = mission_item_amsl(it, home, gb);
        const bool follow = terrain_frame(a.frame) && terrain_frame(it.frame);
        if (!follow && !(ha && hb)) continue;

        const V3 wa = f.world(a.lat, a.lon, 0.0), wb = f.world(it.lat, it.lon, 0.0);
        const double len = std::hypot(wb.x - wa.x, wb.y - wa.y);
        const int    n   = std::clamp((int)std::ceil(len / STEP_M), 1, MAX_SAMPLES);

        std::optional<V3> pt, pg;   // the sample before: path, ground
        for (int k = 0; k <= n; ++k) {
            const double t   = (double)k / n;
            const double lat = a.lat + (it.lat - a.lat) * t, lon = a.lon + (it.lon - a.lon) * t;
            const auto   gnd = terrain_amsl(lat, lon);
            if (!gnd) { pt.reset(); pg.reset(); continue; }
            const double h = follow ? *gnd + a.alt + (it.alt - a.alt) * t
                                    : *ha + (*hb - *ha) * t;
            const V3 top = f.world(lat, lon, h);
            // The track a touch above the ground, so the surface does not
            // swallow it; more with distance, where depth is coarser.
            V3 bot = f.world(lat, lon, *gnd);
            const V3 d = bot - eye;
            bot.z += std::max(1.0, 0.002 * std::sqrt(dot(d, d)));
            if (pt) band_step(L, *pg, *pt, bot, top, false);
            pt = top;
            pg = bot;
        }
    }
}

// The geofence as walls: every uploaded zone, and the parameter cylinder round
// the active vehicle's home, standing on the terrain up to the ceiling —
// FENCE_ALT_MAX above home. Inclusion zones and the cylinder green, exclusion
// zones red. Without a ceiling the walls stand FENCE_DEFAULT_H above the
// ground, so the zone still shows; with one, their tops are drawn too.
constexpr double FENCE_DEFAULT_H = 60.0;

void build_fence(const Frame& f, V3 eye, const FencePlan* zones, const MapVehicle* active,
                 const std::vector<MissionItem>* mission, Projection& out)
{
    const std::optional<float> home = plan_home_amsl(active, mission);
    std::optional<double> ceiling;   // AMSL
    if (active && home && active->fence.ceiling_configured()) ceiling = *home + active->fence.alt_max_m;

    std::vector<std::vector<FenceVertex>> rings[2];   // [0] exclusion, [1] inclusion

    constexpr int CIRCLE_SEG = 72;
    const auto circle = [](FenceVertex c, double r_m) {
        std::vector<FenceVertex> ring;
        const double m_lat = 111320.0, m_lon = 111320.0 * std::cos(c.lat * DEG);
        for (int i = 0; i < CIRCLE_SEG; ++i) {
            const double a = 2 * M_PI * i / CIRCLE_SEG;
            ring.push_back({ c.lat + r_m * std::cos(a) / m_lat, c.lon + r_m * std::sin(a) / m_lon });
        }
        return ring;
    };
    if (zones)
        for (const FenceShape& sh : zones->shapes) {
            if (sh.kind == FenceShape::Kind::Circle) {
                if (sh.radius_m > 0.0f) rings[sh.inclusion].push_back(circle(sh.center, sh.radius_m));
            } else if (sh.pts.size() >= 3) {
                rings[sh.inclusion].push_back(sh.pts);
            }
        }
    if (active && active->has_home && active->fence.circle_configured())
        rings[1].push_back(circle({ active->home_lat, active->home_lon }, active->fence.radius_m));

    constexpr double STEP_M = 25.0;
    constexpr int    MAX_SAMPLES = 200;
    for (int kind = 0; kind < 2; ++kind) {
        if (rings[kind].empty()) continue;
        out.layers.push_back(kind ? TintLayer{ {}, {}, 0.35f, 0.90f, 0.40f }
                                  : TintLayer{ {}, {}, 1.00f, 0.32f, 0.30f });
        TintLayer& L = out.layers.back();
        for (const auto& ring : rings[kind])
            for (size_t e = 0; e < ring.size(); ++e) {
                const FenceVertex& a = ring[e];
                const FenceVertex& b = ring[(e + 1) % ring.size()];
                const V3 wa = f.world(a.lat, a.lon, 0.0), wb = f.world(b.lat, b.lon, 0.0);
                const int n = std::clamp((int)std::ceil(std::hypot(wb.x - wa.x, wb.y - wa.y) / STEP_M),
                                         1, MAX_SAMPLES);
                std::optional<V3> pt, pg;
                for (int k = 0; k <= n; ++k) {
                    const double t   = (double)k / n;
                    const double lat = a.lat + (b.lat - a.lat) * t, lon = a.lon + (b.lon - a.lon) * t;
                    const auto   gnd = terrain_amsl(lat, lon);
                    if (!gnd) { pt.reset(); pg.reset(); continue; }
                    // A ceiling below the ground there leaves no wall to draw.
                    const double h = ceiling ? std::max(*ceiling, (double)*gnd) : *gnd + FENCE_DEFAULT_H;
                    const V3 top = f.world(lat, lon, h);
                    V3 bot = f.world(lat, lon, *gnd);
                    const V3 d = bot - eye;
                    bot.z += std::max(1.0, 0.002 * std::sqrt(dot(d, d)));
                    if (pt) band_step(L, *pg, *pt, bot, top, ceiling.has_value());
                    pt = top;
                    pg = bot;
                }
            }
    }
}

// ── Input ─────────────────────────────────────────────────────────────────────

void handle_input(bool hovered, ImVec2 size, Map3DCenter c)
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

} // namespace

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
    handle_input(hovered, size, center);

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
    if (!clear_at(s_cam.pitch_deg)) {
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
    if (scene.mission && !scene.mission->empty()) build_projection(f, eye, *scene.mission, active, shadow);
    build_fence(f, eye, scene.fence, active, scene.mission, shadow);
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
    if (placing) {
        s_goto = {};
        place_item(dl, f, eye, target, range, hovered, *scene.pick, scene.mission, active);
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
    gl3::DeleteBuffers(1, &g.flat_vbo);
    gl3::DeleteVertexArrays(1, &g.flat_vao);
    gl3::DeleteProgram(g.flat_prog);
    g = Gl{};
}
