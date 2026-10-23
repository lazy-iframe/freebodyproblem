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


#include "sidebar_themes.hpp"
#include "theme.hpp"
#include "ui_kit.hpp"
#include "../settings.hpp"
#include "../audio.hpp"
#include "map_view.hpp"
#include "imgui.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>

// ── Theme cards ───────────────────────────────────────────────────────────────
//
// Each card is drawn in the theme it names — its own panel, accent and status
// colours — so the picker shows what you would get rather than a word for it.
// The ring around it is in the *current* theme's accent, so selection reads
// the same whichever card it lands on.

static constexpr float CARD_H   = 56.0f;
static constexpr float CARD_GAP = 4.0f;

static bool theme_card(const std::string& name, const ThemeVars& t,
                       bool active, bool edited, bool builtin, ImVec2 size)
{
    ImGui::PushID(name.c_str());
    const bool hit = ImGui::InvisibleButton("##card", size);
    const bool hov = ImGui::IsItemHovered();
    ImGui::PopID();

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 p0 = ImGui::GetItemRectMin();
    const ImVec2 p1 = ImGui::GetItemRectMax();

    dl->AddRectFilled(p0, p1, ui_col(t.bg_panel));
    dl->AddRectFilled({ p0.x, p0.y + 4.0f }, { p1.x, p0.y + 22.0f }, ui_col(t.bg_child_dark));
    dl->AddRectFilled(p0, { p1.x, p0.y + 4.0f }, ui_col(t.accent));

    // EDITED sits top-right, and the name is clipped short of it.
    float name_right = p1.x - 8.0f;
    if (edited) {
        const char* tag = "EDITED";
        const float w = ui_tracked_width(g_font_micro, UI_SZ_MICRO, tag);
        ui_tracked_text(dl, g_font_micro, UI_SZ_MICRO, { p1.x - 8.0f - w, p0.y + 7.0f },
                        ui_col(g_theme.col_warning), tag);
        name_right -= w + 8.0f;
    }

    std::string upper = name;
    for (char& ch : upper) ch = (char)std::toupper((unsigned char)ch);
    dl->PushClipRect({ p0.x + 8.0f, p0.y }, { name_right, p1.y }, true);
    ui_tracked_text(dl, g_font_micro, UI_SZ_MICRO, { p0.x + 8.0f, p0.y + 7.0f },
                    ui_col(t.accent), upper.c_str());
    dl->PopClipRect();

    // Status colours as the theme uses them: data, ok, warning, error.
    const ImVec4 chips[] = { t.col_data, t.col_ok, t.col_warning, t.col_error };
    for (int i = 0; i < 4; ++i) {
        const ImVec2 c0 = { p0.x + 8.0f + i * 14.0f, p1.y - 20.0f };
        dl->AddRectFilled(c0, { c0.x + 10.0f, c0.y + 10.0f }, ui_col(chips[i]));
    }

    const char* kind = builtin ? "BUILT-IN" : "CUSTOM";
    const float kw = ui_tracked_width(g_font_micro, UI_SZ_MICRO, kind);
    ui_tracked_text(dl, g_font_micro, UI_SZ_MICRO, { p1.x - 8.0f - kw, p1.y - 22.0f },
                    ui_col(t.col_log), kind);

    if (active)
        dl->AddRect(p0, p1, ui_col(g_theme.accent), 0.0f, 0, 2.0f);
    else
        dl->AddRect(p0, p1, hov ? ui_col(g_theme.accent, 0.6f) : ui_col_seam());

    if (hov)
        ImGui::SetTooltip(builtin ? "%s \xe2\x80\x94 built-in theme" : "%s \xe2\x80\x94 your theme",
                          name.c_str());
    return hit;
}

// ── Theme name prompt ─────────────────────────────────────────────────────────
//
// Shared by SAVE AS and RENAME. Says why a name is refused instead of just
// greying the button, and says so when a name will replace a saved theme.
// Returns true on the frame a usable name is committed, written to `out`.

static std::string trimmed(const char* s)
{
    std::string r = s;
    const size_t b = r.find_first_not_of(' ');
    if (b == std::string::npos) return {};
    return r.substr(b, r.find_last_not_of(' ') - b + 1);
}

