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


#include "frontend/source/settings.hpp"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <algorithm>
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
    TV_TO_J(highlight);
    TV_TO_J(col_warning);     TV_TO_J(col_error);        TV_TO_J(col_error_alt);
    TV_TO_J(col_log);         TV_TO_J(col_armed);        TV_TO_J(col_disarmed);
    TV_TO_J(col_no_link);     TV_TO_J(col_no_link_muted);TV_TO_J(col_reboot);
    TV_TO_J(col_active_text);   TV_TO_J(col_text_on_dark);  TV_TO_J(col_text);
    // Backgrounds
    TV_TO_J(bg_panel);        TV_TO_J(bg_topbar);        TV_TO_J(bg_bottombar);
    TV_TO_J(bg_sidebar_left); TV_TO_J(bg_video);         TV_TO_J(bg_map);
    TV_TO_J(bg_child_dark);   TV_TO_J(bg_child_darker);  TV_TO_J(bg_param_list);
    // Borders
    TV_TO_J(separator);       TV_TO_J(border_tab);       TV_TO_J(border_form);
    TV_TO_J(panel_border);
    // Buttons
    TV_TO_J(btn_arm_base);         TV_TO_J(btn_disarm_base);
    TV_TO_J(btn_disconnect_base);  TV_TO_J(btn_disconnect_hov);
    TV_TO_J(btn_stop_base);        TV_TO_J(btn_stop_hov);
    TV_TO_J(btn_intlk_off_base);
    TV_TO_J(btn_write_base);       TV_TO_J(btn_write_hov);
    TV_TO_J(btn_tab_active_base);  TV_TO_J(btn_tab_active_hov);
    // Plugin rail
    TV_TO_J(plugin_accent);        TV_TO_J(plugin_accent_dim);
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
    TV_TO_J(map_track);
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
    // Highlight — absent from themes saved before it was a color of its own.
    // Those drew it from their accent by these ratios, so that is what they get.
    if (j.contains("highlight"))
        t.highlight = json_to_vec4(j["highlight"], t.highlight);
    else
        t.highlight = { t.accent.x * 0.35f, t.accent.y * 0.21f, t.accent.z * 0.35f, 1.0f };
    TV_FROM_J(col_warning);     TV_FROM_J(col_error);        TV_FROM_J(col_error_alt);
    TV_FROM_J(col_log);         TV_FROM_J(col_armed);        TV_FROM_J(col_disarmed);
    TV_FROM_J(col_no_link);     TV_FROM_J(col_no_link_muted);TV_FROM_J(col_reboot);
    TV_FROM_J(col_active_text); TV_FROM_J(col_text_on_dark); TV_FROM_J(col_text);
    // Backgrounds
    TV_FROM_J(bg_panel);        TV_FROM_J(bg_topbar);        TV_FROM_J(bg_bottombar);
    TV_FROM_J(bg_sidebar_left); TV_FROM_J(bg_video);         TV_FROM_J(bg_map);
    TV_FROM_J(bg_child_dark);   TV_FROM_J(bg_child_darker);  TV_FROM_J(bg_param_list);
    // Borders
    TV_FROM_J(separator);       TV_FROM_J(border_tab);       TV_FROM_J(border_form);
    TV_FROM_J(panel_border);
    // Buttons
    TV_FROM_J(btn_arm_base);         TV_FROM_J(btn_disarm_base);
    TV_FROM_J(btn_disconnect_base);  TV_FROM_J(btn_disconnect_hov);
    TV_FROM_J(btn_stop_base);        TV_FROM_J(btn_stop_hov);
    TV_FROM_J(btn_intlk_off_base);
    TV_FROM_J(btn_write_base);       TV_FROM_J(btn_write_hov);
    TV_FROM_J(btn_tab_active_base);  TV_FROM_J(btn_tab_active_hov);
    // Plugin rail — absent from themes saved before the rail existed, which is
    // why both fields fall back to the value they were constructed with.
    TV_FROM_J(plugin_accent);        TV_FROM_J(plugin_accent_dim);
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
    TV_FROM_J(map_track);
    TV_FROM_J(map_attr_bg);          TV_FROM_J(map_attr_text);
    // EKF
    TV_FROM_J(ekf_bg);  TV_FROM_J(ekf_outline);  TV_FROM_J(ekf_label);
    return t;
}

#undef TV_TO_J
#undef TV_FROM_J

// ── Built-in themes ───────────────────────────────────────────────────────────

