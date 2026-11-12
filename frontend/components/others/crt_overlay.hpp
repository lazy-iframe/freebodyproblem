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

#include "imgui.h"

struct AppSettings;

// ─────────────────────────────────────────────────────────────────────────────
// crt_overlay — purely cosmetic old-CRT treatment. Scanlines live inside the
// boxes and buttons, where a phosphor display would have lit them: still
// two-pixel lines in a lighter tone of each box's own colour, drawn by the UI
// kit into each plate, tile and header strip over its contents.
// Darkened edges lie over the whole window, on the foreground draw list, so
// they cover everything (popups included) and never take a click.
// ─────────────────────────────────────────────────────────────────────────────

// Call once per frame, after ImGui::NewFrame() and before any UI: takes this
// frame's effect settings.
void crt_begin_frame(const AppSettings& settings);

// The scanlines inside the box p0..p1, just within its 1 px frame, on `dl`
// — the box's own draw list, so whatever is drawn over the box later covers
// them too — in the colour of `fill`, the box's own: a red button's lines are
// red. Rows fall on the same screen rows in every box, so boxes side by side
// read as one raster. A no-op with scanlines off.
void crt_box_scanlines(ImDrawList* dl, ImVec2 p0, ImVec2 p1, ImVec4 fill);

// Call once per frame, after all other UI, before ImGui::Render().
void draw_crt_overlay(const AppSettings& settings);
