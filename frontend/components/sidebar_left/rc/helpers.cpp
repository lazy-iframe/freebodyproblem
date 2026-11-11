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

// RC tab: helpers both the radio and the joystick sections use.

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

// ── Helpers ──────────────────────────────────────────────────────────────────

// The vehicle's own value for a parameter, when the PARAMS tab has fetched it.
const ParamEntry* find_param(
    const std::unordered_map<std::string, ParamEntry>* params, const char* id)
{
    if (!params || !id) return nullptr;
    auto it = params->find(id);
    return (it == params->end()) ? nullptr : &it->second;
}

// True once a complete parameter set is in hand.
//
// main.cpp only publishes the map when a fetch actually finishes — a vehicle
// volunteers the odd PARAM_VALUE on its own, and those are counted but not
// published — so an empty map here is exactly "nobody has pressed FETCH ALL".
bool params_ready(const std::unordered_map<std::string, ParamEntry>* params)
{
    return params && !params->empty();
}

// The type the vehicle reported for a parameter, falling back to REAL32.
// Writing a parameter under a type the vehicle does not agree with is how a
// perfectly good value gets rejected, so prefer what it told us over what we
// assumed.
uint8_t param_type_for(
    const std::unordered_map<std::string, ParamEntry>* params, const char* id)
{
    const ParamEntry* p = find_param(params, id);
    return p ? p->type : (uint8_t)MAV_PARAM_TYPE_REAL32;
}

// One PARAM_SET or PARAM_EXT_SET, per the protocol switch above. The two
// messages have deliberately identical signatures on MavlinkSender, so the
// choice is this one line and nothing downstream has to care.
void commit_write(MavlinkSender* sender, uint8_t tsys, uint8_t tcomp,
                  const char* id, float value, uint8_t type)
{
    if (st().use_param_ext) sender->set_param_ext(tsys, tcomp, id, value, type);
    else                 sender->set_param    (tsys, tcomp, id, value, type);
}

// The value a parameter row should show: the operator's staged edit if there is
// one, otherwise what the vehicle reported, otherwise the given default.
float staged_value(const std::unordered_map<std::string, ParamEntry>* params,
                   const char* id, float fallback)
{
    auto ed = st().bind_edits.find(id);
    if (ed != st().bind_edits.end()) return ed->second;
    if (const ParamEntry* p = find_param(params, id)) return p->value;
    return fallback;
}

// Wrapped text with every line centred.
//
// ImGui's own wrapping is left-aligned only: it hands the whole string to the
// draw list with a wrap width and never reports where it broke, so there is no
// line to centre after the fact. Doing the breaking here gives us both.
//
// Returns the number of lines. Called once to measure and once to draw, so the
// box drawn around it is always exactly the height of what goes inside — a
// fixed guess would clip on a narrow sidebar and leave a gap on a wide one.
int centered_wrapped(const char* text, float wrap_w, ImVec4 col, bool draw)
{
    int lines = 0;

    if (draw) ImGui::PushStyleColor(ImGuiCol_Text, col);
    const float x0 = ImGui::GetCursorPosX();

    for (const char* p = text; *p; ) {
        while (*p == ' ') ++p;              // a wrapped line does not open on a space
        if (!*p) break;

        // The longest run of whole words that still fits.
        const char* fit = nullptr;
        for (const char* q = p; ; ) {
            while (*q && *q != ' ') ++q;
            if (ImGui::CalcTextSize(p, q).x > wrap_w) break;
            fit = q;
            if (!*q) break;
            ++q;                            // step over the space, try one more word
        }
        // A single word wider than the box: break it by character rather than
        // let it run out past the border.
        if (!fit) {
            fit = p + 1;
            while (*fit && ImGui::CalcTextSize(p, fit + 1).x <= wrap_w) ++fit;
        }

        // Trailing spaces are part of the run but not part of what you see;
        // measuring them would push the visible text left of centre.
        const char* vis_end = fit;
        while (vis_end > p && vis_end[-1] == ' ') --vis_end;

        if (draw) {
            const float w = ImGui::CalcTextSize(p, vis_end).x;
            ImGui::SetCursorPosX(x0 + std::max(0.0f, (wrap_w - w) * 0.5f));
            ImGui::TextUnformatted(p, vis_end);
        }
        ++lines;
        p = fit;
    }

    if (draw) ImGui::PopStyleColor();
    return lines;
}

