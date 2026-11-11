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

// frontend/components/map/map3d/airframes.cpp
//
// The airframe meshes the 3D map draws each vehicle as, built in code at start,
// and the texture they are painted with.

#include "frontend/components/map/map3d/map3d_internal.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace map3d_detail {

// ── Airframe meshes ───────────────────────────────────────────────────────────
//
// Built from tubes, ellipsoids, lofted bodies and airfoil wings in the body
// frame, about a unit across, and textured: white paint with the livery in
// red — the colour that stood out best over the imagery of all that were
// tried — glass, carbon and metal.

struct UV  { double u, v; };

// The airframe texture's layers. The paint is white with the livery in red,
// as the map's own reds; which side is which reads off the navigation
// lights, red to port and green to starboard, as on a real aircraft.
enum Layer : int {
    L_FUSE = 0,   // a body's skin: u nose to tail, v round from the top — a red
                  // cheatline down each side (v .25 starboard, .75 port), a
                  // grey belly, panel seams
    L_WING,       // a wing's skin: u leading to trailing edge, v root to tip —
                  // spar and rib seams, flap and aileron, a red tip
    L_TAIL,       // the fin's livery, uv as a wing's: red, a white sweep
    L_GLASS,      // canopy
    L_CARBON,     // twill weave, tiled
    L_METAL,      // brushed along u
    L_BLADE,      // a blade along u: grey, white tips at both ends; tinted
    L_PAINT,      // plain white paint; tinted
    L_LIGHT,      // unlit, the tint as it is: navigation lights
    L_COUNT
};
constexpr int TEX_PX = 256;

// A part's look: its layer, a tint, and how many times the layer repeats
// along u and v.
struct Mat { int layer; Col tint; float us = 1.0f, vs = 1.0f; };

constexpr Col WHITE      = { 1.0f, 1.0f, 1.0f };
constexpr Col LIVERY     = { 0.78f, 0.10f, 0.10f };
constexpr Col PORT       = { 196 / 255.f,  34 / 255.f,  34 / 255.f };   // the 2D arrow's reds
constexpr Col STBD       = { 245 / 255.f,  72 / 255.f,  60 / 255.f };
constexpr Col DARK       = { 0.30f, 0.30f, 0.32f };
constexpr Col NAV_RED    = { 1.0f, 0.16f, 0.12f };
constexpr Col NAV_GREEN  = { 0.18f, 1.0f, 0.38f };
constexpr Col NAV_WHITE  = { 1.0f, 1.0f, 1.0f };

const Mat M_FUSE   { L_FUSE,   WHITE };
const Mat M_WING   { L_WING,   WHITE };
const Mat M_TAIL   { L_TAIL,   WHITE };
const Mat M_GLASS  { L_GLASS,  WHITE };
const Mat M_METAL  { L_METAL,  WHITE };
const Mat M_RED    { L_PAINT,  LIVERY };
const Mat M_WHITE  { L_PAINT,  WHITE };

static Mat carbon(float us, float vs) { return { L_CARBON, WHITE, us, vs }; }
static Mat light(Col c)               { return { L_LIGHT, c }; }

static void add_vertex(std::vector<CV>& v, V3 p, V3 n, UV t, const Mat& m)
{
    v.push_back({ { (float)p.x, (float)p.y, (float)p.z },
                  { (float)n.x, (float)n.y, (float)n.z },
                  { m.tint.r, m.tint.g, m.tint.b },
                  { (float)t.u * m.us, (float)t.v * m.vs }, (float)m.layer });
}

static void add_tri(std::vector<CV>& v, V3 a, V3 b, V3 c, const Mat& m,
             UV ta = { 0, 0 }, UV tb = { 1, 0 }, UV tc = { 1, 1 })
{
    const V3 n = norm(cross(b - a, c - a));
    add_vertex(v, a, n, ta, m);
    add_vertex(v, b, n, tb, m);
    add_vertex(v, c, n, tc, m);
}

