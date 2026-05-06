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
#include "../param_meta.hpp"
#include "../../app_log.hpp"
#include "imgui.h"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <cstdio>
#include <string>
#include <unordered_map>
#include <vector>

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
    push_flash_colors(CmdFlashState::Normal);
    if (ImGui::Button("FETCH ALL", { -1.0f, 26.0f }) && connected) {
        sender->request_param_list(tsys, tcomp);
        gcs_log("requesting parameter list");
    }
    ImGui::PopStyleColor(3);
    ImGui::EndDisabled();

    if (vs && vs->param_count > 0) {
        ImGui::Spacing();
        const uint16_t received = vs->params_received;
        const uint16_t total    = vs->param_count;
        if (received < total) {
            char prog_lbl[32];
            snprintf(prog_lbl, sizeof(prog_lbl), "%u / %u", received, total);
            ImGui::ProgressBar((float)received / total, { -1.0f, 12.0f }, prog_lbl);
        } else {
            ImGui::TextDisabled("%u params loaded", total);
        }
    }

    ImGui::Spacing();

    if (!params || params->empty()) {
        ImGui::TextDisabled("Press \"Fetch All\" to load parameters.");
        return;
    }

    // Search filter
    static char param_search[64] = {};
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputTextWithHint("##param_search", "Search...",
                             param_search, sizeof(param_search));
    ImGui::Spacing();

    // Sorted IDs — rebuilt once per fetch session when count changes
    static std::vector<std::string> sorted_ids;
    static size_t last_param_count = 0;
    if (params->size() != last_param_count) {
        sorted_ids.clear();
        sorted_ids.reserve(params->size());
        for (auto& [k, _] : *params)
            sorted_ids.push_back(k);
        std::sort(sorted_ids.begin(), sorted_ids.end());
        last_param_count = params->size();
    }

    // Pending edits — initialised to current FC value on first appearance
    static std::unordered_map<std::string, float> edited_vals;

    // Upper-cased search string for case-insensitive filtering
    char search_upper[64] = {};
    for (int i = 0; i < 63 && param_search[i]; ++i)
        search_upper[i] = (char)toupper((unsigned char)param_search[i]);

    const float avail_h = ImGui::GetContentRegionAvail().y;
    const float val_w   = 90.0f;
    const float btn_w   = 46.0f;

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

            const ParamMeta* meta = get_param_meta(p.param_id);

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(p.param_id);

            if (meta && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
                ImGui::BeginTooltip();
                ImGui::TextColored(accent_col(), "%s", p.param_id);
                ImGui::Separator();
                ImGui::PushTextWrapPos(320.0f);
                ImGui::TextUnformatted(meta->short_desc);
                ImGui::PopTextWrapPos();
                ImGui::Spacing();
                if (meta->units)
                    ImGui::Text("Units:   %s", meta->units);
                ImGui::Text("Default: %.4g", meta->default_val);
                if (meta->widget == ParamWidgetType::Float &&
                    meta->min_val != meta->max_val)
                    ImGui::Text("Range:   %.4g  \xe2\x80\x93  %.4g",
                                meta->min_val, meta->max_val);
                if (meta->reboot_required)
                    ImGui::TextColored(col_reboot(), "! Reboot required");
                ImGui::EndTooltip();
            }

            ImGui::SameLine(name_w);

            auto [ev_it, inserted] = edited_vals.emplace(id, p.value);
            float& edit_val = ev_it->second;

            if (meta && meta->widget == ParamWidgetType::Enum) {
                const char* preview = "Unknown";
                int32_t cur_int = (int32_t)edit_val;
                for (int e = 0; e < meta->enum_count; ++e) {
                    if (meta->enum_entries[e].value == cur_int) {
                        preview = meta->enum_entries[e].label;
                        break;
                    }
                }
                ImGui::SetNextItemWidth(val_w);
                if (ImGui::BeginCombo("##v", preview, ImGuiComboFlags_HeightLargest)) {
                    for (int e = 0; e < meta->enum_count; ++e) {
                        bool sel = (meta->enum_entries[e].value == cur_int);
                        if (ImGui::Selectable(meta->enum_entries[e].label, sel))
                            edit_val = (float)meta->enum_entries[e].value;
                        if (sel) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

            } else if (meta && meta->widget == ParamWidgetType::Bitmask) {
                int32_t bitmask_val = (int32_t)edit_val;
                char btn_lbl[24];
                snprintf(btn_lbl, sizeof(btn_lbl), "0x%04X###bm", bitmask_val);
                if (ImGui::Button(btn_lbl, { val_w, 0.0f }))
                    ImGui::OpenPopup("##bitmask_popup");

                if (ImGui::BeginPopup("##bitmask_popup")) {
                    ImGui::TextColored(accent_col(), "%s", p.param_id);
                    ImGui::Separator();
                    ImGui::Spacing();
                    for (int b = 0; b < meta->bitmask_count; ++b) {
                        bool set = (bitmask_val >> meta->bitmask_bits[b].bit) & 1;
                        if (ImGui::Checkbox(meta->bitmask_bits[b].label, &set)) {
                            if (set) bitmask_val |=  (1 << meta->bitmask_bits[b].bit);
                            else     bitmask_val &= ~(1 << meta->bitmask_bits[b].bit);
                            edit_val = (float)bitmask_val;
                        }
                    }
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Text("Value: 0x%04X (%d)", bitmask_val, bitmask_val);
                    ImGui::EndPopup();
                }

            } else {
                ImGui::SetNextItemWidth(val_w);
                ImGui::InputFloat("##v", &edit_val, 0.0f, 0.0f, "%.4g");
            }

            ImGui::SameLine(0, 4);

            const bool dirty = (edit_val != p.value);
            ImGui::BeginDisabled(!connected || !dirty);
            ImGui::PushStyleColor(ImGuiCol_Button,
                dirty ? btn_write_base() : ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f });
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                dirty ? btn_write_hov()  : ImVec4{ 1.0f, 1.0f, 1.0f, 0.07f });
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                dirty ? btn_write_base() : ImVec4{ 1.0f, 1.0f, 1.0f, 0.14f });
            if (ImGui::Button("WRITE", { btn_w, 0.0f })) {
                sender->set_param(tsys, tcomp, p.param_id, edit_val, p.type);
                gcs_log("writing %s = %.6g", p.param_id, (double)edit_val);
            }
            ImGui::PopStyleColor(3);
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
