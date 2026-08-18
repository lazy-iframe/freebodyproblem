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


#include "topbar.hpp"
#include "layout.hpp"
#include "ui_kit.hpp"
#include "imgui.h"
#include "../app_log.hpp"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <string>

// ── Geometry ─────────────────────────────────────────────────────────────────

static constexpr float BTN_H       = 34.0f;
// Interlock and arm annunciators are identical slabs — sized for the longest
// caption either can show ("INTLK HIGH" at UI_SZ_BIG) so both stay uniform.
static constexpr float BTN_W_INTLK = 184.0f;
static constexpr float BTN_W_ARM   = 184.0f;
static constexpr float BTN_GAP     =  6.0f;
static constexpr float BTN_CLOSE   = 22.0f;   // × exit button

static constexpr float CELL_PAD    = 13.0f;   // horizontal padding inside a cell
static constexpr float LABEL_Y     = 10.0f;   // baseline row for the tiny label
static constexpr float VALUE_Y     = 28.0f;   // baseline row for the value

static bool s_interlock = false;

// ── Helpers ──────────────────────────────────────────────────────────────────

// ArduCopter custom_mode → short name. Only correct for Copter — mode
// numbering is frame-specific, so this mislabels a Plane (Plane 3 is TRAINING,
// Copter 3 is AUTO). Used only when the vehicle does not publish
// AVAILABLE_MODES; see mode_display_name() below.
static const char* mode_short_name_fallback(uint32_t custom_mode)
{
    switch (custom_mode) {
    case 0:  return "STAB";
    case 1:  return "ACRO";
    case 2:  return "ALTH";
    case 3:  return "AUTO";
    case 4:  return "GUID";
    case 5:  return "LOIT";
    case 6:  return "RTL";
    case 7:  return "CIRC";
    case 9:  return "LAND";
    case 11: return "DRFT";
    case 13: return "SPRT";
    case 16: return "POSH";
    case 17: return "BRAKE";
    case 20: return "GUID-NG";
    case 21: return "SMRTRTL";
    default: return "MODE ?";
    }
}

// Resolve the current custom_mode to a display name, preferring the vehicle's
// own AVAILABLE_MODES list over the Copter-only table above.
static std::string mode_display_name(const VehicleState& vs)
{
    for (const FlightModeInfo& m : vs.available_modes) {
        if (m.custom_mode == vs.custom_mode) {
            std::string out = m.name;
            for (char& c : out)
                c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            return out;
        }
    }
    return mode_short_name_fallback(vs.custom_mode);
}

static const char* link_status_text(LinkStatus s)
{
    switch (s) {
    case LinkStatus::Connected:  return "ONLINE";
    case LinkStatus::Connecting: return "SYNC";
    case LinkStatus::Timeout:    return "TIMEOUT";
    case LinkStatus::Error:      return "FAULT";
    default:                     return "OFFLINE";
    }
}

