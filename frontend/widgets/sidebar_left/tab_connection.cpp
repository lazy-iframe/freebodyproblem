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


#include "sidebar_internal.hpp"
#include "../sidebar_themes.hpp"
#include "../../settings.hpp"
#include "imgui.h"
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <vector>

void draw_tab_connection(const VehicleState* vs,
                         ConnectionRequest* conn_out,
                         LinkStatus link_status,
                         AppSettings* settings)
{
    ImGui::Spacing();
    ImGui::TextColored(accent_col(), "CONNECTION");
    themed_sep();
    ImGui::Spacing();

    // Mini-tab bar: Serial | TCP | UDP
    static int  conn_tab     = 2; // default: UDP
    static char udp_host[64] = "0.0.0.0";
    static int  udp_port     = 14550;
    static char tcp_host[64] = "192.168.1.1";
    static int  tcp_port     = 5760;
    static int  serial_sel   = 0;
    static int  baud_sel     = 3;

    // Serial port cache — refresh every ~3 s
    static std::vector<SerialPortInfo> s_ports;
    static float s_ports_age = 999.0f;
    s_ports_age += ImGui::GetIO().DeltaTime;
    if (s_ports_age > 3.0f) {
        s_ports     = list_serial_ports();
        s_ports_age = 0.0f;
        if (serial_sel >= (int)s_ports.size())
            serial_sel = 0;
    }

    static const char* const tab_labels[] = { "SERIAL", "TCP", "UDP" };
    static const int baud_rates[]         = { 9600, 57600, 115200, 230400, 460800, 921600 };
    constexpr int N_BAUDS = (int)(sizeof(baud_rates) / sizeof(baud_rates[0]));

    const float tab_w = (ImGui::GetContentRegionAvail().x - 2.0f * 2.0f) / 3.0f;
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, FRAME_ROUNDING_SM);
    for (int i = 0; i < 3; ++i) {
        if (i > 0) ImGui::SameLine(0, 2);
        const bool active = (conn_tab == i);
        if (active) {
            ImGui::PushStyleColor(ImGuiCol_Button,        btn_tab_active_base());
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, btn_tab_active_hov());
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  btn_tab_active_base());
            ImGui::PushStyleColor(ImGuiCol_Text,          { 1.0f, 1.0f, 1.0f, 1.0f });
        } else {
            push_flash_colors(CmdFlashState::Normal);
        }
        if (ImGui::Button(tab_labels[i], { tab_w, 22.0f }))
            conn_tab = i;
        ImGui::PopStyleColor(active ? 4 : 3);
    }
    ImGui::PopStyleVar(); // FrameRounding
    ImGui::Spacing();

    if (conn_tab == 0) {
        // Serial
        ImGui::TextDisabled("PORT");
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::BeginCombo("##serial_dev",
                s_ports.empty() ? "(none)" : s_ports[serial_sel].device.c_str()))
        {
            for (int i = 0; i < (int)s_ports.size(); ++i) {
                const bool sel = (serial_sel == i);
                char label[128];
                snprintf(label, sizeof(label), "%s  %s",
                         s_ports[i].device.c_str(),
                         s_ports[i].description.c_str());
                if (ImGui::Selectable(label, sel))
                    serial_sel = i;
                if (sel) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::Spacing();
        ImGui::TextDisabled("BAUD");
        ImGui::SetNextItemWidth(-1.0f);
        char baud_preview[16];
        snprintf(baud_preview, sizeof(baud_preview), "%d", baud_rates[baud_sel]);
        if (ImGui::BeginCombo("##baud", baud_preview)) {
            for (int i = 0; i < N_BAUDS; ++i) {
                char lbl[16]; snprintf(lbl, sizeof(lbl), "%d", baud_rates[i]);
                if (ImGui::Selectable(lbl, baud_sel == i))
                    baud_sel = i;
                if (baud_sel == i) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::Spacing();
        ImGui::BeginDisabled(s_ports.empty());
        push_flash_colors(CmdFlashState::Normal);
        if (ImGui::Button("CONNECT##serial", { -1.0f, 0.0f }) && !s_ports.empty()) {
            conn_out->type = ConnType::Serial;
            strncpy(conn_out->device, s_ports[serial_sel].device.c_str(),
                    sizeof(conn_out->device) - 1);
            conn_out->baud      = baud_rates[baud_sel];
            conn_out->requested = true;
        }
        ImGui::PopStyleColor(3);
        ImGui::EndDisabled();

    } else if (conn_tab == 1) {
        // TCP
        ImGui::TextDisabled("HOST");
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputText("##tcp_host", tcp_host, sizeof(tcp_host));
        ImGui::Spacing();
        ImGui::TextDisabled("PORT");
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputInt("##tcp_port", &tcp_port, 0, 0);
        ImGui::Spacing();
        push_flash_colors(CmdFlashState::Normal);
        if (ImGui::Button("CONNECT##tcp", { -1.0f, 0.0f })) {
            conn_out->type = ConnType::TCP;
            strncpy(conn_out->host, tcp_host, sizeof(conn_out->host) - 1);
            conn_out->port      = tcp_port;
            conn_out->requested = true;
        }
        ImGui::PopStyleColor(3);

    } else {
        // UDP
        ImGui::TextDisabled("BIND ADDRESS");
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputText("##udp_host", udp_host, sizeof(udp_host));
        ImGui::Spacing();
        ImGui::TextDisabled("PORT");
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputInt("##udp_port", &udp_port, 0, 0);
        ImGui::Spacing();
        push_flash_colors(CmdFlashState::Normal);
        if (ImGui::Button("CONNECT##udp", { -1.0f, 0.0f })) {
            conn_out->type = ConnType::UDP;
            strncpy(conn_out->host, udp_host, sizeof(conn_out->host) - 1);
            conn_out->port      = udp_port;
            conn_out->requested = true;
        }
        ImGui::PopStyleColor(3);
    }

    // Connection status + disconnect button
    ImGui::Spacing();
    const bool is_active = (link_status == LinkStatus::Connecting ||
                            link_status == LinkStatus::Connected);
    switch (link_status) {
    case LinkStatus::Connecting:
        ImGui::TextColored(col_warning(), "CONNECTING...");
        break;
    case LinkStatus::Connected:
        ImGui::TextColored(col_ok(), "CONNECTED");
        break;
    case LinkStatus::Timeout:
        ImGui::TextColored(col_error(), "TIMEOUT (10 S)");
        break;
    case LinkStatus::Error:
        ImGui::TextColored(col_error(), "CONNECTION ERROR");
        break;
    default:
        break;
    }
    if (is_active) {
        ImGui::SameLine(0, 8);
        ImGui::PushStyleColor(ImGuiCol_Button,        btn_disconnect_base());
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, btn_disconnect_hov());
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  btn_disconnect_base());
        if (ImGui::Button("DISCONNECT"))
            conn_out->disconnect = true;
        ImGui::PopStyleColor(3);
    }

    // ── Connection profiles ───────────────────────────────────────────────────
    if (settings) {
        ImGui::Spacing();
        themed_sep();
        ImGui::Spacing();
        ImGui::TextColored(accent_col(), "PROFILES");
        ImGui::Spacing();

        // Profile name input + SAVE button on same line
        static char profile_name[64] = {};
        const float save_w  = 46.0f;
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - save_w - 4.0f);
        ImGui::InputTextWithHint("##profile_name", "profile name...",
                                 profile_name, sizeof(profile_name));
        ImGui::SameLine(0, 4);
        const bool name_ok = profile_name[0] != '\0';
        ImGui::BeginDisabled(!name_ok);
        push_flash_colors(CmdFlashState::Normal);
        if (ImGui::Button("SAVE##profile", { save_w, 0.0f }) && name_ok) {
            ConnectionProfile p;
            p.name = profile_name;
            p.type = conn_tab;
            if (conn_tab == 0) {
                p.device = s_ports.empty() ? "" : s_ports[serial_sel].device;
                p.baud   = baud_rates[baud_sel];
            } else if (conn_tab == 1) {
                p.host = tcp_host;
                p.port = tcp_port;
            } else {
                p.host = udp_host;
                p.port = udp_port;
            }
            settings->connection_profiles.push_back(p);
            settings_save(*settings);
            profile_name[0] = '\0';
        }
        ImGui::PopStyleColor(3);
        ImGui::EndDisabled();

        // Saved profiles list
        if (!settings->connection_profiles.empty()) {
            ImGui::Spacing();
            const int  visible   = (int)std::min((int)settings->connection_profiles.size(), 4);
            const float row_h    = ImGui::GetFrameHeightWithSpacing();
            const float list_h   = visible * row_h + 4.0f;
            ImGui::BeginChild("##profile_list", { 0.0f, list_h }, false);

            int to_delete = -1;
            for (int i = 0; i < (int)settings->connection_profiles.size(); ++i) {
                const auto& p = settings->connection_profiles[i];
                ImGui::PushID(i);

                static const char* const type_tag[] = { "SER", "TCP", "UDP" };
                const char* tag = (p.type >= 0 && p.type <= 2) ? type_tag[p.type] : "???";

                // Type badge + name (name clips naturally at child window edge)
                ImGui::TextDisabled("%s", tag);
                ImGui::SameLine(0, 4);
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted(p.name.c_str());

                // Right-align buttons: position from window left edge
                constexpr float btn_connect_w = 64.0f;
                constexpr float btn_del_w     = 22.0f;
                constexpr float gap           = 4.0f;
                const float btn_x = ImGui::GetWindowWidth()
                                    - btn_connect_w - btn_del_w - gap
                                    - ImGui::GetStyle().WindowPadding.x;
                ImGui::SameLine(btn_x);

                // CONNECT button
                push_flash_colors(CmdFlashState::Normal);
                if (ImGui::Button("CONNECT##cp", { 64.0f, 0.0f })) {
                    if (p.type == 0) {
                        conn_tab = 0;
                        conn_out->type = ConnType::Serial;
                        strncpy(conn_out->device, p.device.c_str(),
                                sizeof(conn_out->device) - 1);
                        conn_out->baud      = p.baud;
                        conn_out->requested = true;
                    } else if (p.type == 1) {
                        conn_tab = 1;
                        strncpy(tcp_host, p.host.c_str(), sizeof(tcp_host) - 1);
                        tcp_port = p.port;
                        conn_out->type = ConnType::TCP;
                        strncpy(conn_out->host, p.host.c_str(),
                                sizeof(conn_out->host) - 1);
                        conn_out->port      = p.port;
                        conn_out->requested = true;
                    } else {
                        conn_tab = 2;
                        strncpy(udp_host, p.host.c_str(), sizeof(udp_host) - 1);
                        udp_port = p.port;
                        conn_out->type = ConnType::UDP;
                        strncpy(conn_out->host, p.host.c_str(),
                                sizeof(conn_out->host) - 1);
                        conn_out->port      = p.port;
                        conn_out->requested = true;
                    }
                }
                ImGui::PopStyleColor(3);

                ImGui::SameLine(0, 4);

                // Delete (X) button
                ImGui::PushStyleColor(ImGuiCol_Button,        btn_disconnect_base());
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, btn_disconnect_hov());
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  btn_disconnect_base());
                if (ImGui::Button("X##del", { 22.0f, 0.0f }))
                    to_delete = i;
                ImGui::PopStyleColor(3);

                ImGui::PopID();
            }

            ImGui::EndChild();

            if (to_delete >= 0) {
                settings->connection_profiles.erase(
                    settings->connection_profiles.begin() + to_delete);
                settings_save(*settings);
            }
        }
    }

    // Firmware info — shown once AUTOPILOT_VERSION has been received
    if (vs && vs->has_fw_info) {
        ImGui::Spacing();
        themed_sep();
        ImGui::Spacing();
        const char* ap_name = "Unknown";
        if (vs->autopilot == MAV_AUTOPILOT_ARDUPILOTMEGA) {
            switch (vs->type) {
            case MAV_TYPE_FIXED_WING:   ap_name = "ArduPlane";  break;
            case MAV_TYPE_GROUND_ROVER: ap_name = "ArduRover";  break;
            case MAV_TYPE_SUBMARINE:    ap_name = "ArduSub";    break;
            default:                    ap_name = "ArduCopter"; break;
            }
        } else if (vs->autopilot == MAV_AUTOPILOT_PX4) {
            ap_name = "PX4";
        }
        ImGui::TextColored(accent_col(), "FIRMWARE");
        ImGui::SameLine(0, 4);
        ImGui::Text("%s %s", ap_name, vs->fw_version);
        ImGui::TextDisabled("%.8s", vs->fw_hash);
    }
}
