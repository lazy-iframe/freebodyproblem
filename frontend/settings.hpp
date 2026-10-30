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
#include <map>
#include <string>
#include <vector>
#include "terrain.hpp"
#include "widgets/theme.hpp"

// ─────────────────────────────────────────────────────────────────────────────
// settings.hpp — Persistent application settings
//
// Saved to:  ~/.config/freebodyproblem/settings.json  (Linux / macOS)
//            %APPDATA%/freebodyproblem/settings.json   (Windows)
//
// Designed to grow: theme presets live here now; connection profiles and other
// user preferences will be added to AppSettings in future sections.
// ─────────────────────────────────────────────────────────────────────────────

// Names reserved for hard-coded built-in themes — never removed from the UI.
static constexpr const char* THEME_BUILTIN_NAME  = "Tactical";
static constexpr const char* THEME_GRUVBOX_NAME  = "Gruvbox Light";
static constexpr const char* THEME_AMBER_NAME    = "Retro Amber";
static constexpr const char* THEME_MATRIX_NAME   = "Matrix";

// The built-in themes, in the order the picker shows them. This is the one
// list: startup, the picker and REVERT all resolve a built-in through it.
struct BuiltinTheme { const char* name; ThemeVars (*factory)(); };
extern const BuiltinTheme k_builtin_themes[];
extern const size_t       k_builtin_theme_count;

// Saved connection profile (type: 0=Serial, 1=TCP, 2=UDP — matches conn_tab).
struct ConnectionProfile {
    std::string name;
    int         type   = 2;
    std::string host   = "0.0.0.0";
    int         port   = 14550;
    std::string device;
    int         baud   = 57600;
};

// Persistent configuration for one data-grid cell.
struct GridCellPersist {
    uint32_t    msg_id     = UINT32_MAX;  // UINT32_MAX = empty
    std::string field_name;
    std::string title;
};

struct AppSettings {
    // ── Themes ────────────────────────────────────────────────────────────────
    std::string                    active_theme = THEME_BUILTIN_NAME;
    std::map<std::string, ThemeVars> themes;     // user-saved presets (not the builtin)

    // ── Data grid ─────────────────────────────────────────────────────────────
    GridCellPersist grid_cells[9];

    // ── Connection profiles ───────────────────────────────────────────────────
    std::vector<ConnectionProfile> connection_profiles;

    // ── Audio ─────────────────────────────────────────────────────────────────
    bool  audio_enabled = true;
    float audio_volume  = 0.7f;   // linear 0..1

    // ── HUD style ─────────────────────────────────────────────────────────────
    // true: the full-panel HUD with speed / heading / altitude tapes.
    // false: the classic round attitude ball.
    bool hud_tapes_main  = true;    // right sidebar and fullscreen-map overlay
    bool hud_tapes_swarm = false;   // swarm cards

    // ── Map ───────────────────────────────────────────────────────────────────
    int map_icon_style = 1;   // MapIconStyle: 0 legacy arrow, 1 by vehicle type, 2-4 forced

    // ── CRT effects (cosmetic) ────────────────────────────────────────────────
    bool  crt_scanlines         = false;
    float crt_scanline_strength = 0.5f;   // 0..1
    bool  crt_vignette          = false;
    float crt_vignette_strength = 0.5f;   // 0..1

    // ── MCP server (backend/mcp_server.hpp) ──────────────────────────────────
    // Off unless asked for. Loopback only; the token is generated the first
    // time the server starts and kept, so a configured client keeps working.
    bool        mcp_enabled         = false;
    int         mcp_port            = 8737;
    std::string mcp_token;
    // Off: the assistant can only read. On: it can act, and each action in
    // mcp_auto_allow that is true runs without asking — the rest wait for the
    // operator to ALLOW them on screen. Keyed by MCP tool name (mcp_actions()).
    bool                        mcp_actions_enabled = false;
    std::map<std::string, bool> mcp_auto_allow;
    // Free text from the operator — site rules, limits, how to behave — sent
    // to every agent when it connects and returned by get_guidelines.
    std::string                 mcp_instructions;

    // ── Terrain ───────────────────────────────────────────────────────────────
    // The least height above the terrain data a planned path may come: a
    // mission leg or a GO HERE line closer than this warns.
    float terrain_clearance_m = 30.0f;

    // Where terrain heights come from: a {z}/{x}/{y} URL of PNG tiles, how
    // their pixels encode height (TerrainEncoding: 0 Terrarium, 1 Terrain-RGB),
    // and the line the map credits it with — its accuracy included.
    std::string terrain_url         = TERRAIN_DEFAULT_URL;
    int         terrain_encoding    = (int)TerrainEncoding::Terrarium;
    std::string terrain_attribution = TERRAIN_DEFAULT_ATTRIBUTION;

    // ── Map tile source ───────────────────────────────────────────────────────
    std::string tile_url         = "https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/{z}/{y}/{x}";
    std::string tile_attribution = "\xc2\xa9 Esri \xe2\x80\x94 Source: Esri, Maxar, Earthstar Geographics";
};

// Returns the path to the settings JSON file for this platform.
std::string settings_path();

// Reads settings.json and fills `out`.  Silently uses defaults on any error.
void settings_load(AppSettings& out);

// Writes the full settings state to disk.  Creates parent directories as needed.
void settings_save(const AppSettings& s);

// ── Themes ───────────────────────────────────────────────────────────────────

bool theme_is_builtin(const std::string& name);

// The colours `name` was last saved with: the factory colours for a built-in,
// the stored copy for a user theme. False if there is no such theme.
bool theme_lookup(const AppSettings& s, const std::string& name, ThemeVars& out);
