// Free Body Problem: A modern, simple and minimalist Ground Control Station
// Copyright (C) 2026  Azhar Tanweer
// Contact: azhar.tanweer404@gmail.com

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


#include "settings.hpp"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <cstdlib>

namespace fs = std::filesystem;
using json   = nlohmann::json;

// ── Path helper ───────────────────────────────────────────────────────────────

std::string settings_path()
{
#ifdef _WIN32
    const char* base = std::getenv("APPDATA");
    if (base) return std::string(base) + "/freebodyproblem/settings.json";
#else
    const char* cfg = std::getenv("XDG_CONFIG_HOME");
    if (cfg && cfg[0]) return std::string(cfg) + "/freebodyproblem/settings.json";
    const char* home = std::getenv("HOME");
    if (home) return std::string(home) + "/.config/freebodyproblem/settings.json";
#endif
    return "./freebodyproblem_settings.json";
}

// ── ThemeVars serialization ───────────────────────────────────────────────────

static json vec4_to_json(const ImVec4& v)
{
    return json::array({ v.x, v.y, v.z, v.w });
}

static ImVec4 json_to_vec4(const json& j, const ImVec4& fallback)
{
    if (!j.is_array() || j.size() != 4) return fallback;
    return { j[0].get<float>(), j[1].get<float>(),
             j[2].get<float>(), j[3].get<float>() };
}

// Macro to reduce boilerplate: serialize one ImVec4 field.
#define TV_TO_J(field)   j[#field] = vec4_to_json(t.field)
#define TV_FROM_J(field) if (j.contains(#field)) t.field = json_to_vec4(j[#field], t.field)

static json theme_to_json(const ThemeVars& t)
{
    json j;
    // Palette
    TV_TO_J(accent);          TV_TO_J(col_data);         TV_TO_J(col_ok);
    TV_TO_J(col_warning);     TV_TO_J(col_error);        TV_TO_J(col_error_alt);
    TV_TO_J(col_log);         TV_TO_J(col_armed);        TV_TO_J(col_disarmed);
    TV_TO_J(col_no_link);     TV_TO_J(col_no_link_muted);TV_TO_J(col_reboot);
    TV_TO_J(col_active_text);
    // Backgrounds
    TV_TO_J(bg_panel);        TV_TO_J(bg_topbar);        TV_TO_J(bg_bottombar);
    TV_TO_J(bg_sidebar_left); TV_TO_J(bg_video);         TV_TO_J(bg_map);
    TV_TO_J(bg_child_dark);   TV_TO_J(bg_child_darker);  TV_TO_J(bg_param_list);
    // Borders
    TV_TO_J(separator);       TV_TO_J(border_tab);       TV_TO_J(border_form);
    TV_TO_J(panel_border);
    // Buttons
    TV_TO_J(btn_arm_base);         TV_TO_J(btn_arm_hov);
    TV_TO_J(btn_disarm_base);      TV_TO_J(btn_disarm_hov);
    TV_TO_J(btn_disconnect_base);  TV_TO_J(btn_disconnect_hov);
    TV_TO_J(btn_stop_base);        TV_TO_J(btn_stop_hov);        TV_TO_J(btn_stop_act);
    TV_TO_J(btn_intlk_on_base);    TV_TO_J(btn_intlk_on_hov);
    TV_TO_J(btn_intlk_off_base);   TV_TO_J(btn_intlk_off_hov);
    TV_TO_J(btn_mode_active_base); TV_TO_J(btn_mode_active_hov);
    TV_TO_J(btn_write_base);       TV_TO_J(btn_write_hov);
    TV_TO_J(btn_tab_active_base);  TV_TO_J(btn_tab_active_hov);
    // Flash
    TV_TO_J(flash_accepted_base);  TV_TO_J(flash_accepted_hov);
    TV_TO_J(flash_rejected_base);  TV_TO_J(flash_rejected_hov);
    TV_TO_J(flash_pending_base);   TV_TO_J(flash_pending_hov);
    TV_TO_J(flash_normal_hov);     TV_TO_J(flash_normal_press);
    // AH
    TV_TO_J(ah_sky);            TV_TO_J(ah_ground);          TV_TO_J(ah_horizon_line);
    TV_TO_J(ah_pitch_ladder);   TV_TO_J(ah_pitch_label_bg);  TV_TO_J(ah_pitch_label);
    TV_TO_J(ah_crosshair);      TV_TO_J(ah_overlay_bg);      TV_TO_J(ah_airspeed_text);
    TV_TO_J(ah_throttle_text);  TV_TO_J(ah_heading_text);
    TV_TO_J(ah_roll_arc);       TV_TO_J(ah_roll_tick);        TV_TO_J(ah_roll_label);
    TV_TO_J(ah_roll_pointer);   TV_TO_J(ah_border);
    // Map
    TV_TO_J(map_tile_placeholder); TV_TO_J(map_vehicle_fill);  TV_TO_J(map_vehicle_ring);
    TV_TO_J(map_attr_bg);          TV_TO_J(map_attr_text);
    // EKF
    TV_TO_J(ekf_bg);  TV_TO_J(ekf_outline);  TV_TO_J(ekf_label);
    return j;
}

