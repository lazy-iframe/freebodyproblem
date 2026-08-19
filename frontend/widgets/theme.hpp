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
#include "../../backend/mavlink_sender.hpp"

// ─────────────────────────────────────────────────────────────────────────────
// theme.hpp — Centralized GCS color and style tokens
//
// Palette: tactical olive (default)
//   Background : dark olive-black  #12160C / panels #1B2012
//   Chrome     : header strips #242A19, seams #46552F
//   Primary    : amber  #ECB94E  (labels, chips, active state)
//   Data       : pale green  #97EA89  (live telemetry values)
//   Muted      : dim olive  #6E7B55  (field labels, log)
//   Danger     : salmon red  #EF8271
//   Warning    : amber-orange  #E0913A
// ─────────────────────────────────────────────────────────────────────────────

enum class CmdFlashState;   // defined in backend/mavlink_sender.hpp

// ── Section 1: Runtime-mutable theme variables ────────────────────────────────
//
// All color tokens live here. The inline functions below are kept so that
// call sites (push/pop style, etc.) do not need to change.

struct ThemeVars {
    // ── Palette ───────────────────────────────────────────────────────────────
    ImVec4 accent             = { 0.925f, 0.725f, 0.306f, 1.0f }; // amber #ECB94E — labels, chips
    ImVec4 col_data           = { 0.592f, 0.918f, 0.537f, 1.0f }; // pale green #97EA89 — live values
    ImVec4 col_ok             = { 0.592f, 0.918f, 0.537f, 1.0f }; // pale green — nominal state
    ImVec4 col_warning        = { 0.878f, 0.569f, 0.227f, 1.0f }; // amber-orange #E0913A
    ImVec4 col_error          = { 0.937f, 0.510f, 0.443f, 1.0f }; // salmon red #EF8271
    ImVec4 col_error_alt      = { 0.945f, 0.604f, 0.541f, 1.0f };
    ImVec4 col_log            = { 0.431f, 0.482f, 0.333f, 1.0f }; // dim olive #6E7B55 — labels/log
    ImVec4 col_armed          = { 0.937f, 0.420f, 0.353f, 1.0f }; // hot salmon red
    ImVec4 col_disarmed       = { 0.925f, 0.725f, 0.306f, 1.0f }; // amber
    ImVec4 col_no_link        = { 0.431f, 0.482f, 0.333f, 1.0f }; // dim olive
    ImVec4 col_no_link_muted  = { 0.278f, 0.318f, 0.216f, 1.0f };
    ImVec4 col_reboot         = { 0.878f, 0.569f, 0.227f, 1.0f }; // amber-orange
    ImVec4 col_active_text    = { 0.055f, 0.067f, 0.043f, 1.0f }; // near-black — text on amber
    ImVec4 col_text_on_dark   = { 0.902f, 0.918f, 0.878f, 1.0f }; // bone — text on dark plates (red/olive)

    // ── Backgrounds ───────────────────────────────────────────────────────────
    ImVec4 bg_panel           = { 0.106f, 0.125f, 0.071f, 1.0f }; // #1B2012
    ImVec4 bg_topbar          = { 0.071f, 0.086f, 0.047f, 1.0f }; // #12160C
    ImVec4 bg_bottombar       = { 0.071f, 0.086f, 0.047f, 1.0f };
    ImVec4 bg_sidebar_left    = { 0.086f, 0.106f, 0.059f, 1.0f }; // #161B0F
    ImVec4 bg_video           = { 0.047f, 0.059f, 0.031f, 1.0f }; // #0C0F08
    ImVec4 bg_map             = { 0.047f, 0.059f, 0.031f, 1.0f };
    ImVec4 bg_child_dark      = { 0.141f, 0.165f, 0.098f, 1.0f }; // #242A19 — header strips
    ImVec4 bg_child_darker    = { 0.094f, 0.114f, 0.063f, 1.0f }; // #181D10 — tiles / wells
    ImVec4 bg_param_list      = { 0.071f, 0.086f, 0.047f, 1.0f };

    // ── Borders ───────────────────────────────────────────────────────────────
    ImVec4 separator          = { 0.243f, 0.290f, 0.173f, 1.0f }; // #3E4A2C
    ImVec4 border_tab         = { 0.243f, 0.290f, 0.173f, 0.90f };
    ImVec4 border_form        = { 0.243f, 0.290f, 0.173f, 0.70f };
    ImVec4 panel_border       = { 0.275f, 0.333f, 0.184f, 0.95f }; // panel seam lines

    // ── Named buttons ─────────────────────────────────────────────────────────
    ImVec4 btn_arm_base         = { 0.278f, 0.086f, 0.059f, 1.0f };
    ImVec4 btn_arm_hov          = { 0.420f, 0.129f, 0.090f, 1.0f };
    ImVec4 btn_disarm_base      = { 0.204f, 0.243f, 0.129f, 1.0f };
    ImVec4 btn_disarm_hov       = { 0.294f, 0.345f, 0.176f, 1.0f };
    ImVec4 btn_disconnect_base  = { 0.278f, 0.086f, 0.059f, 1.0f };
    ImVec4 btn_disconnect_hov   = { 0.420f, 0.129f, 0.090f, 1.0f };
    ImVec4 btn_stop_base        = { 0.310f, 0.098f, 0.067f, 0.90f };
    ImVec4 btn_stop_hov         = { 0.463f, 0.145f, 0.098f, 1.00f };
    ImVec4 btn_stop_act         = { 0.235f, 0.075f, 0.051f, 1.00f };
    ImVec4 btn_intlk_on_base    = { 0.353f, 0.259f, 0.086f, 1.0f };
    ImVec4 btn_intlk_on_hov     = { 0.482f, 0.357f, 0.118f, 1.0f };
    ImVec4 btn_intlk_off_base   = { 0.110f, 0.133f, 0.075f, 1.0f };
    ImVec4 btn_intlk_off_hov    = { 0.169f, 0.200f, 0.114f, 1.0f };
    ImVec4 btn_mode_active_base = { 0.353f, 0.259f, 0.086f, 1.0f };
    ImVec4 btn_mode_active_hov  = { 0.482f, 0.357f, 0.118f, 1.0f };
    ImVec4 btn_write_base       = { 0.353f, 0.259f, 0.086f, 1.0f };
    ImVec4 btn_write_hov        = { 0.482f, 0.357f, 0.118f, 1.0f };
    ImVec4 btn_tab_active_base  = { 0.925f, 0.725f, 0.306f, 0.88f };
    ImVec4 btn_tab_active_hov   = { 0.980f, 0.780f, 0.360f, 0.95f };

