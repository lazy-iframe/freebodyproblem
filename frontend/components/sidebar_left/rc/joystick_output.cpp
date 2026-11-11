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

// RC tab, joystick: RC override channels and what goes out.

#include "frontend/components/sidebar_left/sidebar_internal.hpp"
#include "frontend/components/sidebar_themes.hpp"
#include "frontend/source/vehicle_ui_state.hpp"
#include "frontend/source/app_log.hpp"
#include "frontend/source/audio.hpp"
#include "frontend/source/joystick.hpp"
#include "backend/vehicle/firmware_profile.hpp"
#include "backend/input/rc_binding.hpp"
#include "backend/calibration/rc.hpp"
#include "imgui.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>
#include "frontend/components/sidebar_left/rc/rc_internal.hpp"
#include "frontend/components/sidebar_left/rc/joystick_internal.hpp"

namespace rc_tab {

// One channel's row: the RC channel it fills, its direction, and what it sends.
// `taken` marks RC channels another row has, which cannot be chosen twice.
static void joy_rc_row(const char* lbl, JoyRcOut out, uint32_t taken, uint16_t value,
                       bool is_virtual, int index)
{
    constexpr float LBL_W = 32.0f, CMB_W = 92.0f;

    ImGui::PushID(is_virtual ? 100 + index : index);
    ImGui::AlignTextToFramePadding();
    if (out.channel) ImGui::TextColored(accent_col(), "%s", lbl);
    else             ImGui::TextDisabled("%s", lbl);

    char preview[16] = "--";
    if (out.channel) snprintf(preview, sizeof(preview), "RC %d", out.channel);

    bool changed = false;
    ImGui::SameLine(LBL_W);
    ImGui::SetNextItemWidth(CMB_W);
    if (ImGui::BeginCombo("##rc_ch", preview, ImGuiComboFlags_HeightLargest)) {
        if (ImGui::Selectable("--", out.channel == 0)) { out.channel = 0; changed = true; }
        for (int ch = 1; ch <= JOY_RC_MAX_CHANNEL; ++ch) {
            const bool other = (taken & (1u << ch)) != 0 && ch != out.channel;
            char item[16];
            snprintf(item, sizeof(item), "RC %d", ch);
            if (ImGui::Selectable(item, ch == out.channel,
                                  other ? ImGuiSelectableFlags_Disabled : 0)) {
                out.channel = ch;
                changed = true;
            }
        }
        ImGui::EndCombo();
    }

    ImGui::SameLine(0, 4);
    ImGui::BeginDisabled(out.channel == 0);
    changed |= ImGui::Checkbox("REV", &out.reversed);
    ImGui::EndDisabled();

    if (out.channel && value != JOY_RC_IGNORE) {
        ImGui::SameLine(0, 8);
        ImGui::TextColored(col_data(), "%u", (unsigned)value);
    }

    if (changed) {
        if (is_virtual) joystick_set_virtual_rc(index, out);
        else            joystick_set_axis_rc(index, out);
    }
    ImGui::PopID();
}

void draw_joystick_rc_override(const RcChannels& frame, bool editable)
{
    const JoystickProfile& profile = joystick_profile();

    ImGui::Spacing();
    ImGui::PushID("joyrc");
    ImGui::BeginDisabled(!editable);

    bool on = profile.rc_override;
    if (ImGui::Checkbox("RC OVERRIDE", &on)) joystick_set_rc_override(on);
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        ImGui::SetTooltip("Disconnect any other RC transmitter before using this.\n"
                          "A radio left on is overridden while this sends, and takes\n"
                          "those channels back a few seconds after it stops.\n\n"
                          "Sends RC_CHANNELS_OVERRIDE (#70) instead of MANUAL_CONTROL.");

    if (on) {
        uint16_t values[RcChannels::MAX_CHANNELS];
        joystick_rc_values(values);

        uint32_t taken = 0;
        for (int a = 0; a < frame.usable_count(); ++a)
            taken |= 1u << profile.rc_axes[a].channel;
        for (const JoyVirtualChannel& v : profile.virtuals)
            if (v.used()) taken |= 1u << v.rc.channel;

        const auto value_of = [&](const JoyRcOut& o) {
            return o.channel >= 1 && o.channel <= JOY_RC_MAX_CHANNEL
                 ? values[o.channel - 1] : JOY_RC_IGNORE;
        };
        for (int a = 0; a < frame.usable_count(); ++a) {
            char lbl[8];
            snprintf(lbl, sizeof(lbl), "%d", a + 1);
            joy_rc_row(lbl, profile.rc_axes[a], taken, value_of(profile.rc_axes[a]), false, a);
        }
        for (int i = 0; i < JOY_MAX_VIRTUAL; ++i) {
            const JoyVirtualChannel& v = profile.virtuals[i];
            if (!v.used()) continue;
            char lbl[8];
            snprintf(lbl, sizeof(lbl), "V%d", i + 1);
            joy_rc_row(lbl, v.rc, taken, value_of(v.rc), true, i);
        }
    }

    ImGui::EndDisabled();
    ImGui::PopID();
}

// What is on its way to the vehicle: the fields of the message itself, not the
// stick. Off unless asked for.
void draw_joystick_output(bool profile_ok)
{
    static bool s_show = false;

    ImGui::Spacing();
    if (ui_grid_button("READ OUTPUT##joy", { -1.0f, 0.0f }, s_show)) s_show = !s_show;
    if (!s_show) return;

    const bool live = joystick_live();
    const bool rc   = joystick_profile().rc_override;

    // While sending this is the frame the link last wrote. Otherwise nothing is
    // going anywhere, so it is a preview, and has to say so: the numbers look the
    // same either way.
    MavlinkSender::ManualControlFrame f;
    if (live) {
        f = joystick_output_frame();
        ImGui::TextColored(col_warning(), "SENDING to vehicle (%u)",
                           (unsigned)joystick_live_number());
    } else {
        const JoyCommand c = joystick_command();
        const uint32_t   b = joystick_output_buttons();
        f.x = c.x; f.y = c.y; f.z = c.z; f.r = c.r;
        f.buttons  = (uint16_t)(b & 0xFFFFu);
        f.buttons2 = (uint16_t)(b >> 16);
        ImGui::TextDisabled("PREVIEW. Nothing is being sent.");
    }

    ImGui::PushStyleColor(ImGuiCol_ChildBg, bg_param_list());
    ImGui::PushStyleColor(ImGuiCol_Border,  col_separator());
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, FRAME_BORDER_NORMAL);