// Its uv runs 0 to 1 from a to b, and from a to d.
static void add_quad(std::vector<CV>& v, V3 a, V3 b, V3 c, V3 d, const Mat& m)
{
    add_tri(v, a, b, c, m, { 0, 0 }, { 1, 0 }, { 1, 1 });
    add_tri(v, a, c, d, m, { 0, 0 }, { 1, 1 }, { 0, 1 });
}

// A box about `c` with half-extents `h`, turned `yaw` radians about body z.
// On the four faces along its x, u runs along x.
static void add_box(std::vector<CV>& v, V3 c, V3 h, double yaw, const Mat& m)
{
    const double cs = std::cos(yaw), sn = std::sin(yaw);
    V3 k[8];
    for (int i = 0; i < 8; ++i) {
        const double x = (i & 1) ? h.x : -h.x, y = (i & 2) ? h.y : -h.y, z = (i & 4) ? h.z : -h.z;
        k[i] = { c.x + x * cs - y * sn, c.y + x * sn + y * cs, c.z + z };
    }
    add_quad(v, k[0], k[1], k[3], k[2], m);   // -z
    add_quad(v, k[4], k[5], k[7], k[6], m);   // +z
    add_quad(v, k[0], k[1], k[5], k[4], m);   // -y
    add_quad(v, k[2], k[3], k[7], k[6], m);   // +y
    add_quad(v, k[0], k[2], k[6], k[4], m);   // -x
    add_quad(v, k[1], k[3], k[7], k[5], m);   // +x
}

// A smooth surface through a grid of points, `rows` × `cols`, each with its
// uv. With `ring`, each row's last point is its first again, closing it, and
// the normals there see round the seam. Normals are `nrm`'s, else the grid's
// own differences; their sign does not matter — the shader turns every
// normal to the eye.
static void add_grid(std::vector<CV>& v, const std::vector<V3>& pts, const std::vector<UV>& uv,
              int rows, int cols, bool ring, const Mat& m, const std::vector<V3>* nrm = nullptr)
{
    const auto at = [&](int r, int c) { return pts[(size_t)r * cols + c]; };
    std::vector<V3> n(pts.size());
    for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols; ++c) {
            const size_t i = (size_t)r * cols + c;
            if (nrm) { n[i] = (*nrm)[i]; continue; }
            const int r0 = std::max(r - 1, 0), r1 = std::min(r + 1, rows - 1);
            int c0 = c - 1, c1 = c + 1;
            if (ring) { if (c0 < 0) c0 = cols - 2; if (c1 > cols - 1) c1 = 1; }
            else      { c0 = std::max(c0, 0);      c1 = std::min(c1, cols - 1); }
            const V3 d = cross(at(r1, c) - at(r0, c), at(r, c1) - at(r, c0));
            n[i] = dot(d, d) > 1e-24 ? norm(d) : V3{ 0, 0, -1 };
        }
    for (int r = 0; r + 1 < rows; ++r)
        for (int c = 0; c + 1 < cols; ++c) {
            const size_t a = (size_t)r * cols + c, b = a + 1, d = a + cols, e = d + 1;
            for (size_t i : { a, b, e, a, e, d }) add_vertex(v, pts[i], n[i], uv[i], m);
        }
}

// A flat fan closing one row of a ring: the end of a body, a wing tip.
static void add_cap(std::vector<CV>& v, const std::vector<V3>& pts, int row, int cols, const Mat& m)
{
    V3 c{ 0, 0, 0 };
    for (int k = 0; k + 1 < cols; ++k) c = c + pts[(size_t)row * cols + k];
    c = c * (1.0 / (cols - 1));
    for (int k = 0; k + 1 < cols; ++k)
        add_tri(v, c, pts[(size_t)row * cols + k], pts[(size_t)row * cols + k + 1], m,
                { 0.5, 0.5 }, { 0.5, 0.5 }, { 0.5, 0.5 });
}

