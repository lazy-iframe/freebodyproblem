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


#include "ui_kit.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>

ImFont* g_font_ui    = nullptr;
ImFont* g_font_micro = nullptr;
ImFont* g_font_value = nullptr;

// ── Tracked text ─────────────────────────────────────────────────────────────

// Advance `p` past one UTF-8 code point; returns the byte length of that glyph.
static int utf8_step(const char* p)
{
    const unsigned char c = (unsigned char)*p;
    int n = (c < 0x80) ? 1 : (c < 0xE0) ? 2 : (c < 0xF0) ? 3 : 4;
    // Guard against truncated sequences — never step past the terminator.
    for (int i = 1; i < n; ++i)
        if (p[i] == '\0') return i;
    return n;
}

float ui_tracked_width(ImFont* font, float size, const char* text, float track)
{
    if (!font || !text || !*text) return 0.0f;
    float w = 0.0f;
    for (const char* c = text; *c; ) {
        const int n = utf8_step(c);
        w += font->CalcTextSizeA(size, FLT_MAX, 0.0f, c, c + n).x + track;
        c += n;
    }
    return w - track;
}

void ui_tracked_text(ImDrawList* dl, ImFont* font, float size, ImVec2 pos,
                     ImU32 col, const char* text, float track)
{
    if (!dl || !font || !text) return;
    for (const char* c = text; *c; ) {
        const int n = utf8_step(c);
        dl->AddText(font, size, pos, col, c, c + n);
        pos.x += font->CalcTextSizeA(size, FLT_MAX, 0.0f, c, c + n).x + track;
        c += n;
    }
}

// ── Panel chrome ─────────────────────────────────────────────────────────────

void ui_frame(ImDrawList* dl, ImVec2 p0, ImVec2 p1, ImU32 col)
{
    dl->AddRect(p0, p1, col, 0.0f, 0, 1.0f);
}

void ui_panel_header_at(ImDrawList* dl, ImVec2 p0, float width,
                        const char* title, const char* right_meta,
                        float bg_alpha, float height)
{
    const ImVec2 p1 = { p0.x + width, p0.y + height };

    dl->AddRectFilled(p0, p1, ui_col(g_theme.bg_child_dark, bg_alpha));
    dl->AddLine({ p0.x, p1.y - 0.5f }, { p1.x, p1.y - 0.5f }, ui_col_seam(), 1.0f);

    ImFont* fm = g_font_micro ? g_font_micro : ImGui::GetFont();
    const float ty = p0.y + (height - UI_SZ_MICRO) * 0.5f - 1.0f;

    // Amber tick marking the head of the strip.
    dl->AddRectFilled({ p0.x + 4.0f, p0.y + 5.0f },
                      { p0.x + 6.0f, p1.y - 5.0f }, ui_col_accent());

    if (title)
        ui_tracked_text(dl, fm, UI_SZ_MICRO, { p0.x + 12.0f, ty },
                        ui_col_accent(), title);

    if (right_meta && *right_meta) {
        const float mw = ui_tracked_width(fm, UI_SZ_MICRO, right_meta);
        ui_tracked_text(dl, fm, UI_SZ_MICRO, { p1.x - mw - 8.0f, ty },
                        ui_col_label(), right_meta);
    }
}

void ui_panel_header(const char* title, const char* right_meta)
{
    ImDrawList*  dl    = ImGui::GetWindowDrawList();
    const ImVec2 p0    = ImGui::GetCursorScreenPos();
    const float  width = ImGui::GetContentRegionAvail().x;

    ui_panel_header_at(dl, p0, width, title, right_meta);

    ImGui::Dummy({ width, UI_HEADER_H });
    ImGui::Spacing();
}

// ── Readouts ─────────────────────────────────────────────────────────────────

