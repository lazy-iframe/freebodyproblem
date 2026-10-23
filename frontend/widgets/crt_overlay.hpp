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

struct AppSettings;

// ─────────────────────────────────────────────────────────────────────────────
// crt_overlay — purely cosmetic old-CRT treatment laid over the whole window:
// scanlines in the theme's crt_scanline color with a little line noise and a
// slow rolling band, and darkened edges. Drawn on the foreground draw list, so
// it covers everything (popups included) and never takes a click.
// ─────────────────────────────────────────────────────────────────────────────

// Call once per frame, after all other UI, before ImGui::Render().
void draw_crt_overlay(const AppSettings& settings, double time_s);
