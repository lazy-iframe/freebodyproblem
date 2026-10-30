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


// frontend/cache_dir.hpp
//
// Where downloaded tiles are kept across sessions: map imagery and terrain
// heights each get a folder under one per-user cache root.

#pragma once
#include <string>

// The cache folder for `name`, e.g. "tiles" or "terrain":
//   Windows   %LOCALAPPDATA%\freebodyproblem\<name>
//   elsewhere $XDG_CACHE_HOME/freebodyproblem/<name>, else ~/.cache/freebodyproblem/<name>
// Falls back to a folder in the working directory when none of those is known.
// Nothing is created here; see mkdirs_for_file().
std::string app_cache_dir(const char* name);

// Create every directory leading to `filepath` (best-effort, errors ignored).
void mkdirs_for_file(const std::string& filepath);