    // ── Plugin rail ───────────────────────────────────────────────────────────
    //
    // The rail wears the same dark olive panels and wells as everything else —
    // only its accent differs. Blue is applied exactly where amber is applied
    // elsewhere: seams, captions and the title strip, nothing more. That marks
    // the column as user code without carving a light hole out of the display.
    //
    // Deliberately left out of the Amber and Matrix factories — both build on
    // these defaults, so the accent stays blue whatever the rest of the UI
    // wears. An engaged button is not themed here either: it borrows col_armed,
    // the same red as the ARMED annunciator, so red means one thing throughout.
    ImVec4 plugin_accent        = { 0.614f, 0.837f, 0.951f, 1.0f }; // #9DD5F3 light blue
    ImVec4 plugin_accent_dim    = { 0.402f, 0.582f, 0.683f, 1.0f }; // #6694AE idle seams/captions

    // ── Command flash ─────────────────────────────────────────────────────────
    ImVec4 flash_accepted_base  = { 0.204f, 0.412f, 0.153f, 0.80f };
    ImVec4 flash_accepted_hov   = { 0.278f, 0.541f, 0.204f, 0.88f };
    ImVec4 flash_rejected_base  = { 0.451f, 0.129f, 0.090f, 1.0f };
    ImVec4 flash_rejected_hov   = { 0.600f, 0.176f, 0.122f, 1.0f };
    ImVec4 flash_pending_base   = { 0.353f, 0.259f, 0.086f, 1.0f };
    ImVec4 flash_pending_hov    = { 0.482f, 0.357f, 0.118f, 1.0f };
    ImVec4 flash_normal_hov     = { 0.925f, 0.725f, 0.306f, 0.14f };
    ImVec4 flash_normal_press   = { 0.925f, 0.725f, 0.306f, 0.26f };

    // ── Artificial horizon ────────────────────────────────────────────────────
    ImVec4 ah_sky            = { 0.227f, 0.310f, 0.369f, 1.000f }; // #3A4F5E
    ImVec4 ah_ground         = { 0.286f, 0.243f, 0.145f, 1.000f }; // #493E25
    ImVec4 ah_horizon_line   = { 0.902f, 0.918f, 0.878f, 0.950f }; // bone
    ImVec4 ah_pitch_ladder   = { 0.722f, 0.761f, 0.702f, 0.800f }; // #B8C2B3
    ImVec4 ah_pitch_label_bg = { 0.047f, 0.059f, 0.031f, 0.000f };
    ImVec4 ah_pitch_label    = { 0.827f, 0.855f, 0.812f, 0.850f };
    ImVec4 ah_crosshair      = { 0.969f, 0.812f, 0.396f, 1.000f }; // #F7CF65
    ImVec4 ah_overlay_bg     = { 0.047f, 0.059f, 0.031f, 0.700f };
    ImVec4 ah_airspeed_text  = { 0.592f, 0.918f, 0.537f, 1.000f };
    ImVec4 ah_throttle_text  = { 0.592f, 0.918f, 0.537f, 1.000f };
    ImVec4 ah_heading_text   = { 0.925f, 0.725f, 0.306f, 0.950f };
    ImVec4 ah_roll_arc       = { 0.722f, 0.761f, 0.702f, 0.350f };
    ImVec4 ah_roll_tick      = { 0.827f, 0.855f, 0.812f, 0.600f };
    ImVec4 ah_roll_label     = { 0.722f, 0.761f, 0.702f, 0.600f };
    ImVec4 ah_roll_pointer   = { 0.969f, 0.812f, 0.396f, 0.950f };
    ImVec4 ah_border         = { 0.275f, 0.333f, 0.184f, 0.900f };

    // ── Map ───────────────────────────────────────────────────────────────────
    ImVec4 map_tile_placeholder = { 0.047f, 0.059f, 0.031f, 1.000f };
    ImVec4 map_vehicle_fill     = { 0.592f, 0.918f, 0.537f, 1.000f };
    ImVec4 map_vehicle_ring     = { 0.925f, 0.725f, 0.306f, 0.850f };
    ImVec4 map_attr_bg          = { 0.047f, 0.059f, 0.031f, 0.800f };
    ImVec4 map_attr_text        = { 0.431f, 0.482f, 0.333f, 0.850f };

    // ── EKF ───────────────────────────────────────────────────────────────────
    ImVec4 ekf_bg      = { 0.094f, 0.114f, 0.063f, 1.000f };
    ImVec4 ekf_outline = { 0.243f, 0.290f, 0.173f, 0.900f };
    ImVec4 ekf_label   = { 0.431f, 0.482f, 0.333f, 0.900f };
};

// Single runtime-mutable instance — all color functions read from here.
inline ThemeVars g_theme;

// ── Section 2: Color accessor functions ──────────────────────────────────────
// Thin wrappers so all call sites compile without change.

inline ImVec4 accent_col()        { return g_theme.accent; }
inline ImVec4 panel_bg()          { return g_theme.bg_panel; }

inline ImVec4 bg_topbar()         { return g_theme.bg_topbar; }
inline ImVec4 bg_bottombar()      { return g_theme.bg_bottombar; }
inline ImVec4 bg_sidebar_left()   { return g_theme.bg_sidebar_left; }
inline ImVec4 bg_video()          { return g_theme.bg_video; }
inline ImVec4 bg_map()            { return g_theme.bg_map; }
inline ImVec4 bg_child_dark()     { return g_theme.bg_child_dark; }
inline ImVec4 bg_child_darker()   { return g_theme.bg_child_darker; }
inline ImVec4 bg_param_list()     { return g_theme.bg_param_list; }

