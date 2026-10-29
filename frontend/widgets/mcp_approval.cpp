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


#include "mcp_approval.hpp"

#include <cstdint>

#include "imgui.h"
#include "theme.hpp"
#include "ui_kit.hpp"
#include "../../backend/mcp_server.hpp"

static constexpr const char* POPUP_ID = "##mcp_approval";

// The request the popup is showing. A request can expire under an open dialog,
// so the popup is keyed to this and not to "whatever is first in the queue".
static uint64_t s_shown_id = 0;

void draw_mcp_approval(McpServer& server)
{
    McpApproval req;
    const bool have = server.pending_approval(req);

    if (have && !ImGui::IsPopupOpen(POPUP_ID)) {
        s_shown_id = req.id;
        ImGui::OpenPopup(POPUP_ID);
    }

    constexpr ImGuiWindowFlags kFlags = ImGuiWindowFlags_AlwaysAutoResize |
                                        ImGuiWindowFlags_NoTitleBar       |
                                        ImGuiWindowFlags_NoMove;

    ui_push_dialog_style();
    if (ImGui::BeginPopupModal(POPUP_ID, nullptr, kFlags)) {
        // Expired, or another request took its place at the front: close, and
        // the next frame opens a fresh dialog for whatever is waiting now.
        if (!have || req.id != s_shown_id) {
            ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
            ui_pop_dialog_style();
            return;
        }

        ui_dialog_title("ASSISTANT REQUEST", 380.0f);
        ui_dialog_text(req.summary.c_str(), ui_col_accent());
        ui_dialog_text(req.target.c_str(), ui_col_value());
        if (!req.reason.empty()) {
            ImGui::Spacing();
            ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + 380.0f);
            ImGui::TextColored(g_theme.col_log, "\"%s\"", req.reason.c_str());
            ImGui::PopTextWrapPos();
        }
        // Advisory, from the separation check: the operator decides, but
        // decides having read it.
        if (!req.warning.empty()) {
            ImGui::Spacing();
            ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + 380.0f);
            ImGui::TextColored(g_theme.col_error, "SEPARATION: %s", req.warning.c_str());
            ImGui::PopTextWrapPos();
        }
        ImGui::Spacing();

        constexpr float BW_OK = 168.0f, BW_NO = 104.0f, GAP = 10.0f;
        ui_dialog_row(BW_OK + GAP + BW_NO);

        ImVec4 hov = g_theme.col_armed;
        hov.x = hov.x * 1.15f > 1.0f ? 1.0f : hov.x * 1.15f;
        const bool allow = ui_solid_button("ALLOW", { BW_OK, UI_DIALOG_BH },
                                           g_theme.col_armed, hov);
        ImGui::SameLine(0, GAP);
        const bool refuse = ui_grid_button("REFUSE", { BW_NO, UI_DIALOG_BH })
                         || ImGui::IsKeyPressed(ImGuiKey_Escape, false);

        // No keyboard shortcut for ALLOW, unlike the other confirm dialogs.
        // This one opens on its own, whenever the assistant asks, and an Enter
        // meant for a field the operator was typing in must not approve it.
        ui_dialog_text("CLICK ALLOW TO SEND    [ESC] REFUSE", ui_col_label());

        if (allow || refuse) {
            server.resolve_approval(req.id, allow);
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    ui_pop_dialog_style();
}
