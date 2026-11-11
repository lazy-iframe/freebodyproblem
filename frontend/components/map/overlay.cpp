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

// Labels and lines drawn over the map, by the flat view and the 3D view alike.

#include "frontend/components/map/map_internal.hpp"
#include "frontend/source/terrain.hpp"
#include "frontend/source/terrain_clearance.hpp"
#include "frontend/source/geodesic.hpp"
#include <algorithm>
#include <cmath>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#ifndef M_PI
#  define M_PI 3.14159265358979323846
#endif

namespace map_detail {

float s_ac_right = 0.0f;   // see map_ac_block_right()

// `text` split into lines no wider than `wrap_w`, as the radio tab's notice
// splits its own: the longest run of whole words that fits, a word too long
// for a line broken by character, no line opening or closing on a space.
static std::vector<std::pair<const char*, const char*>> wrap_lines(const char* text, float wrap_w)
{
    std::vector<std::pair<const char*, const char*>> lines;
    for (const char* p = text; *p; ) {
        while (*p == ' ') ++p;
        if (!*p) break;
        const char* fit = nullptr;
        for (const char* q = p; ; ) {
            while (*q && *q != ' ') ++q;
            if (ImGui::CalcTextSize(p, q).x > wrap_w) break;
            fit = q;
            while (*q == ' ') ++q;
            if (!*q) break;
        }
        if (!fit) {
            fit = p + 1;
            while (*fit && *fit != ' ' && ImGui::CalcTextSize(p, fit + 1).x <= wrap_w) ++fit;
        }
        lines.push_back({ p, fit });
        p = fit;
    }
    return lines;
}

constexpr float TAG_PAD_X = 5.0f, TAG_PAD_Y = 3.0f;

// The tags over the map are drawn at this opacity, box and text alike, so the
// imagery under them shows through a little.
constexpr float TAG_OPACITY = 0.8f;

// The part of the line a to b inside the rectangle, as fractions along it. False if
// none of it is.
static bool clip_to_rect(ImVec2 a, ImVec2 b, ImVec2 r0, ImVec2 r1, double& t0, double& t1)
{
    t0 = 0.0;
    t1 = 1.0;
    const auto axis = [&](double p, double d, double lo, double hi) {
        if (std::fabs(d) < 1e-6) return p >= lo && p <= hi;
        const double u = (lo - p) / d, v = (hi - p) / d;
        t0 = std::max(t0, std::min(u, v));
        t1 = std::min(t1, std::max(u, v));
        return t0 <= t1;
    };
    return axis(a.x, (double)b.x - a.x, r0.x, r1.x) && axis(a.y, (double)b.y - a.y, r0.y, r1.y);
}

} // namespace map_detail

using namespace map_detail;

float map_ac_block_right() { return s_ac_right; }

ImVec2 map_tag_size(const char* text)
{
    const ImVec2 ts = ImGui::CalcTextSize(text);
    return { ts.x + TAG_PAD_X * 2.0f, ts.y + TAG_PAD_Y * 2.0f };
}

void map_draw_tag(ImDrawList* dl, ImVec2 left_mid, const char* text, ImVec4 col, float alpha)
{
    alpha *= TAG_OPACITY;
    const ImVec2 sz  = map_tag_size(text);
    const ImVec2 a   = { left_mid.x, left_mid.y - sz.y * 0.5f };
    const ImVec2 b   = { a.x + sz.x, a.y + sz.y };
    const float  rnd = ImGui::GetStyle().ChildRounding;
    dl->AddRectFilled(a, b, ui_col(g_theme.bg_topbar, alpha), rnd);
    dl->AddRectFilled(a, b, ImGui::GetColorU32({ col.x, col.y, col.z, 0.14f * alpha }), rnd);
    dl->AddRect      (a, b, ImGui::GetColorU32({ col.x, col.y, col.z, alpha }), rnd, 0, FRAME_BORDER_NORMAL);
    dl->AddText({ a.x + TAG_PAD_X, a.y + TAG_PAD_Y }, ImGui::GetColorU32({ col.x, col.y, col.z, alpha }), text);
}

