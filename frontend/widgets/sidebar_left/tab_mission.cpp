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
#include "../../../backend/firmware_profile.hpp"
#include "../vehicle_ui_state.hpp"
#include "../sidebar_themes.hpp"
#include "../../app_log.hpp"
#include "../../audio.hpp"
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

// ── Add grid ─────────────────────────────────────────────────────────────────
//
// The commands a mission is mostly made of, one button each. A command with a
// position arms a map click — its button reads LOC? until the map is clicked
// there — and one without is appended at once. MORE opens the rest of
// CMD_TABLE as the same kind of grid.
struct QuickCmd { uint16_t id; const char* label; };
static constexpr QuickCmd QUICK_CMDS[] = {
    {  16, "WAYPOINT"   }, {  22, "TAKEOFF"    }, {  21, "LAND"     },
    {  20, "RTL"        }, {  17, "LOITER"     }, {  19, "LOIT TIME"},
    {  18, "LOIT TURNS" }, { 201, "ROI"        }, { 178, "SPEED"    },
    {  93, "DELAY"      }, { 177, "JUMP"       },
};
static constexpr int QUICK_COUNT = (int)(sizeof(QUICK_CMDS) / sizeof(QUICK_CMDS[0]));
static constexpr int GRID_COLS   = 3;

static bool is_quick(uint16_t cmd)
{
    for (const QuickCmd& q : QUICK_CMDS)
        if (q.id == cmd) return true;
    return false;
}

// Altitude for a new item: the last one given, else a safe default.
static constexpr float DEFAULT_ALT_M = 20.0f;

// ── Editor state ─────────────────────────────────────────────────────────────

// Per vehicle, not per panel — see widgets/vehicle_ui_state.hpp.
//
// The editor holds a mission the operator is building. Switching the callsign
// chip to look at another aircraft and then pressing UPLOAD would otherwise
// send this aircraft's plan to that one; keying it also means the plan is still
// there when they switch back.
struct MissionTabState {
    std::vector<bool>        open;   // per-item expand state, one per waypoint
    std::vector<MissionItem> edit;
    uint32_t fetch_gen        = 0;      // mission_generation when last synced
    bool     has_edit         = false;
    bool     upload_done      = false;  // show result badge once
    bool     cleared          = false;  // user pressed CLEAR; don't auto-re-sync
    bool     fetching         = false;  // FETCH pressed; awaiting fresh reply

    // ── FENCE view ────────────────────────────────────────────────────────────
    // The uploaded geofence, edited as shapes and kept in step with the
    // vehicle's own fence list exactly as `edit` is with its mission.
    bool      fence_view     = false;
    FencePlan fence;
    uint32_t  fence_gen      = 0;   // fence_generation when last synced
    bool      fence_has_edit = false;
    bool      fence_cleared  = false;
    bool      fence_fetching = false;
    int       fence_open     = -1;  // the zone expanded in the list
};

static VehicleUiState<MissionTabState> s_state;
static MissionTabState& st() { return *s_state; }

// Sync the editor from vehicle state when a fresh download completes.
// Will NOT fire after the user has pressed CLEAR, or while there are pending
// edits, to avoid clobbering unsaved work — except right after FETCH, which
// asked for exactly that.
//
// "Fresh" is a change of mission_generation, which ticks once per completed
// download. Watching has_mission fall and rise again instead missed any
// download quicker than the snapshot cadence — a short mission over SITL —
// and left the editor showing the old list.
static void maybe_sync(const VehicleState* vs)
{
    if (!vs || !vs->has_mission) return;
    if (vs->mission_generation == st().fetch_gen) return;   // already up to date
    if (!st().fetching && (st().cleared || st().has_edit)) return;

    st().edit      = vs->mission;
    st().fetch_gen = vs->mission_generation;
    st().has_edit  = true;
    st().cleared   = false;
    st().fetching  = false;
    st().open.assign(st().edit.size(), false);
    st().upload_done = false;
}

// The fence editor's counterpart: a fresh fence download replaces the plan
// unless the operator has edits or cleared it, or asked with FETCH.
static void maybe_sync_fence(const VehicleState* vs)
{
    if (!vs || !vs->has_fence) return;
    if (vs->fence_generation == st().fence_gen) return;
    if (!st().fence_fetching && (st().fence_cleared || st().fence_has_edit)) return;

    st().fence          = fence_from_items(vs->fence);
    st().fence_gen      = vs->fence_generation;
    st().fence_has_edit = true;
    st().fence_cleared  = false;
    st().fence_fetching = false;
    st().fence_open     = -1;
}

