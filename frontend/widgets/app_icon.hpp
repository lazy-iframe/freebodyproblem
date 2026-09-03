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

#include <string>
#include <vector>

struct GLFWwindow;

// ─────────────────────────────────────────────────────────────────────────────
// The application icon — the top bar's callsign chip: amber plate, near-black
// FBP in the same font at the same tracking. The rendered set lives in
// assets/icons/ and installs to share/freebodyproblem/icons/.
//
// The sizes a window manager might want, smallest first. Callers resolve these
// through the same asset search the fonts use, so a build-tree run and an
// installed run both find them.
// ─────────────────────────────────────────────────────────────────────────────

inline constexpr int APP_ICON_SIZES[] = { 16, 24, 32, 48, 64, 128, 256 };
inline constexpr int APP_ICON_SIZE_COUNT =
    (int)(sizeof(APP_ICON_SIZES) / sizeof(APP_ICON_SIZES[0]));

// Decodes each file and hands the set to GLFW, which picks the size it wants.
// Silent about a file that is missing or will not decode, and about a platform
// that carries no window icon at all — Wayland has no protocol for one, and
// macOS takes it from the bundle. A window without its icon is not worth
// refusing to start over.
void app_set_window_icon(GLFWwindow* window,
                         const std::vector<std::string>& png_paths);
