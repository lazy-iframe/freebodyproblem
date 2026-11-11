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

// frontend/components/sidebar_left/params/info.cpp
//
// The "i" popup: everything the parameter reference says about one
// parameter, and the vehicle's side of it.

#include "frontend/components/sidebar_left/params/params_internal.hpp"
#include <algorithm>
#include <cstdarg>
#include <cstdio>

namespace params_tab {

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


// Which firmware's documentation fits a vehicle, or nullptr for none.
//
// ArduPilot documents each vehicle separately, Copter and Plane covered here:
// their tables differ in exactly the vehicle-specific entries an operator is
// likeliest to open, so neither stands in for the other, and Rover and Sub
// get none rather than the wrong one. A QuadPlane reports a fixed-wing or VTOL
// type and reads Plane's, which documents its Q_ parameters. PX4 publishes
// one table for every airframe. A parameter one stack shares by name with the
// other is a different parameter, so neither stands in for the other.
const ParamDocSource* param_docs_for(const VehicleState* vs)
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
void draw_param_info(const ParamEntry& p, const ParamMeta* meta,
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
            param_format_range(meta, rng, sizeof(rng));
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
                param_bitmask_hex(hex, sizeof(hex), param_bitmask_from_value(d, p.type), p.type);
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
            param_bitmask_hex(hex, sizeof(hex), param_bitmask_from_value(p.value, p.type), p.type);
            info_row("Vehicle", "%s  (%.0f)", hex, (double)p.value);
            if (shown != p.value) {
                param_bitmask_hex(hex, sizeof(hex), param_bitmask_from_value(shown, p.type), p.type);
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
        const uint32_t bits = param_bitmask_from_value(shown, p.type);

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
            param_bitmask_hex(hex, sizeof(hex), param_bitmask_from_value(e.value, p.type), p.type);
            if (e.value == shown)
                ImGui::TextColored(accent_col(), "%s  %s", hex, e.label);
            else
                ImGui::Text("%s  %s", hex, e.label);
        }
    }

    ImGui::PopTextWrapPos();
}

} // namespace params_tab