// ── Helpers ───────────────────────────────────────────────────────────────────

// The editor's list as it goes on the wire. A stack that keeps home in seq 0
// gets a home item in front, or the operator's first item would land in that
// slot and be thrown away — reading back as home, with the plan one short.
// The vehicle overwrites it with its own home either way; the last one it
// reported is sent so the upload reads true.
static std::vector<MissionItem> wire_mission(const VehicleState& vs,
                                             const std::vector<MissionItem>& edit)
{
    std::vector<MissionItem> out;
    out.reserve(edit.size() + 1);
    if (firmware_profile(vs).mission_home_at_seq0()) {
        MissionItem home{};
        if (vs.has_mission_home) {
            home = vs.mission_home;
        } else {
            home.command = 16;  // MAV_CMD_NAV_WAYPOINT
            home.frame   = 0;   // MAV_FRAME_GLOBAL — home altitude is AMSL
            home.lat     = vs.home_lat;
            home.lon     = vs.home_lon;
            home.alt     = vs.home_alt_asl;
        }
        out.push_back(home);
    }
    out.insert(out.end(), edit.begin(), edit.end());
    for (size_t i = 0; i < out.size(); ++i) {
        out[i].seq        = (uint16_t)i;
        out[i].is_current = false;
    }
    // Current is the first item the vehicle will fly: the operator's first,
    // never the home slot.
    out[out.size() > edit.size() ? 1 : 0].is_current = true;
    return out;
}

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

// Append a new item. `lat`/`lon` are where the map was clicked, or 0/0 for a
// command that has no position.
static void append_item(uint16_t cmd, double lat, double lon)
{
    MissionItem ni{};
    ni.seq     = (uint16_t)st().edit.size();
    ni.command = cmd;
    ni.frame   = 3; // MAV_FRAME_GLOBAL_RELATIVE_ALT
    ni.lat     = lat;
    ni.lon     = lon;
    ni.alt     = DEFAULT_ALT_M;
    for (int k = (int)st().edit.size() - 1; k >= 0; --k) {
        const MissionItem& prev = st().edit[k];
        const CmdMeta* pm = find_meta(prev.command);
        if (!pm || pm->show_alt || pm->show_latlon) { ni.alt = prev.alt; break; }
    }
    // The new item opens and every other one closes: what was just added is
    // what is about to be filled in, and the list stays one item tall else.
    st().edit.push_back(ni);
    st().open.assign(st().edit.size() - 1, false);
    st().open.push_back(true);
    st().has_edit = true;      // protect manual edits from auto-sync
    st().cleared  = false;
}

// One button of the add grid. A positional command arms the map and reads
// LOC? on the active plate while it waits; pressing it again disarms.
// Returns true when it closed over a popup's worth of choice.
static bool add_button(MissionPickState* pick, uint16_t cmd, const char* label,
                       ImVec2 size)
{
    const CmdMeta* m      = find_meta(cmd);
    const bool     placed = m && m->show_latlon;
    const bool     armed  = pick && pick->add_command == cmd;

    char id[48];
    snprintf(id, sizeof(id), "%s###add%u", armed ? "LOC?" : label, (unsigned)cmd);
    if (!ui_grid_button(id, size, armed)) {
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
            ImGui::SetTooltip(placed ? "%s \xe2\x80\x94 then click the map to place it"
                                     : "%s \xe2\x80\x94 added to the end of the list",
                              m ? m->name : label);
        return false;
    }

    if (armed) {
        pick->add_command = 0;
    } else if (placed && pick) {
        pick->add_command  = cmd;
        pick->add_label    = m->name;
    } else {
        append_item(cmd, 0.0, 0.0);
        if (pick) pick->add_command = 0;
    }
    return true;
}