inline ImVec4 col_separator()     { return g_theme.separator; }
inline ImVec4 col_border_tab()    { return g_theme.border_tab; }
inline ImVec4 col_border_form()   { return g_theme.border_form; }

// Panel seam line color — replaces the old COL32_PANEL_BORDER constexpr.
inline ImU32  col32_panel_border()
{
    return ImGui::ColorConvertFloat4ToU32(g_theme.panel_border);
}

inline ImVec4 col_app_header()    { return g_theme.accent; }
inline ImVec4 col_armed()         { return g_theme.col_armed; }
inline ImVec4 col_disarmed()      { return g_theme.col_disarmed; }
inline ImVec4 col_no_link()       { return g_theme.col_no_link; }
inline ImVec4 col_no_link_muted() { return g_theme.col_no_link_muted; }
inline ImVec4 col_error()         { return g_theme.col_error; }
inline ImVec4 col_error_alt()     { return g_theme.col_error_alt; }
inline ImVec4 col_ok()            { return g_theme.col_ok; }
inline ImVec4 col_data()          { return g_theme.col_data; }
inline ImVec4 col_warning()       { return g_theme.col_warning; }
inline ImVec4 col_reboot()        { return g_theme.col_reboot; }
inline ImVec4 col_active_text()   { return g_theme.col_active_text; }
inline ImVec4 col_text_on_dark()  { return g_theme.col_text_on_dark; }
inline ImVec4 col_log()           { return g_theme.col_log; }

// ── Section 3: Computed colors (derived from palette) ────────────────────────

inline ImVec4 col_battery(float frac)
{
    if (frac > 0.4f) return g_theme.col_disarmed;
    if (frac > 0.2f) return g_theme.col_warning;
    return g_theme.col_error;
}

inline ImVec4 col_status_severity(int severity)
{
    if (severity <= 2) return g_theme.col_error;
    if (severity == 3) return { g_theme.col_warning.x, g_theme.col_warning.y * 0.82f, 0.0f, 1.0f };
    if (severity == 4) return { g_theme.accent.x, g_theme.accent.y * 1.09f, 0.0f, 1.0f };
    if (severity == 5) return g_theme.accent;
    return g_theme.col_log;
}

// ── Section 4: Named button color accessors ───────────────────────────────────

inline ImVec4 btn_arm_base()           { return g_theme.btn_arm_base; }
inline ImVec4 btn_arm_hov()            { return g_theme.btn_arm_hov; }
inline ImVec4 btn_disarm_base()        { return g_theme.btn_disarm_base; }
inline ImVec4 btn_disarm_hov()         { return g_theme.btn_disarm_hov; }
inline ImVec4 btn_disconnect_base()    { return g_theme.btn_disconnect_base; }
inline ImVec4 btn_disconnect_hov()     { return g_theme.btn_disconnect_hov; }
inline ImVec4 btn_stop_base()          { return g_theme.btn_stop_base; }
inline ImVec4 btn_stop_hov()           { return g_theme.btn_stop_hov; }
inline ImVec4 btn_stop_act()           { return g_theme.btn_stop_act; }
inline ImVec4 btn_intlk_on_base()      { return g_theme.btn_intlk_on_base; }
inline ImVec4 btn_intlk_on_hov()       { return g_theme.btn_intlk_on_hov; }
inline ImVec4 btn_intlk_off_base()     { return g_theme.btn_intlk_off_base; }
inline ImVec4 btn_intlk_off_hov()      { return g_theme.btn_intlk_off_hov; }
inline ImVec4 btn_mode_active_base()   { return g_theme.btn_mode_active_base; }
inline ImVec4 btn_mode_active_hov()    { return g_theme.btn_mode_active_hov; }
inline ImVec4 btn_write_base()         { return g_theme.btn_write_base; }
inline ImVec4 btn_write_hov()          { return g_theme.btn_write_hov; }
inline ImVec4 btn_tab_active_base()    { return g_theme.btn_tab_active_base; }
inline ImVec4 btn_tab_active_hov()     { return g_theme.btn_tab_active_hov; }

// ── Plugin rail ──────────────────────────────────────────────────────────────

inline ImVec4 plugin_accent()       { return g_theme.plugin_accent; }
inline ImVec4 plugin_accent_dim()   { return g_theme.plugin_accent_dim; }

// An engaged plugin button wears the ARMED red, but washed back over the panel
// rather than laid on solid: the annunciator is the one that has to win the
// room when the vehicle is armed, and a rail of full-strength plates would
// argue with it. Hover firms the plate up without ever reaching the slab.
inline ImVec4 btn_plugin_engaged_base()
{
    ImVec4 red = g_theme.col_armed;
    red.w *= 0.52f;
    return red;
}
inline ImVec4 btn_plugin_engaged_hov()
{
    ImVec4 red = g_theme.col_armed;
    red.w *= 0.66f;
    return red;
}

// ── Section 5: Command flash push helpers ────────────────────────────────────

inline void push_flash_colors(CmdFlashState fs)
{
    switch (fs) {
    case CmdFlashState::Accepted:
        ImGui::PushStyleColor(ImGuiCol_Button,        g_theme.flash_accepted_base);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, g_theme.flash_accepted_hov);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  g_theme.flash_accepted_base);
        break;
    case CmdFlashState::Rejected:
        ImGui::PushStyleColor(ImGuiCol_Button,        g_theme.flash_rejected_base);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, g_theme.flash_rejected_hov);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  g_theme.flash_rejected_base);
        break;
    case CmdFlashState::Pending:
        ImGui::PushStyleColor(ImGuiCol_Button,        g_theme.flash_pending_base);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, g_theme.flash_pending_hov);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  g_theme.flash_pending_base);
        break;
    default: // Normal — dark plate with themed amber hover glow
        ImGui::PushStyleColor(ImGuiCol_Button,        g_theme.btn_intlk_off_base);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, g_theme.flash_normal_hov);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  g_theme.flash_normal_press);
        break;
    }
}

