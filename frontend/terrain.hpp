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


// frontend/terrain.hpp
//
// Ground height, from elevation tiles: PNGs on the same Web Mercator
// {z}/{x}/{y} grid as the map's imagery, each pixel's RGB encoding metres
// above mean sea level. Two encodings are read:
//
//   Terrarium    (R·256 + G + B/256) − 32768          — AWS Terrain Tiles, the default
//   Terrain-RGB  (R·65536 + G·256 + B) · 0.1 − 10000  — Mapbox, MapTiler and others
//
// The default is mostly SRTM underneath — about 30 m across — and checked
// against Copernicus GLO-30 and Google Earth it agrees within about 25 m in
// steep mountains. It shows the lie of the land, not a safe clearance over
// trees or buildings; its attribution line says as much.
//
// Tiles are fetched on background threads and kept on disk across sessions
// beside the map's, so nothing here ever blocks: a lookup that misses queues
// its tile and answers "not yet", and the caller asks again next frame.

#pragma once
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

// Terrain tiles go no deeper than this.
constexpr int TERRAIN_MAX_ZOOM = 15;

// Where tiles come from, and how their pixels are read. Stored in settings as
// the int, so the numbering is fixed.
enum class TerrainEncoding : int { Terrarium = 0, TerrainRgb = 1 };

constexpr char TERRAIN_DEFAULT_URL[] =
    "https://s3.amazonaws.com/elevation-tiles-prod/terrarium/{z}/{x}/{y}.png";
constexpr char TERRAIN_DEFAULT_ATTRIBUTION[] =
    "Terrain: AWS Terrain Tiles (SRTM, USGS & others) \xc2\xb7 \xc2\xb1" "25 m";

// Switch the source. Tiles from the old one are dropped, from memory and from
// anything built on them (see terrain_generation); each source keeps its own
// disk cache, so switching back finds its tiles still there. `url` takes
// {z} {x} {y}. Tiles must be 256 or 512 px PNG (512 is halved).
void terrain_set_source(const std::string& url, TerrainEncoding enc, const std::string& attribution);

// The attribution line for the source in use: who made the data, and how far
// to trust it.
std::string terrain_attribution();

// Goes up by one each time the source changes. Anything cached from terrain
// heights — the 3D view's meshes, path clearances — is stale once it moves.
uint64_t terrain_generation();

// The zoom point lookups read: ~38 m a pixel at the equator, about SRTM's own
// spacing, and a tile ~10 km across, so a whole mission is a handful of tiles.
constexpr int TERRAIN_LOOKUP_ZOOM = 12;

// One decoded tile: TERRAIN_TILE_PX² heights in metres AMSL, row-major from
// the tile's north-west corner, each the value at that pixel's centre.
constexpr int TERRAIN_TILE_PX = 256;
struct TerrainTile {
    int                x = 0, y = 0, z = 0;
    std::vector<float> h;

    float at(int px, int py) const { return h[(size_t)py * TERRAIN_TILE_PX + px]; }
};

// The tile at (z, x, y), or nullptr while it loads — the call queues it.
// A tile that failed to load is asked for again after a pause, so an operator
// who planned offline gets heights once the link comes back.
// Safe from any thread; z above TERRAIN_MAX_ZOOM returns nullptr.
std::shared_ptr<const TerrainTile> terrain_tile(int z, int x, int y);

// Ground height in metres AMSL under (lat, lon), interpolated between the four
// nearest pixels at TERRAIN_LOOKUP_ZOOM. nullopt while its tile loads.
std::optional<float> terrain_amsl(double lat, double lon);

// Stop the fetch threads. Call once at exit; lookups after it return nothing.
void terrain_shutdown();