// An ellipsoid about `c` with radii `r`. u runs along x from its front, v
// round from the top — as a body's, so L_FUSE's cheatlines fall on its sides.
// `slices` round it and `stacks` along it.
static void add_ellipsoid(std::vector<CV>& v, V3 c, V3 r, const Mat& m, int slices = 24, int stacks = 14)
{
    const int SLICES = slices, STACKS = stacks;
    std::vector<V3> pts, nrm;
    std::vector<UV> uv;
    for (int j = 0; j <= STACKS; ++j)
        for (int i = 0; i <= SLICES; ++i) {
            const double th = M_PI * j / STACKS, ph = 2 * M_PI * i / SLICES;
            const V3 u = { std::cos(th), std::sin(th) * std::sin(ph), -std::sin(th) * std::cos(ph) };
            pts.push_back({ c.x + r.x * u.x, c.y + r.y * u.y, c.z + r.z * u.z });
            nrm.push_back(norm({ u.x / r.x, u.y / r.y, u.z / r.z }));   // the ellipsoid's own normal
            uv.push_back({ (double)j / STACKS, (double)i / SLICES });
        }
    add_grid(v, pts, uv, STACKS + 1, SLICES + 1, true, m, &nrm);
}

// A tube from `a` to `b`, radius `ra` at one end and `rb` at the other — a
// tapered boom, a strut, a skid — round, with flat caps. u along it, v round
// from its top where it has one.
static void add_tube(std::vector<CV>& v, V3 a, V3 b, double ra, double rb, const Mat& m)
{
    constexpr int SEG = 16;
    const V3 ax  = norm(b - a);
    const V3 ref = std::fabs(ax.z) < 0.9 ? V3{ 0, 0, -1 } : V3{ 1, 0, 0 };
    const V3 e1  = norm(ref - ax * dot(ref, ax)), e2 = cross(ax, e1);
    const double len = std::sqrt(dot(b - a, b - a));
    std::vector<V3> pts, nrm;
    std::vector<UV> uv;
    for (int r = 0; r < 2; ++r)
        for (int k = 0; k <= SEG; ++k) {
            const double t = 2 * M_PI * k / SEG;
            const V3 d = e1 * std::cos(t) + e2 * std::sin(t);
            pts.push_back((r ? b : a) + d * (r ? rb : ra));
            nrm.push_back(norm(d + ax * ((ra - rb) / len)));   // a cone's normal leans by the taper
            uv.push_back({ (double)r, (double)k / SEG });
        }
    add_grid(v, pts, uv, 2, SEG + 1, true, m, &nrm);
    add_cap(v, pts, 0, SEG + 1, m);
    add_cap(v, pts, 1, SEG + 1, m);
}

// A fuselage: a body of revolution along body x, elliptical in section, given
// by stations from nose to tail — at x, half-width ry, half-height rz, the
// section's centre at z = zc (z is down). uv as an ellipsoid's.
struct Station { double x, ry, rz, zc; };

static void add_body(std::vector<CV>& v, const std::vector<Station>& st, const Mat& m)
{
    constexpr int SEG = 28;
    const double x0 = st.front().x, x1 = st.back().x;
    std::vector<V3> pts;
    std::vector<UV> uv;
    for (const Station& s : st)
        for (int k = 0; k <= SEG; ++k) {
            const double ph = 2 * M_PI * k / SEG;
            pts.push_back({ s.x, s.ry * std::sin(ph), s.zc - s.rz * std::cos(ph) });
            uv.push_back({ (x0 - s.x) / (x0 - x1), (double)k / SEG });
        }
    const int rows = (int)st.size();
    add_grid(v, pts, uv, rows, SEG + 1, true, m);
    add_cap(v, pts, 0, SEG + 1, m);
    add_cap(v, pts, rows - 1, SEG + 1, m);
}