void draw_topbar(const VehicleState& vs,
                 const std::unordered_map<uint32_t, MessageStats>& /*stats*/,
                 uint64_t total_messages,
                 uint64_t total_bytes,
                 uint64_t parse_errors,
                 MavlinkSender* sender,
                 LinkStatus link_status,
                 bool* close_requested)
{
    const ImGuiIO& io = ImGui::GetIO();

    ImGui::SetNextWindowPos ({ 0.0f, 0.0f },          ImGuiCond_Always);
    ImGui::SetNextWindowSize({ io.DisplaySize.x, TOPBAR_H }, ImGuiCond_Always);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, bg_topbar());
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f });
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,   { 8.0f, 0.0f });

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar    |
        ImGuiWindowFlags_NoResize      |
        ImGuiWindowFlags_NoMove        |
        ImGuiWindowFlags_NoScrollbar   |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoBringToFrontOnFocus;

    if (ImGui::Begin("##topbar", nullptr, flags)) {
        ImDrawList*  dl  = ImGui::GetWindowDrawList();
        ImFont*      fu  = g_font_ui    ? g_font_ui    : ImGui::GetFont();
        ImFont*      fm  = g_font_micro ? g_font_micro : ImGui::GetFont();
        const ImVec2 wp  = ImGui::GetWindowPos();

        // Bottom-edge seam between topbar and main panels
        dl->AddLine({ wp.x, wp.y + TOPBAR_H },
                    { wp.x + io.DisplaySize.x, wp.y + TOPBAR_H },
                    col32_panel_border(), PANEL_BORDER_THICKNESS);

        // ── Shared colors ─────────────────────────────────────────────────────
        const ImU32 C_LABEL = ui_col_label();
        const ImU32 C_VALUE = ui_col_value();
        const ImU32 C_TEXT  = ui_col_text();
        const ImU32 C_DIM   = ui_col(g_theme.col_no_link_muted);
        const ImU32 C_AMBER = ui_col_accent();
        const ImU32 C_ERROR = ui_col(g_theme.col_error);

        // ── Button area / right-hand annunciators ─────────────────────────────
        const float btn_y      = (TOPBAR_H - BTN_H) * 0.5f;
        const float arm_x0     = io.DisplaySize.x - BTN_W_ARM - 10.0f;
        const float intlk_x0   = arm_x0 - BTN_GAP - BTN_W_INTLK;

        // Mission clock: elapsed since the GCS came up (reset on each link start).
        static double s_clock_epoch = -1.0;
        static bool   s_was_linked  = false;
        const bool    linked        = (link_status == LinkStatus::Connected);
        if (linked && !s_was_linked) s_clock_epoch = ImGui::GetTime();
        s_was_linked = linked;
        if (s_clock_epoch < 0.0) s_clock_epoch = ImGui::GetTime();

        const int    clk    = (int)(ImGui::GetTime() - s_clock_epoch);
        char         clk_s[16];
        snprintf(clk_s, sizeof(clk_s), "%02d:%02d:%02d",
                 clk / 3600, (clk / 60) % 60, clk % 60);
        const float  clk_w  = fu->CalcTextSizeA(17.0f, FLT_MAX, 0.0f, clk_s).x;
        const float  clk_x0 = intlk_x0 - BTN_GAP - clk_w - CELL_PAD * 2.0f;

        // ── Close (×) button — far left ───────────────────────────────────────
        const float close_x = 8.0f;
        const float close_y = (TOPBAR_H - BTN_CLOSE) * 0.5f;
        ImGui::SetCursorPos({ close_x, close_y });
        if (ui_solid_button("\xc3\x97##close", { BTN_CLOSE, BTN_CLOSE },
                            btn_stop_base(), btn_stop_hov())) {
            const bool has_link = (link_status == LinkStatus::Connecting ||
                                   link_status == LinkStatus::Connected);
            if (has_link)
                ImGui::OpenPopup("##confirm_close");
            else
                *close_requested = true;
        }

        // Confirmation modal — centered on screen
        {
            const ImVec2 center = { io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f };
            ImGui::SetNextWindowPos(center, ImGuiCond_Always, { 0.5f, 0.5f });
            if (ui_confirm_popup("##confirm_close", "TERMINATE SESSION",
                                 "Link is active. Exit anyway?",
                                 "EXIT", g_theme.btn_stop_base,
                                 /*modal=*/true) == UiConfirm::Confirmed)
                *close_requested = true;
        }

        // ── Callsign chip ─────────────────────────────────────────────────────
        float x = close_x + BTN_CLOSE + 8.0f;
        {
            const float chip_h = 32.0f;
            const float chip_y = (TOPBAR_H - chip_h) * 0.5f;

            char id_s[24];
            if (vs.has_heartbeat)
                snprintf(id_s, sizeof(id_s), "SYS%d\xc2\xb7%d", (int)vs.sysid, (int)vs.compid);
            else
                snprintf(id_s, sizeof(id_s), "NO A/C");

            const float name_w = ui_tracked_width(fu, 20.0f, "FBP", UI_TRACK * 1.4f);
            const float id_w   = ui_tracked_width(fm, UI_SZ_MICRO, id_s);
            const float chip_w = name_w + id_w + 26.0f;

            const ImVec2 c0 = { wp.x + x,          wp.y + chip_y };
            const ImVec2 c1 = { c0.x + chip_w,     c0.y + chip_h };
            dl->AddRectFilled(c0, c1, C_AMBER);

            const ImU32 on_amber = ui_col(g_theme.col_active_text);
            ui_tracked_text(dl, fu, 20.0f,
                            { c0.x + 9.0f, c0.y + (chip_h - 20.0f) * 0.5f - 1.0f },
                            on_amber, "FBP", UI_TRACK * 1.4f);
            ui_tracked_text(dl, fm, UI_SZ_MICRO,
                            { c0.x + 9.0f + name_w + 8.0f,
                              c0.y + (chip_h - UI_SZ_MICRO) * 0.5f },
                            ui_col(g_theme.col_active_text, 0.75f), id_s);

            x += chip_w + 12.0f;
        }

        // ── Segmented telemetry cells ─────────────────────────────────────────
        // Each cell: tracked micro label over a 15 px value, 1 px rule on the right.
        const float cells_clip_x = clk_x0 - 6.0f;
        dl->PushClipRect({ wp.x, wp.y }, { wp.x + cells_clip_x, wp.y + TOPBAR_H }, true);

        auto cell = [&](const char* label, const char* value,
                        ImU32 label_col, ImU32 value_col) {
            const float lw = ui_tracked_width(fm, UI_SZ_MICRO, label);
            const float vw = fu->CalcTextSizeA(17.0f, FLT_MAX, 0.0f, value).x;
            const float cw = std::max(lw, vw) + CELL_PAD * 2.0f;

            ui_tracked_text(dl, fm, UI_SZ_MICRO,
                            { wp.x + x + CELL_PAD, wp.y + LABEL_Y }, label_col, label);
            dl->AddText(fu, 17.0f,
                        { wp.x + x + CELL_PAD, wp.y + VALUE_Y }, value_col, value);

            // right-hand rule
            dl->AddLine({ wp.x + x + cw, wp.y + 9.0f },
                        { wp.x + x + cw, wp.y + TOPBAR_H - 9.0f },
                        ui_col(g_theme.separator, 0.75f), 1.0f);
            x += cw;
        };

        // DATALINK
        {
            char v[32];
            if (vs.has_heartbeat && total_bytes > 0)
                snprintf(v, sizeof(v), "%s \xc2\xb7 %llu", link_status_text(link_status),
                         (unsigned long long)total_messages);
            else
                snprintf(v, sizeof(v), "%s", link_status_text(link_status));
            const ImU32 vc = (link_status == LinkStatus::Connected) ? C_VALUE
                           : (link_status == LinkStatus::Connecting) ? C_AMBER
                           : (link_status == LinkStatus::Idle)       ? C_DIM
                                                                     : C_ERROR;
            cell("DATALINK", v, C_LABEL, vc);
        }

        // GNSS
        {
            char v[48];
            if (vs.has_gps_raw)
                snprintf(v, sizeof(v), "FIX %d \xc2\xb7 SAT %d \xc2\xb7 H%.1f",
                         (int)vs.gps_fix_type, (int)vs.satellites_vis, (double)vs.hdop);
            else
                snprintf(v, sizeof(v), "NO FIX");
            cell("GNSS", v, C_LABEL, vs.has_gps_raw ? C_VALUE : C_DIM);
        }

        // POSITION
        {
            char v[48];
            if (vs.has_global_pos)
                snprintf(v, sizeof(v), "%.5f%c %.5f%c",
                         vs.lat < 0 ? -vs.lat : vs.lat, vs.lat < 0 ? 'S' : 'N',
                         vs.lon < 0 ? -vs.lon : vs.lon, vs.lon < 0 ? 'W' : 'E');
            else
                snprintf(v, sizeof(v), "-- / --");
            cell("POSITION", v, C_LABEL, vs.has_global_pos ? C_TEXT : C_DIM);
        }

        // ALT AGL
        {
            char v[24];
            if (vs.has_global_pos) snprintf(v, sizeof(v), "%.1f m", (double)vs.alt_rel);
            else                   snprintf(v, sizeof(v), "--");
            cell("ALT AGL", v, C_LABEL, vs.has_global_pos ? C_TEXT : C_DIM);
        }

        // SPEED
        {
            char v[32];
            if (vs.has_vfr)
                snprintf(v, sizeof(v), "A%.1f \xc2\xb7 G%.1f",
                         (double)vs.airspeed, (double)vs.groundspeed);
            else
                snprintf(v, sizeof(v), "--");
            cell("SPEED m/s", v, C_LABEL, vs.has_vfr ? C_TEXT : C_DIM);
        }

        // HDG
        {
            char v[16];
            if (vs.has_vfr) snprintf(v, sizeof(v), "%03d\xc2\xb0", (int)vs.heading);
            else            snprintf(v, sizeof(v), "---");
            cell("HDG", v, C_LABEL, vs.has_vfr ? C_TEXT : C_DIM);
        }

        // PWR
        {
            char  v[32] = "--";
            float frac  = 1.0f;
            if (vs.has_sys_status) {
                if (vs.battery_remaining >= 0) {
                    frac = vs.battery_remaining / 100.0f;
                    snprintf(v, sizeof(v), "%.2fV \xc2\xb7 %d%%",
                             (double)vs.voltage_V, (int)vs.battery_remaining);
                } else {
                    snprintf(v, sizeof(v), "%.2fV", (double)vs.voltage_V);
                }
            }
            const ImU32 vc = vs.has_sys_status
                             ? ImGui::ColorConvertFloat4ToU32(col_battery(frac)) : C_DIM;
            cell("PWR", v, C_LABEL, vc);
        }

        // MODE
        const std::string mode_txt = vs.has_heartbeat ? mode_display_name(vs)
                                                      : std::string("--");
        cell("MODE", mode_txt.c_str(),
             C_LABEL, vs.has_heartbeat ? C_AMBER : C_DIM);

        // LINK DATA / parse errors
        {
            char v[48];
            if (total_bytes >= 1024)
                snprintf(v, sizeof(v), "%.1f kB", total_bytes / 1024.0);
            else
                snprintf(v, sizeof(v), "%llu B", (unsigned long long)total_bytes);
            cell("RX", v, C_LABEL, C_LABEL);

            if (parse_errors > 0) {
                char e[24];
                snprintf(e, sizeof(e), "%llu", (unsigned long long)parse_errors);
                cell("ERR", e, C_ERROR, C_ERROR);
            }
        }

        dl->PopClipRect();

        // ── Mission clock ─────────────────────────────────────────────────────
        {
            ui_tracked_text(dl, fm, UI_SZ_MICRO,
                            { wp.x + clk_x0 + CELL_PAD, wp.y + LABEL_Y },
                            C_LABEL, "MSN TIME");
            dl->AddText(fu, 17.0f, { wp.x + clk_x0 + CELL_PAD, wp.y + VALUE_Y },
                        linked ? C_VALUE : C_DIM, clk_s);
        }

        // ── ARM / DISARM / INTERLOCK ─────────────────────────────────────────
        if (sender) {
            const bool    connected = vs.has_heartbeat;
            const uint8_t tsys     = connected ? vs.sysid  : 1;
            const uint8_t tcomp    = connected ? vs.compid : 1;

            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,   FRAME_ROUNDING_SM);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, FRAME_BORDER_NORMAL);

            // ── INTERLOCK annunciator ─────────────────────────────────────────
            ImGui::SetCursorPos({ intlk_x0, btn_y });
            ImGui::InvisibleButton("##ilkbtn", { BTN_W_INTLK, BTN_H });
            const bool ilk_hovered = ImGui::IsItemHovered();
            {
                const ImVec2 i0 = { wp.x + intlk_x0,    wp.y + btn_y };
                const ImVec2 i1 = { i0.x + BTN_W_INTLK, i0.y + BTN_H };
                if (s_interlock)
                    ui_status_block(dl, i0, i1, "INTLK HIGH", g_theme.col_warning, false);
                else
                    ui_status_block(dl, i0, i1, "INTLK LOW", g_theme.col_no_link,
                                    !ilk_hovered);
            }
            if (ImGui::IsItemClicked()) {
                s_interlock = !s_interlock;
                if (connected) {
                    sender->do_aux_function(tsys, tcomp, 32,
                                            s_interlock ? 2 : 0); // 2=HIGH, 0=LOW
                    gcs_log("interlock \xe2\x86\x92 %s", s_interlock ? "HIGH" : "LOW");
                }
            }

            // ── ARM / DISARM annunciator ──────────────────────────────────────
            ImGui::SetCursorPos({ arm_x0, btn_y });
            ImGui::BeginDisabled(!connected);
            ImGui::InvisibleButton("##armbtn", { BTN_W_ARM, BTN_H });
            const bool arm_hovered = ImGui::IsItemHovered();
            const bool arm_clicked = ImGui::IsItemClicked();
            {
                const ImVec2 a0 = { wp.x + arm_x0,           wp.y + btn_y };
                const ImVec2 a1 = { a0.x + BTN_W_ARM,        a0.y + BTN_H };
                if (!connected) {
                    ui_status_block(dl, a0, a1, "NO LINK", g_theme.col_no_link, true);
                } else if (vs.armed) {
                    ImVec4 red = g_theme.col_armed;
                    if (arm_hovered) { red.x = std::min(1.0f, red.x * 1.12f); }
                    ui_status_block(dl, a0, a1, "ARMED", red, false);
                } else {
                    ImVec4 amber = g_theme.accent;
                    ui_status_block(dl, a0, a1, "DISARMED", amber, !arm_hovered);
                }
            }
            if (arm_clicked) {
                if (vs.armed) ImGui::OpenPopup("##confirm_disarm");
                else          ImGui::OpenPopup("##confirm_arm");
            }
            ImGui::EndDisabled();

            ImGui::PopStyleVar(2);

            // ── Confirmation popups (anchored below the ARM/DISARM block) ────
            const float popup_rx = wp.x + arm_x0 + BTN_W_ARM;
            const float popup_ty = wp.y + btn_y + BTN_H + 4.0f;

            ImGui::SetNextWindowPos({ popup_rx, popup_ty }, ImGuiCond_Always,
                                    { 1.0f, 0.0f }); // right-align to button edge
            if (ui_confirm_popup("##confirm_arm", "ARM VEHICLE",
                                 "Arm the vehicle?",
                                 "CONFIRM ARM", g_theme.btn_arm_base)
                    == UiConfirm::Confirmed) {
                sender->arm(tsys, tcomp);
                gcs_log("arm command sent");
            }

            ImGui::SetNextWindowPos({ popup_rx, popup_ty }, ImGuiCond_Always,
                                    { 1.0f, 0.0f });
            if (ui_confirm_popup("##confirm_disarm", "DISARM VEHICLE",
                                 "Disarm the vehicle?",
                                 "CONFIRM DISARM", g_theme.btn_disarm_base)
                    == UiConfirm::Confirmed) {
                sender->disarm(tsys, tcomp);
                gcs_log("disarm command sent");
            }
        }
    }
    ImGui::End();

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
}