inline void push_solid_flash_colors(CmdFlashState fs, ImVec4 base_btn, ImVec4 base_hov)
{
    switch (fs) {
    case CmdFlashState::Accepted:
        ImGui::PushStyleColor(ImGuiCol_Button,        g_theme.flash_accepted_base);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, g_theme.flash_accepted_hov);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  g_theme.flash_accepted_base);
        break;
    case CmdFlashState::Rejected:
        ImGui::PushStyleColor(ImGuiCol_Button,        g_theme.flash_rejected_base);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, g_theme.flash_rejected_hov);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  g_theme.flash_rejected_base);
        break;
    case CmdFlashState::Pending:
        ImGui::PushStyleColor(ImGuiCol_Button,        g_theme.flash_pending_base);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, g_theme.flash_pending_hov);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  g_theme.flash_pending_base);
        break;
    default:
        ImGui::PushStyleColor(ImGuiCol_Button,        base_btn);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, base_hov);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  base_btn);
        break;
    }
}

// ── Section 6: Built-in theme factories ──────────────────────────────────────

// Retro Amber — the original phosphor-terminal palette (amber on warm black).
inline ThemeVars retro_amber_theme_vars()
{
    ThemeVars t;

    t.accent                 = { 1.0f,  0.69f, 0.0f,  1.0f }; // bright amber #FFB000
    t.col_data               = { 1.0f,  0.82f, 0.15f, 1.0f }; // gold — telemetry values
    t.col_ok                 = { 1.0f,  0.85f, 0.18f, 1.0f }; // bright gold — OK/good state
    t.col_warning            = { 1.0f,  0.55f, 0.0f,  1.0f }; // orange-amber
    t.col_error              = { 1.0f,  0.10f, 0.20f, 1.0f }; // hot red
    t.col_error_alt          = { 1.0f,  0.22f, 0.30f, 1.0f };
    t.col_log                = { 0.72f, 0.46f, 0.0f,  1.0f }; // dim amber — console log
    t.col_armed              = { 1.0f,  0.10f, 0.20f, 1.0f }; // hot red
    t.col_disarmed           = { 1.0f,  0.69f, 0.0f,  1.0f }; // amber
    t.col_no_link            = { 0.80f, 0.50f, 0.0f,  1.0f }; // dim amber
    t.col_no_link_muted      = { 0.42f, 0.26f, 0.0f,  1.0f };
    t.col_reboot             = { 1.0f,  0.60f, 0.0f,  1.0f }; // amber-orange
    t.col_active_text        = { 0.06f, 0.03f, 0.00f, 1.0f };
    t.col_text_on_dark       = { 1.00f, 0.87f, 0.65f, 1.0f }; // dark — text on bright bg

    // ── Backgrounds ───────────────────────────────────────────────────────────
    t.bg_panel               = { 0.05f, 0.03f, 0.00f, 1.0f };
    t.bg_topbar              = { 0.04f, 0.02f, 0.00f, 1.0f };
    t.bg_bottombar           = { 0.04f, 0.02f, 0.00f, 1.0f };
    t.bg_sidebar_left        = { 0.05f, 0.03f, 0.01f, 1.0f };
    t.bg_video               = { 0.03f, 0.02f, 0.00f, 1.0f };
    t.bg_map                 = { 0.04f, 0.02f, 0.00f, 1.0f };
    t.bg_child_dark          = { 0.07f, 0.04f, 0.01f, 1.0f };
    t.bg_child_darker        = { 0.05f, 0.03f, 0.01f, 1.0f };
    t.bg_param_list          = { 0.04f, 0.02f, 0.00f, 1.0f };

    // ── Borders ───────────────────────────────────────────────────────────────
    t.separator              = { 0.62f, 0.38f, 0.0f,  1.0f };
    t.border_tab             = { 0.68f, 0.42f, 0.0f,  0.75f };
    t.border_form            = { 0.55f, 0.34f, 0.0f,  0.65f };
    t.panel_border           = { 0.608f, 0.373f, 0.0f, 0.784f }; // panel seam lines

    // ── Named buttons ─────────────────────────────────────────────────────────
    t.btn_arm_base           = { 0.55f, 0.04f, 0.10f, 1.0f };
    t.btn_arm_hov            = { 0.78f, 0.08f, 0.16f, 1.0f };
    t.btn_disarm_base        = { 0.38f, 0.23f, 0.0f,  1.0f };
    t.btn_disarm_hov         = { 0.54f, 0.34f, 0.0f,  1.0f };
    t.btn_disconnect_base    = { 0.38f, 0.04f, 0.08f, 1.0f };
    t.btn_disconnect_hov     = { 0.55f, 0.08f, 0.14f, 1.0f };
    t.btn_stop_base          = { 0.50f, 0.04f, 0.08f, 0.85f };
    t.btn_stop_hov           = { 0.72f, 0.08f, 0.14f, 1.00f };
    t.btn_stop_act           = { 0.38f, 0.03f, 0.06f, 1.00f };
    t.btn_intlk_on_base      = { 0.44f, 0.27f, 0.0f,  1.0f };
    t.btn_intlk_on_hov       = { 0.62f, 0.38f, 0.0f,  1.0f };
    t.btn_intlk_off_base     = { 0.10f, 0.06f, 0.01f, 1.0f };
    t.btn_intlk_off_hov      = { 0.16f, 0.10f, 0.02f, 1.0f };
    t.btn_mode_active_base   = { 0.38f, 0.23f, 0.0f,  1.0f };
    t.btn_mode_active_hov    = { 0.54f, 0.34f, 0.0f,  1.0f };
    t.btn_write_base         = { 0.38f, 0.24f, 0.0f,  1.0f };
    t.btn_write_hov          = { 0.55f, 0.35f, 0.0f,  1.0f };
    t.btn_tab_active_base    = { 1.0f,  0.75f, 0.0f,  0.70f };
    t.btn_tab_active_hov     = { 1.0f,  0.75f, 0.0f,  0.80f };

    // ── Command flash ─────────────────────────────────────────────────────────
    t.flash_accepted_base    = { 0.08f, 0.52f, 0.16f, 0.72f }; // semi-transparent green
    t.flash_accepted_hov     = { 0.12f, 0.68f, 0.22f, 0.80f };
    t.flash_rejected_base    = { 0.65f, 0.06f, 0.12f, 1.0f };
    t.flash_rejected_hov     = { 0.82f, 0.10f, 0.18f, 1.0f };
    t.flash_pending_base     = { 0.50f, 0.30f, 0.0f,  1.0f };
    t.flash_pending_hov      = { 0.68f, 0.42f, 0.0f,  1.0f };
    t.flash_normal_hov       = { 1.0f,  0.69f, 0.0f,  0.14f };
    t.flash_normal_press     = { 1.0f,  0.69f, 0.0f,  0.24f };

    // ── Artificial horizon ────────────────────────────────────────────────────
    t.ah_sky                 = { 0.000f, 0.000f, 0.000f, 0.000f };
    t.ah_ground              = { 0.314f, 0.180f, 0.000f, 0.784f };
    t.ah_horizon_line        = { 1.000f, 0.690f, 0.000f, 0.902f };
    t.ah_pitch_ladder        = { 0.824f, 0.529f, 0.000f, 0.627f };
    t.ah_pitch_label_bg      = { 0.047f, 0.027f, 0.004f, 0.431f };
    t.ah_pitch_label         = { 0.902f, 0.580f, 0.000f, 0.784f };
    t.ah_crosshair           = { 1.000f, 0.690f, 0.000f, 1.000f };
    t.ah_overlay_bg          = { 0.047f, 0.027f, 0.004f, 0.725f };
    t.ah_airspeed_text       = { 1.000f, 0.690f, 0.000f, 1.000f };
    t.ah_throttle_text       = { 1.000f, 0.824f, 0.235f, 1.000f };
    t.ah_heading_text        = { 1.000f, 0.784f, 0.392f, 0.902f };
    t.ah_roll_arc            = { 0.627f, 0.373f, 0.000f, 0.431f };
    t.ah_roll_tick           = { 0.824f, 0.510f, 0.000f, 0.608f };
    t.ah_roll_label          = { 0.745f, 0.463f, 0.000f, 0.608f };
    t.ah_roll_pointer        = { 1.000f, 0.824f, 0.157f, 0.902f };
    t.ah_border              = { 0.588f, 0.353f, 0.000f, 0.608f };

    // ── Map ───────────────────────────────────────────────────────────────────
    t.map_tile_placeholder   = { 0.047f, 0.027f, 0.004f, 1.000f };
    t.map_vehicle_fill       = { 1.000f, 0.690f, 0.000f, 1.000f };
    t.map_vehicle_ring       = { 1.000f, 0.824f, 0.235f, 0.824f };
    t.map_attr_bg            = { 0.047f, 0.027f, 0.004f, 0.784f };
    t.map_attr_text          = { 0.824f, 0.529f, 0.000f, 0.784f };

    // ── EKF ───────────────────────────────────────────────────────────────────
    t.ekf_bg                 = { 0.047f, 0.027f, 0.004f, 1.000f };
    t.ekf_outline            = { 0.549f, 0.333f, 0.000f, 0.706f };
    t.ekf_label              = { 0.902f, 0.580f, 0.000f, 0.784f };

    return t;
}