// A wing, tailplane or fin: an airfoil (NACA four-digit, symmetric, `tr`
// thick for its chord) lofted from the root's leading and trailing edges to
// the tip's, its thickness along `n`. u runs leading to trailing edge, v root
// to tip; capped at both ends.
static void add_wing(std::vector<CV>& v, V3 rl, V3 rt, V3 tl, V3 tt, V3 n, double tr, const Mat& m)
{
    constexpr int K = 10, SPAN = 4;
    constexpr int COLS = 2 * K - 1;   // trailing edge, over the top to the leading edge, back under
    std::vector<V3> pts;
    std::vector<UV> uv;
    for (int r = 0; r <= SPAN; ++r) {
        const double s = (double)r / SPAN;
        const V3 le = rl + (tl - rl) * s, te = rt + (tt - rt) * s;
        const double chord = std::sqrt(dot(te - le, te - le));
        for (int k = 0; k < COLS; ++k) {
            const bool   top = k < K;
            const double q   = top ? 1.0 - (double)k / (K - 1) : (double)(k - K + 1) / (K - 1);
            const double c   = 0.5 * (1.0 - std::cos(M_PI * q));   // closer together at the edges
            const double th  = 5.0 * tr * (0.2969 * std::sqrt(c) - 0.1260 * c - 0.3516 * c * c +
                                           0.2843 * c * c * c - 0.1036 * c * c * c * c);
            pts.push_back(le + (te - le) * c + n * ((top ? -1.0 : 1.0) * th * chord));
            uv.push_back({ c, s });
        }
    }
    add_grid(v, pts, uv, SPAN + 1, COLS, true, m);
    add_cap(v, pts, 0, COLS, m);
    add_cap(v, pts, SPAN, COLS, m);
}

// A navigation light: a small bright bead, a few pixels across, so coarse.
static void add_light(std::vector<CV>& v, V3 p, Col c)
{
    add_ellipsoid(v, p, { 0.03, 0.03, 0.03 }, light(c), 8, 5);
}

// An X quadcopter: a rounded white body with the cheatline down its sides
// under a carbon shell, four carbon arms out on the diagonals, a motor at the
// end of each with a red cap, and a navigation light outboard of it. The
// propellers are meshes of their own, turned as they are drawn — green the
// front pair, blue the rear, so which way it faces reads from any side.
void build_multicopter(std::vector<CV>& v)
{
    // The body half as large again as it was, the arms and propellers where
    // they were: the same span, a heavier centre. It stays clear of the
    // propeller discs, whose inner edges come no nearer than 0.66 of centre.
    add_ellipsoid(v, { 0.03, 0, 0.0 }, { 0.45, 0.30, 0.13 }, M_FUSE);               // body
    add_ellipsoid(v, { 0.06, 0, -0.11 }, { 0.26, 0.17, 0.075 }, carbon(3, 6));      // top shell
    for (int i = 0; i < 4; ++i) {
        const double x = (i & 1) ? QUAD_ARM : -QUAD_ARM, y = (i & 2) ? QUAD_ARM : -QUAD_ARM;
        add_tube(v, { x * 0.3, y * 0.3, 0.0 }, { x, y, 0.0 }, 0.04, 0.03, carbon(8, 2));   // arm
        add_tube(v, { x, y, 0.05 }, { x, y, -0.05 }, 0.055, 0.05, M_METAL);                // motor
        add_tube(v, { x, y, -0.05 }, { x, y, -0.075 }, 0.05, 0.035, M_RED);                // its cap
        add_tube(v, { x, y, 0.05 }, { x, y, 0.16 }, 0.012, 0.012, carbon(2, 1));           // leg
        const double o = 0.075 / std::sqrt(2.0);
        add_light(v, { x + (x > 0 ? o : -o), y + (y > 0 ? o : -o), 0.02 }, y < 0 ? NAV_RED : NAV_GREEN);
    }
}

// A propeller about its hub: two blades and the cap, tinted `col`. Drawn at
// each motor's top, turned about the body's z.
void build_quad_prop(std::vector<CV>& v, Col col)
{
    add_tube(v, { 0, 0, 0.012 }, { 0, 0, -0.02 }, 0.03, 0.02, { L_PAINT, col });
    add_box(v, { 0, 0, 0 }, { 0.36, 0.045, 0.006 }, 0.0, { L_BLADE, col });
}

