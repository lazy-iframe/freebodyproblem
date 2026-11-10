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


#include "frontend/components/sidebar_left/sidebar_internal.hpp"
#include "frontend/components/sidebar_themes.hpp"
#include "frontend/source/app_log.hpp"
#include "frontend/source/settings.hpp"
#include "backend/vehicle/sensor_inventory.hpp"
#include "backend/mavlink/signing.hpp"
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

// ── Vehicles ──────────────────────────────────────────────────────────────────
//
// Every vehicle the links above have found, as the callsign chip's switcher
// lists them, each with a way to restart it. Here rather than in the switcher:
// the switcher is for looking, and a reboot is not something to put one
// mis-click away from choosing which aircraft to look at.
static void draw_vehicle_list(ConnectionRequest* conn_out,
                              const std::vector<VehicleChip>& vehicles,
                              VehicleId active_vehicle)
{
    ImGui::Spacing();
    themed_sep();
    ImGui::Spacing();
    ImGui::TextColored(accent_col(), "VEHICLES");
    ImGui::Spacing();

    if (vehicles.empty()) {
        ImGui::TextDisabled("No vehicles heard yet.");
        return;
    }

    // The state box and the REBOOT button are one size, so the two read as a
    // strip of controls rather than a label and a button.
    const float btn_w = ui_vehicle_box_width("REBOOT");
    const float row_h = ui_vehicle_row_height();

    for (const auto& v : vehicles) {
        // By link and sysid, not by label — see the switcher in topbar.cpp.
        ImGui::PushID((int)v.id.link_id);
        ImGui::PushID((int)v.id.sysid);

        const float  text_w = ImGui::GetContentRegionAvail().x - btn_w * 2.0f - 4.0f - 6.0f;
        const ImVec2 r0     = ImGui::GetCursorScreenPos();
        ImGui::Dummy({ text_w, row_h });
        ui_vehicle_row(v, v.id == active_vehicle, r0, { r0.x + text_w, r0.y + row_h },
                       /*show_state=*/false);

        ImGui::SameLine(0, 6);
        ui_vehicle_state_box(v, { btn_w, row_h });

        // Not while armed: a restart in the air is a crash. ArduPilot refuses
        // it anyway, but the button should not offer what the vehicle will
        // decline. Not while silent either — the command would go nowhere.
        const bool can_reboot = v.has_heartbeat && !v.armed && !v.stale;

        ImGui::SameLine(0, 4);
        ImGui::BeginDisabled(!can_reboot);
        if (ui_solid_button("REBOOT", { btn_w, row_h },
                            btn_disconnect_base(), btn_disconnect_hov()))
            ImGui::OpenPopup("##confirm_reboot_vehicle");
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            if (v.armed)       ImGui::SetTooltip("Disarm before rebooting");
            else if (v.stale)  ImGui::SetTooltip("No signal \xe2\x80\x94 it would not hear the command");
            else               ImGui::SetTooltip("Restart vehicle %u's autopilot", (unsigned)v.number);
        }

        char question[64];
        snprintf(question, sizeof(question), "RESTART VEHICLE %u NOW?", (unsigned)v.number);
        if (ui_confirm_popup("##confirm_reboot_vehicle", "REBOOT VEHICLE", question,
                             "REBOOT", btn_disconnect_base()) == UiConfirm::Confirmed)
            conn_out->reboot_vehicle = v.id;

        ImGui::PopID();
        ImGui::PopID();
    }
}

// ── Message signing ───────────────────────────────────────────────────────────
//
// One connection's key, and where each vehicle on it stands with that key. What makes it
// safe to leave on is that each state says what it means for flying: above all a vehicle
// signing with a key this connection lacks, which is still heard but ignores every
// command, and looks fine until one is needed.
//
// Which link has it open, by id: one at a time, since each is a form as tall as the list.
static uint32_t s_signing_open_link = 0;

