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
static constexpr const char* THEME_BUILTIN_NAME  = "Retro Amber";
static constexpr const char* THEME_DRACULA_NAME  = "Dracula";

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