void ui_readout(ImDrawList* dl, ImVec2 p0, ImVec2 p1,
                const char* label, const char* value,
                ImU32 value_col, float value_size)
{
    ImFont* fm = g_font_micro ? g_font_micro : ImGui::GetFont();
    ImFont* fv = g_font_value ? g_font_value : ImGui::GetFont();

    if (label && *label)
        ui_tracked_text(dl, fm, UI_SZ_MICRO,
                        { p0.x + UI_CELL_PAD, p0.y + 6.0f },
                        ui_col_label(), label);

    if (value && *value) {
        // Shrink the value until it fits the cell — telemetry strings vary a lot.
        const float max_w = (p1.x - p0.x) - UI_CELL_PAD * 2.0f;
        float size = value_size;
        while (size > 10.0f &&
               fv->CalcTextSizeA(size, FLT_MAX, 0.0f, value).x > max_w)
            size -= 1.0f;

        const float vy = p1.y - size - 7.0f;
        dl->AddText(fv, size, { p0.x + UI_CELL_PAD, vy }, value_col, value);
    }
}

void ui_readout_tile(ImDrawList* dl, ImVec2 p0, ImVec2 p1,
                     const char* label, const char* value,
                     ImU32 value_col, bool hovered,
                     float value_size)
{
    dl->AddRectFilled(p0, p1, hovered ? ui_col_strip() : ui_col_well());
    ui_frame(dl, p0, p1, hovered ? ui_col_accent() : ui_col_seam());
    ui_readout(dl, p0, p1, label, value, value_col, value_size);
}

void ui_status_block(ImDrawList* dl, ImVec2 p0, ImVec2 p1,
                     const char* text, ImVec4 fill, bool outline_only,
                     float text_size)
{
    ImFont* fv = g_font_value ? g_font_value : ImGui::GetFont();

    ImU32 text_col;
    if (outline_only) {
        ImVec4 bg = fill; bg.x *= 0.18f; bg.y *= 0.18f; bg.z *= 0.18f;
        dl->AddRectFilled(p0, p1, ui_col(bg));
        ui_frame(dl, p0, p1, ui_col(fill, 0.9f));
        text_col = ui_col(fill);
    } else {
        dl->AddRectFilled(p0, p1, ui_col(fill));
        text_col = ui_col(g_theme.col_active_text);
    }

    const float tw = ui_tracked_width(fv, text_size, text, UI_TRACK * 1.6f);
    ui_tracked_text(dl, fv, text_size,
                    { p0.x + (p1.x - p0.x - tw) * 0.5f,
                      p0.y + (p1.y - p0.y - text_size) * 0.5f },
                    text_col, text, UI_TRACK * 1.6f);
}

// ── Controls ─────────────────────────────────────────────────────────────────

// ── Dialogs ──────────────────────────────────────────────────────────────────

void ui_push_dialog_style()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,     { 20.0f, 18.0f });
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,       {  8.0f, 10.0f });
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize,  1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,     0.0f);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, g_theme.bg_panel);
    ImGui::PushStyleColor(ImGuiCol_Border,  ui_col(g_theme.accent, 0.55f));
}

void ui_pop_dialog_style()
{
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(4);
}

void ui_dialog_title(const char* title, float width)
{
    ImDrawList*  dl = ImGui::GetWindowDrawList();
    const ImVec2 p0 = ImGui::GetCursorScreenPos();
    ImFont*      fu = g_font_ui ? g_font_ui : ImGui::GetFont();

    // Reserve the dialog's minimum width before anything else is laid out.
    const float w  = std::max(width, ImGui::GetContentRegionAvail().x);
    const float tw = ui_tracked_width(fu, UI_SZ_BODY, title, UI_TRACK);

    ui_tracked_text(dl, fu, UI_SZ_BODY, { p0.x + (w - tw) * 0.5f, p0.y },
                    ui_col_accent(), title, UI_TRACK);
    dl->AddLine({ p0.x, p0.y + UI_SZ_BODY + 8.0f },
                { p0.x + w, p0.y + UI_SZ_BODY + 8.0f }, ui_col_seam(), 1.0f);

    ImGui::Dummy({ w, UI_SZ_BODY + 10.0f });
}

