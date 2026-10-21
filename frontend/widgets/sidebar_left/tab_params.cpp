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
#include "../vehicle_ui_state.hpp"
#include "../sidebar_themes.hpp"
#include "../param_meta.hpp"
#include "../../app_log.hpp"
#include "../../audio.hpp"
#include "../../param_file.hpp"
#include "../../../backend/rc_binding.hpp"
#include "imgui.h"
#include "ImGuiFileDialog.h"
#include <algorithm>
#include <cctype>
#include <cstdarg>
#include <cstring>
#include <cstdio>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// ── Parameter files ───────────────────────────────────────────────────────────

// Keys ImGuiFileDialog tracks each dialog under. One per direction, so each
// carries its own title and its own flags; only one can be open at a time
// regardless, since IGFD's singleton holds a single dialog's state.
//
// Deliberately without the "##" prefix every other id in this file carries.
// These are not ImGui labels, they are IGFD's keys, and IGFD names its window
// `title + "##" + key` (ImGuiFileDialog.cpp). A "##" here would make that
// four consecutive '#', which ImHashStr and ImHashSkipUncontributingPrefix
// disagree about: the former consumes one "###" and hashes the leftover '#'
// as data, the latter re-matches and skips it. The window would then hash to
// one id and its .ini settings entry to another, and imgui asserts
// (settings->ID == window->ID) the next time it writes imgui.ini.
static constexpr const char* PARAM_LOAD_DLG_KEY = "param_load_dlg";
static constexpr const char* PARAM_SAVE_DLG_KEY = "param_save_dlg";

// Opens one of the two browsers, seeded with the last file touched so a save
// after a load starts in the same directory.
//
// ConfirmOverwrite only on the save side: it is the only one of the two that
// writes, and warning about an overwrite while picking a file to *read* would
// be asking about something that is not going to happen.
static void open_param_file_dialog(const char* key, const char* title,
                                   const std::string& current_path, bool for_save)
{
    IGFD::FileDialogConfig cfg;
    if (!current_path.empty())
        cfg.filePathName = current_path;   // IGFD splits this into path + name
    else
        cfg.fileName = "parameters.params";

    cfg.flags = ImGuiFileDialogFlags_Modal;
    if (for_save)
        cfg.flags |= ImGuiFileDialogFlags_ConfirmOverwrite;

    ImGuiFileDialog::Instance()->OpenDialog(
        key, title, "Parameter files (*.params){.params},All files{.*}", cfg);
}