static void draw_add_grid(MissionPickState* pick)
{
    const float gap   = 4.0f;
    const float col_w = (ImGui::GetContentRegionAvail().x - gap * (GRID_COLS - 1)) / GRID_COLS;
    const ImVec2 sz   = { col_w, 26.0f };

    for (int i = 0; i < QUICK_COUNT; ++i) {
        if (i % GRID_COLS != 0) ImGui::SameLine(0, gap);
        add_button(pick, QUICK_CMDS[i].id, QUICK_CMDS[i].label, sz);
    }

    // MORE: everything else, lit while one of those waits for its LOC.
    if (QUICK_COUNT % GRID_COLS != 0) ImGui::SameLine(0, gap);
    const bool more_armed = pick && pick->add_command != 0 && !is_quick(pick->add_command);
    if (ui_grid_button(more_armed ? "LOC?###more" : "MORE...###more", sz, more_armed)) {
        if (more_armed) pick->add_command = 0;
        else            ImGui::OpenPopup("##more_cmds");
    }

    ui_push_dialog_style();
    if (ImGui::BeginPopup("##more_cmds")) {
        constexpr float POP_W = 300.0f;
        ui_dialog_title("ADD MISSION ITEM", POP_W);
        const float pw = (POP_W - gap) * 0.5f;
        int n = 0;
        for (int c = 0; c < CMD_COUNT; ++c) {
            if (is_quick(CMD_TABLE[c].id)) continue;
            if (n++ % 2 != 0) ImGui::SameLine(0, gap);
            if (add_button(pick, CMD_TABLE[c].id, CMD_TABLE[c].name, { pw, 24.0f }))
                ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    ui_pop_dialog_style();
}

// ── FENCE view ────────────────────────────────────────────────────────────────

// Ground distance in metres, near enough for a fence radius: the two points
// are a click apart on the map.
static float ground_dist_m(const FenceVertex& a, const FenceVertex& b)
{
    const double k  = 111320.0;
    const double dy = (b.lat - a.lat) * k;
    const double dx = (b.lon - a.lon) * k * std::cos(a.lat * 3.14159265358979 / 180.0);
    return (float)std::sqrt(dx * dx + dy * dy);
}

// A finished shape joins the plan, opened in the list; the drawing ends.
static void add_fence_shape(MissionPickState* pick, FenceShape shape)
{
    st().fence.shapes.push_back(std::move(shape));
    st().fence_open     = (int)st().fence.shapes.size() - 1;
    st().fence_has_edit = true;
    st().fence_cleared  = false;
    pick->cancel_fence_draw();
}

static void finish_fence_polygon(MissionPickState* pick)
{
    if (pick->fence_pts.size() < 3) return;
    FenceShape s;
    s.kind      = FenceShape::Kind::Polygon;
    s.inclusion = pick->fence_inclusion;
    s.pts       = pick->fence_pts;
    gcs_log("fence: added %s polygon, %d vertices",
            s.inclusion ? "inclusion" : "exclusion", (int)s.pts.size());
    add_fence_shape(pick, std::move(s));
}

// A map click while a fence shape is being drawn.
static void consume_fence_pick(MissionPickState* pick)
{
    const FenceVertex at = { pick->picked_lat, pick->picked_lon };
    if (pick->fence_draw == MissionPickState::FenceDraw::Polygon) {
        if (pick->fence_close) finish_fence_polygon(pick);
        else                   pick->fence_pts.push_back(at);
    } else if (pick->fence_pts.empty()) {
        pick->fence_pts.push_back(at);                    // the centre
    } else {
        FenceShape s;
        s.kind      = FenceShape::Kind::Circle;
        s.inclusion = pick->fence_inclusion;
        s.center    = pick->fence_pts[0];
        s.radius_m  = std::max(1.0f, std::round(ground_dist_m(s.center, at)));
        gcs_log("fence: added %s circle, radius %.0f m",
                s.inclusion ? "inclusion" : "exclusion", (double)s.radius_m);
        add_fence_shape(pick, std::move(s));
    }
    pick->fence_close = false;
}

// What the vehicle's parameter-set fence is: the cylinder the map rings home
// with. Read-only here; the PARAMS tab edits it.
static void draw_param_fence_summary(const VehicleState* vs)
{
    ImGui::TextDisabled("PARAMETER FENCE");
    const FenceParams* f = vs ? &vs->fence_params : nullptr;
    if (!f || (!f->has_radius && !f->has_enable)) {
        ImGui::TextDisabled("  not reported yet");
        return;
    }
    const bool on = !f->has_enable || f->enabled;
    if (f->circle_configured()) {
        if (f->ceiling_configured())
            ImGui::Text("  Cylinder  R %.0f m, ceiling %.0f m", (double)f->radius_m,
                        (double)f->alt_max_m);
        else
            ImGui::Text("  Circle  R %.0f m", (double)f->radius_m);
    } else if (f->ceiling_configured()) {
        ImGui::Text("  Ceiling %.0f m only", (double)f->alt_max_m);
    } else {
        ImGui::TextDisabled("  No radius or ceiling set");
    }
    ImGui::SameLine(0, 8);
    if (on) ImGui::TextColored(col_ok(), "ON");
    else    ImGui::TextColored(col_warning(), "OFF");
    if (f->has_action)
        ImGui::TextDisabled("  %s %d  \xe2\x80\x94  edit in PARAMS",
                            f->px4 ? "GF_ACTION" : "FENCE_ACTION", f->action);
}

static void draw_fence_view(MavlinkSender* sender, const VehicleState* vs,
                            MissionPickState* pick, bool connected,
                            uint8_t tsys, uint8_t tcomp)
{
    using FD = MissionPickState::FenceDraw;
    FencePlan& plan = st().fence;

    // ── Fetch / Clear ─────────────────────────────────────────────────────────
    const float half = (ImGui::GetContentRegionAvail().x - 4.0f) * 0.5f;
    ImGui::BeginDisabled(!connected);
    if (ui_grid_button("FETCH###fence_fetch", { half, 24.0f }) && connected) {
        sender->request_mission_list(tsys, tcomp, MAV_MISSION_TYPE_FENCE);
        st().fence_fetching = true;
        st().fence_gen      = vs->fence_generation;
        if (pick) pick->cancel_fence_draw();
        gcs_log("requesting fence list");
    }
    ImGui::EndDisabled();
    ImGui::SameLine(0, 4);
    ImGui::BeginDisabled(plan.shapes.empty() && !plan.has_return);
    if (ui_solid_button("CLEAR###fence_clear", { half, 24.0f },
                        btn_disconnect_base(), btn_disconnect_hov())) {
        // Kept as an edit, so UPLOAD can send the empty fence and clear the
        // vehicle's too; and held off from re-syncing until the next FETCH.
        plan                = FencePlan{};
        st().fence_has_edit = true;
        st().fence_cleared  = true;
        st().fence_open     = -1;
        if (pick) { pick->cancel_fence_draw(); pick->fence_edit = false; }
    }
    ImGui::EndDisabled();

    if (vs && vs->fence_count > 0 && vs->fence_received < vs->fence_count) {
        ImGui::Spacing();
        char prog[32];
        snprintf(prog, sizeof(prog), "%u / %u", vs->fence_received, vs->fence_count);
        const float frac = (float)vs->fence_received / vs->fence_count;
        ui_progress_bar(prog, frac, 10.0f);
        gcs_progress("fence", frac);
    }

    ImGui::Spacing();
    draw_param_fence_summary(vs);

    // Uploaded zones are only enforced on ArduPilot with FENCE_TYPE's polygon
    // bit, and nothing is with the fence disabled — said here, where they are
    // drawn, rather than left to be found out in the air.
    if (vs) {
        const FenceParams& f = vs->fence_params;
        if (!f.px4 && f.has_type && !(f.type & FenceParams::TYPE_POLYGON))
            ImGui::TextColored(col_warning(),
                               "FENCE_TYPE has no Polygon bit: uploaded zones are not enforced");
        if (f.has_enable && !f.enabled && !f.px4)
            ImGui::TextColored(col_warning(), "FENCE_ENABLE is 0: no fence is enforced");
        if (!vs->has_fence && connected)
            ImGui::TextDisabled("FETCH first to keep the vehicle's existing zones");
    }

    ImGui::Spacing();
    themed_sep();
    ImGui::Spacing();

    if (pick) {
        // ── EDIT ──────────────────────────────────────────────────────────────
        const bool editing = pick->fence_edit;
        ImGui::BeginDisabled(plan.shapes.empty() && !editing);
        if (ui_grid_button(editing ? "EDITING \xe2\x80\x94 DRAG FENCE POINTS ON THE MAP###fedit"
                                   : "EDIT###fedit",
                           { -1.0f, 32.0f }, editing)) {
            pick->fence_edit = !editing;
            if (pick->fence_edit) pick->cancel_fence_draw();
        }
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_AllowWhenDisabled))
            ImGui::SetTooltip("Drag a polygon vertex or a circle's centre on the map (Esc to stop)");
        ImGui::Spacing();

        // ── Shape ─────────────────────────────────────────────────────────────
        // Only the shape is chosen here. A new zone is an inclusion zone; the
        // list switches any zone to exclusion once it exists.
        pick->fence_inclusion = true;
        const float  gap = 4.0f;
        const float  w2  = (ImGui::GetContentRegionAvail().x - gap) * 0.5f;
        const ImVec2 sz  = { w2, 26.0f };

        const bool poly_armed = pick->fence_draw == FD::Polygon;
        const bool circ_armed = pick->fence_draw == FD::Circle;
        char poly_lbl[48];
        if (!poly_armed)                       snprintf(poly_lbl, sizeof(poly_lbl), "POLYGON###fpoly");
        else if (pick->fence_pts.size() >= 3)  snprintf(poly_lbl, sizeof(poly_lbl), "CLOSE (%d PTS)###fpoly",
                                                        (int)pick->fence_pts.size());
        else                                   snprintf(poly_lbl, sizeof(poly_lbl), "LOC? (%d PTS)###fpoly",
                                                        (int)pick->fence_pts.size());
        if (ui_grid_button(poly_lbl, sz, poly_armed)) {
            if (!poly_armed) {
                pick->cancel_fence_draw();
                pick->fence_draw  = FD::Polygon;
                pick->add_command = 0;
                pick->drag_edit   = false;
                pick->fence_edit  = false;
            } else if (pick->fence_pts.size() >= 3) {
                finish_fence_polygon(pick);
            } else {
                pick->cancel_fence_draw();
            }
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
            ImGui::SetTooltip(poly_armed
                ? "Click the map for each vertex; click the first vertex, press Enter or this button to close"
                : "Draw a polygon zone on the map");
        ImGui::SameLine(0, gap);
        const char* circ_lbl = !circ_armed ? "CIRCLE###fcirc"
                             : pick->fence_pts.empty() ? "CENTRE?###fcirc" : "RADIUS?###fcirc";
        if (ui_grid_button(circ_lbl, sz, circ_armed)) {
            if (circ_armed) {
                pick->cancel_fence_draw();
            } else {
                pick->cancel_fence_draw();
                pick->fence_draw  = FD::Circle;
                pick->add_command = 0;
                pick->drag_edit   = false;
                pick->fence_edit  = false;
            }
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
            ImGui::SetTooltip("Click the map for the centre, then again for the radius");
    }

    ImGui::Spacing();
    ImGui::TextDisabled("%d zones", (int)plan.shapes.size());
    ImGui::Spacing();

    // ── Zone list ─────────────────────────────────────────────────────────────
    const float list_h = std::max(80.0f, ImGui::GetContentRegionAvail().y - 64.0f);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, bg_child_dark());
    if (ImGui::BeginChild("##fence_list", { -1.0f, list_h }, false)) {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 4.0f, 2.0f });
        if (plan.shapes.empty()) {
            const char*  msg = "NO FENCE ZONES";
            const ImVec2 ts  = ImGui::CalcTextSize(msg);
            const ImVec2 av  = ImGui::GetContentRegionAvail();
            ImGui::SetCursorPos({ (av.x - ts.x) * 0.5f, (av.y - ts.y) * 0.5f });
            ImGui::TextDisabled("%s", msg);
        }

        int to_delete = -1;
        for (int i = 0; i < (int)plan.shapes.size(); ++i) {
            FenceShape& z = plan.shapes[i];
            ImGui::PushID(i);

            const float  del_w  = 18.0f;
            const ImVec2 del_sz = { del_w, ImGui::GetFontSize() + 2.0f };
            const float  row_w  = ImGui::GetContentRegionAvail().x - del_w - 4.0f;

            char hdr[64];
            if (z.kind == FenceShape::Kind::Polygon)
                snprintf(hdr, sizeof(hdr), "F%d  POLYGON  %s  %d PTS", i + 1,
                         z.inclusion ? "IN" : "EX", (int)z.pts.size());
            else
                snprintf(hdr, sizeof(hdr), "F%d  CIRCLE  %s  %.0f M", i + 1,
                         z.inclusion ? "IN" : "EX", (double)z.radius_m);

            const bool open = st().fence_open == i;
            ImGui::PushStyleColor(ImGuiCol_Text, z.inclusion ? col_ok() : col_error());
            if (ImGui::Selectable(hdr, open, ImGuiSelectableFlags_AllowOverlap,
                                  { row_w, del_sz.y }))
                st().fence_open = open ? -1 : i;
            ImGui::PopStyleColor();
            ImGui::SameLine(0, 4);
            if (ui_solid_button("x", del_sz, btn_disconnect_base(), btn_disconnect_hov()))
                to_delete = i;

            if (open) {
                ImGui::Indent(12.0f);
                const float w2 = (ImGui::GetContentRegionAvail().x - 4.0f) * 0.5f;
                if (ui_tab_button("INCLUSION###zin", { w2, 22.0f }, z.inclusion))
                    z.inclusion = true;
                ImGui::SameLine(0, 4);
                if (ui_tab_button("EXCLUSION###zex", { w2, 22.0f }, !z.inclusion))
                    z.inclusion = false;

                if (z.kind == FenceShape::Kind::Circle) {
                    labeled_float("Radius", &z.radius_m, "m");
                    z.radius_m = std::max(1.0f, z.radius_m);
                    ImGui::TextDisabled("Lat"); ImGui::SameLine(60.0f);
                    ImGui::SetNextItemWidth(-1.0f);
                    ImGui::InputDouble("##clat", &z.center.lat, 0.0, 0.0, "%.7f");
                    ImGui::TextDisabled("Lon"); ImGui::SameLine(60.0f);
                    ImGui::SetNextItemWidth(-1.0f);
                    ImGui::InputDouble("##clon", &z.center.lon, 0.0, 0.0, "%.7f");
                } else {
                    // Each vertex as numbers, for a fence copied off a plan;
                    // EDIT drags them on the map. A polygon keeps three.
                    int drop = -1;
                    for (int v = 0; v < (int)z.pts.size(); ++v) {
                        ImGui::PushID(v);
                        ImGui::TextDisabled("%d", v + 1);
                        ImGui::SameLine(24.0f);
                        const float fw = (ImGui::GetContentRegionAvail().x - del_w - 8.0f) * 0.5f;
                        ImGui::SetNextItemWidth(fw);
                        ImGui::InputDouble("##vlat", &z.pts[v].lat, 0.0, 0.0, "%.6f");
                        ImGui::SameLine(0, 4);
                        ImGui::SetNextItemWidth(fw);
                        ImGui::InputDouble("##vlon", &z.pts[v].lon, 0.0, 0.0, "%.6f");
                        ImGui::SameLine(0, 4);
                        ImGui::BeginDisabled(z.pts.size() <= 3);
                        if (ui_solid_button("x", del_sz, btn_disconnect_base(), btn_disconnect_hov()))
                            drop = v;
                        ImGui::EndDisabled();
                        ImGui::PopID();
                    }
                    if (drop >= 0) z.pts.erase(z.pts.begin() + drop);
                }
                ImGui::Unindent(12.0f);
            }
            ImGui::Separator();
            ImGui::PopID();
        }
        if (to_delete >= 0) {
            plan.shapes.erase(plan.shapes.begin() + to_delete);
            st().fence_open     = -1;
            st().fence_has_edit = true;
        }
        ImGui::PopStyleVar();
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();

    // ── Upload ────────────────────────────────────────────────────────────────
    // An empty plan uploads too, on purpose: it is how the vehicle's fence is
    // cleared, and the button says so.
    ImGui::Spacing();
    const bool empty = plan.shapes.empty();
    ImGui::BeginDisabled(!connected || !st().fence_has_edit);
    if (ui_grid_button(empty ? "UPLOAD EMPTY FENCE (CLEARS VEHICLE'S)###fup" : "UPLOAD FENCE###fup",
                       { -1.0f, 26.0f }) && connected) {
        const std::vector<MissionItem> items = fence_to_items(plan);
        sender->start_upload(tsys, tcomp, items, MAV_MISSION_TYPE_FENCE);
        gcs_log("uploading fence: %d zones, %d items", (int)plan.shapes.size(), (int)items.size());
    }
    ImGui::EndDisabled();

    if (vs) {
        using US = VehicleState::UploadStatus;
        if (vs->fence_upload_status == US::InProgress) {
            ImGui::TextColored(col_warning(), "uploading...");
        } else if (vs->fence_upload_status == US::Accepted) {
            ImGui::TextColored(col_ok(), "Fence upload accepted");
        } else if (vs->fence_upload_status == US::Failed) {
            ImGui::TextColored(col_error(), "Fence upload FAILED (err %u)",
                               (unsigned)vs->fence_upload_ack_result);
        }
    }
}