static bool theme_name_popup(const char* id, const char* title, const char* verb,
                             const std::string& initial, const std::string& current,
                             const AppSettings& settings, std::string& out)
{
    bool committed = false;
    ui_push_dialog_style();
    if (ImGui::BeginPopup(id)) {
        constexpr float BODY_W = 280.0f;
        ui_dialog_title(title, BODY_W);

        static char buf[64] = {};
        if (ImGui::IsWindowAppearing()) {
            std::snprintf(buf, sizeof(buf), "%s", initial.c_str());
            ImGui::SetKeyboardFocusHere();
        }
        ImGui::SetNextItemWidth(BODY_W);
        const bool enter = ImGui::InputText("##theme_name", buf, sizeof(buf),
                                            ImGuiInputTextFlags_EnterReturnsTrue);

        const std::string name = trimmed(buf);
        const bool builtin  = theme_is_builtin(name);
        const bool replaces = !builtin && name != current && settings.themes.count(name);
        const bool ok       = !name.empty() && !builtin && name != current;

        if (builtin)
            ImGui::TextColored(col_error(), "\"%s\" is a built-in theme.", name.c_str());
        else if (replaces)
            ImGui::TextColored(col_warning(), "Replaces your saved \"%s\".", name.c_str());
        else
            ImGui::TextDisabled(" ");
        ImGui::Spacing();

        constexpr float BW = 130.0f, BGAP = 8.0f;
        ui_dialog_row(BW * 2.0f + BGAP);

        ImGui::BeginDisabled(!ok);
        if ((ui_solid_button(replaces ? "REPLACE" : verb, { BW, UI_DIALOG_BH },
                             btn_write_base(), btn_write_hov()) || enter) && ok) {
            out = name;
            committed = true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndDisabled();

        ImGui::SameLine(0, BGAP);
        if (ui_grid_button("CANCEL", { BW, UI_DIALOG_BH }) ||
            ImGui::IsKeyPressed(ImGuiKey_Escape, false))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }
    ui_pop_dialog_style();
    return committed;
}

// "<name> copy", or "<name> copy 2", 3 … — the first one not already taken.
static std::string copy_name(const AppSettings& settings, const std::string& base)
{
    std::string n = base + " copy";
    for (int i = 2; settings.themes.count(n) || theme_is_builtin(n); ++i)
        n = base + " copy " + std::to_string(i);
    return n;
}

static void section(const char* title, bool first = false)
{
    if (!first) {
        themed_sep();
        ImGui::Spacing();
    }
    ImGui::TextColored(accent_col(), "%s", title);
    themed_sep();
    ImGui::Spacing();
}

static void disabled_tip(const char* text)
{
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        ImGui::SetTooltip("%s", text);
}

// ── Base colors ───────────────────────────────────────────────────────────────
//
// BASIC edits a dozen colors instead of eighty. Each base color is one token of
// the theme (its anchor) plus the tokens that follow it: hover shades, darker
// plates, the instrument and map details drawn in the same color. The ratios
// are measured off Tactical, so a base edit lands where the hand-tuned palette
// would have put it.
//
// Followers are only rewritten when their base is edited. Until then every
// token keeps the value its theme gave it — opening BASIC on a built-in changes
// nothing, and an ADVANCED tweak stands until its base color is moved.

// `c` scaled by `k`, keeping `dst`'s own alpha: a follower's transparency is
// part of how it is drawn (an overlay, a wash), not part of its color.
static void follow(ImVec4& dst, const ImVec4& c, float k = 1.0f)
{
    dst = { std::min(1.0f, c.x * k), std::min(1.0f, c.y * k),
            std::min(1.0f, c.z * k), dst.w };
}

// Text on an accent plate (a lit tab, the DISARMED slab): near-background on a
// bright accent, body text on a dark one. Depends on two bases, so both call it.
static void follow_text_on_accent(ThemeVars& t)
{
    const ImVec4& a = t.accent;
    const float lum = 0.2126f * a.x + 0.7152f * a.y + 0.0722f * a.z;
    if (lum > 0.45f) follow(t.col_active_text, t.bg_panel, 0.52f);
    else             follow(t.col_active_text, t.col_text, 1.1f);
}

struct BaseColor {
    const char*         label;
    const char*         hint;
    ImVec4 ThemeVars::* anchor;
    void              (*follow)(ThemeVars&);   // null: nothing follows it
};

static const BaseColor k_base_colors[] = {
    { "Background", "Panels and bars; wells, strips and backdrops follow",
      &ThemeVars::bg_panel, [](ThemeVars& t) {
        const ImVec4 c = t.bg_panel;
        follow(t.bg_topbar, c, 0.67f);        follow(t.bg_bottombar, c, 0.67f);
        follow(t.bg_param_list, c, 0.67f);    follow(t.bg_sidebar_left, c, 0.81f);
        follow(t.bg_video, c, 0.44f);         follow(t.bg_map, c, 0.44f);
        follow(t.map_tile_placeholder, c, 0.44f);
        follow(t.ah_overlay_bg, c, 0.44f);    follow(t.ah_pitch_label_bg, c, 0.44f);
        follow(t.map_attr_bg, c, 0.44f);
        follow(t.bg_child_dark, c, 1.33f);    // header strips
        follow(t.bg_child_darker, c, 0.89f);  follow(t.ekf_bg, c, 0.89f);
        follow(t.btn_intlk_off_base, c, 1.04f);  // the plain button plate
        follow_text_on_accent(t);
    } },
    { "Lines", "Separators and panel seams",
      &ThemeVars::separator, [](ThemeVars& t) {
        const ImVec4 c = t.separator;
        follow(t.panel_border, c, 1.13f);     follow(t.ah_border, c, 1.13f);
        follow(t.border_tab, c);              follow(t.border_form, c);
        follow(t.ekf_outline, c);
    } },
    { "Text", "Body text, controls, horizon markings",
      &ThemeVars::col_text, [](ThemeVars& t) {
        const ImVec4 c = t.col_text;
        follow(t.col_text_on_dark, c, 1.11f); follow(t.ah_horizon_line, c, 1.11f);
        follow(t.ah_pitch_label, c, 1.02f);   follow(t.ah_pitch_ladder, c, 0.89f);
        follow_text_on_accent(t);
    } },
    { "Labels", "Field labels and log; muted text follows",
      &ThemeVars::col_log, [](ThemeVars& t) {
        const ImVec4 c = t.col_log;
        follow(t.map_attr_text, c);           follow(t.ekf_label, c);
        follow(t.ah_roll_arc, c);             follow(t.ah_roll_tick, c, 1.12f);
        follow(t.ah_roll_label, c, 1.06f);
        follow(t.col_no_link_muted, c, 0.82f); follow(t.col_no_link, c, 0.61f);
        follow(t.btn_disarm_base, c, 0.29f);
    } },
    { "Accent", "Headings, selection, DISARMED; write buttons follow",
      &ThemeVars::accent, [](ThemeVars& t) {
        const ImVec4 c = t.accent;
        follow(t.col_disarmed, c);            follow(t.map_vehicle_ring, c);
        follow(t.ah_heading_text, c);         follow(t.ah_crosshair, c, 1.05f);
        follow(t.ah_roll_pointer, c, 1.05f);
        follow(t.btn_tab_active_base, c);     follow(t.btn_tab_active_hov, c, 1.06f);
        follow(t.flash_normal_hov, c);        follow(t.flash_normal_press, c);
        follow(t.btn_write_base, c, 0.38f);   follow(t.btn_write_hov, c, 0.52f);
        follow(t.flash_pending_base, c, 0.38f); follow(t.flash_pending_hov, c, 0.52f);
        follow_text_on_accent(t);
    } },
    { "Highlight", "Selected rows, open headers, scrollbars, sliders",
      &ThemeVars::highlight, nullptr },   // apply_global_theme shades it
    { "Data", "Live telemetry values",
      &ThemeVars::col_data, [](ThemeVars& t) {
        const ImVec4 c = t.col_data;
        follow(t.ah_airspeed_text, c);        follow(t.ah_throttle_text, c);
        follow(t.map_vehicle_fill, c);
    } },
    { "OK", "Nominal state; accepted commands follow",
      &ThemeVars::col_ok, [](ThemeVars& t) {
        const ImVec4 c = t.col_ok;
        follow(t.flash_accepted_base, c, 0.40f); follow(t.flash_accepted_hov, c, 0.54f);
    } },
    { "Warning", "Cautions, low battery, reboot needed",
      &ThemeVars::col_warning, [](ThemeVars& t) {
        follow(t.col_reboot, t.col_warning);
    } },
    { "Danger", "Errors, the ARMED annunciator, flight track",
      &ThemeVars::col_error, [](ThemeVars& t) {
        const ImVec4 c = t.col_error;
        follow(t.col_error_alt, c, 1.1f);     follow(t.col_armed, c);
        follow(t.map_track, c, 1.07f);
    } },
    { "Danger buttons", "Exit, arm, stop, disconnect, delete; rejected commands",
      &ThemeVars::btn_stop_base, [](ThemeVars& t) {
        const ImVec4 c = t.btn_stop_base;
        follow(t.btn_stop_hov, c, 1.49f);
        follow(t.btn_arm_base, c, 0.90f);
        follow(t.btn_disconnect_base, c, 0.90f); follow(t.btn_disconnect_hov, c, 1.35f);
        follow(t.flash_rejected_base, c, 1.45f); follow(t.flash_rejected_hov, c, 1.94f);
    } },
    { "Sky", "Artificial horizon, above the line",
      &ThemeVars::ah_sky, nullptr },
    { "Ground", "Artificial horizon, below the line",
      &ThemeVars::ah_ground, nullptr },
    { "Plugin rail", "Plugin column seams and captions",
      &ThemeVars::plugin_accent, [](ThemeVars& t) {
        follow(t.plugin_accent_dim, t.plugin_accent, 0.81f);
    } },
};

// ── draw_themes_panel ─────────────────────────────────────────────────────────
//
// Colour edits apply to the live UI at once but are only kept on SAVE — the
// panel says which theme is edited, and asks before anything throws edits away.
// Everything outside APPEARANCE (tones, HUD, map source) saves on change.

void draw_themes_panel(AppSettings& settings)
{
    static constexpr ImGuiColorEditFlags kPickFlags =
        ImGuiColorEditFlags_NoInputs        |  // compact: swatch only, popup on click
        ImGuiColorEditFlags_AlphaBar        |
        ImGuiColorEditFlags_AlphaPreviewHalf;

    const std::string active = settings.active_theme;
    const bool active_builtin = theme_is_builtin(active);
    ThemeVars saved;
    const bool known  = theme_lookup(settings, active, saved);
    const bool edited = known && !theme_equal(g_theme, saved);

    auto activate = [&](const std::string& name) {
        ThemeVars t;
        if (!theme_lookup(settings, name, t)) return;
        settings.active_theme = name;
        g_theme = t;
        apply_global_theme(ImGui::GetStyle());
        settings_save(settings);
    };

    section("APPEARANCE", true);

    // ── Theme cards ───────────────────────────────────────────────────────────
    static std::string s_pending;   // theme asked for while the active one is edited
    {
        const float card_w = (ImGui::GetContentRegionAvail().x - CARD_GAP) * 0.5f;
        int col = 0;
        auto card = [&](const std::string& name, bool builtin) {
            ThemeVars t;
            if (!theme_lookup(settings, name, t)) return;
            const bool is_active = (name == active);
            if (col++ % 2) ImGui::SameLine(0, CARD_GAP);
            // The active card previews the live colours, edits and all.
            if (theme_card(name, is_active ? g_theme : t, is_active, is_active && edited,
                           builtin, { card_w, CARD_H }) && !is_active) {
                if (edited) {
                    s_pending = name;
                    ImGui::OpenPopup("##theme_switch");
                } else {
                    activate(name);
                }
            }
        };
        for (size_t i = 0; i < k_builtin_theme_count; ++i)
            card(k_builtin_themes[i].name, true);
        for (const auto& kv : settings.themes)
            if (!theme_is_builtin(kv.first)) card(kv.first, false);
    }
    ImGui::Spacing();

    // ── What the active theme is, and what can be done with it ────────────────
    if (edited)
        ImGui::TextColored(col_warning(), "Unsaved color changes.");
    else if (active_builtin)
        ImGui::TextDisabled("Built-in themes are read-only.");
    else
        ImGui::TextDisabled("Saved.");
    ImGui::Spacing();

    const float avail_w = ImGui::GetContentRegionAvail().x;
    static std::string s_saveas_initial;

    if (active_builtin) {
        const float w = (avail_w - 4.0f) * 0.5f;
        if (ui_grid_button("SAVE AS COPY\xe2\x80\xa6##theme", { w, 0.0f })) {
            s_saveas_initial = copy_name(settings, active);
            ImGui::OpenPopup("##theme_saveas");
        }
        disabled_tip("Keep these colors as a theme of your own.");
        ImGui::SameLine(0, 4);
        ImGui::BeginDisabled(!edited);
        if (ui_grid_button("REVERT##theme", { -1.0f, 0.0f }))
            ImGui::OpenPopup("##theme_revert");
        ImGui::EndDisabled();
        disabled_tip(edited ? "Go back to the built-in colors." : "Nothing to revert.");
    } else {
        const float w = (avail_w - 8.0f) / 3.0f;
        ImGui::BeginDisabled(!edited);
        // SAVE lights up only when there is something to save.
        const bool save = edited
            ? ui_solid_button("SAVE##theme", { w, 0.0f }, btn_write_base(), btn_write_hov())
            : ui_grid_button("SAVE##theme", { w, 0.0f });
        if (save) {
            settings.themes[active] = g_theme;
            settings_save(settings);
        }
        ImGui::EndDisabled();
        disabled_tip(edited ? "Keep these colors in this theme." : "No unsaved changes.");

        ImGui::SameLine(0, 4);
        if (ui_grid_button("SAVE AS\xe2\x80\xa6##theme", { w, 0.0f })) {
            s_saveas_initial = copy_name(settings, active);
            ImGui::OpenPopup("##theme_saveas");
        }
        disabled_tip("Keep these colors as a new theme.");

        ImGui::SameLine(0, 4);
        ImGui::BeginDisabled(!edited);
        if (ui_grid_button("REVERT##theme", { -1.0f, 0.0f }))
            ImGui::OpenPopup("##theme_revert");
        ImGui::EndDisabled();
        disabled_tip(edited ? "Go back to the last saved colors." : "Nothing to revert.");

        const float hw = (avail_w - 4.0f) * 0.5f;
        if (ui_grid_button("RENAME\xe2\x80\xa6##theme", { hw, 0.0f }))
            ImGui::OpenPopup("##theme_rename");
        ImGui::SameLine(0, 4);
        if (ui_grid_button("DELETE##theme", { -1.0f, 0.0f }))
            ImGui::OpenPopup("##theme_delete");
    }

    // ── Dialogs ───────────────────────────────────────────────────────────────
    std::string name;
    if (theme_name_popup("##theme_saveas", "SAVE THEME AS", "SAVE",
                         s_saveas_initial, std::string{}, settings, name)) {
        settings.themes[name]  = g_theme;
        settings.active_theme  = name;
        settings_save(settings);
    }

    if (theme_name_popup("##theme_rename", "RENAME THEME", "RENAME",
                         active, active, settings, name)) {
        // The saved copy moves; any unsaved edits stay on screen, still unsaved.
        settings.themes[name] = settings.themes[active];
        settings.themes.erase(active);
        settings.active_theme = name;
        settings_save(settings);
    }

    char question[160];
    std::snprintf(question, sizeof(question), "Delete \"%s\"? This can't be undone.", active.c_str());
    if (ui_confirm_popup("##theme_delete", "DELETE THEME", question, "DELETE",
                         btn_stop_base()) == UiConfirm::Confirmed) {
        settings.themes.erase(active);
        activate(THEME_BUILTIN_NAME);
    }

    if (ui_confirm_popup("##theme_revert", "REVERT THEME", "Discard unsaved color changes?",
                         "REVERT", btn_stop_base()) == UiConfirm::Confirmed) {
        g_theme = saved;
        apply_global_theme(ImGui::GetStyle());
    }

    // Switching away from an edited theme. A custom theme can be saved on the
    // way out; a built-in can't, so its dialog points at SAVE AS COPY instead.
    ui_push_dialog_style();
    if (ImGui::BeginPopup("##theme_switch")) {
        constexpr float BW = 110.0f, BGAP = 8.0f;
        const int n = active_builtin ? 2 : 3;
        ui_dialog_title("UNSAVED CHANGES", BW * n + BGAP * (n - 1));
        char line[160];
        std::snprintf(line, sizeof(line), "\"%s\" has unsaved color changes.", active.c_str());
        ui_dialog_text(line, ui_col_accent());
        if (active_builtin)
            ui_dialog_text("To keep them, cancel and SAVE AS COPY.", ui_col_label());
        ImGui::Spacing();

        ui_dialog_row(BW * n + BGAP * (n - 1));
        if (!active_builtin) {
            if (ui_solid_button("SAVE", { BW, UI_DIALOG_BH }, btn_write_base(), btn_write_hov())) {
                settings.themes[active] = g_theme;
                activate(s_pending);
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine(0, BGAP);
        }
        if (ui_solid_button("DISCARD", { BW, UI_DIALOG_BH }, btn_stop_base(), btn_stop_hov())) {
            activate(s_pending);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine(0, BGAP);
        if (ui_grid_button("CANCEL##switch", { BW, UI_DIALOG_BH }) ||
            ImGui::IsKeyPressed(ImGuiKey_Escape, false))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    ui_pop_dialog_style();

    // ── Color editor ──────────────────────────────────────────────────────────
    ImGui::Spacing();
    if (ImGui::CollapsingHeader("CUSTOMIZE COLORS")) {
        static int s_mode = 0;   // 0 basic, 1 advanced
        ImGui::RadioButton("Basic", &s_mode, 0);
        ImGui::SameLine(0, 12);
        ImGui::RadioButton("Advanced", &s_mode, 1);
        ImGui::TextDisabled(s_mode == 0 ? "A few colors; the rest follow them."
                                        : "Every color on its own.");
        ImGui::TextDisabled("Changes show at once; SAVE keeps them.");
        ImGui::Spacing();

        bool changed = false;

        if (s_mode == 0) {
            for (const BaseColor& b : k_base_colors) {
                ImGui::PushID(b.label);
                ImVec4& col = g_theme.*b.anchor;
                if (ImGui::ColorEdit4("##base", &col.x, kPickFlags | ImGuiColorEditFlags_NoLabel)) {
                    if (b.follow) b.follow(g_theme);
                    changed = true;
                }
                ImGui::PopID();
                ImGui::SameLine(0, 8);
                ImGui::BeginGroup();
                ImGui::TextUnformatted(b.label);
                ImGui::PushFont(g_font_micro);
                ImGui::PushTextWrapPos(0.0f);
                ImGui::TextDisabled("%s", b.hint);
                ImGui::PopTextWrapPos();
                ImGui::PopFont();
                ImGui::EndGroup();
                ImGui::Spacing();
            }
        } else {
            // Each ColorEdit4 with kPickFlags shows a compact swatch; clicking
            // opens a full picker popup.
            auto c4 = [&](const char* label, ImVec4& col) {
                changed |= ImGui::ColorEdit4(label, &col.x, kPickFlags);
            };
            ImGui::Indent(8.0f);

            if (ImGui::CollapsingHeader("TEXT & STATUS", ImGuiTreeNodeFlags_DefaultOpen)) {
                c4("Body Text",        g_theme.col_text);
                c4("Labels / Log",     g_theme.col_log);
                c4("Muted Text",       g_theme.col_no_link_muted);
                c4("Text on Accent",   g_theme.col_active_text);
                c4("Text on Plates",   g_theme.col_text_on_dark);
                c4("Accent",           g_theme.accent);
                c4("Highlight",        g_theme.highlight);
                c4("Data",             g_theme.col_data);
                c4("OK",               g_theme.col_ok);
                c4("Warning",          g_theme.col_warning);
                c4("Reboot Needed",    g_theme.col_reboot);
                c4("Error",            g_theme.col_error);
                c4("Error (alt)",      g_theme.col_error_alt);
                c4("Armed",            g_theme.col_armed);
                c4("Disarmed",         g_theme.col_disarmed);
                c4("No Link Plate",    g_theme.col_no_link);
            }

            if (ImGui::CollapsingHeader("BACKGROUNDS")) {
                c4("Panel",            g_theme.bg_panel);
                c4("Top Bar",          g_theme.bg_topbar);
                c4("Bottom Bar",       g_theme.bg_bottombar);
                c4("Left Sidebar",     g_theme.bg_sidebar_left);
                c4("Video",            g_theme.bg_video);
                c4("Map Backdrop",     g_theme.bg_map);
                c4("Header Strips",    g_theme.bg_child_dark);
                c4("Tiles / Wells",    g_theme.bg_child_darker);
                c4("Lists",            g_theme.bg_param_list);
            }

            if (ImGui::CollapsingHeader("LINES")) {
                c4("Separator",        g_theme.separator);
                c4("Panel Seam",       g_theme.panel_border);
                c4("Tab Border",       g_theme.border_tab);
                c4("Form Border",      g_theme.border_form);
            }

            if (ImGui::CollapsingHeader("BUTTONS")) {
                c4("Button Plate",     g_theme.btn_intlk_off_base);
                c4("Button Hover",     g_theme.flash_normal_hov);
                c4("Button Press",     g_theme.flash_normal_press);
                c4("Arm",              g_theme.btn_arm_base);
                c4("Disarm",           g_theme.btn_disarm_base);
                c4("Disconnect",       g_theme.btn_disconnect_base);
                c4("Disconnect Hover", g_theme.btn_disconnect_hov);
                c4("Exit / Stop / Delete", g_theme.btn_stop_base);
                c4("Exit / Stop Hover",    g_theme.btn_stop_hov);
                c4("Write / Confirm",  g_theme.btn_write_base);
                c4("Write Hover",      g_theme.btn_write_hov);
                c4("Active Tab",       g_theme.btn_tab_active_base);
                c4("Active Tab Hover", g_theme.btn_tab_active_hov);
            }

            if (ImGui::CollapsingHeader("PLUGIN RAIL")) {
                c4("Plugin Accent",     g_theme.plugin_accent);
                c4("Plugin Accent Dim", g_theme.plugin_accent_dim);
                // Engaged has no row of its own: it follows Armed, above.
            }

            if (ImGui::CollapsingHeader("COMMAND FEEDBACK")) {
                c4("Accepted",         g_theme.flash_accepted_base);
                c4("Accepted Hover",   g_theme.flash_accepted_hov);
                c4("Rejected",         g_theme.flash_rejected_base);
                c4("Rejected Hover",   g_theme.flash_rejected_hov);
                c4("Pending",          g_theme.flash_pending_base);
                c4("Pending Hover",    g_theme.flash_pending_hov);
            }

            if (ImGui::CollapsingHeader("ARTIFICIAL HORIZON")) {
                c4("Sky",              g_theme.ah_sky);
                c4("Ground",           g_theme.ah_ground);
                c4("Horizon Line",     g_theme.ah_horizon_line);
                c4("Pitch Ladder",     g_theme.ah_pitch_ladder);
                c4("Pitch Label BG",   g_theme.ah_pitch_label_bg);
                c4("Pitch Label",      g_theme.ah_pitch_label);
                c4("Crosshair",        g_theme.ah_crosshair);
                c4("Overlay BG",       g_theme.ah_overlay_bg);
                c4("Airspeed Text",    g_theme.ah_airspeed_text);
                c4("Throttle Text",    g_theme.ah_throttle_text);
                c4("Heading Text",     g_theme.ah_heading_text);
                c4("Roll Arc",         g_theme.ah_roll_arc);
                c4("Roll Tick",        g_theme.ah_roll_tick);
                c4("Roll Label",       g_theme.ah_roll_label);
                c4("Roll Pointer",     g_theme.ah_roll_pointer);
                c4("Instrument Border",g_theme.ah_border);
            }

            if (ImGui::CollapsingHeader("MAP")) {
                c4("Tile Placeholder", g_theme.map_tile_placeholder);
                c4("Vehicle Fill",     g_theme.map_vehicle_fill);
                c4("Vehicle Ring",     g_theme.map_vehicle_ring);
                c4("Track",            g_theme.map_track);
                c4("Attribution BG",   g_theme.map_attr_bg);
                c4("Attribution Text", g_theme.map_attr_text);
            }

            if (ImGui::CollapsingHeader("CRT")) {
                c4("Scanlines",        g_theme.crt_scanline);
            }

            if (ImGui::CollapsingHeader("EKF")) {
                c4("EKF Background",   g_theme.ekf_bg);
                c4("EKF Outline",      g_theme.ekf_outline);
                c4("EKF Label",        g_theme.ekf_label);
            }
            ImGui::Unindent(8.0f);
        }

        if (changed)
            apply_global_theme(ImGui::GetStyle());
    }
    ImGui::Spacing();

    section("CRT EFFECTS");

    // ── CRT effects ───────────────────────────────────────────────────────────
    //
    // Pure decoration, saved on change like the tones below. Each strength is
    // saved on release rather than on every pixel the slider moves. The
    // scanline color is the exception: it belongs to the theme, so it marks
    // the theme EDITED and is kept by SAVE like any other color.
    {
        auto effect = [&](const char* label, const char* id, bool& on, float& strength,
                          ImVec4* color) {
            ImGui::PushID(id);
            if (ImGui::Checkbox(label, &on))
                settings_save(settings);
            ImGui::SameLine(150.0f);
            ImGui::BeginDisabled(!on);
            if (color) {
                ImGui::ColorEdit4("##color", &color->x, kPickFlags | ImGuiColorEditFlags_NoLabel);
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                    ImGui::SetTooltip("Scanline color \xe2\x80\x94 part of the theme");
                ImGui::SameLine(0, 4);
            }
            ImGui::SetNextItemWidth(-1.0f);
            ImGui::SliderFloat("##strength", &strength, 0.0f, 1.0f, "STRENGTH  %.2f");
            if (ImGui::IsItemDeactivatedAfterEdit())
                settings_save(settings);
            ImGui::EndDisabled();
            ImGui::PopID();
        };
        effect("Scanlines",      "crt_lines",    settings.crt_scanlines, settings.crt_scanline_strength,
               &g_theme.crt_scanline);
        effect("Edge darkening", "crt_vignette", settings.crt_vignette,  settings.crt_vignette_strength,
               nullptr);
        ImGui::TextDisabled("Cosmetic only: scanline noise, a rolling band, dark edges.");
    }

    section("AUDIO");

    // ── Tones ─────────────────────────────────────────────────────────────────
    //
    // Written through to disk on change rather than on some later APPLY: an
    // operator reaching for the mute wants it now and is not going to hunt for
    // a second button, and a mute that did not survive a restart is a mute that
    // will be reached for again.
    {
        bool enabled = settings.audio_enabled;
        if (ImGui::Checkbox("Tones", &enabled)) {
            settings.audio_enabled = enabled;
            audio_set_enabled(enabled);
            settings_save(settings);
        }
        ImGui::SameLine(0, 8);
        ImGui::TextDisabled("success / failure / armed / progress");

        float vol = settings.audio_volume;
        ImGui::BeginDisabled(!enabled);
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::SliderFloat("##audio_vol", &vol, 0.0f, 1.0f, "VOLUME  %.2f")) {
            settings.audio_volume = vol;
            audio_set_volume(vol);
        }
        // Saved on release, not on every pixel the slider moves.
        if (ImGui::IsItemDeactivatedAfterEdit())
            settings_save(settings);

        ImGui::Spacing();
        if (ui_grid_button("TEST##audio", { -1.0f, 0.0f }))
            gcs_tone(GcsTone::Success);
        ImGui::EndDisabled();
    }

    section("HUD");

    // ── HUD style ─────────────────────────────────────────────────────────────
    //
    // Per view, since what suits a panel a third of the screen wide is not
    // what suits a card in a grid of them. Saved on change, as the tones are.
    {
        auto style_row = [&](const char* label, const char* id, bool& tapes) {
            ImGui::TextUnformatted(label);
            ImGui::SameLine(110.0f);
            int v = tapes ? 1 : 0;
            ImGui::PushID(id);
            bool changed = ImGui::RadioButton("Classic", &v, 0);
            ImGui::SameLine(0, 12);
            changed |= ImGui::RadioButton("Tapes", &v, 1);
            ImGui::PopID();
            if (changed) {
                tapes = (v == 1);
                settings_save(settings);
            }
        };
        style_row("Main view", "hud_main",  settings.hud_tapes_main);
        style_row("Swarm",     "hud_swarm", settings.hud_tapes_swarm);
        ImGui::TextDisabled("Classic: attitude ball. Tapes: speed / heading / altitude.");
    }

    section("MAP ICONS");

    // ── Vehicle symbols ───────────────────────────────────────────────────────
    //
    // Saved on change. The strip below draws what each kind of vehicle gets
    // under the chosen style, so BY VEHICLE TYPE shows its own mapping rather
    // than leaving it to be guessed.
    {
        static const char* k_styles[] = {
            "Legacy arrow", "By vehicle type", "Plane", "Multicopter", "Helicopter",
        };
        int style = std::clamp(settings.map_icon_style, 0, 4);
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::Combo("##map_icon_style", &style, k_styles, IM_ARRAYSIZE(k_styles))) {
            settings.map_icon_style = style;
            map_view_set_icon_style((MapIconStyle)style);
            settings_save(settings);
        }

        struct Sample { const char* label; uint8_t type; };
        static const Sample k_samples[] = {
            { "PLANE",  MAV_TYPE_FIXED_WING   },
            { "MULTI",  MAV_TYPE_QUADROTOR    },
            { "HELI",   MAV_TYPE_HELICOPTER   },
            { "OTHER",  MAV_TYPE_GROUND_ROVER },
        };
        constexpr float CELL_H = 76.0f;
        const float  cell_w = (ImGui::GetContentRegionAvail().x - 3.0f * CARD_GAP) / 4.0f;
        ImDrawList*  dl     = ImGui::GetWindowDrawList();
        for (int i = 0; i < 4; ++i) {
            if (i) ImGui::SameLine(0, CARD_GAP);
            ImGui::Dummy({ cell_w, CELL_H });
            const ImVec2 p0 = ImGui::GetItemRectMin(), p1 = ImGui::GetItemRectMax();
            dl->AddRectFilled(p0, p1, ui_col_well());
            dl->AddRect(p0, p1, ui_col_seam());
            dl->PushClipRect(p0, p1, true);
            map_draw_vehicle_symbol(dl, { (p0.x + p1.x) * 0.5f, p0.y + 31.0f }, k_samples[i].type,
                                    (MapIconStyle)style, 0.0f, 1.0f, 0.85f);
            dl->PopClipRect();
            const float tw = ui_tracked_width(g_font_micro, UI_SZ_MICRO, k_samples[i].label);
            ui_tracked_text(dl, g_font_micro, UI_SZ_MICRO, { (p0.x + p1.x - tw) * 0.5f, p1.y - 16.0f },
                            ui_col_label(), k_samples[i].label);
        }
        ImGui::Spacing();
    }

    section("MAP SOURCE");

    // ── Map tile source ───────────────────────────────────────────────────────
    {
        static constexpr char k_default_url[]   = "https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/{z}/{y}/{x}";
        static constexpr char k_default_attr[]  = "\xc2\xa9 Esri \xe2\x80\x94 Source: Esri, Maxar, Earthstar Geographics";

        // Local edit buffers — initialised from settings on first draw
        static char s_url_buf[512]  = {};
        static char s_attr_buf[128] = {};
        static bool s_map_init      = false;
        if (!s_map_init) {
            strncpy(s_url_buf,  settings.tile_url.c_str(),         sizeof(s_url_buf)  - 1);
            strncpy(s_attr_buf, settings.tile_attribution.c_str(), sizeof(s_attr_buf) - 1);
            s_map_init = true;
        }

        ImGui::TextDisabled("Tile URL  ({z} {x} {y} placeholders)");
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputText("##tile_url", s_url_buf, sizeof(s_url_buf));

        ImGui::Spacing();
        ImGui::TextDisabled("Attribution");
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputText("##tile_attr", s_attr_buf, sizeof(s_attr_buf));

        ImGui::Spacing();

        if (ui_grid_button("APPLY##map", { -1.0f, 0.0f })) {
            settings.tile_url         = s_url_buf;
            settings.tile_attribution = s_attr_buf;
            map_view_set_tile_source(settings.tile_url, settings.tile_attribution);
            settings_save(settings);
        }

        ImGui::Spacing();
        if (ui_grid_button("RESET TO DEFAULT##map", { -1.0f, 0.0f })) {
            strncpy(s_url_buf,  k_default_url,  sizeof(s_url_buf)  - 1);
            strncpy(s_attr_buf, k_default_attr, sizeof(s_attr_buf) - 1);
            settings.tile_url         = k_default_url;
            settings.tile_attribution = k_default_attr;
            map_view_set_tile_source(settings.tile_url, settings.tile_attribution);
            settings_save(settings);
        }
        ImGui::Spacing();
    }
}
