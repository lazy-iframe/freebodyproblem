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


#include "plugin_rail.hpp"
#include "center_view.hpp"
#include "ui_kit.hpp"
#include "../app_log.hpp"
#include "../../plugins/plugin_api.hpp"
#include "imgui.h"

#include <cstdio>
#include <cstring>
#include <exception>
#include <string>
#include <vector>

// The rail's own title strip lines up with the feed banner across the video
// beside it, so both read as one row.
static constexpr float RAIL_HEADER_H = UI_HEADER_H;
static constexpr float RAIL_PAD      = 8.0f;
static constexpr float LABEL_GAP     = 3.0f;    // button → caption
static constexpr float ENTRY_GAP     = 10.0f;   // caption → next button

// Tag size inside the square. The plate is ~76 px, so the 16 px body caption the
// rest of the app uses reads as an afterthought on it; this is drawn from the
// big readout atlas instead of scaling that one up.
static constexpr float TAG_SZ = UI_SZ_VALUE;

// How long a pressed button holds the engaged red before falling back to blue.
// A plugin that latches itself engaged keeps the red past this; for a momentary
// one this is the whole of the feedback, so it has to outlast a fast click.
static constexpr double PRESS_FLASH_S = 0.35;

// Captions run the full rail width rather than the button's, and drop the
// letter-spacing the rest of the chrome uses: "FUNCTION 1" is already at the
// edge of what 92 px holds, and tracking would cost two characters.
static constexpr float LABEL_TRACK = 0.0f;

// Anything still too long is clipped rather than wrapped — the tooltip carries
// the full text, the same bargain the flight-mode grid makes.
static std::string fit_label(const char* text, float budget)
{
    ImFont* fm = g_font_micro ? g_font_micro : ImGui::GetFont();
    std::string out = text ? text : "";
    while (out.size() > 1 &&
           ui_tracked_width(fm, UI_SZ_MICRO, out.c_str(), LABEL_TRACK) > budget)
        out.pop_back();
    return out;
}

// When each button was last pressed, indexed by slot. Kept here rather than in
// the registry because it is a property of the chrome, not of the plugin.
static std::vector<double> s_last_press;

static bool press_flash_active(size_t slot)
{
    if (slot >= s_last_press.size() || s_last_press[slot] <= 0.0) return false;
    return (ImGui::GetTime() - s_last_press[slot]) < PRESS_FLASH_S;
}

// A plugin is arbitrary user code. Letting it throw through ImGui's frame would
// leave the draw list half-built and take the process with it, so the blast
// radius stops here: log which button broke and keep flying.
static void run_plugin(const PluginSlot& slot, const PluginContext& ctx)
{
    try {
        slot.fn(ctx);
    } catch (const std::exception& e) {
        gcs_log("plugin %s threw: %s", slot.label.c_str(), e.what());
    } catch (...) {
        gcs_log("plugin %s threw", slot.label.c_str());
    }
}