static void draw_link_signing(const LinkInfo& li, const std::vector<VehicleChip>& vehicles,
                              ConnectionRequest* conn_out)
{
    signing::LinkKey* const key = signing::for_link(li.name);

    static char     s_phrase[128] = "";
    static bool     s_show        = false;
    static uint32_t s_phrase_link = 0;
    const auto      wipe_phrase   = [] {
        std::memset(s_phrase, 0, sizeof(s_phrase));
        s_show = false;
    };
    // Typed for one connection, and not to be put to another by accident.
    if (s_phrase_link != li.id) {
        wipe_phrase();
        s_phrase_link = li.id;
    }

    ImGui::Indent(8.0f);
    ImGui::Spacing();
    ImGui::TextColored(accent_col(), "MESSAGE SIGNING, LINK %u", (unsigned)li.id);

    const std::string fp = signing::fingerprint(key);
    const std::string fp_name = signing::saved_name(key);
    if (fp.empty())            ImGui::TextDisabled("No key. Nothing here is signed or checked.");
    else if (fp_name.empty())  ImGui::TextColored(col_ok(), "KEY  %s", fp.c_str());
    else                       ImGui::TextColored(col_ok(), "KEY  %s  (%s)", fp.c_str(), fp_name.c_str());
    if (!fp.empty() && ImGui::IsItemHovered())
        ImGui::SetTooltip("Tells keys apart, and gives nothing of the key away");

    // Set a key: typed, or generated and then visible so it can go in another GCS.
    ImGui::TextDisabled("PASSPHRASE");
    const float gap   = 4.0f;
    const float third = (ImGui::GetContentRegionAvail().x - gap * 2.0f) / 3.0f;
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputText("##sign_phrase", s_phrase, sizeof(s_phrase),
                     s_show ? ImGuiInputTextFlags_None : ImGuiInputTextFlags_Password);
    if (ui_grid_button(s_show ? "HIDE##sign" : "SHOW##sign", { third, 0.0f })) s_show = !s_show;
    ImGui::SameLine(0, gap);
    if (ui_grid_button("GENERATE##sign", { third, 0.0f })) {
        const std::string g = signing::generate_passphrase();
        if (!g.empty()) {
            snprintf(s_phrase, sizeof(s_phrase), "%s", g.c_str());
            s_show = true;
        }
    }
    ImGui::SameLine(0, gap);
    ImGui::BeginDisabled(s_phrase[0] == '\0');
    if (ui_grid_button("USE##sign", { third, 0.0f })) {
        if (fp.empty()) {
            signing::set_passphrase(key, s_phrase);
            gcs_log("signing: %s now has key %s", li.name.c_str(), signing::fingerprint(key).c_str());
            wipe_phrase();
        } else {
            ImGui::OpenPopup("##confirm_sign_replace");
        }
    }
    ImGui::EndDisabled();
    if (ui_confirm_popup("##confirm_sign_replace", "REPLACE KEY",
                         "Vehicles using the old key will stop obeying this GCS.",
                         "REPLACE", btn_disconnect_base()) == UiConfirm::Confirmed) {
        signing::set_passphrase(key, s_phrase);
        gcs_log("signing: %s now has key %s", li.name.c_str(), signing::fingerprint(key).c_str());
        wipe_phrase();
    }

    // What this connection will take. Only worth asking once there is a key to check with.
    ImGui::BeginDisabled(fp.empty());
    const float half = (ImGui::GetContentRegionAvail().x - gap) * 0.5f;
    const bool  req  = signing::require_signed(key);
    if (ui_grid_button(req ? "REQUIRING SIGNED##sign" : "REQUIRE SIGNED##sign", { half, 0.0f }, req)) {
        signing::set_require_signed(key, !req);
        gcs_log("signing: %s: unsigned messages now %s", li.name.c_str(), !req ? "dropped" : "accepted");
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        ImGui::SetTooltip("Drop anything unsigned on this link, whatever it says it is,\n"
                          "telemetry radios' RADIO_STATUS aside. A vehicle with no key then\n"
                          "disappears, and cannot be given one from here.");
    ImGui::SameLine(0, gap);
    if (ui_solid_button("FORGET KEY##sign", { half, 0.0f }, btn_disconnect_base(), btn_disconnect_hov()))
        ImGui::OpenPopup("##confirm_sign_forget");
    ImGui::EndDisabled();
    if (ui_confirm_popup("##confirm_sign_forget", "FORGET KEY",
                         "Vehicles using this key will stop obeying this GCS.",
                         "FORGET", btn_disconnect_base()) == UiConfirm::Confirmed) {
        signing::forget_key(key);
        gcs_log("signing: %s: key forgotten", li.name.c_str());
    }

    // Keys kept by name, for other connections. The link's own key is already remembered
    // for it, so this is for sharing one, and for one generated and never written down.
    {
        static std::string s_chosen;
        static char        s_name[32] = "";

        const std::vector<signing::SavedKey> saved = signing::saved_keys();
        const signing::SavedKey* chosen = nullptr;
        for (const auto& k : saved)
            if (k.name == s_chosen) chosen = &k;

        ImGui::Spacing();
        ImGui::TextDisabled("SAVED KEYS");
        // Save this link's key.
        const auto save_now = [&] {
            if (signing::save_key(key, s_name)) {
                gcs_log("signing: key %s saved as \"%s\"", fp.c_str(), s_name);
                s_chosen = s_name;
                s_name[0] = '\0';
            }
        };
        const float save_w = ImGui::CalcTextSize("SAVE KEY").x + 24.0f;
        ImGui::BeginDisabled(fp.empty());
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - save_w - gap);
        ImGui::InputTextWithHint("##sign_save_name", "name for this key", s_name, sizeof(s_name));
        ImGui::SameLine(0, gap);
        ImGui::BeginDisabled(s_name[0] == '\0');
        if (ui_grid_button("SAVE KEY##sign", { save_w, 0.0f })) {
            if (signing::has_saved_key(s_name)) ImGui::OpenPopup("##confirm_saved_over");
            else save_now();
        }
        ImGui::EndDisabled();
        ImGui::EndDisabled();
        if (ui_confirm_popup("##confirm_saved_over", "REPLACE SAVED KEY",
                             "A key is already saved under this name. It will be lost.",
                             "REPLACE", btn_disconnect_base()) == UiConfirm::Confirmed)
            save_now();

        char preview[64];
        if (chosen)             snprintf(preview, sizeof(preview), "%s  %s", chosen->name.c_str(), chosen->fingerprint.c_str());
        else if (saved.empty()) snprintf(preview, sizeof(preview), "None saved");
        else                    snprintf(preview, sizeof(preview), "Choose...");
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::BeginDisabled(saved.empty());
        if (ImGui::BeginCombo("##sign_saved", preview)) {
            for (const auto& k : saved) {
                char row[64];
                snprintf(row, sizeof(row), "%s  %s", k.name.c_str(), k.fingerprint.c_str());
                if (ImGui::Selectable(row, chosen && k.name == chosen->name)) s_chosen = k.name;
            }
            ImGui::EndCombo();
        }
        ImGui::EndDisabled();

        const auto use_chosen = [&] {
            if (signing::use_saved_key(key, s_chosen))
                gcs_log("signing: %s now has saved key \"%s\"", li.name.c_str(), s_chosen.c_str());
        };
        ImGui::BeginDisabled(!chosen);
        if (ui_grid_button("USE SAVED##sign", { half, 0.0f })) {
            if (fp.empty() || (chosen && chosen->fingerprint == fp)) use_chosen();
            else ImGui::OpenPopup("##confirm_saved_use");
        }
        ImGui::SameLine(0, gap);
        if (ui_solid_button("DELETE SAVED##sign", { half, 0.0f }, btn_disconnect_base(), btn_disconnect_hov()))
            ImGui::OpenPopup("##confirm_saved_delete");
        ImGui::EndDisabled();
        if (ui_confirm_popup("##confirm_saved_use", "USE SAVED KEY",
                             "Vehicles using this link's current key will stop obeying this GCS.",
                             "REPLACE", btn_disconnect_base()) == UiConfirm::Confirmed)
            use_chosen();
        if (ui_confirm_popup("##confirm_saved_delete", "DELETE SAVED KEY",
                             "Links using it keep their copy. It can't be got back from here.",
                             "DELETE", btn_disconnect_base()) == UiConfirm::Confirmed) {
            signing::delete_saved_key(s_chosen);
            gcs_log("signing: saved key \"%s\" deleted", s_chosen.c_str());
            s_chosen.clear();
        }
    }

    // Each vehicle on this link.
    for (const VehicleChip& v : vehicles) {
        if (v.id.link_id != li.id || !v.has_heartbeat) continue;
        ImGui::PushID((int)v.id.sysid);
        ImGui::Spacing();

        // Only what has got through under this key, from a vehicle still being heard,
        // says anything about this key. Anything else is memory, and the wrong key looks
        // just like that: everything dropped, and the last state left standing.
        const bool silent = v.stale || v.heartbeat_signing_gen != signing::generation();

        char who[24];
        snprintf(who, sizeof(who), "(%u) SYS%u", (unsigned)v.number, (unsigned)v.id.sysid);
        // Silence alone is a lost link. It is the key's doing only when frames that fail
        // their signature are what is arriving instead.
        if (silent && (fp.empty() || li.bad_signature == 0))
            ImGui::TextDisabled("%s not heard", who);
        else if (silent)
            ImGui::TextColored(col_error(), "%s NOT HEARD WITH THIS KEY:\nit may be using another", who);
        else if (v.heartbeat_signed && fp.empty())
            ImGui::TextColored(col_error(), "%s SIGNS, THIS LINK HAS NO KEY:\nits commands will be ignored", who);
        else if (v.heartbeat_signed)
            ImGui::TextColored(col_ok(), "%s SIGNED, its messages verified with this key", who);
        else if (!fp.empty())
            ImGui::TextColored(col_warning(), "%s UNSIGNED: anyone can pose as it", who);
        else
            ImGui::TextDisabled("%s unsigned", who);

        // Not over a link that is down: there is nothing to carry it. A vehicle that is
        // only silent is fair game, since one on another key is silent, and a USB port
        // takes this unsigned to put that right.
        const bool link_up = li.status == LinkStatus::Connected;
        ImGui::BeginDisabled(v.armed || fp.empty() || !link_up);
        if (ui_grid_button("SEND KEY##sign", { half, 0.0f }))
            ImGui::OpenPopup("##confirm_sign_send");
        ImGui::EndDisabled();
        if (v.armed && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            ImGui::SetTooltip("Disarm first: vehicles refuse this armed");
        ImGui::SameLine(0, gap);
        ImGui::BeginDisabled(v.armed || !v.heartbeat_signed || !link_up);
        if (ui_grid_button("STOP SIGNING##sign", { half, 0.0f }))
            ImGui::OpenPopup("##confirm_sign_stop");
        ImGui::EndDisabled();

        // The key goes in the clear, and only MAVLink's own rules say where that is
        // safe. This station cannot tell a USB cable from a radio on a serial port.
        if (ui_confirm_popup("##confirm_sign_send", "SEND KEY",
                             "The key travels unencrypted. Use USB or wired Ethernet, not a radio.",
                             "SEND KEY", btn_disconnect_base()) == UiConfirm::Confirmed) {
            conn_out->signing_vehicle = v.id;
            conn_out->signing_install = true;
        }
        if (ui_confirm_popup("##confirm_sign_stop", "STOP SIGNING",
                             "The vehicle will accept commands from anyone again.",
                             "STOP SIGNING", btn_disconnect_base()) == UiConfirm::Confirmed) {
            conn_out->signing_vehicle = v.id;
            conn_out->signing_install = false;
        }
        ImGui::PopID();
    }

    // What a signed vehicle's line above does not promise, and this station cannot see:
    // ArduPilot takes anything on its first MAVLink port, so a lost key can be reset. That
    // is the one a default SITL's MAVProxy and every other client of tcp:5760 come in on.
    if (!fp.empty()) {
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, col_warning());
        ImGui::TextWrapped("Not locked yet: ArduPilot still obeys anyone on its first MAVLink "
                           "port, normally USB (tcp:5760 on SITL), and anything relayed through it. "
                           "Other ports refuse unsigned commands.");
        ImGui::PopStyleColor();
    }

    // Frames that never reached a vehicle. A vehicle on another key shows up only here,
    // and on the link's own row, as a link that hears nothing.
    ImGui::Spacing();
    if (li.bad_signature > 0) {
        ImGui::PushStyleColor(ImGuiCol_Text, col_error());
        ImGui::TextWrapped("%llu dropped with a bad signature: another key, a replay, or forged",
                           (unsigned long long)li.bad_signature);
        ImGui::PopStyleColor();
    }
    if (li.unsigned_dropped > 0) {
        ImGui::PushStyleColor(ImGuiCol_Text, col_warning());
        ImGui::TextWrapped("%llu unsigned dropped", (unsigned long long)li.unsigned_dropped);
        ImGui::PopStyleColor();
    }
    ImGui::Unindent(8.0f);
    ImGui::Spacing();
}