// Same border-and-background treatment ui_push_dialog_style() gives the app's
// own popups, without its padding/spacing — those are tuned for a centered
// title and a button row, and would just leave the file browser's table
// looking sparse. Colors alone are enough for the dialog to read as this
// app's rather than a bolted-on default-ImGui one; apply_global_theme()
// already zeroed every corner radius globally, so the squared-off look is
// free.
static void push_file_dialog_style()
{
    ImGui::PushStyleColor(ImGuiCol_PopupBg, g_theme.bg_panel);
    ImGui::PushStyleColor(ImGuiCol_Border,  ui_col(g_theme.accent, 0.55f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
}

static void pop_file_dialog_style()
{
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);
}

// What the file's comment header claims about the vehicle it came off. Only
// what the link has actually told us — a header is documentation, and a guessed
// one is worse than a missing one.
static ParamFileHeader file_header(const VehicleState* vs)
{
    ParamFileHeader h;
    if (!vs) return h;

    h.sysid = vs->sysid ? vs->sysid : 1;

    if (vs->autopilot == MAV_AUTOPILOT_ARDUPILOTMEGA) h.stack = "ArduPilot";
    else if (vs->autopilot == MAV_AUTOPILOT_PX4)      h.stack = "PX4";

    switch (vs->type) {
    case MAV_TYPE_QUADROTOR:      h.vehicle = "Quadrotor";   break;
    case MAV_TYPE_HEXAROTOR:      h.vehicle = "Hexarotor";   break;
    case MAV_TYPE_OCTOROTOR:      h.vehicle = "Octorotor";   break;
    case MAV_TYPE_TRICOPTER:      h.vehicle = "Tricopter";   break;
    case MAV_TYPE_HELICOPTER:     h.vehicle = "Helicopter";  break;
    case MAV_TYPE_FIXED_WING:     h.vehicle = "Fixed Wing";  break;
    case MAV_TYPE_GROUND_ROVER:   h.vehicle = "Ground Rover";break;
    case MAV_TYPE_SURFACE_BOAT:   h.vehicle = "Boat";        break;
    case MAV_TYPE_SUBMARINE:      h.vehicle = "Submarine";   break;
    case MAV_TYPE_ANTENNA_TRACKER:h.vehicle = "Antenna Tracker"; break;
    default: break;
    }

    if (vs->has_fw_info) {
        h.version  = vs->fw_version;
        h.git_hash = vs->fw_hash;
    }
    return h;
}

// One line of a loaded file, waiting to be accepted or dropped. Only parameters
// the vehicle actually has and whose value differs get this far.
struct LoadRow {
    std::string id;
    float       file_val;
    float       vehicle_val;
    bool        selected;
};

// The pending review. Lives across frames because the popup does — cleared when
// the operator applies it or walks away.
// Per vehicle, not per panel — see widgets/vehicle_ui_state.hpp.
//
// Staged edits are the sharp one: a value typed against one aircraft and then
// written after switching the callsign chip would go to a different aircraft.
// The sorted name list matters for a quieter reason — it is rebuilt only when
// the parameter count changes, and two vehicles running the same firmware have
// the same count, so a shared list would keep showing the first vehicle's
// parameter names against the second's values.
struct ParamsTabState {
    std::vector<LoadRow> load_rows;
    int  load_matched = 0;   // in the file, already equal on the vehicle
    int  load_unknown = 0;   // in the file, not on this vehicle
    int  load_skipped = 0;   // unparseable lines

    std::vector<std::string> sorted_ids;
    size_t                   last_param_count = 0;

    std::unordered_map<std::string, float> edited_vals;

    // Enums and bitmasks switched to a typed number from their info popup:
    // for a value the reference does not list, or one known by heart.
    std::unordered_set<std::string> manual;
};

static VehicleUiState<ParamsTabState> s_state;
static ParamsTabState& st() { return *s_state; }

// ── Bitmasks ──────────────────────────────────────────────────────────────────
//
// PARAM_VALUE carries every parameter as a float holding the number, whatever
// the vehicle stores it as. A bitmask in an int8 with bit 7 set therefore
// arrives as -128, and read straight into 32 bits it would claim bits 7–31.
// Masked to the storage width on the way in and sign-folded back on the way
// out, so the checkboxes show exactly the bits the vehicle holds.

static int bitmask_width(uint8_t mav_type)
{
    switch (mav_type) {
    case MAV_PARAM_TYPE_UINT8:  case MAV_PARAM_TYPE_INT8:  return 8;
    case MAV_PARAM_TYPE_UINT16: case MAV_PARAM_TYPE_INT16: return 16;
    default:                                               return 32;
    }
}

static uint32_t bitmask_from_value(float v, uint8_t mav_type)
{
    const uint32_t u = (uint32_t)(int64_t)v;
    const int      w = bitmask_width(mav_type);
    return w < 32 ? u & ((1u << w) - 1u) : u;
}

static float bitmask_to_value(uint32_t bits, uint8_t mav_type)
{
    const int  w      = bitmask_width(mav_type);
    const bool is_sig = mav_type == MAV_PARAM_TYPE_INT8  ||
                        mav_type == MAV_PARAM_TYPE_INT16 ||
                        mav_type == MAV_PARAM_TYPE_INT32;
    if (is_sig && ((bits >> (w - 1)) & 1u))
        return (float)((int64_t)bits - ((int64_t)1 << w));
    return (float)bits;
}

// "0x00FF" — as many hex digits as the storage holds, so a byte-wide mask does
// not read as eight digits of mostly zeros.
static void bitmask_hex(char* out, size_t n, uint32_t bits, uint8_t mav_type)
{
    snprintf(out, n, "0x%0*X", bitmask_width(mav_type) / 4, bits);
}

// ── Parameter info ────────────────────────────────────────────────────────────

constexpr float INFO_W = 380.0f;

static void info_row(const char* key, const char* fmt, ...) IM_FMTARGS(2);
static void info_row(const char* key, const char* fmt, ...)
{
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextDisabled("%s", key);
    ImGui::TableNextColumn();
    va_list args;
    va_start(args, fmt);
    ImGui::TextWrappedV(fmt, args);
    va_end(args);
}

// True when a plain number sits outside the range its reference documents.
// A warning, never a block: the range is the documentation's, not the
// firmware's, and a developer may mean to go past it. Enums and bitmasks have
// their own editors and no range to speak of.
static bool param_out_of_range(const ParamMeta* meta, ParamKind kind, float v)
{
    return meta && (kind == ParamKind::Int || kind == ParamKind::Float) &&
           meta->has(PARAM_FLAG_HAS_RANGE) &&
           (v < meta->min_val || v > meta->max_val);
}

// "0 – 1", or "≥ 0" / "≤ 20" where the page leaves one end open.
static void format_range(const ParamMeta* meta, char* out, size_t n)
{
    const bool lo = meta->min_val > -PARAM_RANGE_OPEN;
    const bool hi = meta->max_val <  PARAM_RANGE_OPEN;
    if (lo && hi)
        snprintf(out, n, "%.7g \xe2\x80\x93 %.7g", (double)meta->min_val, (double)meta->max_val);
    else if (lo)
        snprintf(out, n, "\xe2\x89\xa5 %.7g", (double)meta->min_val);
    else
        snprintf(out, n, "\xe2\x89\xa4 %.7g", (double)meta->max_val);
}

// Which firmware's documentation fits a vehicle, or nullptr for none.
//
// ArduPilot documents each vehicle separately, Copter and Plane covered here:
// their tables differ in exactly the vehicle-specific entries an operator is
// likeliest to open, so neither stands in for the other, and Rover and Sub
// get none rather than the wrong one. A QuadPlane reports a fixed-wing or VTOL
// type and reads Plane's, which documents its Q_ parameters. PX4 publishes
// one table for every airframe. A parameter one stack shares by name with the
// other is a different parameter, so neither stands in for the other.
static const ParamDocSource* param_docs_for(const VehicleState* vs)
{
    if (!vs) return nullptr;
    if (vs->autopilot == MAV_AUTOPILOT_PX4) return &param_docs_px4;
    if (vs->autopilot != MAV_AUTOPILOT_ARDUPILOTMEGA) return nullptr;
    switch (rc_firmware_for((MAV_TYPE)vs->type)) {
    case RcFirmware::Copter: return &param_docs_arducopter;
    case RcFirmware::Plane:  return &param_docs_arduplane;
    default:                 return nullptr;
    }
}

// The body of the "i" popup: everything the reference says about one
// parameter, then the vehicle's side of it — how it stores the value and what
// it holds now. `shown` is the row's value, staged edit included.
//
// `docs` is the documentation that fits the vehicle, nullptr for none;
// `fw_doc` is the release the vehicle runs (-1 unknown) and `doc` the one
// `meta`'s text was published for.
// `manual` is the row's manual-entry switch, offered with the flags for an
// enum or a bitmask; null leaves it out.
static void draw_param_info(const ParamEntry& p, const ParamMeta* meta,
                            ParamKind kind, float shown, const ParamDocSource* docs,
                            int fw_doc, int doc, bool* manual)
{
    ui_dialog_title(p.param_id, INFO_W);
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + INFO_W);

    if (!meta) {
        if (docs) {
            char first[16], last[16];
            param_doc_version_label(*docs, 0, first, sizeof(first));
            param_doc_version_label(*docs, (int)docs->version_count - 1, last, sizeof(last));
            ImGui::TextDisabled("Not in any %s parameter reference (%s to %s).",
                                docs->name, first, last);
        } else {
            ImGui::TextDisabled("No parameter reference for this vehicle " "\xe2\x80\x94 "
                                "only ArduPilot Copter, ArduPilot Plane and PX4 are covered.");
        }
        ImGui::Spacing();
    } else {
        char doc_lbl[16];
        param_doc_version_label(*docs, doc, doc_lbl, sizeof(doc_lbl));

        ImGui::TextColored(accent_col(), "%s", meta->display_name);
        if (meta->variant)
            ImGui::TextDisabled("as documented for %s", meta->variant);

        // Which release's page this is. Said loudly only when it is not the
        // vehicle's own: a parameter its release's page left out, or one
        // documented only before or after it.
        if (fw_doc >= 0 && doc != fw_doc) {
            char fw_lbl[16];
            param_doc_version_label(*docs, fw_doc, fw_lbl, sizeof(fw_lbl));
            ImGui::TextColored(col_reboot(), "Not in the %s reference " "\xe2\x80\x94 "
                               "shown as documented for %s", fw_lbl, doc_lbl);
        } else {
            ImGui::TextDisabled("%s %s reference%s", docs->name, doc_lbl,
                                fw_doc < 0 ? " (firmware version not known yet)" : "");
        }
        ImGui::Spacing();
        if (meta->description[0])
            ImGui::TextUnformatted(meta->description);
        ImGui::Spacing();

        // Flags, each on its own line — the reboot one in the colour the rest
        // of the app gives reboots.
        if (meta->has(PARAM_FLAG_REBOOT))
            ImGui::TextColored(col_reboot(), "! Reboot required after change");
        if (meta->has(PARAM_FLAG_READONLY))
            ImGui::TextColored(col_reboot(), "! Read-only");
        if (meta->has(PARAM_FLAG_ADVANCED))
            ImGui::TextDisabled("For advanced users");

        // Manual entry: the list and the checkboxes only offer what the
        // reference documents, so an enum or a bitmask can be switched to a
        // plain number field on its row — firmware newer than the reference,
        // a value read off a log, a mask known by heart.
        if (manual && (kind == ParamKind::Enum || kind == ParamKind::Bitmask)) {
            if (ui_grid_button(*manual ? "MANUAL ENTRY: ON###manual" : "MANUAL ENTRY###manual",
                               { INFO_W, 0.0f }, *manual))
                *manual = !*manual;
            const char* editor = kind == ParamKind::Enum ? "list" : "checkboxes";
            if (*manual)
                ImGui::TextDisabled("This row takes a typed number. Press again for the %s.",
                                    editor);
            else
                ImGui::TextDisabled("Type the value as a number instead of using the %s.",
                                    editor);
        }
        if (meta->has(PARAM_FLAG_CALIBRATION))
            ImGui::TextDisabled("Set by a calibration routine");
        if (meta->has(PARAM_FLAG_VOLATILE))
            ImGui::TextDisabled("Changed by the firmware at runtime");
        ImGui::Spacing();
    }

    if (ImGui::BeginTable("##info_kv", 2, ImGuiTableFlags_SizingFixedFit,
                          { INFO_W, 0.0f })) {
        ImGui::TableSetupColumn("k", ImGuiTableColumnFlags_WidthFixed, 90.0f);
        ImGui::TableSetupColumn("v", ImGuiTableColumnFlags_WidthStretch);

        info_row("Type", "%s  (vehicle stores %s)",
                 param_kind_name(kind), param_mav_type_name(p.type));
        if (meta && meta->units)
            info_row("Units", "%s", meta->units);
        if (meta && meta->has(PARAM_FLAG_HAS_RANGE)) {
            char rng[64];
            format_range(meta, rng, sizeof(rng));
            info_row("Range", "%s", rng);
        }
        if (meta && meta->has(PARAM_FLAG_HAS_INCREMENT))
            info_row("Increment", "%.7g", (double)meta->increment);
        if (meta && meta->has(PARAM_FLAG_HAS_DEFAULT)) {
            const float d = meta->default_val;
            const ParamEnumEntry* named = nullptr;
            for (int i = 0; i < meta->enum_count; ++i)
                if (meta->enum_entries[i].value == d) named = &meta->enum_entries[i];
            if (kind == ParamKind::Bitmask) {
                char hex[16];
                bitmask_hex(hex, sizeof(hex), bitmask_from_value(d, p.type), p.type);
                info_row("Default", "%s  (%.0f)", hex, (double)d);
            } else if (named) {
                info_row("Default", "%.7g  %s", (double)d, named->label);
            } else {
                info_row("Default", "%.7g", (double)d);
            }
        }
        if (meta && meta->group)
            info_row("Group", "%s", meta->group);

        if (kind == ParamKind::Bitmask) {
            char hex[16];
            bitmask_hex(hex, sizeof(hex), bitmask_from_value(p.value, p.type), p.type);
            info_row("Vehicle", "%s  (%.0f)", hex, (double)p.value);
            if (shown != p.value) {
                bitmask_hex(hex, sizeof(hex), bitmask_from_value(shown, p.type), p.type);
                info_row("Staged", "%s  (%.0f)", hex, (double)shown);
            }
        } else {
            info_row("Vehicle", "%.7g%s", (double)p.value,
                     param_out_of_range(meta, kind, p.value) ? "  (out of range)" : "");
            if (shown != p.value)
                info_row("Staged", "%.7g%s", (double)shown,
                         param_out_of_range(meta, kind, shown) ? "  (out of range)" : "");
        }
        ImGui::EndTable();
    }

    // Accepted values / bits, the current ones lit. Scrolls past a dozen or so
    // rows — SERIALn_PROTOCOL alone has close to fifty. A plain number's listed
    // values are landmarks in its range (ATC_INPUT_TC's "Soft", "Medium", ...),
    // shown for reference only — the field takes anything.
    const bool numeric = kind == ParamKind::Int || kind == ParamKind::Float;
    const bool as_enum = kind == ParamKind::Enum || numeric;
    const int n = !meta ? 0
                : as_enum                    ? meta->enum_count
                : kind == ParamKind::Bitmask ? meta->bitmask_count : 0;
    if (n > 0) {
        ImGui::Spacing();
        themed_sep();
        ImGui::TextDisabled(kind == ParamKind::Enum    ? "VALUES"
                          : kind == ParamKind::Bitmask ? "BITS"
                          : "SUGGESTED VALUES");

        const float row_h  = ImGui::GetTextLineHeightWithSpacing();
        const float list_h = std::min(n, 14) * row_h + 4.0f;
        const uint32_t bits = bitmask_from_value(shown, p.type);

        if (ImGui::BeginTable("##info_vals", 2,
                              ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollY,
                              { INFO_W, list_h })) {
            ImGui::TableSetupColumn("n", ImGuiTableColumnFlags_WidthFixed, 60.0f);
            ImGui::TableSetupColumn("m", ImGuiTableColumnFlags_WidthStretch);
            for (int i = 0; i < n; ++i) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                bool lit;
                if (as_enum) {
                    const ParamEnumEntry& e = meta->enum_entries[i];
                    lit = (e.value == shown);
                    if (lit) ImGui::TextColored(accent_col(), "%.7g", (double)e.value);
                    else     ImGui::TextDisabled("%.7g", (double)e.value);
                    ImGui::TableNextColumn();
                    if (lit) ImGui::TextColored(accent_col(), "%s", e.label);
                    else     ImGui::TextWrapped("%s", e.label);
                } else {
                    const ParamBitmaskBit& b = meta->bitmask_bits[i];
                    lit = (bits >> b.bit) & 1u;
                    if (lit) ImGui::TextColored(accent_col(), "%u", (unsigned)b.bit);
                    else     ImGui::TextDisabled("%u", (unsigned)b.bit);
                    ImGui::TableNextColumn();
                    if (lit) ImGui::TextColored(accent_col(), "%s", b.label);
                    else     ImGui::TextWrapped("%s", b.label);
                }
            }
            ImGui::EndTable();
        }
    }

    // Older releases list common whole-mask settings beside a bitmask's bits
    // (4.0's LOG_BITMASK "Default", "NearlyAll", ...). Read-only here — the
    // hex field in the bitmask editor takes any of them.
    if (meta && kind == ParamKind::Bitmask && meta->enum_count > 0) {
        ImGui::Spacing();
        ImGui::TextDisabled("PRESETS");
        for (int i = 0; i < meta->enum_count; ++i) {
            const ParamEnumEntry& e = meta->enum_entries[i];
            char hex[16];
            bitmask_hex(hex, sizeof(hex), bitmask_from_value(e.value, p.type), p.type);
            if (e.value == shown)
                ImGui::TextColored(accent_col(), "%s  %s", hex, e.label);
            else
                ImGui::Text("%s  %s", hex, e.label);
        }
    }

    ImGui::PopTextWrapPos();
}