// A light single: a white fuselage running smoothly from the spinner back to
// a raised tail, a glass canopy, tapered wings with a little dihedral and red
// tips, a tapered tailplane, the fin in the livery, and the navigation
// lights — red on the port tip, green on the starboard, white at the tail.
// The propeller is a mesh of its own, turned as it is drawn.
void build_plane(std::vector<CV>& v)
{
    add_body(v, {
        {  0.80, 0.030, 0.030,  0.000 }, {  0.76, 0.075, 0.080,  0.000 },
        {  0.68, 0.108, 0.118, -0.002 }, {  0.55, 0.126, 0.138, -0.005 },
        {  0.35, 0.132, 0.145, -0.010 }, {  0.10, 0.126, 0.136, -0.015 },
        { -0.15, 0.112, 0.116, -0.022 }, { -0.45, 0.086, 0.086, -0.032 },
        { -0.75, 0.060, 0.058, -0.045 }, { -0.98, 0.040, 0.040, -0.055 },
        { -1.10, 0.016, 0.018, -0.060 },
    }, M_FUSE);                                                                           // fuselage
    add_ellipsoid(v, { 0.32, 0, -0.115 }, { 0.24, 0.085, 0.07 }, M_GLASS);              // canopy
    add_tube(v, { 0.78, 0, 0.0 }, { 0.92, 0, 0.0 }, 0.062, 0.008, M_RED);               // spinner

    // Wings: root chord at the fuselage, a shorter chord at the tip, the tip
    // a little higher (z is down) — the dihedral.
    for (int side : { -1, 1 }) {
        add_wing(v, { 0.36, 0.11 * side, 0.03 }, { -0.08, 0.11 * side, 0.03 },
                    { 0.26, 1.18 * side, -0.04 }, { 0.06, 1.18 * side, -0.04 },
                 { 0, 0, 1 }, 0.12, M_WING);                                             // wing
        add_wing(v, { -0.80, 0.04 * side, -0.05 }, { -1.04, 0.04 * side, -0.05 },
                    { -0.90, 0.44 * side, -0.05 }, { -1.02, 0.44 * side, -0.05 },
                 { 0, 0, 1 }, 0.10, M_WING);                                             // tailplane
        add_light(v, { 0.17, 1.19 * side, -0.04 }, side < 0 ? NAV_RED : NAV_GREEN);
    }
    add_wing(v, { -0.76, 0, -0.07 }, { -1.08, 0, -0.06 },
                { -0.98, 0, -0.40 }, { -1.11, 0, -0.40 }, { 0, 1, 0 }, 0.10, M_TAIL);  // fin
    add_light(v, { -1.11, 0, -0.06 }, NAV_WHITE);
}

// The propeller about its hub: two blades. Drawn at PLANE_PROP_HUB, turned
// about the body's x.
void build_plane_prop(std::vector<CV>& v)
{
    add_box(v, { 0, 0, 0 }, { 0.32, 0.012, 0.035 }, M_PI / 2, { L_BLADE, DARK });
}

