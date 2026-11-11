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

// frontend/components/sidebar_left/params/editors.cpp
//
// A row's value editor, by the parameter's kind: a list for an enum, a
// checkbox popup for a bitmask, a number field otherwise. Each edits
// `edit_val` in place and says whether the operator changed it.

#include "frontend/components/sidebar_left/params/params_internal.hpp"
#include <algorithm>
#include <cstdint>
#include <cstdio>

namespace params_tab {

static bool edit_enum(const ParamMeta* meta, float& edit_val, float in_w)
{
    bool edited = false;
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
    return edited;
}

static bool edit_bitmask(const ParamEntry& p, const ParamMeta* meta, float& edit_val, float in_w)
{
    bool edited = false;
    // Shown as hex, edited as checkboxes — one per documented bit,
    // plus any set bit the reference has no name for, so nothing
    // the vehicle holds is invisible here or impossible to clear.
    uint32_t bits = param_bitmask_from_value(edit_val, p.type);
    char hex[16];
    param_bitmask_hex(hex, sizeof(hex), bits, p.type);
    char btn_lbl[32];
    snprintf(btn_lbl, sizeof(btn_lbl), "%s###bm", hex);
    if (ui_grid_button(btn_lbl, { in_w, 0.0f }))
        ImGui::OpenPopup("##bitmask_popup");

    ui_push_dialog_style();
    if (ImGui::BeginPopup("##bitmask_popup")) {
        constexpr float BM_W = 300.0f;
        ui_dialog_title(p.param_id, BM_W);

        const int width = param_bitmask_width(p.type);
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
            edit_val = param_bitmask_to_value(bits, p.type);
            edited   = true;
        }
        ImGui::EndPopup();
    }
    ui_pop_dialog_style();
    return edited;
}

static bool edit_number(const ParamEntry& p, const ParamMeta* meta, ParamKind kind,
                        ParamKind ui_kind, bool manual, float& edit_val, float in_w)
{
    bool edited = false;
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
            param_format_range(meta, rng, sizeof(rng));
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
            param_bitmask_hex(hex, sizeof(hex),
                        param_bitmask_from_value(edit_val, p.type), p.type);
            ImGui::SetTooltip("Manual entry \xe2\x80\x94 %s", hex);
        }
    }
    return edited;
}

bool draw_value_editor(const ParamEntry& p, const ParamMeta* meta, ParamKind kind,
                       ParamKind ui_kind, bool manual, float& edit_val, float in_w)
{
    if (ui_kind == ParamKind::Enum)    return edit_enum(meta, edit_val, in_w);
    if (ui_kind == ParamKind::Bitmask) return edit_bitmask(p, meta, edit_val, in_w);
    return edit_number(p, meta, kind, ui_kind, manual, edit_val, in_w);
}

} // namespace params_tab