void draw_tab_params(MavlinkSender* sender, const VehicleState* vs,
                     const std::unordered_map<std::string, ParamEntry>* params)
{
    const bool    connected = (vs && vs->has_heartbeat);
    const uint8_t tsys      = connected ? vs->sysid  : 1;
    const uint8_t tcomp     = connected ? vs->compid : 1;

    ImGui::Spacing();
    ImGui::TextColored(accent_col(), "PARAMETERS");
    if (!connected) {
        ImGui::SameLine(0, 6);
        ImGui::TextColored(col_no_link_muted(), "(no link)");
    }
    themed_sep();
    ImGui::Spacing();

    ImGui::BeginDisabled(!connected);
    if (ui_grid_button("FETCH ALL", { -1.0f, 26.0f }) && connected) {
        sender->request_param_list(tsys, tcomp);
        gcs_log("requesting parameter list");
    }
    ImGui::EndDisabled();

    if (vs && vs->param_count > 0) {
        ImGui::Spacing();
        const uint16_t received = vs->params_received;
        const uint16_t total    = vs->param_count;
        char prog_lbl[32];
        snprintf(prog_lbl, sizeof(prog_lbl), "%u / %u", received, total);

        if (vs->param_fetch_active && received < total) {
            const float frac = (float)received / total;
            ui_progress_bar(prog_lbl, frac, 12.0f);
            // Ticks alongside the bar, speeding up as it fills. A full fetch is
            // a thousand-odd parameters and the better part of a minute, most
            // of it spent looking somewhere other than this panel.
            gcs_progress("params", frac);
        } else if (received >= total) {
            ImGui::TextDisabled("%u params loaded", total);
        } else {
            // Parameters we hold without having asked for them: a vehicle sends
            // a PARAM_VALUE whenever one of its parameters changes, and the
            // stream rates this GCS sets on connect are parameters. That is a
            // count of what we happen to know, not a fetch in progress, so it
            // gets a number and no bar.
            ImGui::TextDisabled("%u of %u known", received, total);
        }
    }

    ImGui::Spacing();

    if (!params || params->empty()) {
        ImGui::TextDisabled("Press \"Fetch All\" to load parameters.");
        return;
    }

    // Sorted IDs — rebuilt once per fetch session when count changes
    auto& sorted_ids       = st().sorted_ids;
    auto& last_param_count = st().last_param_count;
    if (params->size() != last_param_count) {
        sorted_ids.clear();
        sorted_ids.reserve(params->size());
        for (auto& [k, _] : *params)
            sorted_ids.push_back(k);
        std::sort(sorted_ids.begin(), sorted_ids.end());
        last_param_count = params->size();
    }

    // Values the operator has staged: typed into a row, picked from an enum or
    // bitmask, or loaded from a parameter file. A row appears here only once it
    // has actually been moved, and leaves again as soon as the vehicle's value
    // agrees with it — which is what an accepted write looks like.
    //
    // Not every row seeded with the vehicle's value on sight, which is what
    // this used to do. A seeded row is indistinguishable from an edited one, so
    // any parameter the *vehicle* changed on its own afterwards became a
    // pending edit that nobody made: the seed stayed at the old value while the
    // live one moved. Vehicles do this routinely — ArduPilot saves its
    // STAT_RUNTIME / STAT_FLTTIME counters as it goes, and every save is
    // broadcast as a PARAM_VALUE — so a fetch and no typing at all could leave
    // WRITE ALL offering to write one parameter back. Offering, worse, to write
    // the *stale* value over the vehicle's newer one.
    auto& edited_vals = st().edited_vals;

    const ParamDocSource* docs = param_docs_for(vs);
    const bool heli    = vs && vs->type == MAV_TYPE_HELICOPTER;
    // The release whose page fits the firmware, from AUTOPILOT_VERSION; until
    // that arrives, -1, which the lookup reads as the newest.
    const int  fw_doc  = (docs && vs->has_fw_info)
                       ? param_doc_version_for(*docs, vs->fw_version) : -1;

    // Rows the operator has moved away from the vehicle's value, in name order
    // so the write goes out and logs in the same order the list shows.
    // Rebuilt every frame: the vehicle echoes each accepted write back as a
    // PARAM_VALUE, which is what makes a row stop being dirty, so this has to
    // follow the live map rather than a flag set at edit time.
    std::vector<const ParamEntry*> dirty;
    for (const std::string& id : sorted_ids) {
        auto ev = edited_vals.find(id);
        if (ev == edited_vals.end()) continue;
        auto it = params->find(id);
        if (it != params->end() && it->second.value != ev->second)
            dirty.push_back(&it->second);
    }
    const int dirty_n = (int)dirty.size();

    // Staged values outside their documented range, for WRITE ALL to mention.
    int dirty_oor = 0;
    for (const ParamEntry* p : dirty) {
        const ParamMeta* meta = docs ? get_param_meta(*docs, p->param_id, heli, fw_doc) : nullptr;
        if (param_out_of_range(meta, param_effective_kind(meta, p->type),
                               edited_vals[p->param_id]))
            ++dirty_oor;
    }

    // ── Write all / discard all ───────────────────────────────────────────────
    //
    // Both confirmed rather than immediate: a single WRITE is one considered
    // change, where these two act on everything staged — including a whole file
    // loaded a moment ago, which the operator may not have read line by line.
    // Discarding is local and touches no vehicle, but there is no undo behind
    // it and a staged file can be hundreds of rows, so it asks too. The per-row
    // cross is the unconfirmed way out, because it can only lose one value.
    const float half_w = (ImGui::GetContentRegionAvail().x - 4.0f) * 0.5f;

    ImGui::BeginDisabled(!connected || dirty_n == 0);
    {
        char lbl[48];
        snprintf(lbl, sizeof(lbl), "WRITE ALL (%d)###write_all", dirty_n);
        const bool hit = dirty_n > 0
            ? ui_solid_button(lbl, { half_w, 26.0f },
                              btn_write_base(), btn_write_hov())
            : ui_grid_button(lbl, { half_w, 26.0f });
        if (hit) ImGui::OpenPopup("##confirm_write_all");
    }
    ImGui::EndDisabled();

    ImGui::SameLine(0, 4);

    // Not gated on `connected`, unlike WRITE ALL: throwing away your own edits
    // is something you can always do, and being unable to clear them because
    // the link happened to drop would be its own annoyance.
    ImGui::BeginDisabled(dirty_n == 0);
    {
        char lbl[48];
        snprintf(lbl, sizeof(lbl), "DISCARD (%d)###discard_all", dirty_n);
        const bool hit = dirty_n > 0
            ? ui_solid_button(lbl, { half_w, 26.0f },
                              btn_disconnect_base(), btn_disconnect_hov())
            : ui_grid_button(lbl, { half_w, 26.0f });
        if (hit) ImGui::OpenPopup("##confirm_discard_all");
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered() && dirty_n > 0)
        ImGui::SetTooltip("Discard all staged changes — the vehicle is not touched");

    {
        char q[96];
        snprintf(q, sizeof(q), "DISCARD %d STAGED CHANGE%s?",
                 dirty_n, dirty_n == 1 ? "" : "S");
        if (ui_confirm_popup("##confirm_discard_all", "DISCARD CHANGES", q,
                             "DISCARD", btn_disconnect_base()) == UiConfirm::Confirmed) {
            const int n = (int)edited_vals.size();
            edited_vals.clear();
            gcs_log("discarded %d staged param change%s", n, n == 1 ? "" : "s");
        }
    }

    {
        char q[160];
        int  qn = snprintf(q, sizeof(q), "WRITE %d CHANGED PARAMETER%s TO THE VEHICLE?",
                           dirty_n, dirty_n == 1 ? "" : "S");
        if (dirty_oor > 0 && qn > 0 && (size_t)qn < sizeof(q))
            snprintf(q + qn, sizeof(q) - qn, "\n%d %s OUTSIDE THE DOCUMENTED RANGE.",
                     dirty_oor, dirty_oor == 1 ? "IS" : "ARE");
        if (ui_confirm_popup("##confirm_write_all", "WRITE ALL", q,
                             "WRITE ALL", btn_write_base()) == UiConfirm::Confirmed) {
            int sent = 0;
            for (const ParamEntry* p : dirty) {
                auto ev = edited_vals.find(p->param_id);
                if (ev == edited_vals.end()) continue;
                sender->set_param(tsys, tcomp, p->param_id, ev->second, p->type);
                ++sent;
            }
            // No retry behind this: each write is echoed back as a PARAM_VALUE,
            // so anything the vehicle missed stays visibly dirty in the list and
            // the operator can press again.
            gcs_log("writing %d parameters \xe2\x80\x94 rows clear as each is echoed back",
                    sent);
        }
    }

    ImGui::Spacing();

    // ── Parameter file ────────────────────────────────────────────────────────
    //
    // Both buttons name their file through the browser rather than off a typed
    // path: a .params file lives wherever the operator keeps their configs, and
    // an absolute path is a thing to find, not to remember and retype. What is
    // kept between presses is only the last file touched, which seeds the
    // browser so a save after a load lands next to what was loaded.
    static std::string s_param_path = "parameters.params";

    ImGui::TextDisabled("PARAM FILE");
    {
        const float half = (ImGui::GetContentRegionAvail().x - 4.0f) * 0.5f;

        if (ui_grid_button("LOAD", { half, 24.0f }))
            open_param_file_dialog(PARAM_LOAD_DLG_KEY, "LOAD PARAMETERS",
                                   s_param_path, /*for_save=*/false);

        ImGui::SameLine(0, 4);

        if (ui_grid_button("SAVE", { -1.0f, 24.0f }))
            open_param_file_dialog(PARAM_SAVE_DLG_KEY, "SAVE PARAMETERS",
                                   s_param_path, /*for_save=*/true);
    }

    // ── The two browsers ──────────────────────────────────────────────────────
    //
    // Polled every frame whether or not their button was pressed — Display()
    // returns immediately for a key that is not the open dialog, so only one
    // of these ever does anything, and only one can be open at a time.
    //
    // The work each button used to do inline happens here instead, once the
    // operator has named a file. Nothing is read or written until they do.
    push_file_dialog_style();

    if (ImGuiFileDialog::Instance()->Display(PARAM_LOAD_DLG_KEY,
                                             ImGuiWindowFlags_NoCollapse,
                                             { 640.0f, 420.0f })) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            s_param_path = ImGuiFileDialog::Instance()->GetFilePathName();

            // Reads the file and opens the picker below — it neither stages nor
            // writes on its own. A parameter file is a whole vehicle's worth of
            // settings, and pulling one in wholesale is rarely what is wanted.
            std::vector<ParamFileRow> rows;
            std::string err;
            st().load_rows.clear();
            st().load_matched = st().load_unknown = st().load_skipped = 0;

            if (!param_file_load(s_param_path, &rows, &st().load_skipped, &err)) {
                gcs_log("param load failed: %s", err.c_str());
            } else {
                // Only differences are offered. A value the vehicle already
                // holds is not a change to review, and listing 1300 of them
                // would bury the twelve that matter.
                for (const ParamFileRow& r : rows) {
                    auto it = params->find(r.id);
                    if (it == params->end()) { ++st().load_unknown; continue; }
                    if (r.value == it->second.value) { ++st().load_matched; continue; }
                    st().load_rows.push_back(LoadRow{ r.id, r.value,
                                                   it->second.value, true });
                }

                if (st().load_rows.empty()) {
                    gcs_log("%s: nothing to change (%d match, %d not on vehicle, %d bad lines)",
                            s_param_path.c_str(), st().load_matched,
                            st().load_unknown, st().load_skipped);
                } else {
                    // The picker is submitted further down this same function,
                    // so it opens on this frame rather than the next one.
                    ImGui::OpenPopup("##param_load_pick");
                }
            }
        }
        ImGuiFileDialog::Instance()->Close();
    }

    if (ImGuiFileDialog::Instance()->Display(PARAM_SAVE_DLG_KEY,
                                             ImGuiWindowFlags_NoCollapse,
                                             { 640.0f, 420.0f })) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            s_param_path = ImGuiFileDialog::Instance()->GetFilePathName();

            // Writes what the list shows, pending edits included — the file is
            // the configuration you are looking at, not a snapshot of the
            // vehicle. The log says so whenever the two differ.
            std::vector<ParamFileRow> rows;
            rows.reserve(sorted_ids.size());
            int pending = 0;
            for (const std::string& id : sorted_ids) {
                auto it = params->find(id);
                if (it == params->end()) continue;
                const ParamEntry& p = it->second;

                float v  = p.value;
                auto  ev = edited_vals.find(id);
                if (ev != edited_vals.end() && ev->second != p.value) {
                    v = ev->second;
                    ++pending;
                }
                rows.push_back(ParamFileRow{ id, v, p.type, tsys, tcomp });
            }

            std::string err;
            if (!param_file_save(s_param_path, rows, file_header(vs), &err)) {
                gcs_log("param save failed: %s", err.c_str());
            } else if (pending > 0) {
                gcs_log("saved %d params to %s (%d are unwritten edits)",
                        (int)rows.size(), s_param_path.c_str(), pending);
            } else {
                gcs_log("saved %d params to %s", (int)rows.size(),
                        s_param_path.c_str());
            }
        }
        ImGuiFileDialog::Instance()->Close();
    }

    pop_file_dialog_style();

    // ── Load picker ───────────────────────────────────────────────────────────
    //
    // Everything the file would change, one row each, checked by default. The
    // operator's job here is to uncheck what they did not come for — a config
    // file from another airframe carries plenty that should not follow it over.
    ui_push_dialog_style();
    if (ImGui::BeginPopupModal("##param_load_pick", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
        constexpr float PICK_W = 420.0f;
        constexpr float PICK_H = 320.0f;

        ui_dialog_title("PARAMETERS FROM FILE", PICK_W);

        int selected = 0;
        for (const LoadRow& r : st().load_rows)
            if (r.selected) ++selected;

        ImGui::TextDisabled("%s", s_param_path.c_str());
        ImGui::TextDisabled("%d differ \xe2\x80\x94 %d already match, %d not on vehicle, %d bad lines",
                            (int)st().load_rows.size(), st().load_matched,
                            st().load_unknown, st().load_skipped);
        ImGui::Spacing();

        // Bulk selection, with the running count beside it so the APPLY figure
        // below is never a surprise.
        constexpr float SEL_W = 130.0f;
        if (ui_grid_button("SELECT ALL", { SEL_W, 24.0f }))
            for (LoadRow& r : st().load_rows) r.selected = true;
        ImGui::SameLine(0, 4);
        if (ui_grid_button("DESELECT ALL", { SEL_W, 24.0f }))
            for (LoadRow& r : st().load_rows) r.selected = false;
        ImGui::SameLine(0, 8);
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(accent_col(), "%d of %d",
                           selected, (int)st().load_rows.size());

        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_ChildBg, bg_param_list());
        ImGui::PushStyleColor(ImGuiCol_Border,  col_separator());
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, FRAME_BORDER_NORMAL);
        if (ImGui::BeginChild("##load_list", { PICK_W, PICK_H }, true)) {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 4.0f, 3.0f });

            const float row_w  = ImGui::GetContentRegionAvail().x;
            const float from_x = row_w - 190.0f;   // old value column
            const float to_x   = row_w -  90.0f;   // new value column

            for (size_t i = 0; i < st().load_rows.size(); ++i) {
                LoadRow& r = st().load_rows[i];
                ImGui::PushID((int)i);

                ImGui::Checkbox("##sel", &r.selected);
                ImGui::SameLine(0, 6);
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted(r.id.c_str());

                // Vehicle value, then the file's — read left to right as the
                // change that would be staged.
                ImGui::SameLine(from_x);
                ImGui::TextDisabled("%.6g", (double)r.vehicle_val);
                ImGui::SameLine(to_x - 14.0f);
                ImGui::TextDisabled("\xe2\x86\x92");
                ImGui::SameLine(to_x);
                ImGui::TextColored(accent_col(), "%.6g", (double)r.file_val);

                ImGui::PopID();
            }
            ImGui::PopStyleVar();  // ItemSpacing
        }
        ImGui::EndChild();
        ImGui::PopStyleVar();      // FrameBorderSize
        ImGui::PopStyleColor(2);   // Border, ChildBg

        ImGui::Spacing();

        constexpr float BW = 150.0f, BGAP = 8.0f;
        ui_dialog_row(BW * 2.0f + BGAP);

        char apply_lbl[40];
        snprintf(apply_lbl, sizeof(apply_lbl), "STAGE %d###apply_load", selected);

        ImGui::BeginDisabled(selected == 0);
        if (ui_solid_button(apply_lbl, { BW, UI_DIALOG_BH },
                            btn_write_base(), btn_write_hov())) {
            int staged = 0;
            for (const LoadRow& r : st().load_rows) {
                if (!r.selected) continue;
                edited_vals[r.id] = r.file_val;
                ++staged;
            }
            // Staged, not written: they join the pending edits as if typed by
            // hand, and WRITE ALL remains the deliberate second act.
            gcs_log("staged %d parameter%s from %s \xe2\x80\x94 press WRITE ALL to send",
                    staged, staged == 1 ? "" : "s", s_param_path.c_str());
            st().load_rows.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndDisabled();

        ImGui::SameLine(0, BGAP);
        if (ui_grid_button("CANCEL##load", { BW, UI_DIALOG_BH }) ||
            ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
            st().load_rows.clear();
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
    ui_pop_dialog_style();

    ImGui::Spacing();
    themed_sep();
    ImGui::Spacing();

    // Search filter
    static char param_search[64] = {};
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputTextWithHint("##param_search", "Search...",
                             param_search, sizeof(param_search));
    ImGui::Spacing();

    // Upper-cased search string for case-insensitive filtering
    char search_upper[64] = {};
    for (int i = 0; i < 63 && param_search[i]; ++i)
        search_upper[i] = (char)toupper((unsigned char)param_search[i]);

    // Which page the rows are read against, so a mismatch is visible before
    // anyone opens an "i".
    if (docs) {
        char lbl[16];
        param_doc_version_label(*docs, fw_doc >= 0 ? fw_doc : (int)docs->version_count - 1,
                                lbl, sizeof(lbl));
        if (fw_doc >= 0)
            ImGui::TextDisabled("Docs: %s %s reference (firmware %s)",
                                docs->name, lbl, vs->fw_version);
        else
            ImGui::TextDisabled("Docs: %s %s reference until the firmware reports its version",
                                docs->name, lbl);
    } else if (vs && vs->has_heartbeat) {
        ImGui::TextDisabled("Docs: none for this vehicle (ArduPilot Copter and PX4 only)");
    }
    ImGui::Spacing();

    const float avail_h = ImGui::GetContentRegionAvail().y;
    const float val_w   = 150.0f;
    // The info button and WRITE share what WRITE alone used to take, for the
    // same reason the discard cross comes out of the value column below.
    const float btn_w   = 86.0f;
    const float info_w  = 22.0f;
    const float write_w = btn_w - info_w - 4.0f;
    // Discard-one cross, and the input width left once it is taken out. The
    // cross comes out of the value column rather than being added to the row:
    // the name column is already the one under pressure — left_w is 24% of the
    // window, so on a small display it has barely room for a 16-character
    // parameter ID — and widening the row would come out of exactly that.
    // Reserved on every row, drawn or not, so the WRITE column does not jump
    // sideways as rows are edited and cleared.
    const float x_w     = 22.0f;
    const float in_w    = val_w - x_w - 4.0f;

    ImGui::PushStyleColor(ImGuiCol_ChildBg, bg_param_list());
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,   FRAME_ROUNDING_SM);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, FRAME_BORDER_NORMAL);
    ImGui::PushStyleColor(ImGuiCol_Border, col_separator());
    if (ImGui::BeginChild("##params_list", { -1.0f, avail_h }, false)) {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 2.0f, 3.0f });
        const float row_w  = ImGui::GetContentRegionAvail().x;
        const float name_w = row_w - val_w - btn_w - 8.0f;

        for (const std::string& id : sorted_ids) {
            auto it = params->find(id);
            if (it == params->end()) continue;
            const ParamEntry& p = it->second;

            // Filter
            if (search_upper[0] != '\0') {
                char id_upper[17] = {};
                for (int i = 0; i < 16 && p.param_id[i]; ++i)
                    id_upper[i] = (char)toupper((unsigned char)p.param_id[i]);
                if (strstr(id_upper, search_upper) == nullptr) continue;
            }

            ImGui::PushID(p.param_id);

            int doc = -1;
            const ParamMeta* meta = docs
                ? get_param_meta(*docs, p.param_id, heli, fw_doc, &doc) : nullptr;
            const ParamKind  kind = param_effective_kind(meta, p.type);
            const bool       ro   = meta && meta->has(PARAM_FLAG_READONLY);

            // The editor drawn: the parameter's own, unless it was switched to
            // manual entry, which edits an enum or bitmask as the plain number
            // the vehicle stores.
            bool manual = (kind == ParamKind::Enum || kind == ParamKind::Bitmask) &&
                          st().manual.count(id) > 0;
            const ParamKind ui_kind = manual ? param_effective_kind(nullptr, p.type) : kind;

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(p.param_id);

            // Just the name on hover; the whole entry is behind "i".
            if (meta && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted(meta->display_name);
                if (meta->units) ImGui::TextDisabled("%s", meta->units);
                ImGui::EndTooltip();
            }

            ImGui::SameLine(name_w);

            // What the row shows: the staged value if there is one, otherwise
            // the vehicle's. A plain local, not a reference into the map —
            // nothing is inserted until an edit below actually happens. ImGui
            // keeps its own buffer while a field is being typed into, so the
            // backing float only has to survive the frame.
            auto  ev       = edited_vals.find(id);
            float edit_val = (ev != edited_vals.end()) ? ev->second : p.value;
            bool  edited   = false;

            ImGui::BeginDisabled(ro);
            if (ui_kind == ParamKind::Enum) {
                // A value the reference does not list still shows as its
                // number rather than a blank — firmware newer than the table,
                // or a value set by hand.
                const ParamEnumEntry* cur = nullptr;
                for (int e = 0; e < meta->enum_count; ++e)
                    if (meta->enum_entries[e].value == edit_val) {
                        cur = &meta->enum_entries[e];
                        break;
                    }
                char preview[64];
                if (cur) snprintf(preview, sizeof(preview), "%s", cur->label);
                else     snprintf(preview, sizeof(preview), "%.7g (unlisted)",
                                  (double)edit_val);

                ImGui::SetNextItemWidth(in_w);
                if (ImGui::BeginCombo("##v", preview, ImGuiComboFlags_HeightLargest)) {
                    for (int e = 0; e < meta->enum_count; ++e) {
                        const ParamEnumEntry& ent = meta->enum_entries[e];
                        const bool sel = (&ent == cur);
                        char item[160];
                        snprintf(item, sizeof(item), "%-4.7g %s##e%d",
                                 (double)ent.value, ent.label, e);
                        if (ImGui::Selectable(item, sel)) {
                            edit_val = ent.value;
                            edited   = true;
                        }
                        if (sel) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

            } else if (ui_kind == ParamKind::Bitmask) {
                // Shown as hex, edited as checkboxes — one per documented bit,
                // plus any set bit the reference has no name for, so nothing
                // the vehicle holds is invisible here or impossible to clear.
                uint32_t bits = bitmask_from_value(edit_val, p.type);
                char hex[16];
                bitmask_hex(hex, sizeof(hex), bits, p.type);
                char btn_lbl[32];
                snprintf(btn_lbl, sizeof(btn_lbl), "%s###bm", hex);
                if (ui_grid_button(btn_lbl, { in_w, 0.0f }))
                    ImGui::OpenPopup("##bitmask_popup");

                ui_push_dialog_style();
                if (ImGui::BeginPopup("##bitmask_popup")) {
                    constexpr float BM_W = 300.0f;
                    ui_dialog_title(p.param_id, BM_W);

                    const int width = bitmask_width(p.type);
                    uint32_t  next  = bits;

                    // Typed hex, for when the mask is already known.
                    char fmt[16];
                    snprintf(fmt, sizeof(fmt), "%%0%dX", width / 4);
                    ImGui::AlignTextToFramePadding();
                    ImGui::TextDisabled("HEX  0x");
                    ImGui::SameLine(0, 2);
                    ImGui::SetNextItemWidth(BM_W * 0.5f);
                    ImGui::InputScalar("##hex", ImGuiDataType_U32, &next,
                                       nullptr, nullptr, fmt,
                                       ImGuiInputTextFlags_CharsHexadecimal);
                    if (width < 32) next &= (1u << width) - 1u;
                    ImGui::SameLine(0, 8);
                    ImGui::TextDisabled("= %u", (unsigned)next);
                    ImGui::Spacing();

                    const float row_h = ImGui::GetFrameHeightWithSpacing();
                    const int   rows  = meta->bitmask_count;
                    ImGui::BeginChild("##bits", { BM_W, std::min(rows, 16) * row_h },
                                      false);
                    uint32_t documented = 0;
                    for (int b = 0; b < rows; ++b) {
                        const ParamBitmaskBit& bb = meta->bitmask_bits[b];
                        documented |= 1u << bb.bit;
                        bool set = (next >> bb.bit) & 1u;
                        char lbl[160];
                        snprintf(lbl, sizeof(lbl), "%-2u %s##b%u",
                                 (unsigned)bb.bit, bb.label, (unsigned)bb.bit);
                        if (ImGui::Checkbox(lbl, &set))
                            next = set ? next | (1u << bb.bit) : next & ~(1u << bb.bit);
                    }
                    for (int b = 0; b < width; ++b) {
                        if ((documented >> b) & 1u || !((next >> b) & 1u)) continue;
                        bool set = true;
                        char lbl[48];
                        snprintf(lbl, sizeof(lbl), "%-2d (undocumented)##u%d", b, b);
                        if (ImGui::Checkbox(lbl, &set) && !set)
                            next &= ~(1u << b);
                    }
                    ImGui::EndChild();

                    if (next != bits) {
                        bits     = next;
                        edit_val = bitmask_to_value(bits, p.type);
                        edited   = true;
                    }
                    ImGui::EndPopup();
                }
                ui_pop_dialog_style();

            } else {
                // A plain number takes any value, whatever the reference
                // lists — its range is a warning, drawn in amber, not a limit.
                const bool oor = param_out_of_range(meta, ui_kind, edit_val);
                if (oor) {
                    ImGui::PushStyleColor(ImGuiCol_Border, col_warning());
                    ImGui::PushStyleColor(ImGuiCol_Text,   col_warning());
                }
                ImGui::SetNextItemWidth(in_w);
                if (ui_kind == ParamKind::Int) {
                    // 64-bit so a uint32 above INT32_MAX still round-trips.
                    int64_t iv = (int64_t)edit_val;
                    if (ImGui::InputScalar("##v", ImGuiDataType_S64, &iv,
                                           nullptr, nullptr, "%lld")) {
                        edit_val = (float)iv;
                        edited   = true;
                    }
                } else {
                    if (ImGui::InputFloat("##v", &edit_val, 0.0f, 0.0f, "%.4g"))
                        edited = true;
                }
                if (oor) {
                    ImGui::PopStyleColor(2);
                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                        char rng[64];
                        format_range(meta, rng, sizeof(rng));
                        ImGui::SetTooltip("Outside the documented range %s", rng);
                    }
                } else if (manual && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
                    // What the typed number means, since the list is not there
                    // to say.
                    if (kind == ParamKind::Enum) {
                        const char* label = nullptr;
                        for (int e = 0; e < meta->enum_count; ++e)
                            if (meta->enum_entries[e].value == edit_val)
                                label = meta->enum_entries[e].label;
                        ImGui::SetTooltip("Manual entry \xe2\x80\x94 %s",
                                          label ? label : "not a listed value");
                    } else {
                        char hex[16];
                        bitmask_hex(hex, sizeof(hex),
                                    bitmask_from_value(edit_val, p.type), p.type);
                        ImGui::SetTooltip("Manual entry \xe2\x80\x94 %s", hex);
                    }
                }
            }
            ImGui::EndDisabled();

            // Stage on edit; unstage once the vehicle agrees again, which is
            // both how an accepted write clears a row and how typing a value
            // back to what it was clears it.
            if (edited) {
                edited_vals[id] = edit_val;
            } else if (ev != edited_vals.end() && ev->second == p.value) {
                edited_vals.erase(ev);
            }

            bool dirty = (edit_val != p.value);

            // Discard this one row. Unconfirmed — it can lose exactly one value,
            // and the value it restores is sitting right there on the vehicle.
            ImGui::SameLine(0, 4);
            if (dirty) {
                if (ui_grid_button("\xc3\x97" "###discard_one", { x_w, 0.0f })) {
                    edited_vals.erase(id);
                    // Restore the row in this same frame, so the WRITE button
                    // beside it does not stay lit until the next one.
                    edit_val = p.value;
                    dirty    = false;
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Discard this change (back to %.4g)",
                                      (double)p.value);
            } else {
                ImGui::Dummy({ x_w, ImGui::GetFrameHeight() });
            }

            ImGui::SameLine(0, 4);

            // Everything the reference says about this parameter. Live with or
            // without a link — reading documentation needs no vehicle.
            if (ui_grid_button("i###info", { info_w, 0.0f }))
                ImGui::OpenPopup("##param_info");
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
                ImGui::SetTooltip("Parameter info");

            ui_push_dialog_style();
            if (ImGui::BeginPopup("##param_info")) {
                const bool was = manual;
                draw_param_info(p, meta, kind, edit_val, docs, fw_doc, doc,
                                (kind == ParamKind::Enum || kind == ParamKind::Bitmask)
                                    ? &manual : nullptr);
                if (manual != was) {
                    if (manual) st().manual.insert(id);
                    else        st().manual.erase(id);
                }
                ImGui::EndPopup();
            }
            ui_pop_dialog_style();

            ImGui::SameLine(0, 4);

            ImGui::BeginDisabled(!connected || !dirty || ro);
            // Amber only once the field differs from the vehicle's value —
            // an unedited row stays in the neutral well like every other button.
            const bool wrote = dirty
                ? ui_solid_button("WRITE", { write_w, 0.0f },
                                  btn_write_base(), btn_write_hov())
                : ui_grid_button("WRITE", { write_w, 0.0f });
            if (wrote) {
                sender->set_param(tsys, tcomp, p.param_id, edit_val, p.type);
                if (param_out_of_range(meta, kind, edit_val)) {
                    char rng[64];
                    format_range(meta, rng, sizeof(rng));
                    gcs_log("writing %s = %.6g (outside documented range %s)",
                            p.param_id, (double)edit_val, rng);
                } else
                    gcs_log("writing %s = %.6g", p.param_id, (double)edit_val);
            }
            ImGui::EndDisabled();

            ImGui::PopID();
        }
        ImGui::PopStyleVar(); // ItemSpacing
    }
    ImGui::EndChild();
    ImGui::PopStyleColor(); // Border
    ImGui::PopStyleVar(2);  // FrameRounding, FrameBorderSize
    ImGui::PopStyleColor(); // ChildBg
}