void draw_plugin_rail(const VehicleState& vs, MavlinkSender* sender,
                      float x, float y, float w, float h)
{
    const std::vector<PluginSlot>& slots = gcs_plugins();

    s_last_press.resize(slots.size(), 0.0);

    ImGui::SetNextWindowPos ({ x, y }, ImGuiCond_Always);
    ImGui::SetNextWindowSize({ w, h }, ImGuiCond_Always);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, panel_bg());
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,     { 0.0f, 0.0f });
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,    0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize,  0.0f);

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse;

    if (ImGui::Begin("##plugin_rail", nullptr, flags)) {
        // Title strip, lined up with the feed banner over the video beside it.
        // The same strip the rest of the app draws — dark well, hairline under
        // it, tracked micro title — with the head tick and the title in blue
        // where a panel header would put amber. Hand-drawn only because
        // ui_panel_header_at hardcodes the accent it uses.
        {
            ImDrawList*  dl = ImGui::GetWindowDrawList();
            const ImVec2 wp = ImGui::GetWindowPos();
            ImFont*      fm = g_font_micro ? g_font_micro : ImGui::GetFont();
            const ImVec2 p1 = { wp.x + w, wp.y + RAIL_HEADER_H };

            dl->AddRectFilled(wp, p1, ui_col_strip());
            dl->AddLine({ wp.x, p1.y - 0.5f }, { p1.x, p1.y - 0.5f },
                        ui_col_seam(), 1.0f);
            dl->AddRectFilled({ wp.x + 4.0f, wp.y + 5.0f },
                              { wp.x + 6.0f, p1.y - 5.0f },
                              ui_col(plugin_accent()));

            const char* title = "PLUGINS";
            ui_tracked_text(dl, fm, UI_SZ_MICRO,
                            { wp.x + 12.0f,
                              wp.y + (RAIL_HEADER_H - UI_SZ_MICRO) * 0.5f - 1.0f },
                            ui_col(plugin_accent()), title);
        }

        ImGui::SetCursorPos({ RAIL_PAD, RAIL_HEADER_H + RAIL_PAD });

        if (slots.empty()) {
            ImGui::PushTextWrapPos(w - RAIL_PAD);
            ImGui::TextDisabled("No plugins built. See plugins/README.md");
            ImGui::PopTextWrapPos();
            ImGui::End();
            ImGui::PopStyleVar(3);
            ImGui::PopStyleColor();
            return;
        }

        // One context for every button this frame: nothing in it depends on
        // which plugin runs, and it is only read during the call. Only the slot
        // differs per button, filled in as each is drawn.
        PluginContext ctx = center_view_plugin_context(vs, sender);

        // Square by construction: the button is as tall as the rail is wide,
        // less the margins, so the column stays square at any rail width.
        //
        // The rail is short and the plugin count is open-ended, so the buttons
        // live in a scrolling child rather than being squeezed to fit. A
        // scrollbar would otherwise be drawn over the right edge of the
        // buttons, so when the column will overflow they give up its width.
        const float list_h  = h - RAIL_HEADER_H - RAIL_PAD;
        const float list_w  = w - RAIL_PAD * 2.0f;
        float       btn_w   = list_w;
        const float entry_h = btn_w + LABEL_GAP + UI_SZ_MICRO + ENTRY_GAP;
        if (entry_h * (float)slots.size() > list_h)
            btn_w -= ImGui::GetStyle().ScrollbarSize;

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0.0f, 0.0f });
        if (ImGui::BeginChild("##plugin_list", { list_w, list_h }, false,
                              ImGuiWindowFlags_NoBackground)) {

            for (size_t i = 0; i < slots.size(); ++i) {
                // Copied, not referenced: a plugin can rename its own button
                // mid-call, and the row should finish drawing as the name it
                // was pressed under. The new name appears next frame.
                const PluginSlot s = slots[i];
                ImGui::PushID((int)i);

                char btn_id[96];
                std::snprintf(btn_id, sizeof(btn_id), "%s##plug%zu",
                              s.tag.c_str(), i);

                // Red while the plugin holds itself engaged, and for a moment
                // after any press — the same red the ARMED annunciator uses,
                // because it means the same thing: this is live right now.
                // Otherwise the ordinary dark well, tinted blue instead of
                // amber on its seam and caption.
                const bool hot = s.engaged || press_flash_active(i);
                const bool pressed =
                    hot ? ui_solid_button(btn_id, { btn_w, btn_w },
                                          btn_plugin_engaged_base(),
                                          btn_plugin_engaged_hov(),
                                          CmdFlashState::Normal, TAG_SZ)
                        : ui_tinted_button(btn_id, { btn_w, btn_w },
                                           plugin_accent(), plugin_accent_dim(),
                                           TAG_SZ);
                if (pressed) {
                    gcs_log("plugin: %s", s.label.c_str());
                    s_last_press[i] = ImGui::GetTime();
                    ctx.slot = (int)i;
                    run_plugin(s, ctx);
                }
                if (ImGui::IsItemHovered() && !s.tooltip.empty())
                    ImGui::SetTooltip("%s\n%s", s.label.c_str(), s.tooltip.c_str());

                // Caption, centred under the button. It is measured against the
                // full column width, not the button's, so a name that overhangs
                // the plate by a character still reads.
                {
                    ImGui::Dummy({ 0.0f, LABEL_GAP });
                    ImDrawList*  dl  = ImGui::GetWindowDrawList();
                    ImFont*      fm  = g_font_micro ? g_font_micro : ImGui::GetFont();
                    const std::string cap = fit_label(s.label.c_str(), list_w);
                    const float  cw  = ui_tracked_width(fm, UI_SZ_MICRO,
                                                       cap.c_str(), LABEL_TRACK);
                    const ImVec2 pos = ImGui::GetCursorScreenPos();
                    ui_tracked_text(dl, fm, UI_SZ_MICRO,
                                    { pos.x + (btn_w - cw) * 0.5f, pos.y },
                                    ui_col(plugin_accent_dim()), cap.c_str(),
                                    LABEL_TRACK);
                    ImGui::Dummy({ btn_w, UI_SZ_MICRO + ENTRY_GAP });
                }

                ImGui::PopID();
            }
        }
        ImGui::EndChild();
        ImGui::PopStyleVar();   // ItemSpacing
    }
    ImGui::End();
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor();   // WindowBg
}