// ── Live channel bar ─────────────────────────────────────────────────────────

// The display window. Wider than the 1000-2000 a receiver nominally produces,
// so a channel travelling past its nominal ends stays on the bar instead of
// pinning to an edge and looking calibrated when it is not.
static constexpr float BAR_LO = 900.0f;
static constexpr float BAR_HI = 2100.0f;

static float bar_frac(float pwm)
{
    return std::max(0.0f, std::min((pwm - BAR_LO) / (BAR_HI - BAR_LO), 1.0f));
}

// One channel: number, a bar, and the raw microseconds. During a sweep the
// measured travel is drawn behind the live position, so the operator can see
// which channels still have not been taken to their stops.
void draw_channel_row(const char* lbl, uint16_t pwm,
                      const RcChannelCal* cal, const char* bind_label)
{
    constexpr float LBL_W = 30.0f;
    constexpr float VAL_W = 42.0f;
    constexpr float GAP   = 4.0f;

    ImDrawList*  dl    = ImGui::GetWindowDrawList();
    const ImVec2 p0    = ImGui::GetCursorScreenPos();
    const float  row_w = ImGui::GetContentRegionAvail().x;
    const float  bar_w = row_w - LBL_W - VAL_W - GAP * 2.0f;
    if (bar_w <= 0.0f) { ImGui::Dummy({ row_w, ROW_H }); return; }

    const float bar_x0 = p0.x + LBL_W + GAP;
    const float bar_x1 = bar_x0 + bar_w;
    const float text_y = p0.y + (ROW_H - ImGui::GetTextLineHeight()) * 0.5f;

    // A channel with no pulse is drawn as an empty well rather than skipped:
    // the panel shows every channel the protocol can carry, so the gap between
    // "channel 9 is quiet" and "this radio only has 8" stays visible.
    const bool live = (pwm > 0);

    dl->AddText({ p0.x, text_y },
                bind_label ? ui_col_accent()
                           : (live ? ui_col_label() : ui_col(g_theme.col_log, 0.45f)),
                lbl);

    dl->AddRectFilled({ bar_x0, p0.y }, { bar_x1, p0.y + ROW_H }, ui_col_well());

    // Measured travel, behind the live marker.
    if (cal && cal->seen && cal->max > cal->min) {
        const float mx0 = bar_x0 + bar_w * bar_frac((float)cal->min);
        const float mx1 = bar_x0 + bar_w * bar_frac((float)cal->max);
        dl->AddRectFilled({ mx0, p0.y }, { mx1, p0.y + ROW_H },
                          ui_col(g_theme.accent, 0.22f));
    }

    // Live position — a tick rather than a fill from zero. A receiver's centre
    // is 1500 µs, not 1000, so a left-anchored fill would read as "half output"
    // on a stick that is sitting still in the middle.
    if (live) {
        const float x = bar_x0 + bar_w * bar_frac((float)pwm);
        dl->AddRectFilled({ x - 1.5f, p0.y }, { x + 1.5f, p0.y + ROW_H },
                          ui_col_value());
    }

    dl->AddRect({ bar_x0, p0.y }, { bar_x1, p0.y + ROW_H }, ui_col_seam());

    // Trim marker, once a centre has been captured.
    if (cal && cal->seen && cal->trim > 0) {
        const float tx = bar_x0 + bar_w * bar_frac((float)cal->trim);
        dl->AddLine({ tx, p0.y }, { tx, p0.y + ROW_H }, ui_col_accent());
    }

    char val[12];
    if (live) snprintf(val, sizeof(val), "%u", (unsigned)pwm);
    else      snprintf(val, sizeof(val), "--");
    dl->AddText({ bar_x1 + GAP, text_y },
                live ? ui_col_value() : ui_col(g_theme.col_log, 0.45f), val);

    ImGui::Dummy({ row_w, ROW_H });

    if (bind_label && ImGui::IsItemHovered())
        ImGui::SetTooltip("Channel %s \xe2\x80\x94 %s", lbl, bind_label);
}

} // namespace rc_tab