std::string map_home_distance(const MapVehicle* mv, double lat, double lon)
{
    if (!mv || !mv->has_home) return {};
    const double d = geodesic_distance_m(mv->home_lat, mv->home_lon, lat, lon);
    char t[32];
    if (d < 1000.0) snprintf(t, sizeof(t), "HOME %.0f m", d);
    else            snprintf(t, sizeof(t), "HOME %.2f km", d / 1000.0);
    return t;
}

void map_dotted_line(ImDrawList* dl, ImVec2 a, ImVec2 b, ImU32 col, float w)
{
    // Only the part in view, and the dots always this far apart, whatever the width, so
    // a warning's dots land on the path's. One end of a line can be very far off: cut
    // at the camera plane in 3D, an end behind the camera lands millions of pixels away.
    constexpr double ON = 3.0, PERIOD = 8.0;
    const double dx = (double)b.x - a.x, dy = (double)b.y - a.y;
    const double len = std::sqrt(dx * dx + dy * dy);
    if (len < 1.0) return;

    double t0, t1;
    if (!clip_to_rect(a, b, dl->GetClipRectMin(), dl->GetClipRectMax(), t0, t1)) return;

    // Counted from a's end, so a dot stays put as the panel's edge passes over the line.
    const double last = t1 * len;
    for (double s = std::floor(t0 * len / PERIOD) * PERIOD; s < last; s += PERIOD) {
        const double u = std::max(s, 0.0), v = std::min(s + ON, len);
        dl->AddLine({ (float)(a.x + dx * u / len), (float)(a.y + dy * u / len) },
                    { (float)(a.x + dx * v / len), (float)(a.y + dy * v / len) }, col, w);
    }
}

void map_draw_los(ImDrawList* dl, ImVec2 home, ImVec2 vehicle, const MapVehicle& mv)
{
    constexpr float MIN_PX = 40.0f;   // parked on its pad, a label would only cover it
    constexpr float HOME_MAST_M = 2.0f;   // off the ground: an operator or an antenna
    const float dx = vehicle.x - home.x, dy = vehicle.y - home.y;
    if (!mv.has_pos || !mv.has_alt || !mv.has_home || dx * dx + dy * dy < MIN_PX * MIN_PX) return;

    // Each end stood on the terrain data at least. Home is on the ground and the data
    // is tens of metres out in places, so otherwise the line can begin inside it, or
    // end inside it under an aircraft that has landed, and always read as blocked.
    const std::optional<float> gh = terrain_amsl(mv.home_lat, mv.home_lon);
    const std::optional<float> gv = terrain_amsl(mv.lat, mv.lon);
    bool blocked = false, known = false;
    if (gh && gv) {
        const float h0 = std::max(mv.has_home_alt ? mv.home_alt_amsl : *gh, *gh) + HOME_MAST_M;
        const float h1 = std::max(mv.alt_amsl, *gv);
        const Clearance c = line_of_sight_clearance(mv.home_lat, mv.home_lon, h0, mv.lat, mv.lon, h1);
        blocked = c.collides();
        known   = blocked || c.complete;   // one hill is enough, the rest still loading or not
    }

    constexpr ImVec4 CYAN = { 0.0f, 0.9f, 1.0f, 1.0f };
    const ImVec4 col = blocked ? g_theme.col_error : CYAN;
    map_dotted_line(dl, home, vehicle, ui_col(col, 0.9f), INDICATOR_LINE_W);

    const char*  text = blocked ? "NO LOS" : known ? "LOS" : "LOS ?";
    const ImVec2 ts   = map_tag_size(text);

    // At the middle of the part in view: with one end off the panel the true middle
    // may be too, and the line's whole message with it. Nothing if none of it is in view.
    double t0, t1;
    if (!clip_to_rect(home, vehicle, dl->GetClipRectMin(), dl->GetClipRectMax(), t0, t1)) return;
    const float tm = (float)(0.5 * (t0 + t1));
    map_draw_tag(dl, { home.x + dx * tm - ts.x * 0.5f, home.y + dy * tm }, text, col, 1.0f);
}

void map_draw_tag(ImDrawList* dl, ImVec2 left_mid, const char* text)
{
    map_draw_tag(dl, left_mid, text, col_ok());
}

