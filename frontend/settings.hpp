// Free Body Problem: A modern, simple and minimalist Ground Control Station
// Copyright (C) 2026  Azhar Tanweer
// Contact: azhar.tanweer404@gmail.com

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.


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
