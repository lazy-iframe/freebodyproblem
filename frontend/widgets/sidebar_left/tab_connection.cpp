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
#include "../../app_log.hpp"
#include "../../settings.hpp"
#include "../../../backend/sensor_inventory.hpp"
#include "imgui.h"
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <vector>

// The system behind the firmware line: who is on the link, what board it is,
// and what else on the vehicle speaks for itself.
//
// Here rather than with the calibrations it was first written for. It is not
// about a sensor — it is what the link found, which is this tab's subject, and
// it answers the question the FIRMWARE line above raises.
static void draw_system_block(MavlinkSender* sender, const VehicleState* vs,
                              uint8_t tsys, uint8_t tcomp)
{
    ImGui::Spacing();
    themed_sep();
    ImGui::Spacing();
    ImGui::TextColored(accent_col(), "SYSTEM");
    ImGui::Spacing();

    const char* ap   = mav_autopilot_name(vs->autopilot);
    const char* kind = mav_vehicle_type_name(vs->type);

    // The stack's name and version are on the FIRMWARE line above; what is here
    // is what that line leaves out.
    if (ap) ImGui::Text("System %u \xe2\x80\x94 %s", vs->sysid, ap);
    else    ImGui::Text("System %u \xe2\x80\x94 autopilot %u", vs->sysid,
                        (unsigned)vs->autopilot);
    if (kind) ImGui::TextDisabled("%s", kind);

    if (vs->has_fw_info) {
        // Vendor and product are the board's USB identifiers, which is as close
        // as AUTOPILOT_VERSION comes to naming the hardware. Hex because that
        // is how they are published and searched for.
        if (vs->vendor_id || vs->product_id)
            ImGui::TextDisabled("board %04X:%04X  rev %u",
                                vs->vendor_id, vs->product_id, vs->board_version);
        if (vs->board_uid)
            ImGui::TextDisabled("uid %016llx", (unsigned long long)vs->board_uid);
    } else {
        ImGui::TextDisabled("No AUTOPILOT_VERSION yet.");
    }

    if (vs->has_comp_info && vs->comp_info_uri[0]) {
        // The reply is a pointer, not the information: COMPONENT_INFORMATION
        // carries a MAVLink FTP URI to a JSON file on the vehicle. Reading it
        // would mean an FTP client this GCS does not have, so the URI is shown
        // for what it is rather than dressed up as metadata.
        ImGui::Spacing();
        ImGui::TextDisabled("Metadata (component %u):", vs->comp_info_compid);
        ImGui::PushTextWrapPos(0.0f);
        ImGui::TextDisabled("%s", vs->comp_info_uri);
        ImGui::PopTextWrapPos();
    }

    ImGui::Spacing();
    if (ui_grid_button("REFRESH COMPONENT INFO", { -1.0f, 26.0f })) {
        // AUTOPILOT_VERSION for the board and its capabilities;
        // COMPONENT_INFORMATION for where its metadata lives. Both are sent
        // only when asked for, which is why there is a button at all.
        sender->request_message(tsys, tcomp, 148);
        sender->request_message(tsys, tcomp, 395);
        gcs_log("requested AUTOPILOT_VERSION and COMPONENT_INFORMATION");
    }

    // Components, from their heartbeats. Sensors on the autopilot's own buses
    // are not components and never appear here — a compass has no MAV_COMP_ID.
    // What does appear is anything speaking MAVLink for itself: a gimbal, a
    // DroneCAN node, an IMU that reports separately.
    if (vs->component_count > 0) {
        ImGui::Spacing();
        ImGui::TextDisabled("Components heard from:");

        ImGui::PushStyleColor(ImGuiCol_ChildBg, bg_param_list());
        ImGui::PushStyleColor(ImGuiCol_Border,  col_separator());
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, FRAME_BORDER_NORMAL);

        // Sized to its contents, so it has nothing of its own to scroll.
        const float h = vs->component_count * ImGui::GetTextLineHeightWithSpacing() + 10.0f;
        if (ImGui::BeginChild("##components", { -1.0f, h }, true,
                              ImGuiWindowFlags_NoScrollbar |
                              ImGuiWindowFlags_NoScrollWithMouse)) {
            for (int i = 0; i < vs->component_count; ++i) {
                const auto& c    = vs->components[i];
                const char* name = mav_component_name(c.compid);
                const bool  self = (c.sysid == vs->sysid && c.compid == vs->compid);

                if (self) ImGui::TextColored(accent_col(), "%u/%u  %s",
                                             c.sysid, c.compid,
                                             name ? name : "unnamed");
                else if (name) ImGui::Text("%u/%u  %s", c.sysid, c.compid, name);
                else           ImGui::Text("%u/%u", c.sysid, c.compid);

                if (ImGui::IsItemHovered()) {
                    const char* t = mav_vehicle_type_name(c.type);
                    const char* a = mav_autopilot_name(c.autopilot);
                    ImGui::SetTooltip("%s, %s \xe2\x80\x94 %u heartbeats",
                                      t ? t : "unknown type",
                                      a ? a : "unknown stack", c.heartbeats);
                }
            }
        }
        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);
    }
}