static ThemeVars theme_from_json(const json& j)
{
    ThemeVars t; // starts as defaults
    // Palette
    TV_FROM_J(accent);          TV_FROM_J(col_data);         TV_FROM_J(col_ok);
    TV_FROM_J(col_warning);     TV_FROM_J(col_error);        TV_FROM_J(col_error_alt);
    TV_FROM_J(col_log);         TV_FROM_J(col_armed);        TV_FROM_J(col_disarmed);
    TV_FROM_J(col_no_link);     TV_FROM_J(col_no_link_muted);TV_FROM_J(col_reboot);
    TV_FROM_J(col_active_text);
    // Backgrounds
    TV_FROM_J(bg_panel);        TV_FROM_J(bg_topbar);        TV_FROM_J(bg_bottombar);
    TV_FROM_J(bg_sidebar_left); TV_FROM_J(bg_video);         TV_FROM_J(bg_map);
    TV_FROM_J(bg_child_dark);   TV_FROM_J(bg_child_darker);  TV_FROM_J(bg_param_list);
    // Borders
    TV_FROM_J(separator);       TV_FROM_J(border_tab);       TV_FROM_J(border_form);
    TV_FROM_J(panel_border);
    // Buttons
    TV_FROM_J(btn_arm_base);         TV_FROM_J(btn_arm_hov);
    TV_FROM_J(btn_disarm_base);      TV_FROM_J(btn_disarm_hov);
    TV_FROM_J(btn_disconnect_base);  TV_FROM_J(btn_disconnect_hov);
    TV_FROM_J(btn_stop_base);        TV_FROM_J(btn_stop_hov);        TV_FROM_J(btn_stop_act);
    TV_FROM_J(btn_intlk_on_base);    TV_FROM_J(btn_intlk_on_hov);
    TV_FROM_J(btn_intlk_off_base);   TV_FROM_J(btn_intlk_off_hov);
    TV_FROM_J(btn_mode_active_base); TV_FROM_J(btn_mode_active_hov);
    TV_FROM_J(btn_write_base);       TV_FROM_J(btn_write_hov);
    TV_FROM_J(btn_tab_active_base);  TV_FROM_J(btn_tab_active_hov);
    // Flash
    TV_FROM_J(flash_accepted_base);  TV_FROM_J(flash_accepted_hov);
    TV_FROM_J(flash_rejected_base);  TV_FROM_J(flash_rejected_hov);
    TV_FROM_J(flash_pending_base);   TV_FROM_J(flash_pending_hov);
    TV_FROM_J(flash_normal_hov);     TV_FROM_J(flash_normal_press);
    // AH
    TV_FROM_J(ah_sky);            TV_FROM_J(ah_ground);          TV_FROM_J(ah_horizon_line);
    TV_FROM_J(ah_pitch_ladder);   TV_FROM_J(ah_pitch_label_bg);  TV_FROM_J(ah_pitch_label);
    TV_FROM_J(ah_crosshair);      TV_FROM_J(ah_overlay_bg);      TV_FROM_J(ah_airspeed_text);
    TV_FROM_J(ah_throttle_text);  TV_FROM_J(ah_heading_text);
    TV_FROM_J(ah_roll_arc);       TV_FROM_J(ah_roll_tick);        TV_FROM_J(ah_roll_label);
    TV_FROM_J(ah_roll_pointer);   TV_FROM_J(ah_border);
    // Map
    TV_FROM_J(map_tile_placeholder); TV_FROM_J(map_vehicle_fill);  TV_FROM_J(map_vehicle_ring);
    TV_FROM_J(map_attr_bg);          TV_FROM_J(map_attr_text);
    // EKF
    TV_FROM_J(ekf_bg);  TV_FROM_J(ekf_outline);  TV_FROM_J(ekf_label);
    return t;
}