void ui_dialog_text(const char* text, ImU32 col)
{
    const float w  = ImGui::GetContentRegionAvail().x;
    const float tw = ImGui::CalcTextSize(text).x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::max(0.0f, (w - tw) * 0.5f));
    ImGui::PushStyleColor(ImGuiCol_Text, col);
    ImGui::TextUnformatted(text);
    ImGui::PopStyleColor();
}

void ui_dialog_row(float row_w)
{
    const float w = ImGui::GetContentRegionAvail().x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::max(0.0f, (w - row_w) * 0.5f));
}

UiConfirm ui_confirm_popup(const char* id, const char* title, const char* question,
                           const char* confirm_label, ImVec4 confirm_col, bool modal)
{
    UiConfirm result = UiConfirm::None;

    constexpr ImGuiWindowFlags kFlags = ImGuiWindowFlags_AlwaysAutoResize |
                                        ImGuiWindowFlags_NoTitleBar       |
                                        ImGuiWindowFlags_NoMove;

    ui_push_dialog_style();
    const bool open = modal ? ImGui::BeginPopupModal(id, nullptr, kFlags)
                            : ImGui::BeginPopup(id, kFlags);
    if (open) {
        ui_dialog_title(title);
        ui_dialog_text(question, ui_col_accent());
        ImGui::Spacing();

        const bool key_confirm = ImGui::IsKeyPressed(ImGuiKey_Enter, false)
                              || ImGui::IsKeyPressed(ImGuiKey_Y, false);
        const bool key_cancel  = ImGui::IsKeyPressed(ImGuiKey_N, false)
                              || ImGui::IsKeyPressed(ImGuiKey_Escape, false);

        constexpr float BW_OK = 168.0f, BW_NO = 104.0f, GAP = 10.0f;
        ui_dialog_row(BW_OK + GAP + BW_NO);

        ImVec4 hov = confirm_col;
        hov.x = std::min(1.0f, hov.x * 1.45f);
        hov.y = std::min(1.0f, hov.y * 1.45f);
        hov.z = std::min(1.0f, hov.z * 1.45f);
        // The confirm button keeps its caller-supplied colour: these dialogs
        // guard irreversible actions and the colour is the warning.
        const bool hit_ok = ui_solid_button(confirm_label, { BW_OK, UI_DIALOG_BH },
                                            confirm_col, hov);

        ImGui::SameLine(0, GAP);

        char cancel_id[64];
        snprintf(cancel_id, sizeof(cancel_id), "CANCEL%s", id);
        const bool hit_no = ui_grid_button(cancel_id, { BW_NO, UI_DIALOG_BH });

        ui_dialog_text("[Y / ENTER] CONFIRM    [N / ESC] CANCEL", ui_col_label());

        if (hit_ok || key_confirm) {
            result = UiConfirm::Confirmed;
            ImGui::CloseCurrentPopup();
        } else if (hit_no || key_cancel) {
            result = UiConfirm::Cancelled;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    ui_pop_dialog_style();

    return result;
}

// Caption colour for a filled plate. Near-black reads well on the bright amber
// active plate but vanishes on the dark reds and olives the semantic buttons
// use (btn_disconnect_base sits at luminance 0.13, the near-black text at
// 0.06), so the choice follows the fill rather than being fixed. Fills are
// blended against the panel first because several carry alpha < 1.
static ImU32 ui_plate_text_col(ImVec4 fill, float alpha)
{
    const ImVec4& bg = g_theme.bg_panel;
    const float   w  = fill.w;
    const float   r  = fill.x * w + bg.x * (1.0f - w);
    const float   g  = fill.y * w + bg.y * (1.0f - w);
    const float   b  = fill.z * w + bg.z * (1.0f - w);

    const float lum = 0.2126f * r + 0.7152f * g + 0.0722f * b;
    return lum < 0.45f ? ui_col(g_theme.col_text_on_dark, alpha)
                       : ui_col(g_theme.col_active_text,  alpha);
}

// Core of the shared button chrome. `base`/`base_hov` are the idle fills; the
// seam, caption, disabled fade and ACK flash are handled identically for every
// caller, which is what makes these read as one control family.
//
//   pinned — persistent state (tab open, mode engaged): seam stays accent
//   plated — the fill is opaque enough to carry the caption itself, so the
//            caption colour is chosen for contrast against it
//   tint   — optional replacement for the theme's amber accent on the seam and
//            the caption. Only the plugin rail passes one: same dark well, same
//            hover response, blue where the rest of the app goes amber.
//   cap_sz — caption height. Anything above body size is drawn from the big
//            readout atlas rather than scaling the 16 px one, the same trade
//            ui_readout and ui_status_block make for their large text.
struct PlateTint { ImVec4 seam_idle, seam_hot, text_idle, text_hot; };

static bool ui_plate_button(const char* label, ImVec2 size,
                            ImVec4 base, ImVec4 base_hov,
                            bool pinned, bool plated,
                            CmdFlashState flash,
                            const PlateTint* tint = nullptr,
                            float cap_sz = UI_SZ_BODY)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImFont*     fu = (cap_sz > UI_SZ_BODY && g_font_value) ? g_font_value
                   : g_font_ui ? g_font_ui : ImGui::GetFont();

    // Caption — strip any "##id" suffix before measuring / drawing.
    char caption[64];
    const char*  hash = strstr(label, "##");
    const size_t n    = hash ? (size_t)(hash - label) : strlen(label);
    const size_t cn   = n < sizeof(caption) - 1 ? n : sizeof(caption) - 1;
    memcpy(caption, label, cn);
    caption[cn] = '\0';

    const float track = UI_TRACK * 0.6f;
    const float tw    = ui_tracked_width(fu, cap_sz, caption, track);

    // Reproduce ImGui::Button's sizing conventions, because callers were
    // written against them: 0 means "fit the label" / one frame tall, and a
    // negative value means "available space minus that much". InvisibleButton
    // has neither — it takes an explicit size and asserts on a zero axis.
    const ImVec2 avail = ImGui::GetContentRegionAvail();
    const ImVec2 pad   = ImGui::GetStyle().FramePadding;
    if (size.x == 0.0f)      size.x = tw + pad.x * 2.0f;
    else if (size.x < 0.0f)  size.x = avail.x + size.x;
    if (size.y == 0.0f)      size.y = ImGui::GetFrameHeight();
    else if (size.y < 0.0f)  size.y = avail.y + size.y;
    if (size.x < 1.0f) size.x = 1.0f;
    if (size.y < 1.0f) size.y = 1.0f;

    const ImVec2 p0 = ImGui::GetCursorScreenPos();
    // InvisibleButton's own return value, not IsItemClicked(): the former fires
    // on release like ImGui::Button (so you can drag off to cancel), the latter
    // fires on press. Confirm dialogs depend on the release semantics.
    const bool clicked = ImGui::InvisibleButton(label, size);
    const bool hovered = ImGui::IsItemHovered();

    const ImVec2 p1 = { p0.x + size.x, p0.y + size.y };

    // Drawing straight to the draw list skips the fade ImGui applies to normal
    // widgets inside BeginDisabled(), so fold the style alpha in by hand.
    const float a = ImGui::GetStyle().Alpha;

    ImVec4 fill    = hovered ? base_hov : base;
    bool   on_plate = plated;

    switch (flash) {
    case CmdFlashState::Accepted:
        fill = hovered ? g_theme.flash_accepted_hov : g_theme.flash_accepted_base;
        on_plate = true;
        break;
    case CmdFlashState::Rejected:
        fill = hovered ? g_theme.flash_rejected_hov : g_theme.flash_rejected_base;
        on_plate = true;
        break;
    case CmdFlashState::Pending:
        fill = hovered ? g_theme.flash_pending_hov : g_theme.flash_pending_base;
        on_plate = true;
        break;
    default:
        break;
    }

    const ImVec4 seam_idle = tint ? tint->seam_idle : g_theme.separator;
    const ImVec4 seam_hot  = tint ? tint->seam_hot  : g_theme.accent;

    dl->AddRectFilled(p0, p1, ui_col(fill, a));
    ui_frame(dl, p0, p1, (hovered || pinned) ? ui_col(seam_hot, a)
                                             : ui_col(seam_idle, 0.9f * a));

    // A plated fill carries the caption itself, so a tint would fight the
    // contrast pick — the ACK flashes and the engaged red land here too.
    const ImU32 col = on_plate ? ui_plate_text_col(fill, a)
                    : tint     ? ui_col(hovered ? tint->text_hot : tint->text_idle, a)
                    : hovered  ? ui_col(g_theme.accent, a)
                               : ui_col_text();
    ui_tracked_text(dl, fu, cap_sz,
                    { p0.x + (size.x - tw) * 0.5f,
                      p0.y + (size.y - cap_sz) * 0.5f - 1.0f },
                    col, caption, track);

    return clicked;
}

bool ui_grid_button(const char* label, ImVec2 size, bool active,
                    CmdFlashState flash)
{
    if (active)
        return ui_plate_button(label, size, g_theme.btn_tab_active_base,
                               g_theme.btn_tab_active_hov, true, true, flash);
    return ui_plate_button(label, size, g_theme.bg_child_darker,
                           g_theme.bg_child_dark, false, false, flash);
}

bool ui_solid_button(const char* label, ImVec2 size, ImVec4 fill, ImVec4 fill_hov,
                     CmdFlashState flash, float caption_size)
{
    return ui_plate_button(label, size, fill, fill_hov, false, true, flash,
                           nullptr, caption_size);
}

bool ui_tinted_button(const char* label, ImVec2 size, ImVec4 tint, ImVec4 tint_dim,
                      float caption_size)
{
    const PlateTint t{ tint_dim, tint, tint_dim, tint };
    return ui_plate_button(label, size, g_theme.bg_child_darker,
                           g_theme.bg_child_dark, false, false,
                           CmdFlashState::Normal, &t, caption_size);
}

bool ui_tab_button(const char* label, ImVec2 size, bool active)
{
    return ui_grid_button(label, size, active);
}

// ── Progress ─────────────────────────────────────────────────────────────────

void ui_progress_bar(const char* text, float frac, float height)
{
    if (!(frac >= 0.0f)) frac = 0.0f;   // also catches NaN from a 0 denominator
    if (frac > 1.0f)     frac = 1.0f;

    const float  avail = ImGui::GetContentRegionAvail().x;
    const ImVec2 start = ImGui::GetCursorScreenPos();

    // Micro font for the caption, as everywhere else a number annotates a
    // control. g_font_micro is null when neither font file was found and ImGui
    // is on its built-in font; null is its documented "keep the current font",
    // and the size still applies, so that case needs no branch here.
    ImGui::PushFont(g_font_micro, UI_SZ_MICRO);

    char pct[8];
    snprintf(pct, sizeof(pct), "%d%%", (int)(frac * 100.0f + 0.5f));
    ImGui::TextDisabled("%s", pct);

    // Right-aligned on the same line. Placed by cursor rather than by an
    // ImGui::SameLine offset so it stays pinned to the panel edge at any
    // sidebar width.
    if (text && *text) {
        const float tw = ImGui::CalcTextSize(text).x;
        ImGui::SameLine(0, 0);
        ImGui::SetCursorScreenPos({ start.x + avail - tw, start.y });
        ImGui::TextDisabled("%s", text);
    }
    ImGui::PopFont();

    // Empty overlay, not nullptr: nullptr makes ImGui draw its own "%.0f%%"
    // inside the bar, which is the clipped text this widget exists to avoid.
    ImGui::ProgressBar(frac, { -1.0f, height }, "");
}