    const float line = ImGui::GetTextLineHeightWithSpacing();
    // The same frame's other half. Only the message the profile sends is shown:
    // the other is not going out, however its numbers would read.
    uint16_t rc_chan[RcChannels::MAX_CHANNELS];
    int      rc_rows = 0;
    if (rc) {
        if (live) {
            const MavlinkSender::RcOverrideFrame rf = joystick_output_rc_frame();
            std::copy(rf.chan, rf.chan + RcChannels::MAX_CHANNELS, rc_chan);
            f.target = rf.target;
        } else {
            joystick_rc_values(rc_chan);
        }
        for (uint16_t v : rc_chan)
            if (v != JOY_RC_IGNORE && v != 0) ++rc_rows;
    }

    const float mc_lines = rc ? 3.0f + (float)rc_rows : 8.0f;
    if (ImGui::BeginChild("##joy_out_mc", { -1.0f, line * mc_lines + 10.0f }, true,
                          ImGuiWindowFlags_NoScrollbar)) {
        ImGui::TextColored(accent_col(), rc ? "RC_CHANNELS_OVERRIDE #70"
                                            : "MANUAL_CONTROL #69");
        if (live) {
            const char* rate_hint = joystick_output_has_frame() ? "" : "  waiting for link";
            ImGui::SameLine(0, 6);
            ImGui::TextColored(col_data(), "%.1f Hz%s", joystick_output_hz(), rate_hint);
        }

        const auto field = [](const char* name, const char* meaning, int value) {
            ImGui::TextDisabled("%-8s", name);
            ImGui::SameLine(72.0f);
            ImGui::TextColored(col_data(), "%+6d", value);
            ImGui::SameLine(0, 8);
            ImGui::TextDisabled("%s", meaning);
        };
        if (live) {
            ImGui::TextDisabled("%-8s", "target");
            ImGui::SameLine(72.0f);
            ImGui::TextColored(col_data(), "%6u", (unsigned)f.target);
        } else {
            ImGui::TextDisabled("target    none");
        }

        if (rc) {
            for (int i = 0; i < RcChannels::MAX_CHANNELS; ++i) {
                if (rc_chan[i] == JOY_RC_IGNORE || rc_chan[i] == 0) continue;
                char name[16];
                snprintf(name, sizeof(name), "chan%d_raw", i + 1);
                ImGui::TextDisabled("%-11s", name);
                ImGui::SameLine(104.0f);
                ImGui::TextColored(col_data(), "%6u", (unsigned)rc_chan[i]);
            }
            ImGui::TextDisabled("All others UINT16_MAX: left alone.");
        } else if (!live && !profile_ok) {
            ImGui::TextDisabled("Calibrate and bind the sticks");
            ImGui::TextDisabled("to see their values.");
        } else {
            field("x", "pitch",    f.x);
            field("y", "roll",     f.y);
            field("z", "throttle", f.z);
            field("r", "yaw",      f.r);
        }
        if (!rc) {
            ImGui::TextDisabled("%-8s", "buttons");
            ImGui::SameLine(72.0f);
            ImGui::TextColored(col_data(), "0x%04X", (unsigned)f.buttons);
            ImGui::TextDisabled("%-8s", "buttons2");
            ImGui::SameLine(72.0f);
            ImGui::TextColored(col_data(), "0x%04X", (unsigned)f.buttons2);
        }
    }
    ImGui::EndChild();

    // Commands that have actually gone, whether or not sending has since stopped.
    const auto& cmds = joystick_output_commands();
    if (ImGui::BeginChild("##joy_out_cmd", { -1.0f, line * 5.0f + 10.0f }, true)) {
        ImGui::TextColored(accent_col(), "COMMAND_LONG #76");
        if (cmds.empty()) ImGui::TextDisabled("None sent yet.");
        const double now = joystick_now();
        for (const JoyOutputCommand& c : cmds) {
            ImGui::TextDisabled("%4.0fs ago  %s", now - c.time, c.source.c_str());
            ImGui::TextWrapped("%s", c.text.c_str());
        }
    }
    ImGui::EndChild();

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);
}

} // namespace rc_tab
