// Free Body Problem: A modern, simple and minimalist Ground Control Station
// Copyright (C) 2026  Azhar Tanweer
// Contact: azhar.tanweer404@gmail.com

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


#include "sidebar_internal.hpp"
#include "../sidebar_themes.hpp"
#include "../../app_log.hpp"
#include "imgui.h"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <cstdio>

// ── EKF variance bars ─────────────────────────────────────────────────────────

static void draw_ekf_bars(const VehicleState& vs)
{
    if (!vs.has_ekf_status) {
        ImGui::TextDisabled("No data");
        return;
    }

    struct Bar { const char* label; float value; };
    const Bar bars[] = {
        { "Vel",  vs.ekf_velocity_variance    },
        { "PosH", vs.ekf_pos_horiz_variance   },
        { "PosV", vs.ekf_pos_vert_variance    },
        { "Comp", vs.ekf_compass_variance     },
        { "TerH", vs.ekf_terrain_alt_variance },
        { "AS",   vs.ekf_airspeed_variance    },
    };
    constexpr int N = 6;

    ImDrawList*  dl      = ImGui::GetWindowDrawList();
    const float  avail_w = ImGui::GetContentRegionAvail().x;
    const ImVec2 p0      = ImGui::GetCursorScreenPos();
    const float  fh      = ImGui::GetTextLineHeight();

    constexpr float BAR_H = 56.0f;
    constexpr float GAP   = 3.0f;
    const float bar_w = (avail_w - GAP * (N - 1)) / N;

    for (int i = 0; i < N; ++i) {
        const float x   = p0.x + i * (bar_w + GAP);
        const float val = std::max(0.0f, std::min(bars[i].value, 1.0f));

        dl->AddRectFilled({x, p0.y}, {x + bar_w, p0.y + BAR_H}, ekf_bg());

        // Color: green (0) → yellow (0.5) → red (1)
        ImU32 fill_col;
        if (val < 0.5f) {
            const uint8_t r = (uint8_t)(val * 2.0f * 220);
            fill_col = IM_COL32(r, 200, 40, 255);
        } else {
            const uint8_t g = (uint8_t)((1.0f - (val - 0.5f) * 2.0f) * 200);
            fill_col = IM_COL32(220, g, 40, 255);
        }

        // Filled portion grows from the bottom
        const float fill_h = BAR_H * val;
        dl->AddRectFilled(
            {x,         p0.y + BAR_H - fill_h},
            {x + bar_w, p0.y + BAR_H},
            fill_col);

        dl->AddRect({x, p0.y}, {x + bar_w, p0.y + BAR_H}, ekf_outline());

        // Centred label below the bar
        const char* lbl = bars[i].label;
        const ImVec2 tsz = ImGui::CalcTextSize(lbl);
        dl->AddText(
            {x + (bar_w - tsz.x) * 0.5f, p0.y + BAR_H + 2.0f},
            ekf_label(), lbl);
    }

    ImGui::Dummy({avail_w, BAR_H + fh + 4.0f});
}

// ── ArduCopter flight mode values ─────────────────────────────────────────────

namespace FlightMode {
    static constexpr uint32_t STABILIZE = 0;
    static constexpr uint32_t ACRO      = 1;
    static constexpr uint32_t ALT_HOLD  = 2;
    static constexpr uint32_t AUTO      = 3;
    static constexpr uint32_t GUIDED    = 4;
    static constexpr uint32_t LOITER    = 5;
    static constexpr uint32_t RTL       = 6;
    static constexpr uint32_t LAND      = 9;
}

static constexpr float TAKEOFF_ALT_M = 5.0f;

// ─────────────────────────────────────────────────────────────────────────────

