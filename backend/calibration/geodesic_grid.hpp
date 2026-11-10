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

#include <cstdint>

// ─────────────────────────────────────────────────────────────────────────────
// The 80 sections of ArduPilot's geodesic grid.
//
// MAG_CAL_PROGRESS carries completion_mask[10] — 80 bits, one per section of
// the sphere of directions the compass has been turned through. The sphere is
// an icosahedron tessellated once: 20 faces, each cut into 4 by the midpoints
// of its edges, every vertex pushed out onto the unit sphere. A bit is set when
// a sample's corrected field vector has fallen inside that section.
//
// This is the same tessellation ArduPilot's AP_GeodesicGrid builds, in the same
// order, because a section is only meaningful as the particular patch of sphere
// the vehicle means by it. The triangle table and the sub-triangle numbering
// come from libraries/AP_Math/AP_GeodesicGrid.h, and were checked against the
// _inverses and _mid_inverses matrices in the .cpp — see the note in
// geodesic_grid.cpp.
//
// The vectors are in body frame, the frame the mask itself is in: x forward,
// y right, z down. Nothing here talks to a link or draws anything.
// ─────────────────────────────────────────────────────────────────────────────

inline constexpr int GEODESIC_SECTION_COUNT = 80;

// One section: three unit vectors, counter-clockwise seen from outside.
struct GeodesicSection {
    float v[3][3];
};

// The 80 sections, section s at index s. Built once on first call.
const GeodesicSection* geodesic_sections();

// Whether the vehicle has covered section `s`. The bits are LSB-first within
// each byte, as ArduPilot writes them: mask[s / 8] & (1 << (s % 8)).
bool geodesic_section_covered(const uint8_t* mask, int s);

// How many of the 80 are covered.
int geodesic_covered_count(const uint8_t* mask);