const BuiltinTheme k_builtin_themes[] = {
    { THEME_BUILTIN_NAME,  []() { return ThemeVars{}; } },
    { THEME_GRUVBOX_NAME,  gruvbox_light_theme_vars    },
    { THEME_AMBER_NAME,    retro_amber_theme_vars      },
    { THEME_MATRIX_NAME,   matrix_theme_vars           },
};
const size_t k_builtin_theme_count = sizeof(k_builtin_themes) / sizeof(k_builtin_themes[0]);

bool theme_is_builtin(const std::string& name)
{
    for (size_t i = 0; i < k_builtin_theme_count; ++i)
        if (name == k_builtin_themes[i].name) return true;
    return false;
}

bool theme_lookup(const AppSettings& s, const std::string& name, ThemeVars& out)
{
    for (size_t i = 0; i < k_builtin_theme_count; ++i)
        if (name == k_builtin_themes[i].name) { out = k_builtin_themes[i].factory(); return true; }
    auto it = s.themes.find(name);
    if (it == s.themes.end()) return false;
    out = it->second;
    return true;
}

// ── Public API ────────────────────────────────────────────────────────────────

static void joy_binding_from_json(const json& jb, JoyButtonBinding& b)
{
    const auto num = [&](const char* k, int lo, int hi, int dflt) {
        return (jb.contains(k) && jb[k].is_number_integer())
             ? std::clamp(jb[k].get<int>(), lo, hi) : dflt;
    };
    b.action  = (JoyBtnAction) num("action",  0, 3, 0);
    b.trigger = (JoyBtnTrigger)num("trigger", 0, 2, 0);
    if (jb.contains("mode_name") && jb["mode_name"].is_string())
        b.mode_name = jb["mode_name"].get<std::string>();
    if (jb.contains("mode_custom") && jb["mode_custom"].is_number_unsigned())
        b.mode_custom = jb["mode_custom"].get<uint32_t>();
    b.mode_standard  = (uint8_t) num("mode_standard",  0, 255, 0);
    b.mode_autopilot = (uint8_t) num("mode_autopilot", 0, 255, 0);
    b.mode_type      = (uint8_t) num("mode_type",      0, 255, 0);
    b.aux_fn = (uint16_t)num("aux_fn", 0, 65535, 0);
    b.servo  = (uint8_t) num("servo",  1, 16,    1);
    b.on     = (uint16_t)num("on",     0, 2200,  0);
    b.off    = (uint16_t)num("off",    0, 2200,  0);
    // A hand-edited position past HIGH is not a position.
    if (b.action == JoyBtnAction::Aux) {
        b.on  = std::min<uint16_t>(b.on,  2);
        b.off = std::min<uint16_t>(b.off, 2);
    }
}

static void joy_rc_from_json(const json& j, JoyRcOut& o)
{
    if (!j.is_object()) return;
    if (j.contains("channel") && j["channel"].is_number_integer()) {
        const int ch = j["channel"].get<int>();
        if (ch >= 1 && ch <= JOY_RC_MAX_CHANNEL) o.channel = ch;
    }
    if (j.contains("reversed") && j["reversed"].is_boolean())
        o.reversed = j["reversed"].get<bool>();
}

