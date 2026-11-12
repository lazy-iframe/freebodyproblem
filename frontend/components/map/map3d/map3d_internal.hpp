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

#pragma once

// Shared by the 3D map's files (map_view_3d.cpp and map3d/). Not part of the
// public API, which is map_view_3d.hpp.

#include "frontend/components/map/map_view_3d.hpp"
#include "frontend/components/map/map_internal.hpp"
#include "frontend/source/gl3.hpp"
#include "frontend/source/terrain_clearance.hpp"
#include "imgui.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace map3d_detail {

constexpr double EARTH_R   = 6371000.0;      // for the curve; the terrain shader has its own copy
constexpr double DEG       = M_PI / 180.0;
constexpr int    GRID      = 32;             // cells per tile edge
constexpr int    GV        = GRID + 1;       // vertices per tile edge
constexpr int    ZMAX      = 18;             // deepest imagery tile drawn
constexpr int    TERRAIN_Z = 12;             // deepest terrain read; SRTM has no more
constexpr double SPLIT_K   = 2.0;            // split while distance < SPLIT_K × tile size
constexpr int    MAX_LEAVES = 250;

// How far out the camera may go, and how far it then sees. The work is all in
// the range: tiles fetched, heights built, draws made, and it stays bounded because
// the quadtree only splits near the camera. Far enough out to see the earth curve
// away. Past that this frame stops being a fair picture of it: the map is still
// Mercator, bent by a parabola about the centre.
constexpr double MIN_DIST   = 30.0;
constexpr double MAX_DIST   = 1500000.0;
constexpr double RANGE_K    = 8.0;           // range, in camera distances
constexpr double MIN_RANGE  = 3000.0;
constexpr double MAX_RANGE  = 4000000.0;

// Past this no terrain can stand between the camera and what it looks at, so that
// is not checked: it would only fetch detailed tiles from far under the camera.
constexpr double CAMERA_LOS_MAX_DIST = 100000.0;

// The finest tile worth drawing, as a width against the camera's distance from what it
// looks at. The split below follows the distance to each tile, which alone would still
// go deep under a high camera tilted low, in ground no one can make out from up there.
// This leaves the views flown in as they were: from 1 km it allows the finest tiles.
constexpr double FINEST_TILE_K = 0.1;
constexpr int    BUILDS_PER_FRAME = 24;      // height textures made per frame, at most
constexpr int    TRIES_PER_FRAME  = 96;      // terrain reads attempted per frame, at most
constexpr float  FOV_Y     = 45.0f * (float)DEG;

// ── Small linear algebra ──────────────────────────────────────────────────────