// Matrix — phosphor green on near-black; classic falling-code aesthetic with
// old-Mac-terminal warmth.  Primary: #00FF41.  Background: deep green-black.
inline ThemeVars matrix_theme_vars()
{
    ThemeVars t;

    // ── Palette ───────────────────────────────────────────────────────────────
    t.accent            = { 0.000f, 1.000f, 0.255f, 1.0f }; // phosphor green #00FF41
    t.col_data          = { 0.000f, 0.800f, 0.200f, 1.0f }; // mid green      #00CC33
    t.col_ok            = { 0.000f, 1.000f, 0.000f, 1.0f }; // pure green     #00FF00
    t.col_warning       = { 0.667f, 1.000f, 0.000f, 1.0f }; // yellow-green   #AAFF00
    t.col_error         = { 1.000f, 0.133f, 0.200f, 1.0f }; // hot red        #FF2233
    t.col_error_alt     = { 1.000f, 0.267f, 0.400f, 1.0f };
    t.col_log           = { 0.000f, 0.400f, 0.067f, 1.0f }; // dim green      #006611
    t.col_armed         = { 1.000f, 0.133f, 0.200f, 1.0f }; // hot red
    t.col_disarmed      = { 0.000f, 1.000f, 0.255f, 1.0f }; // phosphor green
    t.col_no_link       = { 0.000f, 0.533f, 0.133f, 1.0f }; // dim green      #008822
    t.col_no_link_muted = { 0.000f, 0.267f, 0.067f, 1.0f }; // very dim green
    t.col_reboot        = { 0.667f, 1.000f, 0.000f, 1.0f }; // yellow-green
    t.col_active_text   = { 0.008f, 0.039f, 0.008f, 1.0f };
    t.col_text_on_dark  = { 0.800f, 1.000f, 0.850f, 1.0f }; // near-black (text on bright bg)

    // ── Backgrounds ───────────────────────────────────────────────────────────
    t.bg_panel          = { 0.012f, 0.047f, 0.012f, 1.0f }; // ~#030C03
    t.bg_topbar         = { 0.008f, 0.031f, 0.008f, 1.0f }; // ~#020803
    t.bg_bottombar      = { 0.008f, 0.031f, 0.008f, 1.0f };
    t.bg_sidebar_left   = { 0.016f, 0.055f, 0.016f, 1.0f }; // ~#040E04
    t.bg_video          = { 0.004f, 0.020f, 0.004f, 1.0f };
    t.bg_map            = { 0.008f, 0.031f, 0.008f, 1.0f };
    t.bg_child_dark     = { 0.020f, 0.071f, 0.020f, 1.0f };
    t.bg_child_darker   = { 0.016f, 0.055f, 0.016f, 1.0f };
    t.bg_param_list     = { 0.012f, 0.047f, 0.012f, 1.0f };

    // ── Borders ───────────────────────────────────────────────────────────────
    t.separator         = { 0.000f, 0.600f, 0.150f, 0.70f };
    t.border_tab        = { 0.000f, 0.600f, 0.150f, 0.60f };
    t.border_form       = { 0.000f, 0.600f, 0.150f, 0.45f };
    t.panel_border      = { 0.000f, 0.600f, 0.150f, 0.55f };

    // ── Named buttons ─────────────────────────────────────────────────────────
    t.btn_arm_base         = { 0.55f,  0.04f,  0.10f,  1.0f }; // dark red
    t.btn_arm_hov          = { 0.78f,  0.08f,  0.16f,  1.0f };
    t.btn_disarm_base      = { 0.000f, 0.250f, 0.060f, 1.0f }; // dark green
    t.btn_disarm_hov       = { 0.000f, 0.370f, 0.090f, 1.0f };
    t.btn_disconnect_base  = { 0.38f,  0.04f,  0.08f,  1.0f };
    t.btn_disconnect_hov   = { 0.55f,  0.08f,  0.14f,  1.0f };
    t.btn_stop_base        = { 0.50f,  0.04f,  0.08f,  0.85f };
    t.btn_stop_hov         = { 0.72f,  0.08f,  0.14f,  1.00f };
    t.btn_stop_act         = { 0.38f,  0.03f,  0.06f,  1.00f };
    t.btn_intlk_on_base    = { 0.000f, 0.300f, 0.070f, 1.0f }; // dark green
    t.btn_intlk_on_hov     = { 0.000f, 0.450f, 0.110f, 1.0f };
    t.btn_intlk_off_base   = { 0.016f, 0.055f, 0.016f, 1.0f };
    t.btn_intlk_off_hov    = { 0.024f, 0.078f, 0.024f, 1.0f };
    t.btn_mode_active_base = { 0.000f, 0.250f, 0.060f, 1.0f };
    t.btn_mode_active_hov  = { 0.000f, 0.370f, 0.090f, 1.0f };
    t.btn_write_base       = { 0.000f, 0.250f, 0.060f, 1.0f };
    t.btn_write_hov        = { 0.000f, 0.370f, 0.090f, 1.0f };
    t.btn_tab_active_base  = { 0.000f, 1.000f, 0.255f, 0.65f }; // phosphor semi
    t.btn_tab_active_hov   = { 0.000f, 1.000f, 0.255f, 0.80f };

    // ── Command flash ─────────────────────────────────────────────────────────
    t.flash_accepted_base  = { 0.000f, 0.420f, 0.100f, 0.72f }; // green semi
    t.flash_accepted_hov   = { 0.000f, 0.560f, 0.140f, 0.80f };
    t.flash_rejected_base  = { 0.65f,  0.06f,  0.12f,  1.0f  }; // red
    t.flash_rejected_hov   = { 0.82f,  0.10f,  0.18f,  1.0f  };
    t.flash_pending_base   = { 0.300f, 0.450f, 0.000f, 1.0f  }; // yellow-green
    t.flash_pending_hov    = { 0.450f, 0.650f, 0.000f, 1.0f  };
    t.flash_normal_hov     = { 0.000f, 1.000f, 0.255f, 0.14f }; // phosphor glow
    t.flash_normal_press   = { 0.000f, 1.000f, 0.255f, 0.24f };

    // ── Artificial horizon ────────────────────────────────────────────────────
    t.ah_sky            = { 0.000f, 0.000f, 0.000f, 0.000f };    // transparent
    t.ah_ground         = { 0.000f, 0.157f, 0.039f, 0.784f };    // dark green tint
    t.ah_horizon_line   = { 0.000f, 1.000f, 0.255f, 0.902f };    // phosphor green
    t.ah_pitch_ladder   = { 0.000f, 0.667f, 0.157f, 0.627f };    // dim green
    t.ah_pitch_label_bg = { 0.008f, 0.047f, 0.012f, 0.431f };
    t.ah_pitch_label    = { 0.000f, 0.800f, 0.200f, 0.784f };
    t.ah_crosshair      = { 0.000f, 1.000f, 0.255f, 1.000f };    // phosphor green
    t.ah_overlay_bg     = { 0.008f, 0.039f, 0.012f, 0.725f };
    t.ah_airspeed_text  = { 0.000f, 1.000f, 0.255f, 1.000f };
    t.ah_throttle_text  = { 0.000f, 0.800f, 0.200f, 1.000f };
    t.ah_heading_text   = { 0.667f, 1.000f, 0.000f, 0.902f };    // yellow-green
    t.ah_roll_arc       = { 0.000f, 0.467f, 0.118f, 0.431f };
    t.ah_roll_tick      = { 0.000f, 0.533f, 0.133f, 0.608f };
    t.ah_roll_label     = { 0.000f, 0.467f, 0.118f, 0.608f };
    t.ah_roll_pointer   = { 0.667f, 1.000f, 0.000f, 0.902f };    // yellow-green
    t.ah_border         = { 0.000f, 0.467f, 0.118f, 0.608f };

    // ── Map ───────────────────────────────────────────────────────────────────
    t.map_tile_placeholder = { 0.008f, 0.039f, 0.012f, 1.000f };
    t.map_vehicle_fill     = { 0.000f, 1.000f, 0.255f, 1.000f }; // phosphor green
    t.map_vehicle_ring     = { 0.667f, 1.000f, 0.000f, 0.824f }; // yellow-green ring
    t.map_attr_bg          = { 0.008f, 0.039f, 0.012f, 0.784f };
    t.map_attr_text        = { 0.000f, 0.533f, 0.133f, 0.784f };

    // ── EKF ───────────────────────────────────────────────────────────────────
    t.ekf_bg      = { 0.008f, 0.039f, 0.012f, 1.000f };
    t.ekf_outline = { 0.000f, 0.467f, 0.118f, 0.706f };
    t.ekf_label   = { 0.000f, 0.800f, 0.200f, 0.784f };

    return t;
}

