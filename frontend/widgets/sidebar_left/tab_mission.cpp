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


#include "sidebar_internal.hpp"
#include "../sidebar_themes.hpp"
#include "../../app_log.hpp"
#include "imgui.h"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <algorithm>

// ── Command metadata ──────────────────────────────────────────────────────────

struct CmdMeta {
    uint16_t    id;
    const char* name;
    const char* p1;        // param1 label (nullptr = unused)
    const char* p2;
    const char* p3;
    const char* p4;
    bool        show_latlon; // show Lat / Lon / Alt fields
    bool        show_alt;    // show Alt even when no lat/lon
};

static constexpr CmdMeta CMD_TABLE[] = {
    // id    name                     p1              p2                p3              p4              ll    alt
    {  16, "WAYPOINT",          "Delay (s)",    "Accept R (m)",   "Pass R (m)",   "Yaw (°)",      true,  true  },
    {  17, "LOITER_UNLIM",      nullptr,        nullptr,          "Radius (m)",   "Hdg req",      true,  true  },
    {  18, "LOITER_TURNS",      "Turns",        "Hdg req",        "Radius (m)",   "Xtrack exit",  true,  true  },
    {  19, "LOITER_TIME",       "Time (s)",     "Hdg req",        "Radius (m)",   "Xtrack exit",  true,  true  },
    {  20, "RTL",               nullptr,        nullptr,          nullptr,        nullptr,        false, false },
    {  21, "LAND",              "Abort alt",    "Mode",           nullptr,        "Yaw (°)",      true,  true  },
    {  22, "TAKEOFF",           "Pitch min",    nullptr,          nullptr,        "Yaw (°)",      true,  true  },
    {  30, "CONT_CHANGE_ALT",   "Hdg req",      nullptr,          nullptr,        nullptr,        false, true  },
    {  31, "LOITER_TO_ALT",     "Hdg req",      "Radius (m)",     nullptr,        "Xtrack exit",  true,  true  },
    {  82, "ROI",               "ROI mode",     nullptr,          nullptr,        nullptr,        true,  true  },
    {  93, "NAV_DELAY",         "Hold (s)",     "Hour (UTC)",     "Minute (UTC)", "Second (UTC)", false, false },
    { 112, "COND_DELAY",        "Time (s)",     nullptr,          nullptr,        nullptr,        false, false },
    { 113, "COND_CHANGE_ALT",   "Rate (m/s)",   nullptr,          nullptr,        nullptr,        false, true  },
    { 114, "COND_DISTANCE",     "Dist (m)",     nullptr,          nullptr,        nullptr,        false, false },
    { 115, "COND_YAW",          "Angle (°)",    "Speed (°/s)",    "Direction",    "Relative",     false, false },
    { 176, "DO_SET_MODE",       "Mode",         "Custom mode",    nullptr,        nullptr,        false, false },
    { 177, "DO_JUMP",           "WP#",          "Repeat",         nullptr,        nullptr,        false, false },
    { 178, "DO_CHANGE_SPEED",   "Type",         "Speed (m/s)",    "Throttle %",   nullptr,        false, false },
    { 179, "DO_SET_HOME",       "Current",      nullptr,          nullptr,        nullptr,        true,  true  },
    { 181, "DO_SET_RELAY",      "Relay#",       "State",          nullptr,        nullptr,        false, false },
    { 182, "DO_REPEAT_RELAY",   "Relay#",       "Cycles",         "Dwell (s)",    nullptr,        false, false },
    { 183, "DO_SET_SERVO",      "Servo#",       "PWM (µs)",       nullptr,        nullptr,        false, false },
    { 184, "DO_REPEAT_SERVO",   "Servo#",       "PWM (µs)",       "Cycles",       "Dwell (s)",    false, false },
    { 189, "DO_LAND_START",     nullptr,        nullptr,          nullptr,        nullptr,        true,  true  },
    { 201, "DO_SET_ROI",        "ROI mode",     "WP#",            "ROI idx",      nullptr,        true,  true  },
    { 203, "DIGICAM_CONFIG",    "Exp mode",     "Shutter",        "Aperture",     "ISO",          false, false },
    { 204, "DIGICAM_CTRL",      "Session",      "Zoom step",      "Zoom type",    "Shutter cmd",  false, false },
    { 206, "MOUNT_CONFIGURE",   "Mode",         "Stabilize roll", "Stabilize tilt","Stabilize pan",false, false },
    { 207, "MOUNT_CONTROL",     "Pitch (°)",    "Roll (°)",       "Yaw (°)",      "Save",         false, false },
};
static constexpr int CMD_COUNT = (int)(sizeof(CMD_TABLE) / sizeof(CMD_TABLE[0]));

