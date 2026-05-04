// Free Body Problem: A modern, simple and minimalist Ground Control Station
// Copyright (C) 2026  Azhar Tanweer

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.


#include "topbar.hpp"
#include "layout.hpp"
#include "imgui.h"
#include "../app_log.hpp"
#include <algorithm>
#include <cstdio>

static constexpr float BTN_H       = 38.0f;
static constexpr float BTN_W_INTLK = 96.0f;
static constexpr float BTN_W_ARM   = 130.0f;
static constexpr float BTN_GAP     =  8.0f;
static constexpr float BTN_CLOSE   = 26.0f;   // × exit button

static bool s_interlock = false;

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
        ImFont*      fn  = ImGui::GetFont();
        const float  fs  = ImGui::GetFontSize();
        const ImVec2 wp  = ImGui::GetWindowPos();

        // Bottom-edge border line (seam between topbar and main panels)
        dl->AddLine({ wp.x, wp.y + TOPBAR_H },
                    { wp.x + io.DisplaySize.x, wp.y + TOPBAR_H },
                    col32_panel_border(), PANEL_BORDER_THICKNESS);

        // ── Shared colors ─────────────────────────────────────────────────────
        const ImU32 C_ACCENT   = ImGui::ColorConvertFloat4ToU32(accent_col());
        const ImU32 C_DATA     = ImGui::ColorConvertFloat4ToU32(col_data());
        const ImU32 C_DIM      = ImGui::ColorConvertFloat4ToU32(
                                     ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        const ImU32 C_ARMED    = ImGui::ColorConvertFloat4ToU32(col_armed());
        const ImU32 C_DISARMED = ImGui::ColorConvertFloat4ToU32(col_disarmed());
        const ImU32 C_NO_LINK  = ImGui::ColorConvertFloat4ToU32(col_no_link());
        const ImU32 C_ERROR    = ImGui::ColorConvertFloat4ToU32(col_error());

        // ── Two-row geometry ──────────────────────────────────────────────────
        const float text_h  = fn->CalcTextSizeA(fs, FLT_MAX, 0.0f, "A").y;
        const float row_gap = 3.0f;
        const float two_h   = text_h * 2.0f + row_gap;
        const float row1_y  = (TOPBAR_H - two_h) * 0.5f;   // label row
        const float row2_y  = row1_y + text_h + row_gap;   // value row

        const float COL_GAP = 14.0f;  // gap between badge groups
        const float SUB_GAP =  8.0f;  // gap between sub-items within a group

        // Helper: pixel width of a string at current font/size
        auto tw = [&](const char* s) -> float {
            return fn->CalcTextSizeA(fs, FLT_MAX, 0.0f, s).x;
        };

        // Helper: draw text at window-relative position
        auto dt = [&](float rx, float ry, ImU32 col, const char* text) {
            dl->AddText(fn, fs, { wp.x + rx, wp.y + ry }, col, text);
        };

        // ── Button area / title geometry ──────────────────────────────────────
        const float btn_area_w = BTN_W_INTLK + BTN_GAP + BTN_W_ARM + 20.0f;
        const float btn_x0     = io.DisplaySize.x - btn_area_w + 10.0f;
        const float btn_y      = (TOPBAR_H - BTN_H) * 0.5f;

        static constexpr char  TITLE[]  = "FREE BODY PROBLEM";
        static constexpr float TITLE_SZ = 22.0f;
        const ImVec2 title_sz = fn->CalcTextSizeA(TITLE_SZ, FLT_MAX, 0.0f, TITLE);
        const float  title_cx = io.DisplaySize.x * 0.5f - title_sz.x * 0.5f;
        const float  title_cy = (TOPBAR_H - title_sz.y) * 0.5f;

        // ── Centered title (drawn first, no clip rect) ────────────────────────
        // dl->AddText(fn, TITLE_SZ, { wp.x + title_cx, wp.y + title_cy }, C_ACCENT, TITLE);

        // ── Close (×) button — top-left corner ───────────────────────────────
        const float close_x = 8.0f;
        const float close_y = (TOPBAR_H - BTN_CLOSE) * 0.5f;
        ImGui::SetCursorPos({ close_x, close_y });
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,   FRAME_ROUNDING_SM);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_Button,        btn_stop_base());
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, btn_stop_hov());
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  btn_stop_act());
        if (ImGui::Button("\xc3\x97##close", { BTN_CLOSE, BTN_CLOSE })) {
            const bool has_link = (link_status == LinkStatus::Connecting ||
                                   link_status == LinkStatus::Connected);
            if (has_link)
                ImGui::OpenPopup("##confirm_close");
            else
                *close_requested = true;
        }
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(2);

        // Confirmation modal — centered on screen
        {
            const ImVec2 center = { io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f };
            ImGui::SetNextWindowPos(center, ImGuiCond_Always, { 0.5f, 0.5f });
            if (ImGui::BeginPopupModal("##confirm_close", nullptr,
                    ImGuiWindowFlags_AlwaysAutoResize |
                    ImGuiWindowFlags_NoTitleBar       |
                    ImGuiWindowFlags_NoMove)) {
                ImGui::Text("Active connection. Exit anyway?");
                ImGui::Spacing();
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, FRAME_ROUNDING_SM);

                const bool key_confirm = ImGui::IsKeyPressed(ImGuiKey_Enter, false)
                                      || ImGui::IsKeyPressed(ImGuiKey_Y, false);
                const bool key_cancel  = ImGui::IsKeyPressed(ImGuiKey_N, false)
                                      || ImGui::IsKeyPressed(ImGuiKey_Escape, false);

                ImGui::PushStyleColor(ImGuiCol_Button,        btn_stop_base());
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, btn_stop_hov());
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  btn_stop_act());
                const bool do_confirm = ImGui::Button("EXIT", { 80.0f, 0.0f }) || key_confirm;
                ImGui::PopStyleColor(3);
                ImGui::SameLine(0, 8);
                push_flash_colors(CmdFlashState::Normal);
                const bool do_cancel = ImGui::Button("CANCEL##close", { 80.0f, 0.0f }) || key_cancel;
                ImGui::PopStyleColor(3);
                ImGui::Spacing();
                ImGui::TextDisabled("[Y / Enter] exit     [N / Esc] cancel");
                ImGui::PopStyleVar();

                if (do_confirm) {
                    *close_requested = true;
                    ImGui::CloseCurrentPopup();
                } else if (do_cancel) {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }

        // ── Telemetry: clipped to [left edge … title left − margin] ──────────
        const float telem_clip_x = title_cx - 10.0f;
        dl->PushClipRect({ wp.x, wp.y }, { wp.x + telem_clip_x, wp.y + TOPBAR_H }, true);

        // Start telemetry after the close button + gap
        float x = close_x + BTN_CLOSE + 10.0f;

        // Separator helper — vertically-centered "|", advances x
        auto sep = [&]() {
            const float sw = tw("|");
            dt(x, (TOPBAR_H - text_h) * 0.5f, C_DIM, "|");
            x += sw + COL_GAP;
        };

        // ── SYS / COMP IDs ────────────────────────────────────────────────────
        {
            const float vc_y = (TOPBAR_H - text_h) * 0.5f; // vertically centred
            if (vs.has_heartbeat) {
                char sys[32];
                snprintf(sys, sizeof(sys), "SYS:%d  COMP:%d", (int)vs.sysid, (int)vs.compid);
                dt(x, vc_y, C_DATA, sys);
                x += tw(sys) + COL_GAP;
            } else {
                const char* lbl = "NO LINK";
                dt(x, vc_y, C_NO_LINK, lbl);
                x += tw(lbl) + COL_GAP;
            }
        }

        sep();

        // ── Position (LAT / LON / ALT) ────────────────────────────────────────
        {
            char lat_s[24], lon_s[24], alt_s[24];
            if (vs.has_global_pos) {
                snprintf(lat_s, sizeof(lat_s), "%.5f",  vs.lat);
                snprintf(lon_s, sizeof(lon_s), "%.5f",  vs.lon);
                snprintf(alt_s, sizeof(alt_s), "%.1fm", (double)vs.alt_rel);
            } else {
                snprintf(lat_s, sizeof(lat_s), "--");
                snprintf(lon_s, sizeof(lon_s), "--");
                snprintf(alt_s, sizeof(alt_s), "--");
            }
            const ImU32 lc = vs.has_global_pos ? C_ACCENT : C_DIM;
            const ImU32 vc = vs.has_global_pos ? C_DATA   : C_DIM;

            const float wl = std::max(tw("LAT"), tw(lat_s));
            const float wo = std::max(tw("LON"), tw(lon_s));
            const float wa = std::max(tw("ALT"), tw(alt_s));

            dt(x + (wl - tw("LAT")) * 0.5f,                           row1_y, lc, "LAT");
            dt(x + wl + SUB_GAP + (wo - tw("LON")) * 0.5f,            row1_y, lc, "LON");
            dt(x + wl + SUB_GAP + wo + SUB_GAP + (wa - tw("ALT")) * 0.5f, row1_y, lc, "ALT");

            dt(x + (wl - tw(lat_s)) * 0.5f,                           row2_y, vc, lat_s);
            dt(x + wl + SUB_GAP + (wo - tw(lon_s)) * 0.5f,            row2_y, vc, lon_s);
            dt(x + wl + SUB_GAP + wo + SUB_GAP + (wa - tw(alt_s)) * 0.5f, row2_y, vc, alt_s);

            x += wl + SUB_GAP + wo + SUB_GAP + wa + COL_GAP;
        }

        sep();

        // ── Speed / Heading (AIR / GND / HDG) ────────────────────────────────
        {
            char air_s[16], gnd_s[16], hdg_s[16];
            if (vs.has_vfr) {
                snprintf(air_s, sizeof(air_s), "%.1f",      (double)vs.airspeed);
                snprintf(gnd_s, sizeof(gnd_s), "%.1f",      (double)vs.groundspeed);
                snprintf(hdg_s, sizeof(hdg_s), "%d\xc2\xb0", (int)vs.heading);
            } else {
                snprintf(air_s, sizeof(air_s), "--");
                snprintf(gnd_s, sizeof(gnd_s), "--");
                snprintf(hdg_s, sizeof(hdg_s), "--");
            }
            const ImU32 lc = vs.has_vfr ? C_ACCENT : C_DIM;
            const ImU32 vc = vs.has_vfr ? C_DATA   : C_DIM;

            const float wa = std::max(tw("AIR"), tw(air_s));
            const float wg = std::max(tw("GND"), tw(gnd_s));
            const float wh = std::max(tw("HDG"), tw(hdg_s));

            dt(x + (wa - tw("AIR")) * 0.5f,                           row1_y, lc, "AIR");
            dt(x + wa + SUB_GAP + (wg - tw("GND")) * 0.5f,            row1_y, lc, "GND");
            dt(x + wa + SUB_GAP + wg + SUB_GAP + (wh - tw("HDG")) * 0.5f, row1_y, lc, "HDG");

            dt(x + (wa - tw(air_s)) * 0.5f,                           row2_y, vc, air_s);
            dt(x + wa + SUB_GAP + (wg - tw(gnd_s)) * 0.5f,            row2_y, vc, gnd_s);
            dt(x + wa + SUB_GAP + wg + SUB_GAP + (wh - tw(hdg_s)) * 0.5f, row2_y, vc, hdg_s);

            x += wa + SUB_GAP + wg + SUB_GAP + wh + COL_GAP;
        }

        sep();

        // ── GPS ───────────────────────────────────────────────────────────────
        {
            const ImU32 lc = vs.has_gps_raw ? C_ACCENT : C_DIM;
            const ImU32 vc = vs.has_gps_raw ? C_DATA   : C_DIM;

            // Row 1: "GPS" label
            // Row 2: fix + sats + hdop + vdop on one compact line
            char val_s[48];
            if (vs.has_gps_raw)
                snprintf(val_s, sizeof(val_s), "FIX:%d SAT:%d H:%.1f V:%.1f",
                         (int)vs.gps_fix_type, (int)vs.satellites_vis,
                         (double)vs.hdop, (double)vs.vdop);
            else
                snprintf(val_s, sizeof(val_s), "--");

            const float w = std::max(tw("GPS"), tw(val_s));
            dt(x + (w - tw("GPS"))   * 0.5f, row1_y, lc, "GPS");
            dt(x + (w - tw(val_s))   * 0.5f, row2_y, vc, val_s);
            x += w + COL_GAP;
        }

        sep();

        // ── Battery ───────────────────────────────────────────────────────────
        {
            char val_s[32] = "--";
            float frac = 1.0f;
            if (vs.has_sys_status) {
                if (vs.battery_remaining >= 0) {
                    frac = vs.battery_remaining / 100.0f;
                    snprintf(val_s, sizeof(val_s), "%d%% %.2fV",
                             (int)vs.battery_remaining, (double)vs.voltage_V);
                } else {
                    snprintf(val_s, sizeof(val_s), "%.2fV", (double)vs.voltage_V);
                }
            }
            const ImU32 lc = vs.has_sys_status ? C_ACCENT : C_DIM;
            const ImU32 vc = vs.has_sys_status
                             ? ImGui::ColorConvertFloat4ToU32(col_battery(frac)) : C_DIM;
            const float w  = std::max(tw("BAT"), tw(val_s));

            dt(x + (w - tw("BAT"))  * 0.5f, row1_y, lc, "BAT");
            dt(x + (w - tw(val_s))  * 0.5f, row2_y, vc, val_s);
            x += w + COL_GAP;
        }

        sep();

        // ── Messages ──────────────────────────────────────────────────────────
        {
            char val_s[64];
            snprintf(val_s, sizeof(val_s), "%llu  %lluB",
                     (unsigned long long)total_messages,
                     (unsigned long long)total_bytes);
            const float w = std::max(tw("MSG"), tw(val_s));
            dt(x + (w - tw("MSG"))  * 0.5f, row1_y, C_DIM, "MSG");
            dt(x + (w - tw(val_s))  * 0.5f, row2_y, C_DIM, val_s);
            x += w + COL_GAP;

            if (parse_errors > 0) {
                char err_s[32];
                snprintf(err_s, sizeof(err_s), "%llu", (unsigned long long)parse_errors);
                const float we = std::max(tw("ERR"), tw(err_s));
                dt(x + (we - tw("ERR"))   * 0.5f, row1_y, C_ERROR, "ERR");
                dt(x + (we - tw(err_s))   * 0.5f, row2_y, C_ERROR, err_s);
            }
        }

        dl->PopClipRect();

        // ── ARM / DISARM / INTERLOCK (rectangular, right-aligned) ───────────
        if (sender) {
            const bool    connected = vs.has_heartbeat;
            const uint8_t tsys     = connected ? vs.sysid  : 1;
            const uint8_t tcomp    = connected ? vs.compid : 1;

            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,   FRAME_ROUNDING_SM);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, FRAME_BORDER_BOLD);

            // ── INTERLOCK toggle ──────────────────────────────────────────────
            ImGui::SetCursorPos({ btn_x0, btn_y });
            ImGui::PushStyleColor(ImGuiCol_Button,        s_interlock ? btn_intlk_on_base() : btn_intlk_off_base());
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, s_interlock ? btn_intlk_on_hov()  : btn_intlk_off_hov());
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  s_interlock ? btn_intlk_on_base() : btn_intlk_off_base());
            ImGui::PushStyleColor(ImGuiCol_Text, { 1.0f, 1.0f, 1.0f, 1.0f });
            if (ImGui::Button(s_interlock ? "INTLK HIGH" : "INTLK  LOW",
                              { BTN_W_INTLK, BTN_H })) {
                s_interlock = !s_interlock;
                if (connected) {
                    sender->do_aux_function(tsys, tcomp, 32,
                                            s_interlock ? 2 : 0); // 2=HIGH, 0=LOW
                    gcs_log("interlock → %s", s_interlock ? "HIGH" : "LOW");
                }
            }
            ImGui::PopStyleColor(4);

            // ── ARM / DISARM toggle ───────────────────────────────────────────
            ImGui::SetCursorPos({ btn_x0 + BTN_W_INTLK + BTN_GAP, btn_y });
            ImGui::BeginDisabled(!connected);
            if (vs.armed) {
                // Armed state — red, click to confirm disarm
                ImGui::PushStyleColor(ImGuiCol_Button,        col_armed());
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, btn_arm_hov());
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  col_armed());
            } else {
                // Disarmed state — amber, click to confirm arm
                ImGui::PushStyleColor(ImGuiCol_Button,        btn_mode_active_base());
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, btn_mode_active_hov());
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  btn_mode_active_base());
            }
            ImGui::PushStyleColor(ImGuiCol_Text, { 1.0f, 1.0f, 1.0f, 1.0f });
            if (ImGui::Button(vs.armed ? "ARMED" : "DISARMED",
                              { BTN_W_ARM, BTN_H })) {
                if (vs.armed)
                    ImGui::OpenPopup("##confirm_disarm");
                else
                    ImGui::OpenPopup("##confirm_arm");
            }
            ImGui::PopStyleColor(4);
            ImGui::EndDisabled();

            ImGui::PopStyleVar(2);

            // ── Confirmation popups (anchored below the ARM/DISARM button) ───
            // Screen position of the ARM button's bottom-right corner.
            // (topbar window is at wp=(0,0) so window coords == screen coords)
            const float popup_rx = wp.x + btn_x0 + BTN_W_INTLK + BTN_GAP + BTN_W_ARM;
            const float popup_ty = wp.y + btn_y + BTN_H + 4.0f;

            ImGui::SetNextWindowPos({ popup_rx, popup_ty }, ImGuiCond_Always,
                                    { 1.0f, 0.0f }); // right-align to button edge
            if (ImGui::BeginPopup("##confirm_arm",
                    ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
                ImGui::Text("Arm the vehicle?");
                ImGui::Spacing();
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, FRAME_ROUNDING_SM);

                const bool key_confirm = ImGui::IsKeyPressed(ImGuiKey_Enter, false)
                                      || ImGui::IsKeyPressed(ImGuiKey_Y, false);
                const bool key_cancel  = ImGui::IsKeyPressed(ImGuiKey_N, false)
                                      || ImGui::IsKeyPressed(ImGuiKey_Escape, false);

                ImGui::PushStyleColor(ImGuiCol_Button,        btn_arm_base());
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, btn_arm_hov());
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  btn_arm_base());
                const bool do_confirm = ImGui::Button("CONFIRM ARM", { 140.0f, 0.0f })
                                        || key_confirm;
                ImGui::PopStyleColor(3);
                ImGui::SameLine(0, 8);
                push_flash_colors(CmdFlashState::Normal);
                const bool do_cancel = ImGui::Button("CANCEL##arm", { 80.0f, 0.0f })
                                       || key_cancel;
                ImGui::PopStyleColor(3);

                ImGui::Spacing();
                ImGui::TextDisabled("[Y / Enter] confirm     [N / Esc] cancel");

                ImGui::PopStyleVar();

                if (do_confirm) {
                    sender->arm(tsys, tcomp);
                    gcs_log("arm command sent");
                    ImGui::CloseCurrentPopup();
                } else if (do_cancel) {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            ImGui::SetNextWindowPos({ popup_rx, popup_ty }, ImGuiCond_Always,
                                    { 1.0f, 0.0f });
            if (ImGui::BeginPopup("##confirm_disarm",
                    ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
                ImGui::Text("Disarm the vehicle?");
                ImGui::Spacing();
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, FRAME_ROUNDING_SM);

                const bool key_confirm = ImGui::IsKeyPressed(ImGuiKey_Enter, false)
                                      || ImGui::IsKeyPressed(ImGuiKey_Y, false);
                const bool key_cancel  = ImGui::IsKeyPressed(ImGuiKey_N, false)
                                      || ImGui::IsKeyPressed(ImGuiKey_Escape, false);

                ImGui::PushStyleColor(ImGuiCol_Button,        btn_disarm_base());
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, btn_disarm_hov());
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  btn_disarm_base());
                const bool do_confirm = ImGui::Button("CONFIRM DISARM", { 140.0f, 0.0f })
                                        || key_confirm;
                ImGui::PopStyleColor(3);
                ImGui::SameLine(0, 8);
                push_flash_colors(CmdFlashState::Normal);
                const bool do_cancel = ImGui::Button("CANCEL##disarm", { 80.0f, 0.0f })
                                       || key_cancel;
                ImGui::PopStyleColor(3);

                ImGui::Spacing();
                ImGui::TextDisabled("[Y / Enter] confirm     [N / Esc] cancel");

                ImGui::PopStyleVar();

                if (do_confirm) {
                    sender->disarm(tsys, tcomp);
                    gcs_log("disarm command sent");
                    ImGui::CloseCurrentPopup();
                } else if (do_cancel) {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }
    }
    ImGui::End();

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
}
