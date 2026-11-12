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


// frontend/source/cache_dir.hpp
//
// Where downloaded tiles are kept across sessions: map imagery and terrain
// heights each get a folder under one per-user cache root.

#pragma once
#include <string>

// The cache folder for `name`, e.g. "tiles" or "terrain":
//   Windows   %LOCALAPPDATA%\freebodyproblem\<name>
//   elsewhere $XDG_CACHE_HOME/freebodyproblem/<name>, else ~/.cache/freebodyproblem/<name>
// Falls back to a folder in the working directory when none of those is known.
// Nothing is created here: tile_disk_save() (lib/tile_fetch.hpp) makes the
// folders a tile goes in as it saves it.
std::string app_cache_dir(const char* name);

// Where what the app saves for the operator goes: telemetry logs in tlogs/,
// video snapshots in snapshots/, and where the parameter file browser starts.
// Not under the cache: a tile can be fetched again and a flight cannot, so this
// is somewhere nothing clears on its own. Unless the operator has chosen a
// folder (SETTINGS, FILES):
//   Windows   Documents\freebodyproblem
//   macOS     ~/Documents/freebodyproblem
//   elsewhere $XDG_DATA_HOME/freebodyproblem, else ~/.local/share/freebodyproblem
// Falls back to the working directory. Folders are made by the first file saved.
std::string app_default_save_dir();

// The folder in use: `chosen`, or the default when that is empty. UI thread.
void        save_dir_set(const std::string& chosen);
// That folder, or the `sub` folder of it: save_dir("tlogs").
std::string save_dir(const char* sub = nullptr);