static const CmdMeta* find_meta(uint16_t cmd)
{
    for (int i = 0; i < CMD_COUNT; ++i)
        if (CMD_TABLE[i].id == cmd) return &CMD_TABLE[i];
    return nullptr;
}

static const char* cmd_short(uint16_t cmd)
{
    const CmdMeta* m = find_meta(cmd);
    if (m) return m->name;
    static char buf[12];
    snprintf(buf, sizeof(buf), "CMD_%u", cmd);
    return buf;
}

// ── Per-item expand state (indexed by waypoint index in s_edit) ───────────────

static std::vector<bool> s_open;  // one entry per waypoint in s_edit

// ── Editable copy ─────────────────────────────────────────────────────────────

static std::vector<MissionItem> s_edit;
static uint16_t  s_fetch_gen         = 0;     // snapshot of mission_count when last synced
static bool      s_has_edit          = false;
static bool      s_upload_done       = false;  // show result badge once
static bool      s_cleared           = false;  // user pressed CLEAR; don't auto-re-sync
static bool      s_fetching          = false;  // FETCH pressed; waiting for fresh FC reply
static bool      s_fetch_reset_seen  = false;  // has_mission went false after FETCH → MISSION_COUNT received

// Sync s_edit from vehicle state when a fresh fetch arrives.
// Will NOT fire after the user has pressed CLEAR (s_cleared) or while the user
// has pending edits (s_has_edit) to avoid clobbering unsaved work.
//
// When s_fetching is true we wait for has_mission to go false (MISSION_COUNT
// received) and then true again (all items downloaded) before we sync.  This
// prevents the stale vs->mission from the previous download from being copied
// into s_edit in the frames between pressing FETCH and the FC responding.
static void maybe_sync(const VehicleState* vs)
{
    const bool cur_has_mission = (vs && vs->has_mission);

    if (s_fetching) {
        if (!cur_has_mission)
            s_fetch_reset_seen = true;  // MISSION_COUNT received, items on the way
        else if (s_fetch_reset_seen) {
            // has_mission just became true again → fresh download complete
            s_edit      = vs->mission;
            s_fetch_gen = vs->mission_count;
            s_has_edit  = true;
            s_fetching          = false;
            s_fetch_reset_seen  = false;
            s_open.assign(s_edit.size(), false);
            s_upload_done = false;
        }
        return;
    }

    if (!cur_has_mission) return;
    if (s_cleared)  return;   // wait for explicit FETCH after a CLEAR
    if (s_has_edit) return;   // don't overwrite unsaved edits
    if (vs->mission_count == s_fetch_gen) return;  // already up to date

    s_edit      = vs->mission;
    s_fetch_gen = vs->mission_count;
    s_has_edit  = true;
    s_open.assign(s_edit.size(), false);
    s_upload_done = false;
}

// ── Helpers ───────────────────────────────────────────────────────────────────

