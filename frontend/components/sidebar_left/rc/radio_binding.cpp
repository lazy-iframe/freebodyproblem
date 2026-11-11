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

// RC tab, radio: which channel each stick, mode switch and aux function is on.

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

namespace rc_tab {

// One editable parameter row: name on the left, a value, and a WRITE that only
// lights once the value differs from the vehicle's. Shared by the stick
// bindings, the aux function and the flight-mode slots so all three behave the
// same way under the operator's hands.
static bool draw_write_row(MavlinkSender* sender,
                           const std::unordered_map<std::string, ParamEntry>* params,
                           const char* id, float val, bool connected,
                           uint8_t tsys, uint8_t tcomp, float btn_w)
{
    // Nothing is writable until the vehicle's parameters have been read.
    // Every row here is "change this from what it is now", and without the
    // fetch there is no "now": the row would be offering to overwrite a setting
    // nobody has looked at, with a value taken from a hardcoded default.
    //
    // A parameter missing from a completed fetch is one the vehicle does not
    // have — a PX4 airframe has no FLTMODE3 — so that stays unwritable too.
    const ParamEntry* p     = find_param(params, id);
    const bool        ready = params_ready(params);
    const bool        dirty = ready && p && (val != p->value);

    ImGui::BeginDisabled(!connected || !dirty);
    const bool wrote = dirty
        ? ui_solid_button("WRITE", { btn_w, 0.0f },
                          btn_write_base(), btn_write_hov())
        : ui_grid_button("WRITE", { btn_w, 0.0f });
    if (wrote) {
        commit_write(sender, tsys, tcomp, id, val, param_type_for(params, id));
        st().bind_edits.erase(id);
    }
    ImGui::EndDisabled();
    return wrote;
}

void draw_binding(MavlinkSender* sender, const VehicleState* vs,
                  const std::unordered_map<std::string, ParamEntry>* params,
                  const RcChannels& rc, RcParamDialect dialect,
                  bool connected, uint8_t tsys, uint8_t tcomp)
{
    ImGui::TextColored(accent_col(), "BINDING");
    ImGui::Spacing();

    // ── Stick / axis bindings ────────────────────────────────────────────────
    //
    // Each section pushes its own ID scope. Without it the rows here (keyed by
    // their index in the axis table) and the flight-mode slots below (keyed by
    // slot number) hash to the same ImGui IDs for 1..4, and since both draw a
    // button captioned WRITE, the two sections fight over every click in that
    // range — only the slots outside the overlap ever responded.
    ImGui::PushID("sticks");
    {
        const auto& bindings = rc_stick_bindings();
        const float row_w    = ImGui::GetContentRegionAvail().x;
        const float lbl_w    = 74.0f;
        const float in_w     = 46.0f;
        const float det_w    = 52.0f;
        const float wr_w     = std::max(46.0f, row_w - lbl_w - in_w - det_w - 12.0f);

        for (size_t i = 0; i < bindings.size(); ++i) {
            const RcStickBinding& b  = bindings[i];
            const char*           id = rc_stick_param(b, dialect);

            ImGui::PushID((int)i);

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(b.label);

            if (!id) {
                // PX4 has no parameter for this axis. Say so rather than
                // silently writing the ArduPilot name at a PX4 vehicle.
                ImGui::SameLine(lbl_w);
                ImGui::TextDisabled("n/a on this stack");
                ImGui::PopID();
                continue;
            }

            float val = staged_value(params, id, (float)b.default_ch);

            ImGui::SameLine(lbl_w);
            ImGui::SetNextItemWidth(in_w);
            int ch = (int)val;
            if (ImGui::InputInt("##ch", &ch, 0, 0)) {
                ch = std::max(1, std::min(ch, (int)RcChannels::MAX_CHANNELS));
                st().bind_edits[id] = (float)ch;
                val = (float)ch;
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", id);

            // Arm detection: take a baseline now, then watch for the first
            // channel to move by more than the noise threshold.
            ImGui::SameLine(0, 4);
            const bool armed = (st().detect_target == (int)i);
            if (ui_grid_button(armed ? "MOVE" : "DETECT", { det_w, 0.0f }, armed)) {
                if (armed) {
                    st().detect_target = -1;
                } else if (rc.valid) {
                    st().detect_baseline = rc;
                    st().detect_target   = (int)i;
                    gcs_log("rc: move the %s stick to bind it", b.label);
                }
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip(armed ? "Waiting \xe2\x80\x94 move the stick"
                                        : "Bind by moving the stick");

            ImGui::SameLine(0, 4);
            if (draw_write_row(sender, params, id, val, connected, tsys, tcomp, wr_w))
                gcs_log("rc: binding %s = channel %d", id, (int)val);

            ImGui::PopID();
        }
    }
    ImGui::PopID();

    if (dialect != RcParamDialect::ArduPilot) return;

    const RcFirmware fw = rc_firmware_for(vs && vs->has_heartbeat
                                          ? vehicle_kind(*vs) : VehicleKind::Copter);

    // ── Flight-mode switch slots ─────────────────────────────────────────────
    //
    // ArduPilot bands the mode channel's travel into six positions and keeps a
    // mode number for each. Editing them here, beside the live channel bar that
    // shows which band the switch is in, is the only place the two halves of
    // "what does this switch position do" are visible together.
    ImGui::PushID("fltmode");
    {
        ImGui::Spacing();
        ImGui::TextColored(accent_col(), "FLIGHT MODES");

        // Where the mode switch is sitting right now, so the row it selects can
        // be marked. Read through the live binding, not assumed to be channel 5.
        const auto& bindings = rc_stick_bindings();
        int mode_ch = 5;
        for (const RcStickBinding& b : bindings) {
            if (std::string(b.label) != "MODE") continue;
            if (const char* id = rc_stick_param(b, dialect))
                mode_ch = (int)staged_value(params, id, (float)b.default_ch);
            break;
        }
        const uint16_t mode_pwm  = rc.at(mode_ch);
        const int      live_slot = rc_mode_slot_for_pwm(mode_pwm);

        if (mode_pwm > 0)
            ImGui::TextDisabled("channel %d at %u \xc2\xb5s \xe2\x86\x92 position %d",
                                mode_ch, (unsigned)mode_pwm, live_slot);
        else
            ImGui::TextDisabled("channel %d has no signal", mode_ch);

        // The vehicle's own mode list where it publishes one — custom_mode
        // numbering is per-frame, so a Plane's 3 is not a Copter's 3 and a
        // hardcoded table is right only by luck. The Copter table is the
        // fallback for firmware that predates AVAILABLE_MODES (#435).
        struct ModeChoice { uint32_t mode; std::string label; };
        std::vector<ModeChoice> choices;
        if (vs && !vs->available_modes.empty()) {
            for (const FlightModeInfo& m : vs->available_modes) {
                if (!m.user_selectable()) continue;
                choices.push_back({ m.custom_mode, m.name });
            }
        } else {
            for (const RcModeOption& m : rc_copter_modes())
                choices.push_back({ m.custom_mode, m.label });
        }

        const float row_w = ImGui::GetContentRegionAvail().x;
        const float lbl_w = 74.0f;
        const float num_w = 46.0f;
        const float wr_w  = 60.0f;
        const float cmb_w = std::max(60.0f, row_w - lbl_w - num_w - wr_w - 12.0f);

        for (const RcModeSlot& slot : rc_mode_slots()) {
            ImGui::PushID(slot.index);

            float val = staged_value(params, slot.param, 0.0f);

            // The position the switch is actually in, called out so the row you
            // are editing is the one you can feel under your thumb.
            const bool is_live = (slot.index == live_slot);
            ImGui::AlignTextToFramePadding();
            if (is_live) ImGui::TextColored(accent_col(), "%d \xe2\x97\x8f", slot.index);
            else         ImGui::TextDisabled("%d", slot.index);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s \xe2\x80\x94 PWM %u to %u%s",
                                  slot.param, (unsigned)slot.pwm_lo,
                                  (unsigned)slot.pwm_hi,
                                  is_live ? "\n(switch is here now)" : "");

            const char* preview = "Unknown";
            for (const ModeChoice& c : choices)
                if (c.mode == (uint32_t)val) { preview = c.label.c_str(); break; }

            ImGui::SameLine(lbl_w - 40.0f);
            ImGui::SetNextItemWidth(cmb_w);
            if (ImGui::BeginCombo("##mode", preview, ImGuiComboFlags_HeightLargest)) {
                for (size_t c = 0; c < choices.size(); ++c) {
                    const bool sel = (choices[c].mode == (uint32_t)val);
                    // ##index keeps the ID unique when a vehicle reports two
                    // modes under the same name.
                    const std::string item =
                        choices[c].label + "##" + std::to_string(c);
                    if (ImGui::Selectable(item.c_str(), sel)) {
                        st().bind_edits[slot.param] = (float)choices[c].mode;
                        val = (float)choices[c].mode;
                    }
                    if (sel) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            // The number stays editable: the list above is only as complete as
            // what the vehicle published, and a mode it did not name is still a
            // mode this parameter accepts.
            ImGui::SameLine(0, 4);
            ImGui::SetNextItemWidth(num_w);
            int mode_num = (int)val;
            if (ImGui::InputInt("##mode_num", &mode_num, 0, 0)) {
                mode_num = std::max(0, mode_num);
                st().bind_edits[slot.param] = (float)mode_num;
                val = (float)mode_num;
            }

            ImGui::SameLine(0, 4);
            if (draw_write_row(sender, params, slot.param, val, connected,
                               tsys, tcomp, wr_w))
                gcs_log("rc: %s = mode %d", slot.param, (int)val);

            ImGui::PopID();
        }
    }
    ImGui::PopID();

    // ── Auxiliary function binding ───────────────────────────────────────────
    ImGui::PushID("aux");
    {
        ImGui::Spacing();
        ImGui::TextColored(accent_col(), "AUX FUNCTION");
        ImGui::SameLine();
        ImGui::TextDisabled("(%s)", rc_firmware_label(fw));

        // Channel picker. A list rather than a stepper: this number chooses
        // which parameter is being edited, not a value being tuned, and walking
        // eighteen channels one click at a time to reach the far end is a
        // stepper doing a list's job. Every channel is offered, live or not —
        // binding a switch before its receiver is powered is ordinary setup.
        {
            char preview[16];
            snprintf(preview, sizeof(preview), "CH %d", st().aux_channel);

            ImGui::SetNextItemWidth(92.0f);
            if (ImGui::BeginCombo("##aux_ch", preview)) {
                for (int c = 1; c <= RcChannels::MAX_CHANNELS; ++c) {
                    char item[40];
                    const uint16_t pwm = rc.at(c);
                    // The live reading beside each entry is what tells the
                    // operator which channel is the switch they just flicked.
                    if (pwm > 0)
                        snprintf(item, sizeof(item), "CH %-2d  %u", c, (unsigned)pwm);
                    else
                        snprintf(item, sizeof(item), "CH %-2d  --", c);

                    if (ImGui::Selectable(item, c == st().aux_channel))
                        st().aux_channel = c;
                    if (c == st().aux_channel) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
        }

        // Built after the picker, so the row below always names the channel the
        // list is currently on.
        char aux_id[24];
        if (!rc_aux_param(st().aux_channel, dialect, aux_id, sizeof(aux_id))) {
            ImGui::TextDisabled("Channel %d has no option parameter.", st().aux_channel);
            ImGui::PopID();
            return;
        }

        float val = staged_value(params, aux_id, 0.0f);

        ImGui::SameLine(0, 6);
        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("%s", aux_id);
        if (rc.at(st().aux_channel) > 0) {
            ImGui::SameLine(0, 6);
            ImGui::TextColored(col_data(), "%u", (unsigned)rc.at(st().aux_channel));
        }

        const char* preview = rc_aux_option_label((int)val);
        char preview_buf[48];
        if (!preview) {
            snprintf(preview_buf, sizeof(preview_buf), "Option %d", (int)val);
            preview = preview_buf;
        }

        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::BeginCombo("##aux_opt", preview, ImGuiComboFlags_HeightLargest)) {
            for (const RcAuxOption& o : rc_aux_options()) {
                // Only what this firmware implements. The full table covers four
                // vehicles and most of it is meaningless on any one of them.
                if (!o.supported(fw)) continue;
                const bool sel = (o.value == (uint16_t)val);
                char item[80];
                snprintf(item, sizeof(item), "%3u  %s", (unsigned)o.value, o.label);
                if (ImGui::Selectable(item, sel)) {
                    st().bind_edits[aux_id] = (float)o.value;
                    val = (float)o.value;
                }
                if (sel) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        {
            const float row_w = ImGui::GetContentRegionAvail().x;
            const float in_w  = 70.0f;

            ImGui::SetNextItemWidth(in_w);
            int opt = (int)val;
            if (ImGui::InputInt("##aux_num", &opt, 0, 0)) {
                opt = std::max(0, opt);
                st().bind_edits[aux_id] = (float)opt;
                val = (float)opt;
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("RCn_OPTION value \xe2\x80\x94 any number the\n"
                                  "firmware accepts, listed or not");

            ImGui::SameLine(0, 4);
            if (draw_write_row(sender, params, aux_id, val, connected, tsys, tcomp,
                               row_w - in_w - 4.0f)) {
                const char* name = rc_aux_option_label((int)val);
                gcs_log("rc: %s = %d%s%s", aux_id, (int)val,
                        name ? " " : "", name ? name : "");
            }
        }
    }
    ImGui::PopID();
}

} // namespace rc_tab
