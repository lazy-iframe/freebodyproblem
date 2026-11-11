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

// frontend/components/sidebar_left/sensors/coverage_sphere.cpp
//
// The compass calibration's coverage, drawn as the sphere of directions
// the airframe has been turned through.

#include "frontend/components/sidebar_left/sensors/sensors_internal.hpp"
#include <cmath>
#include <cstdio>

namespace sensors_tab {

// ── The coverage sphere ──────────────────────────────────────────────────────
//
// MAG_CAL_PROGRESS carries 80 bits, one for each section of the sphere of
// directions the compass has been turned through — ArduPilot's geodesic grid,
// an icosahedron cut once. A percentage says how much is left; this says which
// part, which is the thing the operator can actually act on.
//
// Drawn rather than modelled: 80 triangles, projected orthographically, painted
// far-to-near so the far side shows through the near one. The vectors are in
// body frame and so is the view — the screen is the vehicle seen from in front,
// +Y to the right and +Z down, turned by dragging.
//
// Nothing here rotates with the vehicle's attitude. The mask is in body frame,
// and a sphere that turned with the airframe would be showing the operator
// gaps that are not where it says they are.

static float s_sphere_yaw   = 0.6f;
static float s_sphere_pitch = 0.35f;

void draw_coverage_sphere(const uint8_t* mask, float height)
{
    const GeodesicSection* sections = geodesic_sections();

    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const float  width  = ImGui::GetContentRegionAvail().x;
    ImGui::InvisibleButton("##mag_sphere", { width, height });

    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        const ImVec2 d = ImGui::GetIO().MouseDelta;
        s_sphere_yaw   += d.x * 0.012f;
        // Minus: the sphere follows the cursor. Dragging down brings the near
        // face down and the top of the sphere towards the viewer, which is what
        // a hand on a globe does.
        s_sphere_pitch -= d.y * 0.012f;
        if (s_sphere_pitch >  1.45f) s_sphere_pitch =  1.45f;
        if (s_sphere_pitch < -1.45f) s_sphere_pitch = -1.45f;
    }

    const ImVec2 centre { origin.x + width * 0.5f, origin.y + height * 0.5f };
    const float  radius = (width < height ? width : height) * 0.44f;

    const float cy = cosf(s_sphere_yaw),   sy = sinf(s_sphere_yaw);
    const float cp = cosf(s_sphere_pitch), sp = sinf(s_sphere_pitch);

    // Body frame in, screen out. Yaw about z (down), then pitch about y (right);
    // what is left of x points at the viewer and serves as depth.
    auto project = [&](const float* v, ImVec2& out, float& depth) {
        const float x1 =  v[0] * cy - v[1] * sy;
        const float y1 =  v[0] * sy + v[1] * cy;
        const float z1 =  v[2];
        const float x2 =  x1 * cp + z1 * sp;
        const float z2 = -x1 * sp + z1 * cp;
        out   = { centre.x + radius * y1, centre.y + radius * z2 };
        depth = x2;
    };

    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddCircleFilled(centre, radius * 1.02f,
                        ImGui::GetColorU32(bg_param_list()), 48);

    struct Facet { ImVec2 p[3]; float depth; int index; };
    Facet facets[GEODESIC_SECTION_COUNT];
    for (int s = 0; s < GEODESIC_SECTION_COUNT; ++s) {
        Facet& f = facets[s];
        f.index  = s;
        f.depth  = 0.0f;
        for (int k = 0; k < 3; ++k) {
            float d;
            project(sections[s].v[k], f.p[k], d);
            f.depth += d / 3.0f;
        }
    }

    // Painter's algorithm, far first. An insertion sort over eighty facets is
    // nothing next to the eighty filled triangles that follow it.
    for (int i = 1; i < GEODESIC_SECTION_COUNT; ++i) {
        Facet f = facets[i];
        int j = i - 1;
        while (j >= 0 && facets[j].depth > f.depth) { facets[j + 1] = facets[j]; --j; }
        facets[j + 1] = f;
    }

    const ImVec4 lit   = col_ok();
    const ImVec4 unlit = col_no_link_muted();
    const ImU32  edge  = ImGui::GetColorU32(col_separator());

    for (const Facet& f : facets) {
        const bool covered = geodesic_section_covered(mask, f.index);
        const bool near_   = f.depth >= 0.0f;

        // The far side is drawn faint rather than not at all: a gap behind the
        // vehicle is exactly the one the operator cannot see, and making them
        // orbit the view to find it defeats the point of drawing it.
        ImVec4 c = covered ? lit : unlit;
        c.w = near_ ? (covered ? 0.92f : 0.55f)
                    : (covered ? 0.30f : 0.14f);

        ImVec2 pts[3] = { f.p[0], f.p[1], f.p[2] };
        dl->AddConvexPolyFilled(pts, 3, ImGui::GetColorU32(c));
        if (near_)
            dl->AddPolyline(pts, 3, edge, ImDrawFlags_Closed, 1.0f);
    }

    dl->AddCircle(centre, radius * 1.02f, ImGui::GetColorU32(col_separator()),
                  48, 1.0f);

    // Which way the vehicle is facing in all this. Only the ends pointing at
    // the viewer are labelled; the others are behind the sphere.
    struct Axis { float v[3]; const char* name; };
    const Axis axes[] = {
        { {  1.f,  0.f,  0.f }, "NOSE"  },
        { {  0.f,  1.f,  0.f }, "RIGHT" },
        { {  0.f,  0.f,  1.f }, "DOWN"  },
    };
    for (const Axis& a : axes) {
        ImVec2 p; float d;
        project(a.v, p, d);
        if (d < 0.0f) continue;
        const ImVec2 at { p.x + 4.0f, p.y - ImGui::GetTextLineHeight() * 0.5f };
        dl->AddText(at, ImGui::GetColorU32(accent_col()), a.name);
    }

    ImGui::TextDisabled("%d / %d sections \xe2\x80\x94 drag to turn",
                        geodesic_covered_count(mask), GEODESIC_SECTION_COUNT);
}

} // namespace sensors_tab
