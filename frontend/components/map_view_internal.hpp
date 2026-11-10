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


// frontend/components/map_view_internal.hpp
//
// What the 2D map shares with the 3D view drawn in its place
// (map_view_3d.cpp): the imagery tile cache, the trails, and the symbols.
// Not for anything else — the public face of the map is map_view.hpp.

#pragma once
#include "frontend/components/map_view.hpp"
#include "frontend/components/ui_kit.hpp"
#include <deque>
#include <string>
#include <vector>

// One trail sample. The altitude is AMSL, for the 3D view to draw the trail
// where the aircraft flew rather than along the ground.
struct TrackPoint {
    double lat, lon;
    float  alt_amsl;
};

// The amber the map draws its own marks in — the target, the aircraft labels,
// the forward axis, the north pointer. The theme's accent, except on a light
// theme: its accent is picked to read on a pale panel, and over satellite
// imagery a dark orange reads as red. There the map keeps the default amber.
inline bool map_theme_is_light()
{
    const ImVec4 bg = g_theme.bg_map;
    return 0.299f * bg.x + 0.587f * bg.y + 0.114f * bg.z > 0.5f;
}
inline ImVec4 map_amber()
{
    if (map_theme_is_light()) return { 0.925f, 0.725f, 0.306f, 1.0f };   // Tactical's #ECB94E
    return g_theme.accent;
}
inline ImU32 map_amber_u32(float alpha = 1.0f) { return ui_col(map_amber(), alpha); }


// The imagery tile (z, x, y) as a GL texture, or 0 while it is not loaded —
// in which case `queue` asks for it. Main (GL) thread only.
unsigned map_tile_texture(int z, int x, int y, bool queue);

// Drop every queued imagery request except these, as (z, x, y). The 3D view
// calls it each frame with the tiles it drew: as the camera moves the tiles
// it wants change, and a queue still fetching the old ones in order would
// leave the new ones waiting behind tiles no longer on screen.
struct MapTileId { int z, x, y; };
void map_tile_queue_retain(const std::vector<MapTileId>& keep);

// The trail of one vehicle, or nullptr if it has none.
const std::deque<TrackPoint>* map_track_of(VehicleId id);

// The helipad the 2D map marks home with, centred on `c`.
void map_draw_home_symbol(ImDrawList* dl, ImVec2 c, bool active);

// "HOME 1.24 km": how far (lat, lon) is from this vehicle's home over the ground, by the
// geodesic. Empty with no vehicle, or none that has told us its home.
std::string map_home_distance(const MapVehicle* mv, double lat, double lon);

// A thin dotted line, by the same dots everywhere on the maps.
void map_dotted_line(ImDrawList* dl, ImVec2 a, ImVec2 b, ImU32 col, float w);

// From home to the vehicle, at the screen points given: dotted cyan and "LOS", or red
// and "NO LOS" with the terrain data in the way, "LOS ?" while that is still loading.
// Nothing when the two are too close on screen for a label to fit.
void map_draw_los(ImDrawList* dl, ImVec2 home, ImVec2 vehicle, const MapVehicle& mv);

// The airframe a vehicle is drawn as, under the icon style the settings chose.
enum class SymbolKind { Arrow, Plane, Multicopter, Helicopter };
SymbolKind map_symbol_kind(uint8_t mav_type);

// The right edge of the A/C block as last drawn, screen x — for the 3D view's
// banner to keep clear of it.
float map_ac_block_right();

// An instruction for the operator across the top of the map — "click the
// ground…", "EDIT — drag…" — as wide as its text, but at most half the map
// rectangle (p0, size), wrapped to that and centred line by line; the box
// centred but never over the A/C block. Styled as the radio tab's "parameters not
// fetched" notice — a tinted box, a hairline border in the same colour, the
// text centred in it — but in the theme's green, since it guides rather than
// warns. Drawn on the foreground, over the corner block that floats on a
// fullscreen map.
void map_draw_instruction(ImVec2 p0, ImVec2 size, const char* text);

// A one-line label in the same green box, its left edge at `left_mid` and
// centred on it vertically — the GO HERE readouts and the vehicle labels, on
// both maps. In `col` when given, and faded by `alpha`.
void   map_draw_tag(ImDrawList* dl, ImVec2 left_mid, const char* text);
void   map_draw_tag(ImDrawList* dl, ImVec2 left_mid, const char* text, ImVec4 col, float alpha = 1.0f);
ImVec2 map_tag_size(const char* text);

// A vehicle's labels as tags, centred over `centre`, the lower one's bottom
// edge `above` px up: "(2) SYS 1", and under it its height above the terrain
// data — in red, and saying so, when it is inside the terrain. Above the
// vehicle, so they never sit on the line dropped from it to the ground.
// Dimmed for an inactive vehicle. The 2D map and the 3D view draw the same.
void map_draw_vehicle_tags(ImDrawList* dl, ImVec2 centre, float above, const MapVehicle& mv);

// The imagery source's attribution line, as the 2D map shows it.
std::string map_tile_attribution();
