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

// frontend/components/sidebar_left/sensors/common.cpp
//
// What every section shows: the sensors of its kind, from the parameter
// table, and the vehicle's own instruction, boxed.

#include "frontend/components/sidebar_left/sensors/sensors_internal.hpp"
#include <cstdio>

namespace sensors_tab {

// ── What the vehicle is made of ──────────────────────────────────────────────

// The sensors of one kind, as the parameter table describes them. This is where
// the numbers come from and it is worth being plain about it: there is no
// MAVLink message that lists a vehicle's compasses, so an empty table here
// means the parameters have not been fetched, not that the vehicle has none.
void draw_sensor_devices(const ParamMap* params, SensorKind kind,
                                const char* id, const char* noun)
{
    const std::vector<SensorDevice> devices = sensor_inventory(params, kind);

    if (devices.empty()) {
        if (!params || params->empty())
            ImGui::TextDisabled("Fetch the parameters to list the %s.", noun);
        else
            ImGui::TextDisabled("No %s found in the parameters.", noun);
        return;
    }

    ImGui::PushStyleColor(ImGuiCol_ChildBg, bg_param_list());
    ImGui::PushStyleColor(ImGuiCol_Border,  col_separator());
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, FRAME_BORDER_NORMAL);

    const float h = devices.size() * ImGui::GetTextLineHeightWithSpacing() + 10.0f;
    if (ImGui::BeginChild(id, { -1.0f, h }, true, kNoScroll)) {
        for (const SensorDevice& d : devices) {
            // A device type the table does not know is shown as its number
            // rather than left blank: it is still the chip's identity, and a
            // blank would read as a sensor that failed to identify itself.
            char chip[32];
            if (d.devtype_name[0]) snprintf(chip, sizeof(chip), "%s", d.devtype_name);
            else                   snprintf(chip, sizeof(chip), "type 0x%02X", d.devtype);

            // Greyed when the vehicle is not using it, which is a thing worth
            // seeing before calibrating: a disabled compass calibrates and
            // then goes on being ignored.
            const bool dim = d.has_use && !d.in_use;
            if (dim) ImGui::PushStyleColor(ImGuiCol_Text, ui_col_label());

            ImGui::Text("%d  %-12s %s bus %u  0x%02X%s%s",
                        d.instance, chip, d.bus_name, d.bus, d.address,
                        (d.has_external && d.external) ? "  ext" : "",
                        dim ? "  unused" : "");

            if (dim) ImGui::PopStyleColor();

            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s = %u", d.param.c_str(), d.devid);
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);
}

// The vehicle's own sentence, boxed. Shown verbatim rather than paraphrased —
// it is the vehicle that knows what it wants, and a GCS that rewrote the
// instruction would have one more thing to keep in step with firmware.
void draw_vehicle_message(const char* id, const std::string& msg, ImVec4 col)
{
    ImGui::PushStyleColor(ImGuiCol_ChildBg, bg_child_dark());
    ImGui::PushStyleColor(ImGuiCol_Border,  col);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, FRAME_BORDER_NORMAL);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,   { 6.0f, 5.0f });

    const char* text = msg.empty() ? "Waiting for the vehicle\xe2\x80\xa6" : msg.c_str();
    const float text_w = ImGui::GetContentRegionAvail().x - 12.0f;
    const float box_h  = ImGui::CalcTextSize(text, nullptr, false, text_w).y + 10.0f;

    if (ImGui::BeginChild(id, { -1.0f, box_h }, true, kNoScroll)) {
        ImGui::PushTextWrapPos(text_w);
        ImGui::PushStyleColor(ImGuiCol_Text, msg.empty() ? ui_col_label() : ui_col(col));
        ImGui::TextUnformatted(text);
        ImGui::PopStyleColor();
        ImGui::PopTextWrapPos();
    }
    ImGui::EndChild();

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
}

} // namespace sensors_tab