#undef TV_TO_J
#undef TV_FROM_J

// ── Public API ────────────────────────────────────────────────────────────────

void settings_load(AppSettings& out)
{
    const std::string path = settings_path();
    std::ifstream f(path);
    if (!f.is_open()) return; // first run — use defaults

    json root;
    try { f >> root; } catch (...) { return; }

    if (root.contains("active_theme") && root["active_theme"].is_string())
        out.active_theme = root["active_theme"].get<std::string>();

    if (root.contains("themes") && root["themes"].is_object()) {
        for (auto& [name, jtheme] : root["themes"].items())
            out.themes[name] = theme_from_json(jtheme);
    }

    if (root.contains("grid_cells") && root["grid_cells"].is_array()) {
        const auto& arr = root["grid_cells"];
        for (size_t i = 0; i < 9 && i < arr.size(); ++i) {
            const auto& jc = arr[i];
            if (jc.contains("msg_id")     && jc["msg_id"].is_number_unsigned())
                out.grid_cells[i].msg_id = jc["msg_id"].get<uint32_t>();
            if (jc.contains("field_name") && jc["field_name"].is_string())
                out.grid_cells[i].field_name = jc["field_name"].get<std::string>();
            if (jc.contains("title")      && jc["title"].is_string())
                out.grid_cells[i].title = jc["title"].get<std::string>();
        }
    }

    if (root.contains("tile_url") && root["tile_url"].is_string())
        out.tile_url = root["tile_url"].get<std::string>();
    if (root.contains("tile_attribution") && root["tile_attribution"].is_string())
        out.tile_attribution = root["tile_attribution"].get<std::string>();

    if (root.contains("connection_profiles") && root["connection_profiles"].is_array()) {
        for (const auto& jp : root["connection_profiles"]) {
            ConnectionProfile p;
            if (jp.contains("name")   && jp["name"].is_string())   p.name   = jp["name"].get<std::string>();
            if (jp.contains("type")   && jp["type"].is_number())    p.type   = jp["type"].get<int>();
            if (jp.contains("host")   && jp["host"].is_string())    p.host   = jp["host"].get<std::string>();
            if (jp.contains("port")   && jp["port"].is_number())    p.port   = jp["port"].get<int>();
            if (jp.contains("device") && jp["device"].is_string())  p.device = jp["device"].get<std::string>();
            if (jp.contains("baud")   && jp["baud"].is_number())    p.baud   = jp["baud"].get<int>();
            out.connection_profiles.push_back(p);
        }
    }
}

void settings_save(const AppSettings& s)
{
    const std::string path = settings_path();

    // Ensure parent directory exists
    try { fs::create_directories(fs::path(path).parent_path()); }
    catch (...) { return; }

    json root;
    root["active_theme"] = s.active_theme;

    json jthemes = json::object();
    for (const auto& [name, vars] : s.themes)
        jthemes[name] = theme_to_json(vars);
    root["themes"] = jthemes;

    json jcells = json::array();
    for (const auto& c : s.grid_cells) {
        json jc;
        jc["msg_id"]     = c.msg_id;
        jc["field_name"] = c.field_name;
        jc["title"]      = c.title;
        jcells.push_back(jc);
    }
    root["grid_cells"] = jcells;

    root["tile_url"]         = s.tile_url;
    root["tile_attribution"] = s.tile_attribution;

    json jprofiles = json::array();
    for (const auto& p : s.connection_profiles) {
        json jp;
        jp["name"]   = p.name;
        jp["type"]   = p.type;
        jp["host"]   = p.host;
        jp["port"]   = p.port;
        jp["device"] = p.device;
        jp["baud"]   = p.baud;
        jprofiles.push_back(jp);
    }
    root["connection_profiles"] = jprofiles;

    std::ofstream f(path);
    if (f.is_open()) f << root.dump(2);
}