// ── Section 6: Global ImGui style application ────────────────────────────────
// Call once at startup and whenever g_theme changes.

inline void apply_global_theme(ImGuiStyle& s)
{
    s.WindowRounding    = 0.0f;
    s.FrameRounding     = 0.0f;
    s.ScrollbarRounding = 0.0f;
    s.GrabRounding      = 0.0f;
    s.TabRounding       = 0.0f;
    s.WindowBorderSize  = 1.0f;
    s.FrameBorderSize   = 1.0f;   // every control reads as a boxed cell
    s.ScrollbarSize     = 10.0f;
    s.FramePadding      = { 6.0f, 4.0f };

    const ImVec4& a  = g_theme.accent;
    const ImVec4& bg = g_theme.bg_panel;
    const ImVec4& sp = g_theme.separator;

    ImVec4* c = s.Colors;

    // Backgrounds
    c[ImGuiCol_WindowBg]              = bg;
    c[ImGuiCol_ChildBg]               = { 0.0f, 0.0f, 0.0f, 0.0f };
    c[ImGuiCol_PopupBg]               = g_theme.bg_child_dark;

    // Borders
    c[ImGuiCol_Border]                = { sp.x, sp.y, sp.z, 0.50f };
    c[ImGuiCol_BorderShadow]          = { 0.0f, 0.0f, 0.0f, 0.0f  };

    // Text
    c[ImGuiCol_Text]                  = { 0.812f, 0.831f, 0.769f, 1.0f }; // bone, olive-tinted
    c[ImGuiCol_TextDisabled]          = g_theme.col_no_link_muted;

    // Frames (input fields, combos)
    c[ImGuiCol_FrameBg]               = { bg.x + 0.01f, bg.y, bg.z, 1.0f };
    c[ImGuiCol_FrameBgHovered]        = { a.x * 0.30f, a.y * 0.18f, a.z * 0.30f, 0.50f };
    c[ImGuiCol_FrameBgActive]         = { a.x * 0.45f, a.y * 0.28f, a.z * 0.45f, 0.70f };

    // Title bars
    c[ImGuiCol_TitleBg]               = { bg.x, bg.y * 0.7f, bg.z, 1.0f };
    c[ImGuiCol_TitleBgActive]         = { a.x * 0.20f, a.y * 0.12f, a.z * 0.20f, 1.0f };
    c[ImGuiCol_TitleBgCollapsed]      = { bg.x, bg.y, bg.z, 0.75f };
    c[ImGuiCol_MenuBarBg]             = { bg.x, bg.y, bg.z, 1.0f };

    // Scrollbar
    c[ImGuiCol_ScrollbarBg]           = { bg.x * 0.5f, bg.y * 0.5f, bg.z * 0.5f, 1.0f };
    c[ImGuiCol_ScrollbarGrab]         = { a.x * 0.35f, a.y * 0.21f, a.z * 0.35f, 1.0f };
    c[ImGuiCol_ScrollbarGrabHovered]  = { a.x * 0.50f, a.y * 0.30f, a.z * 0.50f, 1.0f };
    c[ImGuiCol_ScrollbarGrabActive]   = { a.x * 0.65f, a.y * 0.40f, a.z * 0.65f, 1.0f };

    // Check / Slider
    c[ImGuiCol_CheckMark]             = g_theme.accent;
    c[ImGuiCol_SliderGrab]            = { a.x * 0.60f, a.y * 0.37f, a.z * 0.60f, 1.0f };
    c[ImGuiCol_SliderGrabActive]      = g_theme.accent;

    // Buttons — dark plate, amber wash on hover
    c[ImGuiCol_Button]                = g_theme.btn_intlk_off_base;
    c[ImGuiCol_ButtonHovered]         = g_theme.flash_normal_hov;
    c[ImGuiCol_ButtonActive]          = g_theme.flash_normal_press;

    // Headers (combo rows, selectables)
    c[ImGuiCol_Header]                = { a.x * 0.35f, a.y * 0.21f, a.z * 0.35f, 0.50f };
    c[ImGuiCol_HeaderHovered]         = { a.x * 0.52f, a.y * 0.32f, a.z * 0.52f, 0.60f };
    c[ImGuiCol_HeaderActive]          = { a.x * 0.70f, a.y * 0.43f, a.z * 0.70f, 0.70f };

    // Separators
    c[ImGuiCol_Separator]             = { sp.x, sp.y, sp.z, 0.50f };
    c[ImGuiCol_SeparatorHovered]      = { sp.x * 1.30f, sp.y * 1.30f, sp.z * 1.30f, 0.70f };
    c[ImGuiCol_SeparatorActive]       = g_theme.accent;

    // Resize grip
    c[ImGuiCol_ResizeGrip]            = { a.x * 0.55f, a.y * 0.34f, a.z * 0.55f, 0.25f };
    c[ImGuiCol_ResizeGripHovered]     = { a.x * 0.70f, a.y * 0.43f, a.z * 0.70f, 0.60f };
    c[ImGuiCol_ResizeGripActive]      = g_theme.accent;

    // Tabs
    c[ImGuiCol_Tab]                   = { bg.x + 0.01f, bg.y, bg.z, 1.0f };
    c[ImGuiCol_TabHovered]            = { a.x * 0.44f, a.y * 0.27f, a.z * 0.44f, 0.70f };
    c[ImGuiCol_TabActive]             = { a.x * 0.28f, a.y * 0.17f, a.z * 0.28f, 1.0f };
    c[ImGuiCol_TabUnfocused]          = { bg.x, bg.y, bg.z, 1.0f };
    c[ImGuiCol_TabUnfocusedActive]    = { a.x * 0.16f, a.y * 0.10f, a.z * 0.16f, 1.0f };

    // Tables
    c[ImGuiCol_TableHeaderBg]         = g_theme.bg_child_dark;
    c[ImGuiCol_TableBorderStrong]     = { sp.x * 0.89f, sp.y * 0.89f, sp.z * 0.89f, 0.80f };
    c[ImGuiCol_TableBorderLight]      = { sp.x * 0.57f, sp.y * 0.57f, sp.z * 0.57f, 0.50f };
    c[ImGuiCol_TableRowBg]            = { 0.0f, 0.0f, 0.0f, 0.0f };
    c[ImGuiCol_TableRowBgAlt]         = { a.x, a.y, a.z, 0.04f };

    // Nav
    c[ImGuiCol_NavHighlight]          = g_theme.accent;
    c[ImGuiCol_NavWindowingHighlight] = { a.x, a.y, a.z, 0.70f };
    c[ImGuiCol_NavWindowingDimBg]     = { 0.0f, 0.0f, 0.0f, 0.20f };
    c[ImGuiCol_ModalWindowDimBg]      = { 0.0f, 0.0f, 0.0f, 0.45f };
}