void map_draw_vehicle_tags(ImDrawList* dl, ImVec2 centre, float above, const MapVehicle& mv)
{
    if (mv.number == 0 && mv.sysid == 0) return;
    const float A = mv.active ? 1.0f : 0.55f;

    // The GCS's word on the ground, not the vehicle's: its AMSL less the
    // terrain data there, so it differs from the vehicle's own "AGL" (above
    // home) wherever home and the ground under it differ.
    char t[80] = "";
    float agl = 0.0f;
    const std::string home = mv.has_pos ? map_home_distance(&mv, mv.lat, mv.lon) : std::string();
    if (mv.has_pos && mv.has_alt)
        if (const std::optional<float> g = terrain_amsl(mv.lat, mv.lon)) {
            agl = mv.alt_amsl - *g;
            if (agl < 0.0f) snprintf(t, sizeof(t), "BELOW TERRAIN ~%.0f m", (double)-agl);
            else            snprintf(t, sizeof(t), "ALT AGL TER ~%.0f m", (double)agl);
        }
    if (!home.empty()) {
        const size_t len = strlen(t);
        snprintf(t + len, sizeof(t) - len, "%s%s", len ? "  \xc2\xb7  " : "", home.c_str());
    }

    // Stacked upward from the vehicle: the height nearest it, the name on top.
    float bottom = centre.y - above;
    if (*t) {
        const ImVec2 ts = map_tag_size(t);
        map_draw_tag(dl, { centre.x - ts.x * 0.5f, bottom - ts.y * 0.5f }, t,
                     agl < 0.0f ? g_theme.col_error : col_ok(), A);
        bottom -= ts.y + 2.0f;
    }
    char lbl[32];
    snprintf(lbl, sizeof(lbl), "(%u) SYS %u", (unsigned)mv.number, (unsigned)mv.sysid);
    const ImVec2 ls = map_tag_size(lbl);
    map_draw_tag(dl, { centre.x - ls.x * 0.5f, bottom - ls.y * 0.5f }, lbl, col_ok(), A);
}

void map_draw_instruction(ImVec2 p0, ImVec2 size, const char* text)
{
    constexpr float PAD_X = 6.0f, PAD_Y = 5.0f;
    ImDrawList*  dl  = ImGui::GetForegroundDrawList();
    const ImVec4 ok  = col_ok();

    // At most half the map's width: the text wrapped to that and centred line
    // by line, the box only as wide as its widest line and as tall as the
    // lines it took.
    const float max_w = std::floor(size.x * 0.5f);
    const auto  lines = wrap_lines(text, max_w - PAD_X * 2.0f);
    float       text_w = 0.0f;
    for (const auto& ln : lines) text_w = std::max(text_w, ImGui::CalcTextSize(ln.first, ln.second).x);
    const float bw    = std::min(max_w, std::ceil(text_w) + PAD_X * 2.0f);
    const float lh    = ImGui::GetTextLineHeight();
    const float bh    = lines.size() * lh + PAD_Y * 2.0f;
    const float x     = std::max(p0.x + (size.x - bw) * 0.5f, s_ac_right + 8.0f);
    const float y     = p0.y + OVL_M;
    const float rnd   = ImGui::GetStyle().ChildRounding;

    // The notice's tint is over a panel; here it is over imagery, so a dark
    // backing first, or the tint would vanish into whatever is under it.
    dl->AddRectFilled({ x, y }, { x + bw, y + bh }, ui_col(g_theme.bg_topbar, 0.92f), rnd);
    dl->AddRectFilled({ x, y }, { x + bw, y + bh }, ImGui::GetColorU32({ ok.x, ok.y, ok.z, 0.14f }), rnd);
    dl->AddRect      ({ x, y }, { x + bw, y + bh }, ImGui::GetColorU32(ok), rnd, 0, FRAME_BORDER_NORMAL);
    for (size_t i = 0; i < lines.size(); ++i) {
        const float lw = ImGui::CalcTextSize(lines[i].first, lines[i].second).x;
        dl->AddText({ x + (bw - lw) * 0.5f, y + PAD_Y + i * lh }, ImGui::GetColorU32(ok),
                    lines[i].first, lines[i].second);
    }
}