void draw_tab_connection(MavlinkSender* sender,
                         const VehicleState* vs,
                         ConnectionRequest* conn_out,
                         LinkStatus link_status,
                         AppSettings* settings,
                         const std::vector<LinkInfo>& links)
{
    ImGui::Spacing();
    ImGui::TextColored(accent_col(), "CONNECTION");
    themed_sep();
    ImGui::Spacing();

    // The saved profiles, the firmware line and the system block below it run
    // past the fold on a short window, and the sidebar window itself is created
    // with NoScrollbar — so the panel brings its own scroll region rather than
    // quietly clipping the end of itself. The heading above stays outside it.
    if (!ImGui::BeginChild("##conn_scroll", { -1.0f, -1.0f }, false)) {
        ImGui::EndChild();
        return;
    }

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
    for (int i = 0; i < 3; ++i) {
        if (i > 0) ImGui::SameLine(0, 2);
        if (ui_tab_button(tab_labels[i], { tab_w, 24.0f }, conn_tab == i))
            conn_tab = i;
    }
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
        if (ui_grid_button("CONNECT##serial", { -1.0f, 0.0f }) && !s_ports.empty()) {
            conn_out->type = ConnType::Serial;
            strncpy(conn_out->device, s_ports[serial_sel].device.c_str(),
                    sizeof(conn_out->device) - 1);
            conn_out->baud      = baud_rates[baud_sel];
            conn_out->requested = true;
        }
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
        if (ui_grid_button("CONNECT##tcp", { -1.0f, 0.0f })) {
            conn_out->type = ConnType::TCP;
            strncpy(conn_out->host, tcp_host, sizeof(conn_out->host) - 1);
            conn_out->port      = tcp_port;
            conn_out->requested = true;
        }

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
        if (ui_grid_button("CONNECT##udp", { -1.0f, 0.0f })) {
            conn_out->type = ConnType::UDP;
            strncpy(conn_out->host, udp_host, sizeof(conn_out->host) - 1);
            conn_out->port      = udp_port;
            conn_out->requested = true;
        }
    }

    // ── Open links ────────────────────────────────────────────────────────────
    //
    // CONNECT above adds a link rather than replacing the one already open, so
    // this is a list and not a status line. Several links run at once, each on
    // its own thread carrying its own vehicles — a radio on serial and a SITL on
    // UDP at the same time — and each is dropped independently.
    ImGui::Spacing();
    themed_sep();
    ImGui::Spacing();
    ImGui::TextColored(accent_col(), "LINKS");
    ImGui::Spacing();

    if (links.empty()) {
        ImGui::TextDisabled("No links. Connect above.");
    } else {
        constexpr float btn_w = 22.0f;

        for (const auto& li : links) {
            ImGui::PushID((int)li.id);

            // The disconnect button leads the row: it is the only action here,
            // and lining the buttons up on the left keeps them in one column
            // whatever the link names happen to be.
            // Offered for dead links too, not just live ones: a link that
            // timed out or failed to open stays listed so the operator can see
            // why, and this is how it is cleared away.
            if (ui_solid_button("X##drop", { btn_w, 0.0f },
                                btn_disconnect_base(), btn_disconnect_hov())) {
                conn_out->disconnect         = true;
                conn_out->disconnect_link_id = li.id;
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Disconnect %s", li.name.c_str());

            ImGui::SameLine(0, 6);
            ImGui::AlignTextToFramePadding();

            switch (li.status) {
            case LinkStatus::Connecting:
                ImGui::TextColored(col_warning(), "\xe2\x97\x8f"); break;
            case LinkStatus::Connected:
                ImGui::TextColored(col_ok(),      "\xe2\x97\x8f"); break;
            case LinkStatus::Timeout:
            case LinkStatus::Error:
                ImGui::TextColored(col_error(),   "\xe2\x97\x8f"); break;
            default:
                ImGui::TextDisabled("\xe2\x97\x8f");               break;
            }

            ImGui::SameLine(0, 6);
            ImGui::TextUnformatted(li.name.c_str());

            // Second line: what the link is doing and what it found. Indented
            // under the name rather than beside it, because a serial device path
            // already fills the width of this sidebar.
            const char* st = "idle";
            switch (li.status) {
            case LinkStatus::Connecting: st = "connecting..."; break;
            case LinkStatus::Connected:  st = "connected";     break;
            case LinkStatus::Timeout:    st = "timeout";       break;
            case LinkStatus::Error:      st = "error";         break;
            default: break;
            }
            ImGui::Indent(btn_w + 6.0f);
            if (li.vehicles == 1)
                ImGui::TextDisabled("%s \xc2\xb7 1 vehicle", st);
            else
                ImGui::TextDisabled("%s \xc2\xb7 %d vehicles", st, li.vehicles);
            ImGui::Unindent(btn_w + 6.0f);

            ImGui::PopID();
        }
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
        if (ui_grid_button("SAVE##profile", { save_w, 0.0f }) && name_ok) {
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
                if (ui_grid_button("CONNECT##cp", { 64.0f, 0.0f })) {
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

                ImGui::SameLine(0, 4);

                // Delete (X) button — solid red, it destroys a saved profile.
                if (ui_solid_button("X##del", { 22.0f, 0.0f },
                                    btn_disconnect_base(), btn_disconnect_hov()))
                    to_delete = i;

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

    // Below the firmware line, and not gated on it: the components list and the
    // button that asks for AUTOPILOT_VERSION are most wanted exactly when it
    // has not arrived.
    if (vs && vs->has_heartbeat)
        draw_system_block(sender, vs, vs->sysid, vs->compid);

    ImGui::EndChild();
}