static void labeled_float(const char* label, float* v, const char* unit = nullptr,
                           float /*speed*/ = 0.1f)
{
    char id[64];
    snprintf(id, sizeof(id), "##lf_%p", (void*)v);

    ImGui::AlignTextToFramePadding();
    if (unit) {
        char lbl[64];
        snprintf(lbl, sizeof(lbl), "%s (%s)", label, unit);
        ImGui::TextUnformatted(lbl);
    } else {
        ImGui::TextUnformatted(label);
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputFloat(id, v, 0.f, 0.f, "%.4g");
}

static void labeled_double(const char* label, double* v, const char* unit = nullptr)
{
    char id[64];
    snprintf(id, sizeof(id), "##ld_%p", (void*)v);

    ImGui::AlignTextToFramePadding();
    if (unit) {
        char lbl[64];
        snprintf(lbl, sizeof(lbl), "%s (%s)", label, unit);
        ImGui::TextUnformatted(lbl);
    } else {
        ImGui::TextUnformatted(label);
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputDouble(id, v, 0.0, 0.0, "%.6f");
}

// Draw expanded parameter fields for one waypoint
static void draw_wp_fields(MissionItem& item)
{
    ImGui::Indent(12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 4.0f, 3.0f });

    const CmdMeta* m = find_meta(item.command);

    if (m) {
        if (m->p1) labeled_float(m->p1, &item.param1);
        if (m->p2) labeled_float(m->p2, &item.param2);
        if (m->p3) labeled_float(m->p3, &item.param3);
        if (m->p4) labeled_float(m->p4, &item.param4);
        if (m->show_latlon) {
            labeled_double("Lat", &item.lat);
            labeled_double("Lon", &item.lon);
        }
        if (m->show_alt || m->show_latlon) {
            labeled_float("Alt (m)", &item.alt);
        }
    } else {
        // Unknown command — show all fields generically
        labeled_float("Param1", &item.param1);
        labeled_float("Param2", &item.param2);
        labeled_float("Param3", &item.param3);
        labeled_float("Param4", &item.param4);
        labeled_double("Lat",  &item.lat);
        labeled_double("Lon",  &item.lon);
        labeled_float("Alt (m)", &item.alt);
    }

    // Frame selector
    const char* frames[] = { "GLOBAL_ABS", "LOCAL", "MISSION", "GLOBAL_REL", "LOCAL_ENU",
                              "GLOBAL_INT", "GLOBAL_REL_INT", "LOCAL_OFFSET",
                              "BODY_NED", "BODY_OFFSET", "GLOBAL_TERRAIN" };
    const int n_frames = (int)(sizeof(frames) / sizeof(frames[0]));
    int fi = std::min((int)item.frame, n_frames - 1);
    ImGui::TextDisabled("Frame");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1.0f);
    char frame_id[32];
    snprintf(frame_id, sizeof(frame_id), "##frame_%u", item.seq);
    if (ImGui::BeginCombo(frame_id, frames[fi])) {
        for (int f = 0; f < n_frames; ++f) {
            bool sel = (f == fi);
            if (ImGui::Selectable(frames[f], sel))
                item.frame = (uint8_t)f;
            if (sel) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::PopStyleVar();
    ImGui::Unindent(12.0f);
}

// ── Main draw function ────────────────────────────────────────────────────────

void draw_tab_mission(MavlinkSender* sender, const VehicleState* vs,
                      MissionPickState* pick)
{
    const bool    connected = (vs && vs->has_heartbeat);
    const uint8_t tsys      = connected ? vs->sysid  : 1;
    const uint8_t tcomp     = connected ? vs->compid : 1;

    maybe_sync(vs);

    // ── Publish live edit pointer so map_view can draw current edits ──────────
    if (pick)
        pick->edit_mission = s_has_edit ? &s_edit : nullptr;

    // ── Consume a completed map pick ──────────────────────────────────────────
    if (pick && pick->pick_done) {
        const int idx = pick->active_index;
        if (idx >= 0 && idx < (int)s_edit.size()) {
            s_edit[idx].lat = pick->picked_lat;
            s_edit[idx].lon = pick->picked_lon;
        }
        pick->pick_done    = false;
        pick->active_index = -1;
    }

    // ── Cancel pick mode on Escape ────────────────────────────────────────────
    if (pick && pick->active_index >= 0 && ImGui::IsKeyPressed(ImGuiKey_Escape))
        pick->active_index = -1;

    ImGui::Spacing();
    ImGui::TextColored(accent_col(), "MISSION");
    if (!connected) {
        ImGui::SameLine(0, 6);
        ImGui::TextColored(col_no_link_muted(), "(no link)");
    }
    themed_sep();
    ImGui::Spacing();

    // ── Fetch / Clear row ─────────────────────────────────────────────────────
    const float btn_w = (ImGui::GetContentRegionAvail().x - 4.0f) * 0.5f;

    ImGui::BeginDisabled(!connected);
    push_flash_colors(CmdFlashState::Normal);
    if (ImGui::Button("FETCH", { btn_w, 24.0f }) && connected) {
        sender->request_mission_list(tsys, tcomp);
        s_has_edit          = false;
        s_fetch_gen         = 0;
        s_cleared           = false;
        s_fetching          = true;   // wait for fresh FC reply before syncing
        s_fetch_reset_seen  = false;
        if (pick) { pick->active_index = -1; pick->pick_done = false; }
        gcs_log("requesting mission list");
    }
    ImGui::PopStyleColor(3);
    ImGui::EndDisabled();

    ImGui::SameLine(0, 4);

    ImGui::BeginDisabled(!s_has_edit);
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.5f, 0.1f, 0.1f, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
    if (ImGui::Button("CLEAR", { btn_w, 24.0f }) && s_has_edit) {
        s_edit.clear();
        s_open.clear();
        s_has_edit    = false;
        s_cleared     = true;   // block auto-re-sync until user presses FETCH
        s_upload_done = false;
        if (pick) { pick->active_index = -1; pick->pick_done = false; pick->edit_mission = nullptr; }
    }
    ImGui::PopStyleColor(3);
    ImGui::EndDisabled();

    // ── Download progress ─────────────────────────────────────────────────────
    if (vs && vs->mission_count > 0 && vs->mission_received < vs->mission_count) {
        ImGui::Spacing();
        char prog_lbl[32];
        snprintf(prog_lbl, sizeof(prog_lbl), "%u / %u", vs->mission_received, vs->mission_count);
        ImGui::ProgressBar((float)vs->mission_received / vs->mission_count,
                           { -1.0f, 10.0f }, prog_lbl);
    }

    ImGui::Spacing();
    ImGui::TextDisabled("%d waypoints", (int)s_edit.size());
    ImGui::Spacing();
    themed_sep();
    ImGui::Spacing();

    // ── Waypoint list ─────────────────────────────────────────────────────────
    // Reserve: spacing(4) + add-combo(~28) + spacing(4) + upload-btn(26) +
    //          spacing(4) + status-badge-line(~22) + bottom-padding(8) ≈ 96px
    const float list_h = std::max(80.0f,
                                  ImGui::GetContentRegionAvail().y - 96.0f);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, bg_child_dark());
    if (ImGui::BeginChild("##wp_list", { -1.0f, list_h }, false)) {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 4.0f, 2.0f });

        // Ensure open vector is sized
        if ((int)s_open.size() != (int)s_edit.size())
            s_open.resize(s_edit.size(), false);

        int to_delete = -1;

        for (int i = 0; i < (int)s_edit.size(); ++i) {
            MissionItem& item = s_edit[i];
            item.seq = (uint16_t)i;  // keep seq in sync with list position

            ImGui::PushID(i);

            // ── Collapse / expand arrow ───────────────────────────────────────
            const char* arrow = s_open[i] ? "v" : ">";
            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0,0,0,0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1,1,1,0.08f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(1,1,1,0.14f));
            if (ImGui::SmallButton(arrow))
                s_open[i] = !s_open[i];
            ImGui::PopStyleColor(3);
            ImGui::SameLine(0, 4);

            // ── Row header (seq, cmd, alt) ────────────────────────────────────
            if (item.is_current)
                ImGui::PushStyleColor(ImGuiCol_Text, accent_col());

            char hdr[48];
            const CmdMeta* m = find_meta(item.command);
            if (m && m->show_alt)
                snprintf(hdr, sizeof(hdr), "#%u  %s  %.0fm", i, m ? m->name : cmd_short(item.command), (double)item.alt);
            else
                snprintf(hdr, sizeof(hdr), "#%u  %s", i, m ? m->name : cmd_short(item.command));
            ImGui::TextUnformatted(hdr);

            if (item.is_current)
                ImGui::PopStyleColor();

            // ── Pick button (map pin) and Delete button — right-aligned ──────────
            const float del_w  = 18.0f;
            const float pick_w = 18.0f;
            const float right_edge = ImGui::GetContentRegionAvail().x + ImGui::GetCursorPosX();

            // Determine if this command supports lat/lon (show pick button for those)
            const CmdMeta* row_m = find_meta(item.command);
            const bool has_latlon = !row_m || row_m->show_latlon; // show for unknown too

            if (pick && has_latlon) {
                const bool is_picking = (pick->active_index == i);
                ImGui::SameLine(right_edge - del_w - pick_w - 2.0f);
                if (is_picking) {
                    // Highlighted while waiting for map click
                    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.9f, 0.8f, 0.1f, 0.9f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.9f, 0.2f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(1.0f, 1.0f, 0.3f, 1.0f));
                } else {
                    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.2f, 0.4f, 0.8f, 0.4f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.5f, 1.0f, 0.7f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.4f, 0.6f, 1.0f, 1.0f));
                }
                if (ImGui::SmallButton(is_picking ? "*" : "+")) {
                    if (is_picking)
                        pick->active_index = -1;  // cancel
                    else
                        pick->active_index = i;   // start picking
                }
                ImGui::PopStyleColor(3);
            }

            ImGui::SameLine(right_edge - del_w);
            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.6f,0.1f,0.1f,0.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f,0.2f,0.2f,0.8f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(1.0f,0.3f,0.3f,1.0f));
            if (ImGui::SmallButton("x"))
                to_delete = i;
            ImGui::PopStyleColor(3);

            // ── Expanded fields ───────────────────────────────────────────────
            if (s_open[i]) {
                // Command selector
                ImGui::Indent(12.0f);
                ImGui::TextDisabled("Cmd");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(-1.0f);
                if (ImGui::BeginCombo("##cmd", cmd_short(item.command))) {
                    for (int c = 0; c < CMD_COUNT; ++c) {
                        bool sel = (CMD_TABLE[c].id == item.command);
                        char opt[32];
                        snprintf(opt, sizeof(opt), "%s (%u)", CMD_TABLE[c].name, CMD_TABLE[c].id);
                        if (ImGui::Selectable(opt, sel)) {
                            item.command = CMD_TABLE[c].id;
                            item.param1 = item.param2 = item.param3 = item.param4 = 0.f;
                        }
                        if (sel) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
                ImGui::Unindent(12.0f);

                draw_wp_fields(item);
            }

            ImGui::Separator();
            ImGui::PopID();
        }

        if (to_delete >= 0 && to_delete < (int)s_edit.size()) {
            s_edit.erase(s_edit.begin() + to_delete);
            s_open.erase(s_open.begin() + to_delete);
        }

        ImGui::PopStyleVar();
    }
    ImGui::EndChild();
    ImGui::PopStyleColor(); // ChildBg

    // ── Add waypoint ──────────────────────────────────────────────────────────
    ImGui::Spacing();
    if (ImGui::BeginCombo("##add_cmd", "+ Add Waypoint")) {
        for (int c = 0; c < CMD_COUNT; ++c) {
            char opt[32];
            snprintf(opt, sizeof(opt), "%s (%u)", CMD_TABLE[c].name, CMD_TABLE[c].id);
            if (ImGui::Selectable(opt, false)) {
                MissionItem ni{};
                ni.seq     = (uint16_t)s_edit.size();
                ni.command = CMD_TABLE[c].id;
                ni.frame   = 3; // MAV_FRAME_GLOBAL_RELATIVE_ALT
                // Pre-fill lat/lon from last waypoint with valid coords
                for (int k = (int)s_edit.size() - 1; k >= 0; --k) {
                    if (s_edit[k].lat != 0.0 || s_edit[k].lon != 0.0) {
                        ni.lat = s_edit[k].lat;
                        ni.lon = s_edit[k].lon;
                        ni.alt = s_edit[k].alt;
                        break;
                    }
                }
                s_edit.push_back(ni);
                s_open.push_back(true); // expand new item
                s_has_edit = true;      // protect manual edits from auto-sync
            }
        }
        ImGui::EndCombo();
    }

    // ── Upload ────────────────────────────────────────────────────────────────
    ImGui::Spacing();
    ImGui::BeginDisabled(!connected || s_edit.empty());
    push_flash_colors(CmdFlashState::Normal);
    if (ImGui::Button("UPLOAD MISSION", { -1.0f, 26.0f }) && connected && !s_edit.empty()) {
        // Renumber seqs, mark first item current
        for (int i = 0; i < (int)s_edit.size(); ++i) {
            s_edit[i].seq        = (uint16_t)i;
            s_edit[i].is_current = (i == 0);
        }
        sender->start_upload(tsys, tcomp, s_edit);
        s_upload_done = false;
        gcs_log("uploading %d waypoints", (int)s_edit.size());
    }
    ImGui::PopStyleColor(3);
    ImGui::EndDisabled();

    // Upload status badge
    if (vs) {
        using US = VehicleState::UploadStatus;
        if (vs->upload_status == US::InProgress) {
            ImGui::SameLine(0, 6);
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "uploading...");
        } else if (vs->upload_status == US::Accepted) {
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "Upload accepted");
        } else if (vs->upload_status == US::Failed) {
            char fbuf[40];
            snprintf(fbuf, sizeof(fbuf), "Upload FAILED (err %u)", (unsigned)vs->upload_ack_result);
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", fbuf);
        }
    }
}