// ── Section 7: Shared separator helper ───────────────────────────────────────

inline void themed_sep()
{
    ImGui::PushStyleColor(ImGuiCol_Separator, col_separator());
    ImGui::Separator();
    ImGui::PopStyleColor();
}

// ── Section 8: Artificial horizon color accessors ─────────────────────────────
// (values live in ThemeVars — use these instead of the old constexpr ImU32)

inline ImU32 ah_sky()            { return ImGui::ColorConvertFloat4ToU32(g_theme.ah_sky); }
inline ImU32 ah_ground()         { return ImGui::ColorConvertFloat4ToU32(g_theme.ah_ground); }
inline ImU32 ah_horizon_line()   { return ImGui::ColorConvertFloat4ToU32(g_theme.ah_horizon_line); }
inline ImU32 ah_pitch_ladder()   { return ImGui::ColorConvertFloat4ToU32(g_theme.ah_pitch_ladder); }
inline ImU32 ah_pitch_label_bg() { return ImGui::ColorConvertFloat4ToU32(g_theme.ah_pitch_label_bg); }
inline ImU32 ah_pitch_label()    { return ImGui::ColorConvertFloat4ToU32(g_theme.ah_pitch_label); }
inline ImU32 ah_crosshair()      { return ImGui::ColorConvertFloat4ToU32(g_theme.ah_crosshair); }
inline ImU32 ah_overlay_bg()     { return ImGui::ColorConvertFloat4ToU32(g_theme.ah_overlay_bg); }
inline ImU32 ah_airspeed_text()  { return ImGui::ColorConvertFloat4ToU32(g_theme.ah_airspeed_text); }
inline ImU32 ah_throttle_text()  { return ImGui::ColorConvertFloat4ToU32(g_theme.ah_throttle_text); }
inline ImU32 ah_heading_text()   { return ImGui::ColorConvertFloat4ToU32(g_theme.ah_heading_text); }
inline ImU32 ah_roll_arc()       { return ImGui::ColorConvertFloat4ToU32(g_theme.ah_roll_arc); }
inline ImU32 ah_roll_tick()      { return ImGui::ColorConvertFloat4ToU32(g_theme.ah_roll_tick); }
inline ImU32 ah_roll_label()     { return ImGui::ColorConvertFloat4ToU32(g_theme.ah_roll_label); }
inline ImU32 ah_roll_pointer()   { return ImGui::ColorConvertFloat4ToU32(g_theme.ah_roll_pointer); }
inline ImU32 ah_border()         { return ImGui::ColorConvertFloat4ToU32(g_theme.ah_border); }