// ── Main draw function ────────────────────────────────────────────────────────

void draw_tab_mission(MavlinkSender* sender, const VehicleState* vs,
                      MissionPickState* pick)
{
    const bool    connected = (vs && vs->has_heartbeat);
    const uint8_t tsys      = connected ? vs->sysid  : 1;
    const uint8_t tcomp     = connected ? vs->compid : 1;

    maybe_sync(vs);
    maybe_sync_fence(vs);

    // ── Publish live edit pointers so map_view can draw current edits ─────────
    if (pick) {
        pick->edit_mission = st().has_edit ? &st().edit : nullptr;
        pick->edit_fence   = st().fence_has_edit ? &st().fence : nullptr;
    }

    // ── Consume a completed map pick ──────────────────────────────────────────
    if (pick && pick->pick_done && pick->fence_drawing()) {
        consume_fence_pick(pick);
        pick->pick_done = false;
    } else if (pick && pick->pick_done) {
        if (pick->add_command != 0) {
            append_item(pick->add_command, pick->picked_lat, pick->picked_lon);
            gcs_log("mission: added %s at %.6f, %.6f",
                    cmd_short(pick->add_command), pick->picked_lat, pick->picked_lon);
        }
        pick->pick_done   = false;
        pick->add_command = 0;
    }

    // ── Escape: cancel a pending pick, else leave EDIT ────────────────────────
    if (pick && ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        if (pick->picking()) {
            pick->add_command = 0;
            pick->cancel_fence_draw();
        } else {
            pick->drag_edit  = false;
            pick->fence_edit = false;
        }
    }
    // Enter closes a polygon being drawn, once it has three vertices.
    if (pick && pick->fence_draw == MissionPickState::FenceDraw::Polygon &&
        (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)))
        finish_fence_polygon(pick);

    ImGui::Spacing();
    ImGui::TextColored(accent_col(), "MISSION");
    if (!connected) {
        ImGui::SameLine(0, 6);
        ImGui::TextColored(col_no_link_muted(), "(no link)");
    }
    themed_sep();
    ImGui::Spacing();

    // ── MISSION / FENCE ───────────────────────────────────────────────────────
    // Two lists the vehicle keeps apart, edited apart. Switching drops whatever
    // the map was armed for in the other.
    {
        const float w2 = (ImGui::GetContentRegionAvail().x - 4.0f) * 0.5f;
        bool to_fence = st().fence_view;
        if (ui_tab_button("PLAN###v_plan", { w2, 26.0f }, !st().fence_view)) to_fence = false;
        ImGui::SameLine(0, 4);
        if (ui_tab_button("GEOFENCE###v_fence", { w2, 26.0f }, st().fence_view)) to_fence = true;
        if (to_fence != st().fence_view) {
            st().fence_view = to_fence;
            if (pick) {
                pick->add_command = 0;  pick->pick_done  = false;
                pick->drag_edit   = false; pick->fence_edit = false;
                pick->cancel_fence_draw();
            }
        }
        ImGui::Spacing();
    }
    if (st().fence_view) {
        draw_fence_view(sender, vs, pick, connected, tsys, tcomp);
        return;
    }

    // ── Fetch / Clear row ─────────────────────────────────────────────────────
    const float btn_w = (ImGui::GetContentRegionAvail().x - 4.0f) * 0.5f;

    ImGui::BeginDisabled(!connected);
    if (ui_grid_button("FETCH", { btn_w, 24.0f }) && connected) {
        sender->request_mission_list(tsys, tcomp);
        // Only a download newer than this press may replace the editor — not
        // one that finished earlier and was held off by unsaved edits.
        st().fetching          = true;
        st().fetch_gen         = vs->mission_generation;
        if (pick) { pick->add_command = 0; pick->pick_done = false; }
        gcs_log("requesting mission list");
    }
    ImGui::EndDisabled();

    ImGui::SameLine(0, 4);

    ImGui::BeginDisabled(!st().has_edit);
    if (ui_solid_button("CLEAR", { btn_w, 24.0f },
                        btn_disconnect_base(), btn_disconnect_hov()) && st().has_edit) {
        st().edit.clear();
        st().open.clear();
        st().has_edit    = false;
        st().cleared     = true;   // block auto-re-sync until user presses FETCH
        st().upload_done = false;
        if (pick) {
            pick->add_command = 0; pick->pick_done = false;
            pick->drag_edit = false; pick->edit_mission = nullptr;
        }
    }
    ImGui::EndDisabled();

    // ── Download progress ─────────────────────────────────────────────────────
    if (vs && vs->mission_count > 0 && vs->mission_received < vs->mission_count) {
        ImGui::Spacing();
        char prog_lbl[32];
        snprintf(prog_lbl, sizeof(prog_lbl), "%u / %u", vs->mission_received, vs->mission_count);
        const float frac = (float)vs->mission_received / vs->mission_count;
        ui_progress_bar(prog_lbl, frac, 10.0f);
        // Its own channel, so a mission download during a parameter fetch keeps
        // its own tempo instead of the two of them beating against each other.
        gcs_progress("mission", frac);
    }

    ImGui::Spacing();
    themed_sep();
    ImGui::Spacing();

    // ── EDIT + add grid ───────────────────────────────────────────────────────
    // EDIT hands the map's left button to the waypoints: press one and drag it.
    // Off, the map pans as always. Lit on the active plate while on.
    if (pick) {
        const bool editing = pick->drag_edit;
        ImGui::BeginDisabled(st().edit.empty() && !editing);
        if (ui_grid_button(editing ? "EDITING \xe2\x80\x94 DRAG WAYPOINTS ON THE MAP###edit"
                                   : "EDIT###edit",
                           { -1.0f, 32.0f }, editing)) {
            pick->drag_edit = !editing;
            if (pick->drag_edit) pick->add_command = 0;
        }
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_AllowWhenDisabled))
            ImGui::SetTooltip("Drag any waypoint on the map to move it (Esc to stop)");
        ImGui::Spacing();
    }
    draw_add_grid(pick);

    ImGui::Spacing();
    ImGui::TextDisabled("%d items", (int)st().edit.size());
    ImGui::Spacing();

    // ── Waypoint list ─────────────────────────────────────────────────────────
    // Reserve: spacing(4) + upload-btn(26) + spacing(4) +
    //          status-badge-line(~22) + bottom-padding(8) ≈ 64px
    const float list_h = std::max(80.0f,
                                  ImGui::GetContentRegionAvail().y - 64.0f);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, bg_child_dark());
    if (ImGui::BeginChild("##wp_list", { -1.0f, list_h }, false)) {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 4.0f, 2.0f });

        // Ensure open vector is sized
        if ((int)st().open.size() != (int)st().edit.size())
            st().open.resize(st().edit.size(), false);

        int to_delete = -1;

        if (st().edit.empty()) {
            const char*  msg = "MISSION EMPTY";
            const ImVec2 ts  = ImGui::CalcTextSize(msg);
            const ImVec2 av  = ImGui::GetContentRegionAvail();
            ImGui::SetCursorPos({ (av.x - ts.x) * 0.5f, (av.y - ts.y) * 0.5f });
            ImGui::TextDisabled("%s", msg);
        }

        for (int i = 0; i < (int)st().edit.size(); ++i) {
            MissionItem& item = st().edit[i];
            item.seq = (uint16_t)i;  // keep seq in sync with list position

            ImGui::PushID(i);

            // ── Row header (seq, cmd, alt) ────────────────────────────────────
            // The whole row, short of the delete button, toggles the item open
            // and closed.
            const float  del_w  = 18.0f;
            const ImVec2 del_sz = { del_w, ImGui::GetFontSize() + 2.0f };
            const float  row_w  = ImGui::GetContentRegionAvail().x - del_w - 4.0f;

            char hdr[64];
            const CmdMeta* m = find_meta(item.command);
            if (m && m->show_alt)
                snprintf(hdr, sizeof(hdr), "#%u  %s  %.0fm", i, m->name, (double)item.alt);
            else
                snprintf(hdr, sizeof(hdr), "#%u  %s", i, m ? m->name : cmd_short(item.command));

            if (item.is_current)
                ImGui::PushStyleColor(ImGuiCol_Text, accent_col());
            if (ImGui::Selectable(hdr, st().open[i], ImGuiSelectableFlags_AllowOverlap,
                                  { row_w, del_sz.y }))
                st().open[i] = !st().open[i];
            if (item.is_current)
                ImGui::PopStyleColor();

            ImGui::SameLine(0, 4);
            if (ui_solid_button("x", del_sz,
                                btn_disconnect_base(), btn_disconnect_hov()))
                to_delete = i;

            // ── Expanded fields ───────────────────────────────────────────────
            if (st().open[i]) {
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

        if (to_delete >= 0 && to_delete < (int)st().edit.size()) {
            st().edit.erase(st().edit.begin() + to_delete);
            st().open.erase(st().open.begin() + to_delete);
        }

        ImGui::PopStyleVar();
    }
    ImGui::EndChild();
    ImGui::PopStyleColor(); // ChildBg

    // ── Upload ────────────────────────────────────────────────────────────────
    ImGui::Spacing();
    ImGui::BeginDisabled(!connected || st().edit.empty());
    if (ui_grid_button("UPLOAD MISSION", { -1.0f, 26.0f }) && connected && !st().edit.empty()) {
        sender->start_upload(tsys, tcomp, wire_mission(*vs, st().edit));
        st().upload_done = false;
        gcs_log("uploading %d waypoints", (int)st().edit.size());
    }
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