void draw_tab_connection(MavlinkSender* sender,
                         const VehicleState* vs,
                         ConnectionRequest* conn_out,
                         LinkStatus link_status,
                         AppSettings* settings,
                         const std::vector<LinkInfo>& links,
                         const std::vector<VehicleChip>& vehicles,
                         VehicleId active_vehicle)
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
        // Laid out as the VEHICLES list below lays out its rows — two lines of
        // text, a state box, an action block — and with the same box width, so
        // the two lists' columns line up one above the other.
        const float btn_w  = ui_vehicle_box_width("REBOOT");
        const float row_h  = ui_vehicle_row_height();
        const float line_h = ImGui::GetTextLineHeight();

        // Wide enough for its own caption; the other two stay in the VEHICLES columns.
        const float sign_w = ImGui::CalcTextSize("SIGNING").x + 18.0f;

        for (const auto& li : links) {
            ImGui::PushID((int)li.id);

            // State: the box's caption, the word spelled out on hover, and the
            // colour. Only a dead link gets a loud frame.
            const char* cap  = "IDLE";
            const char* word = "idle";
            ImVec4      col  = g_theme.col_log;
            bool        loud = false;
            switch (li.status) {
            case LinkStatus::Connecting: cap = "WAIT"; word = "connecting...";            col = col_warning(); break;
            case LinkStatus::Connected:  cap = "LIVE"; word = "connected";                col = col_ok();      break;
            case LinkStatus::Timeout:    cap = "LOST"; word = "timeout \xe2\x80\x94 nothing heard"; col = col_error(); loud = true; break;
            case LinkStatus::Error:      cap = "ERR";  word = "error \xe2\x80\x94 could not open";  col = col_error(); loud = true; break;
            default: break;
            }

            // Name on the first line, what it found underneath. Clipped to the
            // text column, since a serial device path alone can fill it.
            const float  text_w = ImGui::GetContentRegionAvail().x - btn_w * 2.0f - sign_w - 8.0f - 6.0f;
            const ImVec2 r0     = ImGui::GetCursorScreenPos();
            const ImVec2 r1     = { r0.x + text_w, r0.y + row_h };
            ImGui::Dummy({ text_w, row_h });
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", li.name.c_str());
            {
                ImDrawList* dl = ImGui::GetWindowDrawList();
                dl->PushClipRect(r0, r1, true);
                char title[96];
                snprintf(title, sizeof(title), "%u  %s", (unsigned)li.id, li.name.c_str());
                dl->AddText({ r0.x + 4.0f, r0.y + 1.0f },
                            ui_col(g_theme.col_text_on_dark), title);

                // A wrong key ends in a link that hears nothing, so it is said here
                // too, for when the section is shut.
                char  sub[48];
                ImU32 sub_col = ui_col_label();
                const bool has_key = signing::active(signing::for_link(li.name));
                if (li.bad_signature > 0) {
                    snprintf(sub, sizeof(sub), "BAD SIGNATURES: %llu", (unsigned long long)li.bad_signature);
                    sub_col = ui_col(g_theme.col_error);
                } else if (li.vehicles == 1) {
                    snprintf(sub, sizeof(sub), "1 vehicle%s", has_key ? "  \xc2\xb7  KEY" : "");
                } else {
                    snprintf(sub, sizeof(sub), "%d vehicles%s", li.vehicles, has_key ? "  \xc2\xb7  KEY" : "");
                }
                dl->AddText({ r0.x + 4.0f, r0.y + line_h + 3.0f }, sub_col, sub);
                dl->PopClipRect();
            }

            ImGui::SameLine(0, 6);
            const bool signing_open = s_signing_open_link == li.id;
            if (ui_grid_button("SIGNING##link", { sign_w, row_h }, signing_open))
                s_signing_open_link = signing_open ? 0 : li.id;
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("MAVLink message signing for link %u", (unsigned)li.id);

            ImGui::SameLine(0, 4);
            ui_state_box(cap, word, col, loud, { btn_w, row_h });

            // Offered for dead links too, not just live ones: a link that
            // timed out or failed to open stays listed so the operator can see
            // why, and this is how it is cleared away.
            ImGui::SameLine(0, 4);
            if (ui_solid_button("CLOSE", { btn_w, row_h },
                                btn_disconnect_base(), btn_disconnect_hov())) {
                conn_out->disconnect         = true;
                conn_out->disconnect_link_id = li.id;
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Disconnect %s", li.name.c_str());

            if (s_signing_open_link == li.id) draw_link_signing(li, vehicles, conn_out);

            ImGui::PopID();
        }
    }

    draw_vehicle_list(conn_out, vehicles, active_vehicle);

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
