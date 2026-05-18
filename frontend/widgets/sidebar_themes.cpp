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
#include "../settings.hpp"
#include "map_view.hpp"
#include "imgui.h"
#include <cstring>
#include <string>

// ── draw_themes_panel ─────────────────────────────────────────────────────────

void draw_themes_panel(AppSettings& settings)
{
    static constexpr ImGuiColorEditFlags kPickFlags =
        ImGuiColorEditFlags_NoInputs        |  // compact: swatch only, popup on click
        ImGuiColorEditFlags_AlphaBar        |
        ImGuiColorEditFlags_AlphaPreviewHalf;

    // ── Built-in theme registry (also used for SAVE button guard) ────────────
    struct BuiltIn { const char* name; ThemeVars (*factory)(); };
    static const BuiltIn k_builtins[] = {
        { THEME_BUILTIN_NAME, []() -> ThemeVars { return ThemeVars{}; } },
        { THEME_DRACULA_NAME, dracula_theme_vars },
        { THEME_MATRIX_NAME,  matrix_theme_vars  },
    };

    ImGui::Spacing();
    ImGui::TextColored(accent_col(), "THEME");
    themed_sep();
    ImGui::Spacing();

    // ── Theme selector row ────────────────────────────────────────────────────
    const float avail_w = ImGui::GetContentRegionAvail().x;
    ImGui::SetNextItemWidth(avail_w - 80.0f);

    if (ImGui::BeginCombo("##theme_sel", settings.active_theme.c_str())) {
        for (const auto& bi : k_builtins) {
            const bool sel = (settings.active_theme == bi.name);
            if (ImGui::Selectable(bi.name, sel)) {
                settings.active_theme = bi.name;
                g_theme = bi.factory();
                apply_global_theme(ImGui::GetStyle());
                settings_save(settings);
            }
            if (sel) ImGui::SetItemDefaultFocus();
        }

        // ── User-saved themes ─────────────────────────────────────────────────
        for (const auto& [name, vars] : settings.themes) {
            // skip any name that shadows a built-in
            bool is_builtin = false;
            for (const auto& bi : k_builtins)
                if (name == bi.name) { is_builtin = true; break; }
            if (is_builtin) continue;

            const bool sel = (settings.active_theme == name);
            if (ImGui::Selectable(name.c_str(), sel)) {
                settings.active_theme = name;
                g_theme = vars;
                apply_global_theme(ImGui::GetStyle());
                settings_save(settings);
            }
            if (sel) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::SameLine(0, 4);

    // SAVE — stores current g_theme under active name (can't overwrite builtins)
    bool is_builtin = false;
    for (const auto& bi : k_builtins)
        if (settings.active_theme == bi.name) { is_builtin = true; break; }
    ImGui::BeginDisabled(is_builtin);
    push_flash_colors(CmdFlashState::Normal);
    if (ImGui::Button("SAVE##theme", { -1.0f, 0.0f })) {
        settings.themes[settings.active_theme] = g_theme;
        settings_save(settings);
    }
    ImGui::PopStyleColor(3);
    ImGui::EndDisabled();

    // RESET + SAVE AS row
    push_flash_colors(CmdFlashState::Normal);
    if (ImGui::Button("RESET##theme", { (avail_w - 4.0f) * 0.5f, 0.0f })) {
        if (settings.active_theme == THEME_DRACULA_NAME) {
            g_theme = dracula_theme_vars();
        } else if (settings.active_theme == THEME_BUILTIN_NAME) {
            g_theme = ThemeVars{};
        } else {
            auto it = settings.themes.find(settings.active_theme);
            g_theme = (it != settings.themes.end()) ? it->second : ThemeVars{};
        }
        apply_global_theme(ImGui::GetStyle());
    }
    ImGui::PopStyleColor(3);

    ImGui::SameLine(0, 4);

    push_flash_colors(CmdFlashState::Normal);
    if (ImGui::Button("SAVE AS##theme", { -1.0f, 0.0f }))
        ImGui::OpenPopup("##saveas_popup");
    ImGui::PopStyleColor(3);

    // Save-As popup
    if (ImGui::BeginPopup("##saveas_popup")) {
        ImGui::TextColored(accent_col(), "Save theme as:");
        ImGui::Spacing();
        static char save_name[64] = {};
        if (ImGui::IsWindowAppearing())
            std::strncpy(save_name, settings.active_theme.c_str(), sizeof(save_name) - 1);

        ImGui::SetNextItemWidth(200.0f);
        const bool enter = ImGui::InputText("##save_name", save_name, sizeof(save_name),
                                            ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::Spacing();

        const bool name_ok = save_name[0] != '\0';
        ImGui::BeginDisabled(!name_ok);
        push_flash_colors(CmdFlashState::Normal);
        if ((ImGui::Button("SAVE", { 94.0f, 0.0f }) || enter) && name_ok) {
            settings.active_theme        = save_name;
            settings.themes[save_name]   = g_theme;
            settings_save(settings);
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor(3);
        ImGui::EndDisabled();

        ImGui::SameLine(0, 4);
        push_flash_colors(CmdFlashState::Normal);
        if (ImGui::Button("CANCEL", { -1.0f, 0.0f }) ||
            ImGui::IsKeyPressed(ImGuiKey_Escape, false))
            ImGui::CloseCurrentPopup();
        ImGui::PopStyleColor(3);

        ImGui::EndPopup();
    }

    themed_sep();
    ImGui::Spacing();
    ImGui::TextColored(accent_col(), "MAP SOURCE");
    themed_sep();
    ImGui::Spacing();

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

        push_flash_colors(CmdFlashState::Normal);
        if (ImGui::Button("APPLY##map", { -1.0f, 0.0f })) {
            settings.tile_url         = s_url_buf;
            settings.tile_attribution = s_attr_buf;
            map_view_set_tile_source(settings.tile_url, settings.tile_attribution);
            settings_save(settings);
        }
        ImGui::PopStyleColor(3);

        ImGui::Spacing();
        push_flash_colors(CmdFlashState::Normal);
        if (ImGui::Button("RESET TO DEFAULT##map", { -1.0f, 0.0f })) {
            strncpy(s_url_buf,  k_default_url,  sizeof(s_url_buf)  - 1);
            strncpy(s_attr_buf, k_default_attr, sizeof(s_attr_buf) - 1);
            settings.tile_url         = k_default_url;
            settings.tile_attribution = k_default_attr;
            map_view_set_tile_source(settings.tile_url, settings.tile_attribution);
            settings_save(settings);
        }
        ImGui::PopStyleColor(3);
        ImGui::Spacing();
    }

    themed_sep();
    ImGui::Spacing();
    ImGui::TextColored(accent_col(), "COLORS");
    themed_sep();
    ImGui::Spacing();

    // ── Color editor sections ─────────────────────────────────────────────────
    // Each ColorEdit4 with kPickFlags shows a compact swatch; clicking opens a
    // full picker popup.  Any change applies immediately to g_theme.

    bool changed = false;

    auto c4 = [&](const char* label, ImVec4& col) {
        changed |= ImGui::ColorEdit4(label, &col.x, kPickFlags);
    };

    if (ImGui::CollapsingHeader("PALETTE", ImGuiTreeNodeFlags_DefaultOpen)) {
        c4("Accent",        g_theme.accent);
        c4("Data",          g_theme.col_data);
        c4("OK",            g_theme.col_ok);
        c4("Warning",       g_theme.col_warning);
        c4("Error",         g_theme.col_error);
        c4("Armed",         g_theme.col_armed);
        c4("Disarmed",      g_theme.col_disarmed);
        c4("Log",           g_theme.col_log);
        c4("No Link",       g_theme.col_no_link);
        c4("Active Text",   g_theme.col_active_text);
    }

    if (ImGui::CollapsingHeader("BACKGROUNDS")) {
        c4("Panel",         g_theme.bg_panel);
        c4("Topbar",        g_theme.bg_topbar);
        c4("Bottombar",     g_theme.bg_bottombar);
        c4("Sidebar",       g_theme.bg_sidebar_left);
        c4("Video",         g_theme.bg_video);
        c4("Map",           g_theme.bg_map);
        c4("Child Dark",    g_theme.bg_child_dark);
        c4("Child Darker",  g_theme.bg_child_darker);
        c4("Param List",    g_theme.bg_param_list);
    }

    if (ImGui::CollapsingHeader("BORDERS")) {
        c4("Separator",     g_theme.separator);
        c4("Tab Border",    g_theme.border_tab);
        c4("Form Border",   g_theme.border_form);
        c4("Panel Seam",    g_theme.panel_border);
    }

    if (ImGui::CollapsingHeader("BUTTONS")) {
        c4("Arm",              g_theme.btn_arm_base);
        c4("Arm Hover",        g_theme.btn_arm_hov);
        c4("Disarm",           g_theme.btn_disarm_base);
        c4("Disarm Hover",     g_theme.btn_disarm_hov);
        c4("Disconnect",       g_theme.btn_disconnect_base);
        c4("Disc. Hover",      g_theme.btn_disconnect_hov);
        c4("Stop",             g_theme.btn_stop_base);
        c4("Stop Hover",       g_theme.btn_stop_hov);
        c4("Interlock ON",     g_theme.btn_intlk_on_base);
        c4("Intlk ON Hover",   g_theme.btn_intlk_on_hov);
        c4("Interlock OFF",    g_theme.btn_intlk_off_base);
        c4("Intlk OFF Hover",  g_theme.btn_intlk_off_hov);
        c4("Mode Active",      g_theme.btn_mode_active_base);
        c4("Mode Act. Hover",  g_theme.btn_mode_active_hov);
        c4("Write",            g_theme.btn_write_base);
        c4("Write Hover",      g_theme.btn_write_hov);
        c4("Tab Active",       g_theme.btn_tab_active_base);
        c4("Tab Act. Hover",   g_theme.btn_tab_active_hov);
    }

    if (ImGui::CollapsingHeader("FLASH STATES")) {
        c4("Accepted",         g_theme.flash_accepted_base);
        c4("Accepted Hover",   g_theme.flash_accepted_hov);
        c4("Rejected",         g_theme.flash_rejected_base);
        c4("Rejected Hover",   g_theme.flash_rejected_hov);
        c4("Pending",          g_theme.flash_pending_base);
        c4("Pending Hover",    g_theme.flash_pending_hov);
        c4("Normal Hover",     g_theme.flash_normal_hov);
        c4("Normal Press",     g_theme.flash_normal_press);
    }

    if (ImGui::CollapsingHeader("ARTIFICIAL HORIZON")) {
        c4("Sky",           g_theme.ah_sky);
        c4("Ground",        g_theme.ah_ground);
        c4("Horizon Line",  g_theme.ah_horizon_line);
        c4("Pitch Ladder",  g_theme.ah_pitch_ladder);
        c4("Pitch Label BG",g_theme.ah_pitch_label_bg);
        c4("Pitch Label",   g_theme.ah_pitch_label);
        c4("Crosshair",     g_theme.ah_crosshair);
        c4("Overlay BG",    g_theme.ah_overlay_bg);
        c4("Airspeed Text", g_theme.ah_airspeed_text);
        c4("Throttle Text", g_theme.ah_throttle_text);
        c4("Heading Text",  g_theme.ah_heading_text);
        c4("Roll Arc",      g_theme.ah_roll_arc);
        c4("Roll Tick",     g_theme.ah_roll_tick);
        c4("Roll Label",    g_theme.ah_roll_label);
        c4("Roll Pointer",  g_theme.ah_roll_pointer);
        c4("Border",        g_theme.ah_border);
    }

    if (ImGui::CollapsingHeader("MAP")) {
        c4("Tile Placeholder", g_theme.map_tile_placeholder);
        c4("Vehicle Fill",     g_theme.map_vehicle_fill);
        c4("Vehicle Ring",     g_theme.map_vehicle_ring);
        c4("Attr BG",          g_theme.map_attr_bg);
        c4("Attr Text",        g_theme.map_attr_text);
    }

    if (ImGui::CollapsingHeader("EKF")) {
        c4("Background", g_theme.ekf_bg);
        c4("Outline",    g_theme.ekf_outline);
        c4("Label",      g_theme.ekf_label);
    }

    if (changed)
        apply_global_theme(ImGui::GetStyle());
}