// A light helicopter: a white cabin with the cheatline down its sides and its
// canopy forward, a boom tapering back to a stabiliser and the fin in the
// livery, skids on struts, the mast, and the navigation lights on the
// stabiliser's tips. Both rotors are meshes of their own, turned as they are
// drawn.
void build_helicopter(std::vector<CV>& v)
{
    add_ellipsoid(v, { 0.12, 0, 0.0 },   { 0.56, 0.27, 0.26 }, M_FUSE);     // cabin
    add_ellipsoid(v, { 0.44, 0, -0.04 }, { 0.24, 0.21, 0.18 }, M_GLASS);    // canopy
    add_tube(v, { -0.30, 0, -0.06 }, { -1.36, 0, -0.14 }, 0.10, 0.035, M_FUSE);   // tail boom
    add_wing(v, { -1.26, 0, -0.14 }, { -1.42, 0, -0.12 },
                { -1.38, 0, -0.42 }, { -1.46, 0, -0.42 }, { 0, 1, 0 }, 0.12, M_TAIL);   // fin
    add_tube(v, { 0.12, 0, -0.22 },  { 0.12, 0, -0.36 }, 0.035, 0.03, M_METAL);   // mast
    for (int side : { -1, 1 }) {
        add_wing(v, { -1.00, 0.02 * side, -0.12 }, { -1.12, 0.02 * side, -0.12 },
                    { -1.02, 0.20 * side, -0.12 }, { -1.10, 0.20 * side, -0.12 },
                 { 0, 0, 1 }, 0.14, M_WING);                                          // stabiliser
        add_light(v, { -1.06, 0.215 * side, -0.12 }, side < 0 ? NAV_RED : NAV_GREEN);
        add_tube(v, { -0.38, 0.27 * side, 0.34 }, { 0.62, 0.27 * side, 0.34 }, 0.024, 0.024, M_METAL);   // skid
        add_tube(v, { 0.62, 0.27 * side, 0.34 },  { 0.70, 0.27 * side, 0.28 }, 0.024, 0.024, M_METAL);   // its upturned toe
        for (double x : { -0.12, 0.38 })
            add_tube(v, { x, 0.13 * side, 0.20 }, { x, 0.27 * side, 0.34 }, 0.018, 0.018, M_METAL);       // struts
    }
}

// The main rotor about its hub: four blades and the hub. Drawn at HELI_HUB,
// turned about the body's z by the rotor angle.
void build_heli_rotor(std::vector<CV>& v)
{
    add_tube(v, { 0, 0, 0.02 }, { 0, 0, -0.02 }, 0.06, 0.06, M_METAL);
    for (double yaw : { 0.0, M_PI / 2 })
        add_box(v, { 0, 0, 0 }, { 1.15, 0.045, 0.01 }, yaw, { L_BLADE, DARK });
}

// The tail rotor about its hub: two blades and the hub, on the fin's right.
// Drawn at HELI_TAIL_HUB, turned about the body's y.
void build_heli_tail_rotor(std::vector<CV>& v)
{
    add_tube(v, { 0, -0.015, 0 }, { 0, 0.015, 0 }, 0.03, 0.03, M_METAL);
    add_box(v, { 0, 0, 0 }, { 0.20, 0.008, 0.03 }, 0.0, { L_BLADE, DARK });
}

// The 2D map's arrow, given thickness: a nose and two tail tips about a notch,
// in its two reds.
void build_arrow(std::vector<CV>& v)
{
    const double H = 0.08;
    const V3 nose = { 1.0, 0, 0 }, left = { -0.7, -0.6, 0 }, right = { -0.7, 0.6, 0 }, notch = { -0.35, 0, 0 };
    const auto up = [&](V3 p) { return V3{ p.x, p.y, p.z - H }; };
    const auto dn = [&](V3 p) { return V3{ p.x, p.y, p.z + H }; };
    const Mat port{ L_PAINT, PORT }, stbd{ L_PAINT, STBD };
    add_tri(v, up(nose), up(notch), up(left), port);
    add_tri(v, up(nose), up(right), up(notch), stbd);
    add_tri(v, dn(nose), dn(left), dn(notch), port);
    add_tri(v, dn(nose), dn(notch), dn(right), stbd);
    add_quad(v, up(nose), up(left), dn(left), dn(nose), port);
    add_quad(v, up(left), up(notch), dn(notch), dn(left), port);
    add_quad(v, up(notch), up(right), dn(right), dn(notch), stbd);
    add_quad(v, up(right), up(nose), dn(nose), dn(right), stbd);
}

// ── The airframe texture ──────────────────────────────────────────────────────
//
// Painted here, once, at start: L_COUNT layers of TEX_PX square, RGB the
// colour and A how glossy the surface is. Mipmapped, so the fine seams fade
// out evenly as an aircraft shrinks rather than shimmering.

// A hash, and smooth noise from it, for the slight unevenness of real paint.
static float hash01(int x, int y, int seed)
{
    uint32_t h = (uint32_t)x * 374761393u + (uint32_t)y * 668265263u + (uint32_t)seed * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return (float)((h ^ (h >> 16)) & 0xFFFF) / 65535.0f;
}

