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


#include "geodesic_grid.hpp"

#include <cmath>

namespace {

// The icosahedron's first ten triangles, in ArduPilot's order and winding,
// transcribed from the class comment in AP_GeodesicGrid.h. The units are the
// golden ratio and one; the vectors are normalised below.
//
// The order is not a convention that can be guessed at. It was checked by
// building the matrix whose columns are each triangle's vertices, inverting it,
// and comparing against the _inverses table generated into
// AP_GeodesicGrid.cpp — and the same for the middle triangles against
// _mid_inverses. Both agree to 1e-8, and every sub-triangle centroid below
// classifies back to its own index under the sign rule in
// AP_GeodesicGrid::_subtriangle_index — 80 of 80, and 20000 random directions
// each landing inside the very triangle that arithmetic names for them. Change
// the order here and the sphere still looks like a sphere; it just lights up in
// the wrong places.
//
// Not a thing to check by eye: consecutive section indices are mostly not
// neighbours on the sphere. The four sub-triangles of a face meet only at
// midpoints, and section 40 is the antipode of section 0, so walking 0 to 79
// hops about by a third of a turn even when everything is right.
//
// Triangles 10-19 are the antipodes of 0-9: T_(i+10) = -T_i. That is why only
// ten are written down, and why forgetting the negation gives half a sphere.
constexpr double G = 1.6180339887498949;   // (1 + sqrt(5)) / 2

const double kFaces[10][3][3] = {
    { { -G,  1,  0 }, { -1,  0, -G }, { -G, -1,  0 } },
    { { -1,  0, -G }, { -G, -1,  0 }, {  0, -G, -1 } },
    { { -G, -1,  0 }, {  0, -G, -1 }, {  0, -G,  1 } },
    { { -1,  0, -G }, {  0, -G, -1 }, {  1,  0, -G } },
    { {  0, -G, -1 }, {  0, -G,  1 }, {  G, -1,  0 } },
    { {  0, -G, -1 }, {  1,  0, -G }, {  G, -1,  0 } },
    { {  G, -1,  0 }, {  1,  0, -G }, {  G,  1,  0 } },
    { {  1,  0, -G }, {  G,  1,  0 }, {  0,  G, -1 } },
    { {  1,  0, -G }, {  0,  G, -1 }, { -1,  0, -G } },
    { {  0,  G, -1 }, { -G,  1,  0 }, { -1,  0, -G } },
};

struct Vec3 { double x, y, z; };

Vec3 mid(const Vec3& a, const Vec3& b)
{
    return { (a.x + b.x) * 0.5, (a.y + b.y) * 0.5, (a.z + b.z) * 0.5 };
}

Vec3 unit(const Vec3& v)
{
    const double n = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (n <= 0.0) return { 0.0, 0.0, 0.0 };
    return { v.x / n, v.y / n, v.z / n };
}

void store(GeodesicSection& s, int i, const Vec3& v)
{
    const Vec3 u = unit(v);
    s.v[i][0] = (float)u.x;
    s.v[i][1] = (float)u.y;
    s.v[i][2] = (float)u.z;
}

GeodesicSection g_sections[GEODESIC_SECTION_COUNT];
bool            g_built = false;

void build()
{
    for (int face = 0; face < 20; ++face) {
        const double sign = (face < 10) ? 1.0 : -1.0;   // T_(i+10) = -T_i
        const auto&  raw  = kFaces[face % 10];

        const Vec3 a{ sign * raw[0][0], sign * raw[0][1], sign * raw[0][2] };
        const Vec3 b{ sign * raw[1][0], sign * raw[1][1], sign * raw[1][2] };
        const Vec3 c{ sign * raw[2][0], sign * raw[2][1], sign * raw[2][2] };

        // M = (m_a, m_b, m_c) = ((a+b)/2, (b+c)/2, (c+a)/2)
        const Vec3 ma = mid(a, b);
        const Vec3 mb = mid(b, c);
        const Vec3 mc = mid(c, a);

        // W_0 = M, W_1 = (a, m_a, m_c), W_2 = (m_a, b, m_b), W_3 = (m_c, m_b, c)
        const Vec3 w[4][3] = {
            { ma, mb, mc },
            { a,  ma, mc },
            { ma, b,  mb },
            { mc, mb, c  },
        };

        for (int j = 0; j < 4; ++j) {
            GeodesicSection& s = g_sections[face * 4 + j];   // s = 4 * i + j
            for (int k = 0; k < 3; ++k) store(s, k, w[j][k]);
        }
    }
    g_built = true;
}

} // namespace

const GeodesicSection* geodesic_sections()
{
    if (!g_built) build();
    return g_sections;
}

bool geodesic_section_covered(const uint8_t* mask, int s)
{
    if (!mask || s < 0 || s >= GEODESIC_SECTION_COUNT) return false;
    return (mask[s / 8] & (1u << (s % 8))) != 0;
}

int geodesic_covered_count(const uint8_t* mask)
{
    if (!mask) return 0;
    int n = 0;
    for (int s = 0; s < GEODESIC_SECTION_COUNT; ++s)
        if (geodesic_section_covered(mask, s)) ++n;
    return n;
}