void draw_tab_flight(MavlinkSender* sender, const VehicleState* vs)
{
    const bool    connected = (vs && vs->has_heartbeat);
    const uint8_t tsys      = connected ? vs->sysid  : 1;
    const uint8_t tcomp     = connected ? vs->compid : 1;

    // ── Flight mode controls ──────────────────────────────────────────────────

    ImGui::Spacing();
    ImGui::TextColored(accent_col(), "FLIGHT");
    if (!connected) {
        ImGui::SameLine(0, 6);
        ImGui::TextColored(col_no_link_muted(), "(no link)");
    }
    themed_sep();
    ImGui::Spacing();

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,   0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, FRAME_BORDER_NORMAL);
    ImGui::PushStyleColor(ImGuiCol_Border, col_border_tab());
    ImGui::BeginDisabled(!connected);

    push_flash_colors(sender->query_flash(22));
    if (ImGui::Button("TAKEOFF", { -1.0f, 28.0f })) {
        sender->takeoff(tsys, tcomp, TAKEOFF_ALT_M);
        gcs_log("takeoff command sent (%.0f m)", (double)TAKEOFF_ALT_M);
    }
    ImGui::PopStyleColor(3);

    {
        const bool rtl_active = connected && (vs->custom_mode == FlightMode::RTL);
        const CmdFlashState rtl_fs = sender->query_flash(20);
        if (rtl_fs != CmdFlashState::Normal) {
            push_flash_colors(rtl_fs);
        } else if (rtl_active) {
            ImGui::PushStyleColor(ImGuiCol_Button,        btn_mode_active_base());
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, btn_mode_active_hov());
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  btn_mode_active_base());
        } else {
            push_flash_colors(CmdFlashState::Normal);
        }
        if (ImGui::Button("RTL", { -1.0f, 28.0f })) {
            sender->return_to_launch(tsys, tcomp);
            gcs_log("RTL command sent");
        }
        ImGui::PopStyleColor(3);
    }

    ImGui::Spacing();

    struct ModeBtn { const char* label; uint32_t mode; };
    static const ModeBtn modes[] = {
        { "STAB", FlightMode::STABILIZE },
        { "ACRO", FlightMode::ACRO      },
        { "ALTH", FlightMode::ALT_HOLD  },
        { "LOIT", FlightMode::LOITER    },
        { "GUID", FlightMode::GUIDED    },
        { "AUTO", FlightMode::AUTO      },
        { "LAND", FlightMode::LAND      },
    };
    constexpr int MODE_COUNT = (int)(sizeof(modes) / sizeof(modes[0]));

    const float half = (ImGui::GetContentRegionAvail().x - 4.0f) * 0.5f;
    const CmdFlashState mode_fs = sender->query_flash(176);

    for (int i = 0; i < MODE_COUNT; ++i) {
        if (i % 2 == 1) ImGui::SameLine(0, 4);
        const bool is_active_mode = connected && (vs->custom_mode == modes[i].mode);
        if (mode_fs != CmdFlashState::Normal) {
            push_flash_colors(mode_fs);
        } else if (is_active_mode) {
            ImGui::PushStyleColor(ImGuiCol_Button,        btn_mode_active_base());
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, btn_mode_active_hov());
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  btn_mode_active_base());
        } else {
            push_flash_colors(CmdFlashState::Normal);
        }
        if (ImGui::Button(modes[i].label, { half, 26.0f })) {
            sender->set_mode(tsys, tcomp, modes[i].mode);
            gcs_log("mode → %s", modes[i].label);
        }
        ImGui::PopStyleColor(3);
    }

    ImGui::EndDisabled();
    ImGui::PopStyleColor(); // Border
    ImGui::PopStyleVar(2);  // FrameRounding, FrameBorderSize

    // ── EKF status ────────────────────────────────────────────────────────────

    ImGui::Spacing();
    ImGui::TextColored(accent_col(), "EKF STATUS");
    themed_sep();
    ImGui::Spacing();
    draw_ekf_bars(vs ? *vs : VehicleState{});

    // ── SERVO / AUX sub-tabs ──────────────────────────────────────────────────

    ImGui::Spacing();
    themed_sep();
    ImGui::Spacing();

    static int s_flight_subtab = 0; // 0 = SERVO, 1 = AUX

    {
        const float tab_w = (ImGui::GetContentRegionAvail().x - 2.0f) * 0.5f;
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,   FRAME_ROUNDING_SM);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, FRAME_BORDER_NORMAL);
        ImGui::PushStyleColor(ImGuiCol_Border, col_border_tab());
        static const char* const sub_labels[] = { "SERVO", "AUX" };
        for (int i = 0; i < 2; ++i) {
            if (i > 0) ImGui::SameLine(0, 2);
            const bool active = (s_flight_subtab == i);
            if (active) {
                ImGui::PushStyleColor(ImGuiCol_Button,        btn_tab_active_base());
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, btn_tab_active_hov());
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  btn_tab_active_base());
                ImGui::PushStyleColor(ImGuiCol_Text,          { 1.0f, 1.0f, 1.0f, 1.0f });
            } else {
                push_flash_colors(CmdFlashState::Normal);
            }
            if (ImGui::Button(sub_labels[i], { tab_w, 22.0f }))
                s_flight_subtab = i;
            ImGui::PopStyleColor(active ? 4 : 3);
        }
        ImGui::PopStyleColor(); // Border
        ImGui::PopStyleVar(2);  // FrameRounding, FrameBorderSize
    }

    ImGui::Spacing();
    const float avail_h = ImGui::GetContentRegionAvail().y;

    // ── SERVO panel ───────────────────────────────────────────────────────────
    if (s_flight_subtab == 0) {
        static int  s_srv_pwm[16];
        static bool s_srv_init = false;
        if (!s_srv_init) {
            for (int i = 0; i < 16; ++i) s_srv_pwm[i] = 1500;
            s_srv_init = true;
        }

        ImGui::PushStyleColor(ImGuiCol_ChildBg, bg_child_dark());
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,   FRAME_ROUNDING_SM);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, FRAME_BORDER_NORMAL);
        ImGui::PushStyleColor(ImGuiCol_Border, col_separator());
        if (ImGui::BeginChild("##srv_list", { -1.0f, avail_h }, false)) {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 4.0f, 3.0f });
            ImGui::BeginDisabled(!connected);

            const float row_w = ImGui::GetContentRegionAvail().x;
            const float pwm_w = 68.0f;
            const float btn_w = 46.0f;
            const float lbl_w = row_w - pwm_w - btn_w - 12.0f;

            for (int i = 0; i < 16; ++i) {
                char lbl[12]; snprintf(lbl, sizeof(lbl), "SRV %2d", i + 1);
                ImGui::PushID(i);

                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted(lbl);
                ImGui::SameLine(lbl_w);

                ImGui::SetNextItemWidth(pwm_w);
                ImGui::InputInt("##pwm", &s_srv_pwm[i], 0, 0);
                s_srv_pwm[i] = std::max(0, std::min(2200, s_srv_pwm[i]));
                ImGui::SameLine(0, 4);

                ImGui::PushStyleColor(ImGuiCol_Button,        btn_write_base());
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, btn_write_hov());
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  btn_write_base());
                if (ImGui::Button("SET", { btn_w, 0.0f })) {
                    sender->do_set_servo(tsys, tcomp,
                                         (uint8_t)(i + 1),
                                         (uint16_t)s_srv_pwm[i]);
                    gcs_log("servo %d → %d µs", i + 1, s_srv_pwm[i]);
                }
                ImGui::PopStyleColor(3);
                ImGui::PopID();
            }

            ImGui::EndDisabled();
            ImGui::PopStyleVar(); // ItemSpacing
        }
        ImGui::EndChild();
        ImGui::PopStyleColor(); // Border
        ImGui::PopStyleVar(2);  // FrameRounding, FrameBorderSize
        ImGui::PopStyleColor(); // ChildBg
    }

    // ── AUX panel ─────────────────────────────────────────────────────────────
    if (s_flight_subtab == 1) {
        // Detect active firmware from vehicle type
        enum class FwType { Copter, Plane, Rover, Sub };
        FwType fw = FwType::Copter;
        if (vs && vs->has_heartbeat) {
            switch (vs->type) {
            case MAV_TYPE_FIXED_WING:   fw = FwType::Plane; break;
            case MAV_TYPE_GROUND_ROVER: fw = FwType::Rover; break;
            case MAV_TYPE_SUBMARINE:    fw = FwType::Sub;   break;
            default:                    fw = FwType::Copter; break;
            }
        }

        // Full ArduPilot auxiliary functions table (id, name, Copter, Plane, Rover, Sub)
        struct AuxFn { uint16_t id; const char* label; bool c, p, r, s; };
        static constexpr AuxFn ALL_AUX[] = {
            {  0, "Do Nothing",                true,  true,  true,  true  },
            {  2, "FLIP",                      true,  false, false, false },
            {  3, "SIMPLE mode (Copter)",      true,  false, false, false },
            {  4, "RTL mode",                  true,  true,  true,  false },
            {  5, "Save Trim",                 true,  true,  false, false },
            {  7, "Save Waypoint",             true,  true,  false, false },
            {  9, "Camera Trigger",            true,  true,  true,  true  },
            { 10, "Rangefinder",               true,  false, false, false },
            { 11, "Fence",                     true,  true,  true,  true  },
            { 12, "ResetToArmedYaw",           true,  false, false, false },
            { 13, "SUPERSIMPLE mode",          true,  false, false, false },
            { 14, "Acro Trainer",              true,  false, false, false },
            { 15, "Sprayer",                   true,  false, false, false },
            { 16, "AUTO mode",                 true,  true,  true,  false },
            { 17, "AUTOTUNE mode",             true,  false, false, false },
            { 18, "LAND mode",                 true,  false, false, false },
            { 19, "Gripper",                   true,  true,  true,  true  },
            { 21, "Parachute Enable",          true,  false, false, false },
            { 22, "Parachute Release",         true,  true,  false, false },
            { 23, "Parachute 3-Pos Switch",    true,  false, false, false },
            { 24, "Reset Auto Mission",        true,  true,  true,  true  },
            { 25, "Attitude FF",               true,  false, false, false },
            { 26, "Attitude AccLim",           true,  false, false, false },
            { 27, "Retract Mount1",            true,  true,  true,  true  },
            { 28, "Relay 1 On/Off",            true,  true,  true,  true  },
            { 29, "Landing Gear",              true,  true,  false, false },
            { 30, "Lost Vehicle Sound",        true,  true,  true,  false },
            { 31, "Motor Emergency Stop",      true,  true,  true,  true  },
            { 32, "Motor Interlock",           true,  false, false, false },
            { 33, "BRAKE mode",                true,  false, false, false },
            { 34, "Relay 2 On/Off",            true,  true,  true,  true  },
            { 35, "Relay 3 On/Off",            true,  true,  true,  true  },
            { 36, "Relay 4 On/Off",            true,  true,  true,  true  },
            { 37, "THROW mode",                true,  false, false, false },
            { 38, "ADSB Avoidance Enable",     true,  false, false, false },
            { 39, "Precision Loiter",          true,  false, false, false },
            { 40, "Object Avoidance",          true,  true,  false, false },
            { 41, "Arm/Disarm (4.1-)",         true,  true,  true,  false },
            { 42, "SMARTRTL mode",             true,  true,  false, false },
            { 43, "Inverted Flight",           true,  true,  false, false },
            { 44, "Winch Enable",              true,  false, false, false },
            { 45, "Winch Control",             true,  false, false, false },
            { 46, "RC Override Enable",        true,  true,  true,  true  },
            { 47, "Custom Function 1",         true,  false, false, false },
            { 48, "Custom Function 2",         true,  false, false, false },
            { 49, "Custom Function 3",         true,  false, false, false },
            { 50, "Learn Cruise",              false, false, true,  false },
            { 51, "MANUAL mode",               false, true,  true,  false },
            { 52, "ACRO mode",                 false, true,  true,  false },
            { 53, "STEERING mode",             false, false, true,  false },
            { 54, "HOLD mode",                 false, false, true,  false },
            { 55, "GUIDED mode",               true,  true,  true,  false },
            { 56, "LOITER mode",               true,  true,  false, false },
            { 57, "FOLLOW mode",               true,  true,  false, false },
            { 58, "Clear Waypoints",           true,  true,  true,  true  },
            { 59, "SIMPLE mode (Rover)",       false, false, true,  false },
            { 60, "ZIGZAG mode",               true,  false, false, false },
            { 61, "ZIGZAG Save Waypoints",     true,  false, false, false },
            { 62, "Compass Learn",             true,  true,  true,  true  },
            { 63, "Sailboat Tack",             false, false, true,  false },
            { 64, "Reverse Throttle",          false, true,  false, false },
            { 65, "GPS Disable",               true,  true,  true,  true  },
            { 66, "Relay 5 On/Off",            true,  true,  true,  true  },
            { 67, "Relay 6 On/Off",            true,  true,  true,  true  },
            { 68, "STABILIZE mode",            true,  false, false, false },
            { 69, "POSHOLD mode",              true,  false, false, false },
            { 70, "ALTHOLD mode",              true,  true,  false, false },
            { 71, "FLOWHOLD mode",             true,  false, false, false },
            { 72, "CIRCLE mode",               true,  true,  true,  false },
            { 73, "DRIFT mode",                true,  false, false, false },
            { 74, "Sailboat Motor 3-Pos",      false, false, true,  false },
            { 75, "Surface Tracking",          true,  false, false, false },
            { 76, "STANDBY mode",              true,  false, false, false },
            { 77, "TAKEOFF mode",              true,  false, false, false },
            { 78, "RunCam Control",            true,  true,  true,  true  },
            { 79, "RunCam OSD Control",        true,  true,  true,  true  },
            { 80, "Viso Align",                true,  false, false, false },
            { 81, "Disarm",                    true,  true,  true,  true  },
            { 82, "Q_Assist 3-Pos Sw",         false, true,  false, false },
            { 83, "ZIGZAG Auto",               true,  false, false, false },
            { 84, "AIRMODE",                   true,  true,  false, false },
            { 85, "Generator",                 true,  true,  true,  false },
            { 86, "Non-Auto Terrain Follow",   false, true,  false, false },
            { 87, "CROW Mode Switch",          false, true,  false, false },
            { 88, "Soaring Enable",            false, true,  false, false },
            { 89, "Force Flare",               false, true,  false, false },
            { 90, "EKF Source Set",            true,  true,  true,  false },
            { 91, "Airspeed Ratio Cal",        false, true,  false, false },
            { 92, "FBWA Mode",                 false, true,  false, false },
            { 94, "VTX Power",                 true,  true,  true,  true  },
            { 95, "FBWA Taildragger",          false, true,  false, false },
            { 96, "Mode Switch Reset",         false, true,  false, false },
            { 97, "WindVane Home Dir",         false, false, true,  false },
            {102, "Camera Mode Toggle",        true,  true,  true,  true  },
            {103, "EKF Lane Switch",           true,  true,  true,  true  },
            {104, "EKF Yaw Reset",             true,  true,  true,  true  },
            {105, "GPS Disable Yaw",           true,  true,  true,  true  },
            {106, "Disable Airspeed Use",      true,  true,  false, false },
            {107, "Enable Autotuning",         true,  false, false, false },
            {108, "QRTL Mode",                 false, true,  false, false },
            {111, "Loweheiser Starter",        true,  true,  true,  true  },
            {112, "Switch External AHRS",      true,  true,  true,  true  },
            {113, "Retract Mount2",            true,  false, false, false },
            {150, "CRUISE Mode",               false, true,  false, false },
            {151, "TURTLE Mode",               true,  false, false, false },
            {152, "SIMPLE Heading Reset",      true,  false, false, false },
            {153, "ARM/DISARM (4.2+)",         true,  true,  true,  true  },
            {154, "ARM/DISARM + AIRMODE",      true,  true,  false, false },
            {155, "TRIM RC/SERVO Save",        false, true,  true,  false },
            {156, "Torqeedo Error Clear",      false, false, true,  false },
            {157, "Force FBWA Long FS",        false, true,  false, false },
            {158, "Optflow Calibration",       true,  true,  false, false },
            {159, "Force Flying State",        true,  false, false, false },
            {160, "WeatherVane Enable",        false, true,  false, false },
            {161, "Turbine Start (Heli)",      false, true,  false, false },
            {162, "Auto Throttle Notch",       true,  true,  false, false },
            {163, "Mount Yaw Lock",            true,  true,  true,  true  },
            {164, "Pause Stream Logging",      true,  true,  true,  true  },
            {165, "ARM / Motor E-Stop",        true,  true,  true,  true  },
            {166, "Camera Record Video",       true,  true,  true,  true  },
            {167, "Camera Zoom",               true,  true,  true,  true  },
            {168, "Camera Manual Focus",       true,  true,  true,  true  },
            {169, "Camera Auto Focus",         true,  true,  true,  true  },
            {170, "QSTABILIZE mode",           false, true,  false, false },
            {171, "Compass Calibration",       true,  true,  true,  false },
            {172, "Battery MPPT Enable",       true,  true,  true,  false },
            {173, "Plane AUTO Landing Abort",  false, true,  false, false },
            {174, "Camera Image Tracking",     true,  true,  true,  true  },
            {175, "Camera Lens",               true,  true,  true,  true  },
            {176, "VTOL Fwd Throttle Disable", false, true,  false, false },
            {177, "Mount LRF Enable",          true,  true,  true,  true  },
            {178, "FlightMode Pause/Resume",   true,  false, false, false },
            {179, "ICEngine Start/Stop",       false, true,  false, false },
            {180, "Autotune Test Gains",       true,  true,  false, false },
            {181, "VTOL QuickTune",            false, true,  false, false },
            {182, "AHRS AutoTrim",             true,  false, false, false },
            {183, "AUTOLAND mode",             false, true,  false, false },
            {184, "System ID",                 true,  true,  false, false },
            {185, "Mount RP Lock",             true,  true,  true,  true  },
            {186, "Mount POI Lock",            true,  true,  true,  true  },
            {201, "ROLL Input",                true,  false, false, false },
            {202, "PITCH Input",               true,  false, false, false },
            {203, "THROTTLE Input",            false, false, false, false },
            {204, "YAW Input",                 false, false, false, false },
            {207, "Mainsail",                  false, false, true,  false },
            {208, "Flap Control",              false, true,  false, false },
            {209, "Forward Throttle",          false, true,  false, false },
            {210, "Airbrakes",                 false, true,  false, false },
            {211, "Walking Robot Height",      false, false, true,  false },
            {212, "Mount1 Roll",               true,  true,  true,  true  },
            {213, "Mount1 Pitch",              true,  true,  true,  true  },
            {214, "Mount1 Yaw",                true,  true,  true,  true  },
            {215, "Mount2 Roll",               true,  true,  true,  true  },
            {216, "Mount2 Pitch",              true,  true,  true,  true  },
            {217, "Mount2 Yaw",                true,  true,  true,  true  },
            {218, "Loweheiser Throttle",       true,  true,  true,  true  },
            {219, "TX Tuning Channel",         true,  false, false, false },
            {300, "Scripting RC 1",            true,  true,  true,  true  },
            {301, "Scripting RC 2",            true,  true,  true,  true  },
            {302, "Scripting RC 3",            true,  true,  true,  true  },
            {303, "Scripting RC 4",            true,  true,  true,  true  },
            {304, "Scripting RC 5",            true,  true,  true,  true  },
            {305, "Scripting RC 6",            true,  true,  true,  true  },
            {306, "Scripting RC 7",            true,  true,  true,  true  },
            {307, "Scripting RC 8",            true,  true,  true,  true  },
            {308, "Scripting RC 9",            true,  true,  true,  true  },
            {309, "Scripting RC 10",           true,  true,  true,  true  },
            {310, "Scripting RC 11",           true,  true,  true,  true  },
            {311, "Scripting RC 12",           true,  true,  true,  true  },
            {312, "Scripting RC 13",           true,  true,  true,  true  },
            {313, "Scripting RC 14",           true,  true,  true,  true  },
            {314, "Scripting RC 15",           true,  true,  true,  true  },
            {315, "Scripting RC 16",           true,  true,  true,  true  },
            {316, "Scripting Stop/Restart",    true,  true,  true,  true  },
        };
        constexpr int N_ALL_AUX = (int)(sizeof(ALL_AUX) / sizeof(ALL_AUX[0]));

        // Firmware badge
        const char* fw_label = "COPTER";
        switch (fw) {
        case FwType::Plane: fw_label = "PLANE"; break;
        case FwType::Rover: fw_label = "ROVER"; break;
        case FwType::Sub:   fw_label = "SUB";   break;
        default: break;
        }
        if (!connected) {
            ImGui::TextColored(col_no_link_muted(), "No link — COPTER defaults");
        } else {
            ImGui::TextColored(accent_col(), "Firmware:"); ImGui::SameLine(0, 4);
            ImGui::Text("%s", fw_label);
        }

        // Search bar
        static char aux_search[64] = {};
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputTextWithHint("##aux_search", "Search by name or ID...",
                                 aux_search, sizeof(aux_search));
        ImGui::Spacing();

        // Upper-case search string for case-insensitive matching
        char srch_up[64] = {};
        for (int k = 0; k < 63 && aux_search[k]; ++k)
            srch_up[k] = (char)toupper((unsigned char)aux_search[k]);

        const float aux_list_h = ImGui::GetContentRegionAvail().y;
        ImGui::PushStyleColor(ImGuiCol_ChildBg, bg_child_dark());
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,   FRAME_ROUNDING_SM);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, FRAME_BORDER_NORMAL);
        ImGui::PushStyleColor(ImGuiCol_Border, col_separator());
        if (ImGui::BeginChild("##aux_list", { -1.0f, aux_list_h }, false)) {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 4.0f, 3.0f });
            ImGui::BeginDisabled(!connected);

            const float row_w  = ImGui::GetContentRegionAvail().x;
            const float id_w   = 36.0f;
            const float btn3_w = (row_w * 0.40f) / 3.0f;
            const float lbl_w  = id_w + row_w - id_w - btn3_w * 3.0f - 3.0f * 2.0f - 6.0f;

            static const char* const pos_labels[] = { "LOW", "MID", "HI" };

            for (int i = 0; i < N_ALL_AUX; ++i) {
                const AuxFn& fn = ALL_AUX[i];

                // Firmware compatibility filter
                bool compat = false;
                switch (fw) {
                case FwType::Copter: compat = fn.c; break;
                case FwType::Plane:  compat = fn.p; break;
                case FwType::Rover:  compat = fn.r; break;
                case FwType::Sub:    compat = fn.s; break;
                }
                if (!compat) continue;

                // Search filter — match on name or numeric ID
                if (srch_up[0] != '\0') {
                    char name_up[64] = {};
                    for (int k = 0; k < 63 && fn.label[k]; ++k)
                        name_up[k] = (char)toupper((unsigned char)fn.label[k]);
                    char id_str[8];
                    snprintf(id_str, sizeof(id_str), "%u", (unsigned)fn.id);
                    if (strstr(name_up, srch_up) == nullptr &&
                        strstr(id_str, srch_up) == nullptr) continue;
                }

                ImGui::PushID(i);
                ImGui::AlignTextToFramePadding();
                ImGui::TextDisabled("%3u", (unsigned)fn.id);
                ImGui::SameLine(id_w);
                ImGui::TextUnformatted(fn.label);
                ImGui::SameLine(lbl_w);

                for (int p = 0; p < 3; ++p) {
                    if (p > 0) ImGui::SameLine(0, 2);
                    push_flash_colors(CmdFlashState::Normal);
                    char btn_id[16];
                    snprintf(btn_id, sizeof(btn_id), "%s##%d", pos_labels[p], i);
                    if (ImGui::Button(btn_id, { btn3_w, 0.0f })) {
                        sender->do_aux_function(tsys, tcomp, fn.id, (uint8_t)p);
                        gcs_log("aux fn %u (%s) → %s",
                                (unsigned)fn.id, fn.label, pos_labels[p]);
                    }
                    ImGui::PopStyleColor(3);
                }
                ImGui::PopID();
            }

            ImGui::EndDisabled();
            ImGui::PopStyleVar(); // ItemSpacing
        }
        ImGui::EndChild();
        ImGui::PopStyleColor(); // Border
        ImGui::PopStyleVar(2);  // FrameRounding, FrameBorderSize
        ImGui::PopStyleColor(); // ChildBg
    }
}