static float noise(float x, float y, int seed)
{
    const int   xi = (int)std::floor(x), yi = (int)std::floor(y);
    const float fx = x - xi, fy = y - yi;
    const float sx = fx * fx * (3 - 2 * fx), sy = fy * fy * (3 - 2 * fy);
    const float a = hash01(xi, yi, seed), b = hash01(xi + 1, yi, seed);
    const float c = hash01(xi, yi + 1, seed), d = hash01(xi + 1, yi + 1, seed);
    return (a + (b - a) * sx) + ((c + (d - c) * sx) - (a + (b - a) * sx)) * sy;
}

// How much of a line `w` texels wide at `at` covers coordinate `t` (0..1).
static float line_at(float t, float at, float w)
{
    return std::clamp(1.0f - std::fabs(t - at) * TEX_PX / w, 0.0f, 1.0f);
}

// A band from a to b, its edges a texel soft.
static float band(float t, float a, float b)
{
    const float e = 1.0f / TEX_PX;
    return std::clamp((t - a) / e + 0.5f, 0.0f, 1.0f) * std::clamp((b - t) / e + 0.5f, 0.0f, 1.0f);
}

struct Texel { float r, g, b, a; };
static Texel mix(Texel x, Texel y, float t)
{
    return { x.r + (y.r - x.r) * t, x.g + (y.g - x.g) * t, x.b + (y.b - x.b) * t, x.a + (y.a - x.a) * t };
}
static Texel shade(Texel x, float k) { return { x.r * k, x.g * k, x.b * k, x.a }; }

static Texel paint_texel(int layer, float u, float v)
{
    const Texel white  = { 0.94f, 0.94f, 0.93f, 0.55f };
    const Texel grey   = { 0.76f, 0.77f, 0.79f, 0.45f };
    const Texel red    = { LIVERY.r, LIVERY.g, LIVERY.b, 0.60f };
    const float grain  = 0.97f + 0.03f * noise(u * 24, v * 24, layer);
    switch (layer) {
    case L_FUSE: {
        Texel t = shade(white, grain);
        t = mix(t, grey, band(v, 0.37f, 0.63f));                                       // belly
        for (float c : { 0.235f, 0.765f }) {                                            // cheatlines
            t = mix(t, red, band(v, c - 0.022f, c + 0.022f));
            t = mix(t, shade(red, 0.7f), line_at(v, c + (c < 0.5f ? 0.035f : -0.035f), 1.2f));
        }
        float seam = 0.0f;
        for (int k = 1; k < 10; ++k) seam = std::max(seam, line_at(u, k / 10.0f, 0.9f));
        seam = std::max({ seam, line_at(v, 0.5f, 0.9f), line_at(v, 0.0f, 0.9f), line_at(v, 1.0f, 0.9f) });
        return shade(t, 1.0f - 0.16f * seam);
    }
    case L_WING: {
        Texel t = shade(white, grain);
        t = mix(t, { 0.80f, 0.81f, 0.83f, 0.85f }, band(u, 0.0f, 0.06f));              // leading edge
        const bool aft = u > 0.74f;
        if (aft) t = shade(t, 0.95f);                                                   // flap, aileron
        t = mix(t, red, band(v, 0.87f, 1.01f));                                          // tip
        float seam = std::max(line_at(u, 0.20f, 0.9f), line_at(u, 0.74f, 1.1f));
        for (int k = 1; k < 7; ++k) seam = std::max(seam, line_at(v, k / 7.0f, 0.7f) * 0.6f);
        if (aft) seam = std::max(seam, line_at(v, 0.42f, 1.1f));                        // flap | aileron
        return shade(t, 1.0f - 0.18f * seam);
    }
    case L_TAIL: {
        Texel t = mix(shade(red, 0.78f), red, std::clamp(v * 1.4f, 0.0f, 1.0f));
        t = shade(t, grain);
        const float s = v - (0.12f + 0.75f * u);                                          // the sweep
        t = mix(t, white, band(s, -0.07f, 0.0f));
        t = mix(t, white, band(s, 0.05f, 0.075f));
        return mix(t, white, band(v, -0.01f, 0.06f));                                    // root
    }
    case L_GLASS: {
        const float top = std::exp(-std::pow((v < 0.5f ? v : 1.0f - v) * 6.0f, 2.0f));
        Texel t = { 0.05f, 0.09f, 0.14f, 1.0f };
        t = mix(t, { 0.30f, 0.42f, 0.54f, 1.0f }, 0.6f * top * band(u, 0.15f, 0.65f));
        return t;
    }
    case L_CARBON: {
        // 2/2 twill, sixteen texels a tow: the warp and the weft alternate on
        // a diagonal, each tow lit across its width.
        const float x = u * TEX_PX / 16.0f, y = v * TEX_PX / 16.0f;
        const int   ix = (int)std::floor(x), iy = (int)std::floor(y);
        const bool  warp = ((ix + iy) & 3) < 2;
        const float across = warp ? x - ix : y - iy;
        const float k = 0.10f + 0.10f * std::sin((float)M_PI * across);
        return { k, k, k * 1.08f, 0.45f };
    }
    case L_METAL: {
        const float k = 0.42f + 0.10f * noise(u * 3, v * TEX_PX * 0.5f, 7) + 0.04f * noise(u * 40, v * 40, 8);
        return { k, k, k * 1.03f, 0.90f };
    }
    case L_BLADE: {
        Texel t = { 0.62f, 0.62f, 0.62f, 0.50f };
        const float tip = band(u, -1.0f, 0.07f) + band(u, 0.93f, 2.0f);
        t = mix(t, { 1.0f, 1.0f, 1.0f, 0.55f }, tip);
        return shade(t, 1.0f - 0.3f * std::max(line_at(u, 0.075f, 1.0f), line_at(u, 0.925f, 1.0f)));
    }
    case L_PAINT:
        return shade({ 1.0f, 1.0f, 1.0f, 0.55f }, grain);
    default:   // L_LIGHT
        return { 1.0f, 1.0f, 1.0f, 0.0f };
    }
}