// ── Section 9: Map color accessors ───────────────────────────────────────────

inline ImU32 map_tile_placeholder() { return ImGui::ColorConvertFloat4ToU32(g_theme.map_tile_placeholder); }
inline ImU32 map_vehicle_fill()     { return ImGui::ColorConvertFloat4ToU32(g_theme.map_vehicle_fill); }
inline ImU32 map_vehicle_ring()     { return ImGui::ColorConvertFloat4ToU32(g_theme.map_vehicle_ring); }
inline ImU32 map_attr_bg()          { return ImGui::ColorConvertFloat4ToU32(g_theme.map_attr_bg); }
inline ImU32 map_attr_text()        { return ImGui::ColorConvertFloat4ToU32(g_theme.map_attr_text); }

// ── Section 10: EKF color accessors ──────────────────────────────────────────

inline ImU32 ekf_bg()      { return ImGui::ColorConvertFloat4ToU32(g_theme.ekf_bg); }
inline ImU32 ekf_outline()  { return ImGui::ColorConvertFloat4ToU32(g_theme.ekf_outline); }
inline ImU32 ekf_label()    { return ImGui::ColorConvertFloat4ToU32(g_theme.ekf_label); }

// ── Section 11: Shared style dimensions ──────────────────────────────────────

static constexpr float FRAME_ROUNDING_SM   = 0.0f;
static constexpr float FRAME_ROUNDING_MD   = 0.0f;
static constexpr float FRAME_BORDER_NORMAL = 1.0f;
static constexpr float FRAME_BORDER_BOLD   = 1.5f;
static constexpr float PANEL_BORDER_THICKNESS = 1.5f;

// ── Section 12: Artificial horizon line thicknesses ──────────────────────────

static constexpr float AH_THICK_HORIZON    = 2.5f;
static constexpr float AH_THICK_CROSSHAIR  = 3.0f;
static constexpr float AH_THICK_PITCH_BOLD = 2.0f;
static constexpr float AH_THICK_PITCH_THIN = 1.4f;
static constexpr float AH_THICK_ROLL_ARC   = 2.0f;
static constexpr float AH_THICK_ROLL_TICK  = 1.8f;
static constexpr float AH_THICK_BORDER     = 2.0f;