struct V3 { double x, y, z; };
inline V3     operator+(V3 a, V3 b)     { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
inline V3     operator-(V3 a, V3 b)     { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
inline V3     operator*(V3 a, double s) { return { a.x * s, a.y * s, a.z * s }; }
inline double dot(V3 a, V3 b)           { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline V3     cross(V3 a, V3 b)         { return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x }; }
inline V3     norm(V3 a)                { const double l = std::sqrt(dot(a, a)); return l > 0 ? a * (1.0 / l) : a; }

// Column-major, as GL takes it: m[col * 4 + row].
struct M4 { float m[16]; };

inline M4 mul(const M4& a, const M4& b)
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

inline M4 perspective(float fovy, float aspect, float n, float f)
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

inline M4 look_at(V3 eye, V3 at, V3 up)
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
inline V4 xform(const M4& m, V3 p)
{
    return { m.m[0] * p.x + m.m[4] * p.y + m.m[8]  * p.z + m.m[12],
             m.m[1] * p.x + m.m[5] * p.y + m.m[9]  * p.z + m.m[13],
             m.m[2] * p.x + m.m[6] * p.y + m.m[10] * p.z + m.m[14],
             m.m[3] * p.x + m.m[7] * p.y + m.m[11] * p.z + m.m[15] };
}

// ── map_view_3d.cpp ──────────────────────────────────────────────────────────

struct Camera {
    double yaw_deg   = 0.0;     // where it looks, clockwise from north
    double pitch_deg = 35.0;    // down from the horizon
    double dist      = 1500.0;  // metres from the point it orbits
};
extern Camera s_cam;
extern bool   s_textures;   // imagery on the terrain; off, the bare mesh
extern int    s_drag;       // the button that started a drag on the view, or -1

// ── gl.cpp ───────────────────────────────────────────────────────────────────

struct Mesh { GLuint vao = 0, vbo = 0; GLsizei n = 0; };

struct Gl {
    bool   tried = false, ok = false;
    GLuint prog = 0, vao = 0, vbo = 0, ibo = 0;

    // Airframes: one body mesh per SymbolKind, and the helicopter's main
    // rotor apart from its body, so it can turn.
    GLuint craft_prog = 0;
    Mesh   craft[4], heli_rotor, heli_tail, plane_prop, quad_prop_front, quad_prop_rear;
    GLuint craft_tex = 0;   // the airframe texture, one layer per Layer

    // Flat colour, for the mission's ground track and the curtain under it:
    // world positions streamed each frame.
    GLuint flat_prog = 0, flat_vao = 0, flat_vbo = 0;
    GLint  f_vp, f_col;
    GLint  c_vp, c_model, c_eye, c_fog, c_fog_range, c_dim, c_xray, c_gplane, c_ggrad,
           c_tex;

    GLsizei n_idx = 0;
    GLuint fbo = 0, color = 0, depth = 0;
    int    fb_w = 0, fb_h = 0;
    GLint  u_vp, u_origin, u_size, u_exag, u_href, u_skirt, u_height, u_image,
           u_uv, u_eye, u_has_image, u_flat, u_fog, u_fog_range, u_mesh,
           u_cell_off, u_cell_scale, u_mesh_fill, u_mesh_line;
};
extern Gl g;
bool init_gl();
bool ensure_fbo(int w, int h);

// ── airframes.cpp ────────────────────────────────────────────────────────────

// Every vertex carries texture coordinates into one layer of the airframe
// texture (made below, in code, at start), a tint the texel is multiplied by,
// and its normal. Curved parts are shaded smooth: the normal follows the
// surface, not the facet.
struct CV  { float p[3], n[3], c[3], uv[2], layer; };
struct Col { float r, g, b; };

constexpr Col REAR       = { 40 / 255.f, 110 / 255.f, 230 / 255.f };    // a quad's rear propellers
constexpr Col FRONT_PROP = { 60 / 255.f, 200 / 255.f, 90 / 255.f };     // and its front ones

// Where the moving parts are drawn on each airframe, in body units.
constexpr double QUAD_ARM = 0.72;   // centre to motor, along each axis
constexpr double QUAD_PROP_Z = -0.085;
const V3 PLANE_PROP_HUB = { 0.86, 0, 0.0 };
const V3 HELI_HUB = { 0.12, 0, -0.37 };
const V3 HELI_TAIL_HUB = { -1.34, 0.04, -0.27 };

void build_multicopter(std::vector<CV>& v);
void build_quad_prop(std::vector<CV>& v, Col col);
void build_plane(std::vector<CV>& v);
void build_plane_prop(std::vector<CV>& v);
void build_helicopter(std::vector<CV>& v);
void build_heli_rotor(std::vector<CV>& v);
void build_heli_tail_rotor(std::vector<CV>& v);
void build_arrow(std::vector<CV>& v);
GLuint make_airframe_texture();
Mesh upload(const std::vector<CV>& v);

// ── heights.cpp ──────────────────────────────────────────────────────────────

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
extern std::unordered_map<Key, Heights, KeyHash> s_heights;
extern uint64_t s_frame;

// The tile's heights, made or improved within this frame's budget. Terrain at
// the tile's own zoom (capped at TERRAIN_Z) is asked for; until it arrives a
// coarser terrain zoom already loaded stands in, so ground appears at once
// and sharpens as the tiles come.
// Both caps matter while tiles load: a read that fails still takes locks, and
// a few hundred tiles each trying seven zooms every frame would add up.
struct Budget { int builds = BUILDS_PER_FRAME, tries = TRIES_PER_FRAME; };
const Heights* heights_for(const Key& k, Budget& budget);
void forget_heights();
void evict_heights();

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
        geo_mercator(lat, lon, u, v);
        const double x = (u - u0) * m, y = -(v - v0) * m;
        return { x, y, (amsl - href) * exag - drop(x, y) };
    }

    // How far the earth has curved away at (x, y), from the centre of the view. In world
    // units, so by the same exaggeration as the heights it is measured against. The
    // terrain shader does the same sum.
    double drop(double x, double y) const { return (x * x + y * y) / (2.0 * EARTH_R) * exag; }

    // The other way from world(): the height AMSL of a world point.
    double amsl_at(V3 p) const { return (p.z + drop(p.x, p.y)) / exag + href; }

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

// ── craft.cpp ────────────────────────────────────────────────────────────────

// The body axes — forward, right, down — in the world frame; see body_axes().
struct Basis { V3 x, y, z; };

// Where an aircraft is drawn and how: its position, its axes, and the scale
// that keeps it the same size on screen at any distance, as the 2D symbol is.
struct Pose {
    V3                   p;
    Basis                b;
    double               s;
    std::optional<float> ground;   // terrain AMSL under it
};
Pose vehicle_pose(const Frame& f, V3 eye, const MapVehicle& mv);
void render_craft(const Frame& f, V3 eye, double range, const std::vector<MapVehicle>& fleet);

// ── scene.cpp ────────────────────────────────────────────────────────────────

std::optional<double> ground_z(const Frame& f, double ex, double ey);

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
void band_step(TintLayer& L, V3 bot0, V3 top0, V3 bot, V3 top, bool top_edge);
void render_scene(const Frame& f, V3 eye, double range, int fb_w, int fb_h,
                  const std::vector<MapVehicle>& fleet, const Projection& proj);

// ── overlays.cpp ─────────────────────────────────────────────────────────────

ImU32 drop_col(float alpha);
ImU32 under_col(float alpha = 1.0f);
void draw_path(ImDrawList* dl, const Frame& f, V3 a, V3 b, ImU32 col, float w, ImU32 under);
void drop_line(ImDrawList* dl, const Frame& f, V3 eye, V3 top, double ground_amsl, float alpha = 1.0f);
void draw_trails(ImDrawList* dl, const Frame& f, const std::vector<MapVehicle>& fleet);
void draw_clearance(ImDrawList* dl, const Frame& f, V3 a, V3 b, const Clearance& c);
void draw_mission(ImDrawList* dl, const Frame& f, V3 eye, const std::vector<MissionItem>& mission,
                  const MapVehicle* active);
void draw_vehicles(ImDrawList* dl, const Frame& f, V3 eye, const std::vector<MapVehicle>& fleet);

// ── pick.cpp ─────────────────────────────────────────────────────────────────

// How far either side of the ground a point can ride its line. Going into the terrain
// is the operator's call and only warned of; this is what stops a pointer near the
// horizon throwing the point out of sight.
constexpr double POINT_MAX_REACH = 3000.0;
void height_readout(char* out, size_t n, double rel, double agl, const std::string& home_dist);

// One 3D point being picked: where on the ground, then how high over it.
struct PointPick {
    bool   active    = false;
    bool   alt_phase = false;
    double lat = 0, lon = 0;      // fixed by the ground click
    double ground    = 0;         // AMSL under it
    double amsl      = 0;         // the point's height, AMSL
};
extern PointPick s_place;              // the mission tab's LOC?, left button
extern uint16_t  s_place_cmd;          // the command it is for
extern PointPick s_goto;               // GO HERE, left button
extern bool      s_clear_ready;        // a right-click, until map_view takes it
extern bool      s_goto_ready;         // the GO HERE point, until map_view takes it
extern double    s_goto_lat, s_goto_lon;
extern float     s_goto_rel;
extern bool      s_press_in_view[2];   // the left / right press began over the view
bool view_click(ImGuiMouseButton b, bool hovered);
V3 pointer_ray(const Frame& f, V3 eye, V3 target, ImVec2 mouse);
std::optional<double> vertical_z_nearest(V3 o, V3 d, double x, double y);
ImVec4 ok_or_error(bool error);
void draw_banner(const Frame& f, const char* text);
void place_item(ImDrawList* dl, const Frame& f, V3 eye, V3 target, double range, bool hovered,
                MissionPickState& pick, const std::vector<MissionItem>* mission,
                const MapVehicle* active);
void pick_goto(ImDrawList* dl, const Frame& f, V3 eye, V3 target, double range, bool hovered,
               const GotoTargetState& go, const std::vector<MissionItem>* mission,
               const MapVehicle* active);
void draw_goto_target(ImDrawList* dl, const Frame& f, V3 eye, const GotoTargetState& go,
                      const std::vector<MissionItem>* mission, const MapVehicle* active);

// ── bands.cpp ────────────────────────────────────────────────────────────────

struct BandSet;
void place_bands(const Frame& f, V3 eye, const BandSet& set, Projection& out);
const BandSet& mission_bands(const std::vector<MissionItem>& mission, const MapVehicle* active);
const BandSet& fence_bands(const FencePlan* zones, const MapVehicle* active,
                           const std::vector<MissionItem>* mission);
// The fence's outlines traced on the ground, dotted, while the active vehicle
// has it off — what fence_bands() raises as walls while it is on.
void draw_fence_off(ImDrawList* dl, const Frame& f, const FencePlan* zones,
                    const MapVehicle* active);

// ── edit.cpp ─────────────────────────────────────────────────────────────────

struct WaypointDrag {
    int    index  = -1;      // into the editor's mission; -1 when nothing is held
    bool   height = false;   // on its line, not its marker
    double plane_z = 0.0;    // the plane a marker moves in, world
    double off_x = 0.0, off_y = 0.0;   // pointer to marker on that plane, so a grab
                                       // off-centre does not make it jump
    double off_z = 0.0;      // the same for a height, along the line
    double low   = 0.0;      // the least height it may take, AMSL: see POINT_MAX_REACH
    std::optional<float> home;   // held for the whole drag: see edit_waypoints()
};
extern WaypointDrag s_wp_drag;
void edit_waypoints(ImDrawList* dl, const Frame& f, V3 eye, V3 target, double range, bool hovered,
                    std::vector<MissionItem>& mission, const MapVehicle* active, Map3DCenter center);

} // namespace map3d_detail
