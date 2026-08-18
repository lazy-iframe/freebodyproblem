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
#include "theme.hpp"

// ─────────────────────────────────────────────────────────────────────────────
// ui_kit — tactical-display chrome primitives shared by every panel.
//
// The house style, in one place:
//   • every panel opens with a header strip: tracked uppercase title on the
//     left, dim right-aligned metadata, hairline underneath
//   • readouts are label-over-value: tiny tracked olive label, bright value
//   • everything is boxed with 1 px seams — no rounding, no gradients
// ─────────────────────────────────────────────────────────────────────────────

// ── Fonts (loaded in main.cpp; never null after font_kit_init) ───────────────
extern ImFont* g_font_ui;      // 16 px mono — body text / controls
extern ImFont* g_font_micro;   // 13 px mono — cell labels, log timestamps
extern ImFont* g_font_value;   // 26 px mono — big readouts / annunciators

// Font sizes used across panels.
constexpr float UI_SZ_MICRO = 13.0f;
constexpr float UI_SZ_BODY  = 16.0f;
constexpr float UI_SZ_VALUE = 25.0f;
constexpr float UI_SZ_BIG   = 29.0f;

// Letter-spacing applied to uppercase labels (px, at UI_SZ_MICRO).
constexpr float UI_TRACK = 1.1f;

// Standard chrome metrics.
constexpr float UI_HEADER_H = 25.0f;   // panel header strip
constexpr float UI_CELL_PAD = 8.0f;

// ── Colors derived from the active theme ─────────────────────────────────────

inline ImU32 ui_col(ImVec4 c, float alpha = 1.0f)
{
    c.w *= alpha;
    return ImGui::ColorConvertFloat4ToU32(c);
}

inline ImU32 ui_col_label()   { return ui_col(g_theme.col_log); }        // dim olive
inline ImU32 ui_col_value()   { return ui_col(g_theme.col_data); }       // pale green
inline ImU32 ui_col_accent()  { return ui_col(g_theme.accent); }         // amber
inline ImU32 ui_col_seam()    { return ui_col(g_theme.separator, 0.9f); }
inline ImU32 ui_col_strip()   { return ui_col(g_theme.bg_child_dark); }
inline ImU32 ui_col_well()    { return ui_col(g_theme.bg_child_darker); }
inline ImU32 ui_col_text()    { return ImGui::GetColorU32(ImGuiCol_Text); }

// ── Tracked (letter-spaced) text ─────────────────────────────────────────────

// Width of `text` drawn with per-glyph tracking.
float ui_tracked_width(ImFont* font, float size, const char* text, float track = UI_TRACK);

// Draw `text` glyph-by-glyph with extra advance between characters.
void ui_tracked_text(ImDrawList* dl, ImFont* font, float size, ImVec2 pos,
                     ImU32 col, const char* text, float track = UI_TRACK);

// ── Panel chrome ─────────────────────────────────────────────────────────────

// Header strip across the full content width at the current cursor: tracked
// title on the left, optional dim metadata right-aligned, hairline below.
// Advances the ImGui cursor past the strip.
void ui_panel_header(const char* title, const char* right_meta = nullptr);

// Same, but drawn at an explicit screen rectangle (for overlays on the map /
// video feed, where there is no ImGui layout cursor to advance).
void ui_panel_header_at(ImDrawList* dl, ImVec2 p0, float width,
                        const char* title, const char* right_meta,
                        float bg_alpha = 1.0f, float height = UI_HEADER_H);

// 1 px seam rectangle.
void ui_frame(ImDrawList* dl, ImVec2 p0, ImVec2 p1, ImU32 col);

// ── Readouts ─────────────────────────────────────────────────────────────────

// Label-over-value cell drawn into an arbitrary rect: tiny tracked label at the
// top-left, value below it in `value_size`.
void ui_readout(ImDrawList* dl, ImVec2 p0, ImVec2 p1,
                const char* label, const char* value,
                ImU32 value_col, float value_size = UI_SZ_VALUE);

// Boxed variant: well background + seam, optionally hover-lit.
void ui_readout_tile(ImDrawList* dl, ImVec2 p0, ImVec2 p1,
                     const char* label, const char* value,
                     ImU32 value_col, bool hovered,
                     float value_size = UI_SZ_VALUE);

// Status block — solid slab of colour with dark tracked text (topbar state
// annunciators: ARMED / DISARMED / NO LINK).
void ui_status_block(ImDrawList* dl, ImVec2 p0, ImVec2 p1,
                     const char* text, ImVec4 fill, bool outline_only = false,
                     float text_size = UI_SZ_BIG);

// ── Controls ─────────────────────────────────────────────────────────────────

// Flat tactical button: dark plate, 1 px seam, tracked uppercase caption.
// `active` fills it with amber and inverts the caption.
// The chrome idiom shared by the 3x3 telemetry tiles and every button in the
// app: a dark well that lifts to the header strip on hover, with the seam
// brightening to accent and the caption picking it up too.
//
// `size` follows ImGui::Button's conventions — 0 fits the label / one frame
// tall, negative means available space minus that much.
//
//   active — persistent "current selection" (tab open, flight mode engaged);
//            paints the bright amber plate and pins the seam to accent
//   flash  — transient command-ACK feedback; overrides the fill only, so the
//            hover response stays identical in every state
bool ui_grid_button(const char* label, ImVec2 size, bool active = false,
                    CmdFlashState flash = CmdFlashState::Normal);

// Same chrome and the same hover response, but keeping a semantic fill instead
// of the neutral well — for buttons whose colour carries meaning on its own
// (arm/disarm, disconnect, stop, destructive actions).
bool ui_solid_button(const char* label, ImVec2 size, ImVec4 fill, ImVec4 fill_hov,
                     CmdFlashState flash = CmdFlashState::Normal);

bool ui_tab_button(const char* label, ImVec2 size, bool active);

// ── Dialogs ──────────────────────────────────────────────────────────────────
//
// Every popup shares one shape: generous padding, an amber tracked title over a
// hairline, centred body text, and a centred button row. Push the style around
// the Begin/End pair — ImGui reads padding when the window is created, and the
// pop must run even when the popup is closed.

constexpr float UI_DIALOG_W  = 330.0f;   // default dialog content width
constexpr float UI_DIALOG_BH =  30.0f;   // dialog button height

void ui_push_dialog_style();
void ui_pop_dialog_style();

// Centred tracked amber title + hairline. `width` pins the dialog's minimum
// content width so auto-resized popups don't come out ragged.
void ui_dialog_title(const char* title, float width = UI_DIALOG_W);

// Single line of centred dialog text.
void ui_dialog_text(const char* text, ImU32 col);

// Centre the cursor for a row of total width `row_w`.
void ui_dialog_row(float row_w);

// Confirm / cancel dialog, driven by an OpenPopup elsewhere. Draws the title,
// question, a centred [confirm][cancel] row and the key hints, and closes
// itself on either answer.  Returns what the operator chose this frame.
enum class UiConfirm { None, Confirmed, Cancelled };

UiConfirm ui_confirm_popup(const char* id, const char* title, const char* question,
                           const char* confirm_label, ImVec4 confirm_col,
                           bool modal = false);