static json joy_binding_to_json(const JoyButtonBinding& b)
{
    return { { "action",         (int)b.action },
             { "trigger",        (int)b.trigger },
             { "mode_name",      b.mode_name },
             { "mode_custom",    b.mode_custom },
             { "mode_standard",  b.mode_standard },
             { "mode_autopilot", b.mode_autopilot },
             { "mode_type",      b.mode_type },
             { "aux_fn",         b.aux_fn },
             { "servo",          b.servo },
             { "on",             b.on },
             { "off",            b.off } };
}

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
        for (auto& [name, jtheme] : root["themes"].items()) {
            // Older builds let SAVE AS take a built-in's name, which left a
            // theme the picker could never show. Bring it back under a name
            // of its own rather than keep it hidden.
            std::string key = name;
            if (theme_is_builtin(key)) {
                key += " (saved)";
                if (out.themes.count(key)) continue;
            }
            out.themes[key] = theme_from_json(jtheme);
            // Startup used to load the hidden copy over the built-in, so it is
            // the one the operator has been looking at — keep it on screen.
            if (key != name && out.active_theme == name)
                out.active_theme = key;
        }
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

    if (root.contains("audio_enabled") && root["audio_enabled"].is_boolean())
        out.audio_enabled = root["audio_enabled"].get<bool>();
    if (root.contains("audio_volume") && root["audio_volume"].is_number())
        out.audio_volume = root["audio_volume"].get<float>();

    if (root.contains("hud_tapes_main") && root["hud_tapes_main"].is_boolean())
        out.hud_tapes_main = root["hud_tapes_main"].get<bool>();
    if (root.contains("hud_tapes_swarm") && root["hud_tapes_swarm"].is_boolean())
        out.hud_tapes_swarm = root["hud_tapes_swarm"].get<bool>();

    if (root.contains("ui_font") && root["ui_font"].is_number_integer())
        out.ui_font = std::clamp(root["ui_font"].get<int>(), 0, 1);

    if (root.contains("map_icon_style") && root["map_icon_style"].is_number_integer())
        out.map_icon_style = std::clamp(root["map_icon_style"].get<int>(), 0, 4);

    if (root.contains("crt_scanlines") && root["crt_scanlines"].is_boolean())
        out.crt_scanlines = root["crt_scanlines"].get<bool>();
    if (root.contains("crt_scanline_strength") && root["crt_scanline_strength"].is_number())
        out.crt_scanline_strength = root["crt_scanline_strength"].get<float>();
    if (root.contains("crt_vignette") && root["crt_vignette"].is_boolean())
        out.crt_vignette = root["crt_vignette"].get<bool>();
    if (root.contains("crt_vignette_strength") && root["crt_vignette_strength"].is_number())
        out.crt_vignette_strength = root["crt_vignette_strength"].get<float>();

    if (root.contains("joystick_profiles") && root["joystick_profiles"].is_object()) {
        for (const auto& [guid, jp] : root["joystick_profiles"].items()) {
            if (!jp.is_object()) continue;
            JoystickProfile p;
            if (jp.contains("calibrated") && jp["calibrated"].is_boolean())
                p.calibrated = jp["calibrated"].get<bool>();
            if (jp.contains("ranges") && jp["ranges"].is_array()) {
                const auto& arr = jp["ranges"];
                for (size_t i = 0; i < arr.size() && i < RcChannels::MAX_CHANNELS; ++i) {
                    const auto& r = arr[i];
                    if (!r.is_array() || r.size() != 3 || !r[0].is_number_integer() ||
                        !r[1].is_number_integer() || !r[2].is_number_integer())
                        continue;
                    p.ranges[i].min  = (uint16_t)std::clamp(r[0].get<int>(), 0, 65535);
                    p.ranges[i].trim = (uint16_t)std::clamp(r[1].get<int>(), 0, 65535);
                    p.ranges[i].max  = (uint16_t)std::clamp(r[2].get<int>(), 0, 65535);
                }
            }
            if (jp.contains("bindings") && jp["bindings"].is_array()) {
                const auto& arr = jp["bindings"];
                for (size_t f = 0; f < arr.size() && f < JOY_FN_COUNT; ++f) {
                    const auto& b = arr[f];
                    if (!b.is_object()) continue;
                    if (b.contains("axis") && b["axis"].is_number_integer()) {
                        const int axis = b["axis"].get<int>();
                        if (axis >= 0 && axis < RcChannels::MAX_CHANNELS)
                            p.bindings[f].axis = axis;
                    }
                    if (b.contains("reversed") && b["reversed"].is_boolean())
                        p.bindings[f].reversed = b["reversed"].get<bool>();
                }
            }
            if (jp.contains("buttons") && jp["buttons"].is_array()) {
                for (const auto& jb : jp["buttons"]) {
                    if (!jb.is_object() || !jb.contains("button") ||
                        !jb["button"].is_number_integer()) continue;
                    const int n = jb["button"].get<int>();
                    if (n < 0 || n >= JOY_MAX_BUTTONS) continue;

                    joy_binding_from_json(jb, p.buttons[n]);
                }
            }
            if (jp.contains("virtual") && jp["virtual"].is_array()) {
                uint32_t taken = 0;
                int      slot  = 0;
                for (const auto& jv : jp["virtual"]) {
                    if (slot >= JOY_MAX_VIRTUAL) break;
                    if (!jv.is_object() || !jv.contains("patterns") ||
                        !jv["patterns"].is_array()) continue;

                    JoyVirtualChannel v;
                    const auto& pats = jv["patterns"];
                    if (pats.size() > JOY_VIRTUAL_MAX_POS) continue;
                    v.positions = (int)pats.size();
                    for (size_t i = 0; i < pats.size(); ++i)
                        if (pats[i].is_number_unsigned())
                            v.patterns[i] = pats[i].get<uint32_t>();
                    if (jv.contains("actions") && jv["actions"].is_array()) {
                        const auto& acts = jv["actions"];
                        for (size_t i = 0; i < acts.size() && (int)i < v.positions; ++i)
                            if (acts[i].is_object()) {
                                joy_binding_from_json(acts[i], v.actions[i]);
                                v.actions[i].trigger = JoyBtnTrigger::Press;
                            }
                    }
                    if (jv.contains("rc")) joy_rc_from_json(jv["rc"], v.rc);
                    // A channel that cannot tell its positions apart, or wants a
                    // button another one already has, is not loaded at all.
                    if (!v.valid() || (v.mask() & taken)) continue;
                    taken |= v.mask();
                    p.virtuals[slot++] = v;
                }
                // Enforced here too, for a file edited by hand.
                for (int i = 0; i < JOY_MAX_BUTTONS; ++i)
                    if (taken & (1u << i)) p.buttons[i] = JoyButtonBinding{};
            }
            if (jp.contains("rc_override") && jp["rc_override"].is_boolean())
                p.rc_override = jp["rc_override"].get<bool>();
            if (jp.contains("rc_axes") && jp["rc_axes"].is_array()) {
                const auto& arr = jp["rc_axes"];
                for (size_t i = 0; i < arr.size() && i < RcChannels::MAX_CHANNELS; ++i)
                    joy_rc_from_json(arr[i], p.rc_axes[i]);
            }
            out.joystick_profiles[guid] = p;
        }
    }

    if (root.contains("mcp_enabled") && root["mcp_enabled"].is_boolean())
        out.mcp_enabled = root["mcp_enabled"].get<bool>();
    if (root.contains("mcp_port") && root["mcp_port"].is_number_integer())
        out.mcp_port = std::clamp(root["mcp_port"].get<int>(), 1, 65535);
    if (root.contains("mcp_actions_enabled") && root["mcp_actions_enabled"].is_boolean())
        out.mcp_actions_enabled = root["mcp_actions_enabled"].get<bool>();
    if (root.contains("mcp_instructions") && root["mcp_instructions"].is_string())
        out.mcp_instructions = root["mcp_instructions"].get<std::string>();
    if (root.contains("mcp_auto_allow") && root["mcp_auto_allow"].is_object())
        for (const auto& [tool, on] : root["mcp_auto_allow"].items())
            if (on.is_boolean()) out.mcp_auto_allow[tool] = on.get<bool>();
    if (root.contains("mcp_token") && root["mcp_token"].is_string())
        out.mcp_token = root["mcp_token"].get<std::string>();

    if (root.contains("gcs_sysid") && root["gcs_sysid"].is_number_integer())
        out.gcs_sysid = std::clamp(root["gcs_sysid"].get<int>(), 0, 255);

    if (root.contains("terrain_clearance_m") && root["terrain_clearance_m"].is_number())
        out.terrain_clearance_m = std::clamp(root["terrain_clearance_m"].get<float>(), 0.0f, 1000.0f);

    if (root.contains("terrain_url") && root["terrain_url"].is_string())
        out.terrain_url = root["terrain_url"].get<std::string>();
    if (root.contains("terrain_encoding") && root["terrain_encoding"].is_number_integer())
        out.terrain_encoding = std::clamp(root["terrain_encoding"].get<int>(), 0, 1);
    if (root.contains("terrain_attribution") && root["terrain_attribution"].is_string())
        out.terrain_attribution = root["terrain_attribution"].get<std::string>();

    if (root.contains("tile_url") && root["tile_url"].is_string())
        out.tile_url = root["tile_url"].get<std::string>();
    if (root.contains("tile_attribution") && root["tile_attribution"].is_string())
        out.tile_attribution = root["tile_attribution"].get<std::string>();
    // A file from before there were providers has only a URL: its own URL is a
    // custom source, and the old default is Esri.
    if (root.contains("tile_provider") && root["tile_provider"].is_number_integer())
        out.tile_provider = std::clamp(root["tile_provider"].get<int>(), 0, TILE_PROVIDER_COUNT - 1);
    else
        out.tile_provider = (out.tile_url == TILE_ESRI_URL) ? (int)TileProvider::Esri
                                                            : (int)TileProvider::Custom;
    if (root.contains("tile_kind") && root["tile_kind"].is_number_integer())
        out.tile_kind = std::clamp(root["tile_kind"].get<int>(), 0, TILE_KIND_COUNT - 1);
    if (root.contains("tile_gibs_layer") && root["tile_gibs_layer"].is_number_integer())
        out.tile_gibs_layer = std::clamp(root["tile_gibs_layer"].get<int>(), 0, tile_gibs_layer_count() - 1);
    for (const auto& [name, field] : { std::pair<const char*, std::string*>
             { "tile_gibs_date", &out.tile_gibs_date }, { "tile_key_esri", &out.tile_key_esri }, { "tile_key_google", &out.tile_key_google },
             { "tile_key_custom", &out.tile_key_custom } })
        if (root.contains(name) && root[name].is_string()) *field = root[name].get<std::string>();

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

    root["gcs_sysid"]           = s.gcs_sysid;
    root["terrain_clearance_m"] = s.terrain_clearance_m;
    root["terrain_url"]         = s.terrain_url;
    root["terrain_encoding"]    = s.terrain_encoding;
    root["terrain_attribution"] = s.terrain_attribution;
    root["tile_url"]         = s.tile_url;
    root["tile_attribution"] = s.tile_attribution;
    root["tile_provider"]    = s.tile_provider;
    root["tile_kind"]        = s.tile_kind;
    root["tile_gibs_layer"]  = s.tile_gibs_layer;
    root["tile_gibs_date"]   = s.tile_gibs_date;
    root["tile_key_esri"]    = s.tile_key_esri;
    root["tile_key_google"]  = s.tile_key_google;
    root["tile_key_custom"]  = s.tile_key_custom;
    root["audio_enabled"]    = s.audio_enabled;
    root["audio_volume"]     = s.audio_volume;
    root["hud_tapes_main"]   = s.hud_tapes_main;
    root["hud_tapes_swarm"]  = s.hud_tapes_swarm;
    root["ui_font"]               = s.ui_font;
    root["map_icon_style"]        = s.map_icon_style;
    root["crt_scanlines"]         = s.crt_scanlines;
    root["crt_scanline_strength"] = s.crt_scanline_strength;
    root["crt_vignette"]          = s.crt_vignette;
    root["crt_vignette_strength"] = s.crt_vignette_strength;

    json jjoysticks = json::object();
    for (const auto& [guid, p] : s.joystick_profiles) {
        json jranges = json::array();
        for (const JoyAxisRange& r : p.ranges)
            jranges.push_back({ r.min, r.trim, r.max });
        json jbindings = json::array();
        for (const JoyFnBinding& b : p.bindings)
            jbindings.push_back({ { "axis", b.axis }, { "reversed", b.reversed } });
        json jbuttons = json::array();
        for (int i = 0; i < JOY_MAX_BUTTONS; ++i) {
            const JoyButtonBinding& b = p.buttons[i];
            if (b.action == JoyBtnAction::None) continue;
            json jb = joy_binding_to_json(b);
            jb["button"] = i;
            jbuttons.push_back(jb);
        }
        json jrc_axes = json::array();
        for (const JoyRcOut& o : p.rc_axes)
            jrc_axes.push_back({ { "channel", o.channel }, { "reversed", o.reversed } });
        json jvirtual = json::array();
        for (const JoyVirtualChannel& v : p.virtuals) {
            if (!v.used()) continue;
            json pats = json::array(), acts = json::array();
            for (int i = 0; i < v.positions; ++i) {
                pats.push_back(v.patterns[i]);
                acts.push_back(joy_binding_to_json(v.actions[i]));
            }
            jvirtual.push_back({ { "patterns", pats }, { "actions", acts },
                                 { "rc", { { "channel", v.rc.channel },
                                           { "reversed", v.rc.reversed } } } });
        }
        jjoysticks[guid] = { { "calibrated", p.calibrated },
                             { "ranges",     jranges },
                             { "bindings",   jbindings },
                             { "buttons",    jbuttons },
                             { "virtual",    jvirtual },
                             { "rc_override", p.rc_override },
                             { "rc_axes",    jrc_axes } };
    }
    root["joystick_profiles"] = jjoysticks;

    root["mcp_enabled"]           = s.mcp_enabled;
    root["mcp_port"]              = s.mcp_port;
    root["mcp_actions_enabled"]   = s.mcp_actions_enabled;
    root["mcp_auto_allow"]        = s.mcp_auto_allow;
    root["mcp_instructions"]      = s.mcp_instructions;
    root["mcp_token"]             = s.mcp_token;

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