GLuint make_airframe_texture()
{
    std::vector<uint8_t> px((size_t)TEX_PX * TEX_PX * 4 * L_COUNT);
    for (int l = 0; l < L_COUNT; ++l)
        for (int y = 0; y < TEX_PX; ++y)
            for (int x = 0; x < TEX_PX; ++x) {
                const Texel t = paint_texel(l, (x + 0.5f) / TEX_PX, (y + 0.5f) / TEX_PX);
                uint8_t* p = &px[(((size_t)l * TEX_PX + y) * TEX_PX + x) * 4];
                p[0] = (uint8_t)std::lround(std::clamp(t.r, 0.0f, 1.0f) * 255);
                p[1] = (uint8_t)std::lround(std::clamp(t.g, 0.0f, 1.0f) * 255);
                p[2] = (uint8_t)std::lround(std::clamp(t.b, 0.0f, 1.0f) * 255);
                p[3] = (uint8_t)std::lround(std::clamp(t.a, 0.0f, 1.0f) * 255);
            }
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(gl3::TEXTURE_2D_ARRAY, tex);
    gl3::TexImage3D(gl3::TEXTURE_2D_ARRAY, 0, GL_RGBA8, TEX_PX, TEX_PX, L_COUNT, 0,
                    GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    gl3::GenerateMipmap(gl3::TEXTURE_2D_ARRAY);
    glTexParameteri(gl3::TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(gl3::TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(gl3::TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(gl3::TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(gl3::TEXTURE_2D_ARRAY, 0);
    return tex;
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
    gl3::VertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(CV), (const void*)offsetof(CV, uv));
    gl3::VertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(CV), (const void*)offsetof(CV, layer));
    for (GLuint a = 0; a < 5; ++a) gl3::EnableVertexAttribArray(a);
    gl3::BindVertexArray(0);
    return m;
}

} // namespace map3d_detail
