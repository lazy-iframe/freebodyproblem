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
        ImGui::PushStyleColor(ImGuiCol_Button,        confirm_col);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hov);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  confirm_col);
        const bool hit_ok = ImGui::Button(confirm_label, { BW_OK, UI_DIALOG_BH });
        ImGui::PopStyleColor(3);

        ImGui::SameLine(0, GAP);

        char cancel_id[64];
        snprintf(cancel_id, sizeof(cancel_id), "CANCEL%s", id);
        push_flash_colors(CmdFlashState::Normal);
        const bool hit_no = ImGui::Button(cancel_id, { BW_NO, UI_DIALOG_BH });
        ImGui::PopStyleColor(3);

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

bool ui_tab_button(const char* label, ImVec2 size, bool active)
{
    ImDrawList*  dl = ImGui::GetWindowDrawList();
    const ImVec2 p0 = ImGui::GetCursorScreenPos();

    ImGui::InvisibleButton(label, size);
    const bool hovered = ImGui::IsItemHovered();
    const bool clicked = ImGui::IsItemClicked();

    const ImVec2 p1 = { p0.x + size.x, p0.y + size.y };

    if (active) {
        dl->AddRectFilled(p0, p1, ui_col(g_theme.btn_tab_active_base));
        ui_frame(dl, p0, p1, ui_col_accent());
    } else {
        dl->AddRectFilled(p0, p1, hovered ? ui_col_strip() : ui_col_well());
        ui_frame(dl, p0, p1, hovered ? ui_col(g_theme.accent, 0.7f) : ui_col_seam());
    }

    // Caption — strip any "##id" suffix before measuring / drawing.
    char caption[64];
    const char* hash = strstr(label, "##");
    const size_t n   = hash ? (size_t)(hash - label) : strlen(label);
    const size_t cn  = n < sizeof(caption) - 1 ? n : sizeof(caption) - 1;
    memcpy(caption, label, cn);
    caption[cn] = '\0';

    ImFont*     fu  = g_font_ui ? g_font_ui : ImGui::GetFont();
    const ImU32 col = active ? ui_col(g_theme.col_active_text)
                             : (hovered ? ui_col_accent() : ui_col_text());
    const float tw  = ui_tracked_width(fu, UI_SZ_BODY, caption, UI_TRACK * 0.6f);
    ui_tracked_text(dl, fu, UI_SZ_BODY,
                    { p0.x + (size.x - tw) * 0.5f,
                      p0.y + (size.y - UI_SZ_BODY) * 0.5f - 1.0f },
                    col, caption, UI_TRACK * 0.6f);

    return clicked;
}
