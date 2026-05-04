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
#include "mavlink_display_generated.hpp"
#include "imgui.h"
#include <cstdio>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <cstdint>

static uint32_t s_selected_id  = UINT32_MAX;
static uint32_t s_prev_sel     = UINT32_MAX; // detect selection changes
static int      s_req_msg_id   = 0;
static float    s_req_hz       = 4.0f;

void draw_tab_mavlink(MavlinkSender* sender,
                      const VehicleState* vs,
                      const std::unordered_map<uint32_t, MessageStats>* stats,
                      uint64_t total_messages,
                      uint64_t total_bytes,
                      uint64_t parse_errors)
{
    ImGui::Spacing();
    ImGui::TextColored(accent_col(), "MAVLINK");
    themed_sep();
    ImGui::Spacing();

    // ── Summary line ──────────────────────────────────────────────────────────
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 6.0f, 2.0f });
    ImGui::TextColored(accent_col(), "msgs");  ImGui::SameLine(0, 4);
    ImGui::TextColored(col_data(), "%llu", (unsigned long long)total_messages);
    ImGui::SameLine(0, 10);
    ImGui::TextColored(accent_col(), "bytes"); ImGui::SameLine(0, 4);
    ImGui::TextColored(col_data(), "%llu", (unsigned long long)total_bytes);
    if (parse_errors > 0) {
        ImGui::SameLine(0, 10);
        ImGui::TextColored(col_error(), "err:%llu", (unsigned long long)parse_errors);
    }
    ImGui::PopStyleVar();
    ImGui::Spacing();

    if (!stats || stats->empty()) {
        ImGui::TextDisabled("No messages received.");
        // Still show request panel even with no traffic
    }

    // ── Layout: table | detail | request ─────────────────────────────────────
    constexpr float REQ_H  = 52.0f;
    const float     avail  = ImGui::GetContentRegionAvail().y;
    const float     gap    = ImGui::GetStyle().ItemSpacing.y;
    const float     tbl_h  = (avail - REQ_H - gap * 3.0f) * 0.55f;
    const float     det_h  = avail - REQ_H - tbl_h - gap * 3.0f;

    // ── Message table ─────────────────────────────────────────────────────────
    if (stats && !stats->empty()) {
        struct Row { uint32_t id; double hz; uint64_t count; };
        std::vector<Row> rows;
        rows.reserve(stats->size());
        for (const auto& [id, s] : *stats)
            rows.push_back({ id, s.effective_rate_hz(), s.count });
        std::sort(rows.begin(), rows.end(),
                  [](const Row& a, const Row& b) { return a.id < b.id; });

        ImGui::PushStyleColor(ImGuiCol_ChildBg,  bg_child_dark());
        ImGui::PushStyleVar  (ImGuiStyleVar_FrameRounding,   FRAME_ROUNDING_SM);
        ImGui::PushStyleVar  (ImGuiStyleVar_FrameBorderSize, FRAME_BORDER_NORMAL);
        ImGui::PushStyleColor(ImGuiCol_Border,   col_separator());

        if (ImGui::BeginTable("##mavtbl", 3,
                ImGuiTableFlags_Borders     |
                ImGuiTableFlags_RowBg       |
                ImGuiTableFlags_ScrollY     |
                ImGuiTableFlags_SizingFixedFit,
                { -1.0f, tbl_h }))
        {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("ID",   ImGuiTableColumnFlags_WidthFixed,   36.0f);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Hz",   ImGuiTableColumnFlags_WidthFixed,   48.0f);
            ImGui::TableHeadersRow();

            for (const Row& r : rows) {
                ImGui::TableNextRow();
                const bool selected = (r.id == s_selected_id);

                ImGui::TableSetColumnIndex(0);
                ImGui::PushID((int)r.id);
                char id_str[8];
                snprintf(id_str, sizeof(id_str), "%u", r.id);
                if (ImGui::Selectable(id_str, selected,
                        ImGuiSelectableFlags_SpanAllColumns |
                        ImGuiSelectableFlags_AllowOverlap,
                        { 0.0f, 0.0f }))
                    s_selected_id = selected ? UINT32_MAX : r.id;
                const bool row_hovered = ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal);
                ImGui::PopID();

                ImGui::TableSetColumnIndex(1);
                const char* name = mavlink_display_name(r.id);
                if (name)
                    ImGui::TextColored(col_data(), "%s", name);
                else {
                    char fallback[16];
                    snprintf(fallback, sizeof(fallback), "MSG_%u", r.id);
                    ImGui::TextDisabled("%s", fallback);
                }

                ImGui::TableSetColumnIndex(2);
                if (r.hz >= 0.05)
                    ImGui::TextColored(col_data(), "%.1f", r.hz);
                else
                    ImGui::TextDisabled("--");

                if (row_hovered) {
                    ImGui::BeginTooltip();
                    ImGui::TextColored(accent_col(), "%s", name ? name : id_str);
                    ImGui::TextDisabled("id %u   count %llu",
                        r.id, (unsigned long long)r.count);
                    ImGui::EndTooltip();
                }
            }
            ImGui::EndTable();
        }

        ImGui::PopStyleColor(); // Border
        ImGui::PopStyleVar(2);  // FrameRounding, FrameBorderSize
        ImGui::PopStyleColor(); // ChildBg
    }

    // Sync req ID when selection changes
    if (s_selected_id != s_prev_sel) {
        if (s_selected_id != UINT32_MAX)
            s_req_msg_id = (int)s_selected_id;
        s_prev_sel = s_selected_id;
    }

    // ── Detail panel ──────────────────────────────────────────────────────────
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_ChildBg, bg_child_darker());
    if (ImGui::BeginChild("##mavdetail", { -1.0f, det_h }, false)) {

        if (s_selected_id == UINT32_MAX) {
            ImGui::TextDisabled("Select a message to inspect.");
        } else {
            const char* dname = mavlink_display_name(s_selected_id);
            if (dname) {
                ImGui::TextColored(accent_col(), "%s", dname);
                ImGui::SameLine(0, 6);
                ImGui::TextDisabled("(#%u)", s_selected_id);
            } else {
                ImGui::TextColored(accent_col(), "MSG_%u", s_selected_id);
            }

            if (stats) {
                auto it = stats->find(s_selected_id);
                if (it != stats->end()) {
                    ImGui::SameLine(0, 10);
                    ImGui::TextDisabled("%llu msgs  %.1f Hz",
                        (unsigned long long)it->second.count,
                        it->second.effective_rate_hz());
                }
            }

            ImGui::PushStyleColor(ImGuiCol_Separator, col_separator());
            ImGui::Separator();
            ImGui::PopStyleColor();
            ImGui::Spacing();

            if (vs) {
                auto it = vs->last_messages.find(s_selected_id);
                if (it != vs->last_messages.end()) {
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 4.0f, 2.0f });
                    mavlink_display_draw(s_selected_id, it->second);
                    ImGui::PopStyleVar();
                } else {
                    ImGui::TextDisabled("No data received yet.");
                }
            }
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleColor(); // ChildBg

    // ── Request message panel ─────────────────────────────────────────────────
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_ChildBg, bg_child_dark());
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,   FRAME_ROUNDING_SM);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, FRAME_BORDER_NORMAL);
    ImGui::PushStyleColor(ImGuiCol_Border, col_border_form());
    if (ImGui::BeginChild("##mavreq", { -1.0f, REQ_H }, true)) {

        const bool can_send = (sender != nullptr) &&
                              (vs != nullptr)     &&
                              (vs->sysid != 0);

        // Row 1: label
        ImGui::TextColored(accent_col(), "REQUEST MESSAGE");

        // Row 2: ID input | Hz input | button
        ImGui::SetNextItemWidth(58.0f);
        ImGui::InputInt("##req_id", &s_req_msg_id, 0, 0);
        if (s_req_msg_id < 0)   s_req_msg_id = 0;
        if (s_req_msg_id > 16777215) s_req_msg_id = 16777215; // 24-bit MAVLink msg ID max

        ImGui::SameLine(0, 4);
        ImGui::TextDisabled("ID");

        ImGui::SameLine(0, 8);
        ImGui::SetNextItemWidth(46.0f);
        ImGui::InputFloat("##req_hz", &s_req_hz, 0.f, 0.f, "%.1f");

        ImGui::SameLine(0, 4);
        ImGui::TextDisabled("Hz");

        ImGui::SameLine(0, 8);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, FRAME_ROUNDING_SM);
        if (!can_send) ImGui::BeginDisabled();
        push_flash_colors(CmdFlashState::Normal);
        if (ImGui::Button("SEND##mavreq", { -1.0f, 0.0f })) {
            // Convert Hz to interval_us:
            //   hz > 0  → interval_us = 1 000 000 / hz
            //   hz == 0 → interval_us = 0  (autopilot default rate)
            //   hz < 0  → interval_us = -1 (disable)
            int32_t interval_us;
            if (s_req_hz > 0.f)
                interval_us = (int32_t)(1e6f / s_req_hz);
            else if (s_req_hz == 0.f)
                interval_us = 0;
            else
                interval_us = -1;

            sender->request_message_interval(
                vs->sysid, vs->compid,
                (uint32_t)s_req_msg_id, interval_us);
        }
        ImGui::PopStyleColor(3);
        if (!can_send) ImGui::EndDisabled();
        ImGui::PopStyleVar(); // FrameRounding

        if (!can_send) {
            ImGui::SameLine(0, 6);
            ImGui::TextDisabled("(no link)");
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleColor(); // Border
    ImGui::PopStyleVar(2);  // FrameRounding, FrameBorderSize
    ImGui::PopStyleColor(); // ChildBg
}
