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

// frontend/components/map/map3d/heights.cpp
//
// Height textures: each drawn tile's 33×33 vertex heights, read from the
// terrain tiles and kept on the GPU while the tile is in use.

#include "frontend/components/map/map3d/map3d_internal.hpp"
#include "frontend/source/terrain.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

namespace map3d_detail {

// ── Height textures ───────────────────────────────────────────────────────────

std::unordered_map<Key, Heights, KeyHash> s_heights;
uint64_t s_frame = 0;

// The tile's 33×33 vertex heights from terrain tiles at zoom tz, or false if
// any terrain tile it touches is not loaded yet (asking for those that are
// not). Each vertex reads the terrain at its own position, interpolated
// between pixel centres, so a vertex on the edge two tiles share comes out
// the same from both and they meet without a crack.
static bool sample_heights(const Key& k, int tz, std::vector<float>& out)
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

} // namespace map3d_detail
