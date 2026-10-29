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


#include "mcp_server.hpp"

#include "httplib.h"
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <functional>
#include <random>
#include <thread>
#include <vector>

#include "fleet.hpp"
#include "firmware_profile.hpp"
#include "../frontend/app_log.hpp"
#include "../plugins/plugin_api.hpp"

using json = nlohmann::json;

namespace {

// Newest first. A client asking for one of these gets it back; anything else
// gets the newest, and the client decides whether it can live with that.
// Nothing this server does differs between them.
const char* const PROTOCOL_VERSIONS[] = { "2025-11-25", "2025-06-18", "2025-03-26" };

// How long one tools/call may take in all: waiting for the operator, then for
// the vehicle's answer. Kept under 60 s because that is where common clients
// give up on a call — a call that outlived its client could still run after
// the client stopped wanting it. The cancellation a client sends then is
// honoured (see cancel_request), but not every client sends one.
constexpr std::chrono::seconds CALL_BUDGET{55};

// The part of it the operator gets. Long enough to read the dialog, short
// enough that an agent is not left hanging on a GCS nobody is at.
constexpr std::chrono::seconds APPROVAL_TIMEOUT{45};

// Caps on what one read returns. An ArduPilot parameter table is ~1,000
// entries; an assistant wants a slice of it, not all of it in its context.
constexpr size_t MAX_PARAMS_RETURNED = 200;
constexpr size_t MAX_MESSAGES_RETURNED = 100;

// JSON-RPC 2.0 error codes.
constexpr int RPC_PARSE_ERROR      = -32700;
constexpr int RPC_INVALID_REQUEST  = -32600;
constexpr int RPC_METHOD_NOT_FOUND = -32601;
constexpr int RPC_INVALID_PARAMS   = -32602;

json rpc_result(const json& id, json result)
{
    return { {"jsonrpc", "2.0"}, {"id", id}, {"result", std::move(result)} };
}

json rpc_error(const json& id, int code, const std::string& message)
{
    return { {"jsonrpc", "2.0"}, {"id", id},
             {"error", { {"code", code}, {"message", message} }} };
}

// A tool's answer. Errors the assistant can act on — no such vehicle, the
// operator said no — are results with isError set, not JSON-RPC errors: the
// spec keeps protocol errors for malformed calls, and a model sees the result.
json tool_text(const std::string& text, bool is_error = false)
{
    json r = { {"content", json::array({ { {"type", "text"}, {"text", text} } })} };
    if (is_error) r["isError"] = true;
    return r;
}

json tool_json(const json& j) { return tool_text(j.dump(2)); }

bool iequals(const std::string& a, const std::string& b)
{
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i)
        if (std::tolower((unsigned char)a[i]) != std::tolower((unsigned char)b[i]))
            return false;
    return true;
}

// Loopback names only. Checked on Host as well as Origin: a page on some other
// site can make the browser send a request here by pointing its own hostname at
// 127.0.0.1 (DNS rebinding), and that request carries the other site's name.
bool is_loopback_host(std::string h)
{
    if (h.empty()) return false;
    if (h.front() == '[') {                         // [::1]:port
        const size_t close = h.find(']');
        return close != std::string::npos && h.substr(1, close - 1) == "::1";
    }
    const size_t colon = h.find(':');
    if (colon != std::string::npos) h.resize(colon);
    return h == "127.0.0.1" || iequals(h, "localhost");
}

bool origin_ok(const httplib::Request& req)
{
    if (!is_loopback_host(req.get_header_value("Host"))) return false;
    if (!req.has_header("Origin")) return true;     // not a browser; the token decides
    std::string o = req.get_header_value("Origin");
    const size_t scheme = o.find("://");
    if (scheme == std::string::npos) return false;
    o = o.substr(scheme + 3);
    const size_t slash = o.find('/');
    if (slash != std::string::npos) o.resize(slash);
    return is_loopback_host(o);
}

// Compares every byte whatever it finds, so the time taken says nothing about
// how much of a guess was right.
bool token_matches(const std::string& given, const std::string& want)
{
    if (given.size() != want.size()) return false;
    unsigned char diff = 0;
    for (size_t i = 0; i < want.size(); ++i)
        diff |= (unsigned char)(given[i] ^ want[i]);
    return diff == 0;
}

bool authorised(const httplib::Request& req, const std::string& token)
{
    if (token.empty()) return true;
    const std::string h = req.get_header_value("Authorization");
    const std::string prefix = "Bearer ";
    if (h.compare(0, prefix.size(), prefix) != 0) return false;
    return token_matches(h.substr(prefix.size()), token);
}

// ── Describing a vehicle ─────────────────────────────────────────────────────

std::string mode_name(const VehicleState& vs)
{
    for (const auto& m : vs.available_modes)
        if (m.custom_mode == vs.custom_mode) return m.name;
    return firmware_profile(vs).mode_short_name(vs.custom_mode, vs.type);
}

const char* type_name(MAV_TYPE t)
{
    switch (t) {
    case MAV_TYPE_FIXED_WING:         return "fixed_wing";
    case MAV_TYPE_QUADROTOR:          return "quadrotor";
    case MAV_TYPE_HEXAROTOR:          return "hexarotor";
    case MAV_TYPE_OCTOROTOR:          return "octorotor";
    case MAV_TYPE_TRICOPTER:          return "tricopter";
    case MAV_TYPE_COAXIAL:            return "coaxial";
    case MAV_TYPE_HELICOPTER:         return "helicopter";
    case MAV_TYPE_GROUND_ROVER:       return "ground_rover";
    case MAV_TYPE_SURFACE_BOAT:       return "surface_boat";
    case MAV_TYPE_SUBMARINE:          return "submarine";
    case MAV_TYPE_VTOL_TAILSITTER_DUOROTOR:
    case MAV_TYPE_VTOL_TAILSITTER_QUADROTOR:
    case MAV_TYPE_VTOL_TILTROTOR:
    case MAV_TYPE_VTOL_FIXEDROTOR:
    case MAV_TYPE_VTOL_TAILSITTER:    return "vtol";
    default:                          return "other";
    }
}

// The modes an operator could pick, by name — what set_mode accepts.
json selectable_modes(const VehicleState& vs)
{
    json out = json::array();
    if (!vs.available_modes.empty()) {
        for (const auto& m : vs.available_modes)
            if (m.user_selectable()) out.push_back(m.name);
    } else {
        for (const auto& m : firmware_profile(vs).fallback_modes(vs.type))
            out.push_back(m.label);
    }
    return out;
}

json telemetry_json(const Vehicle& v, const VehicleSnapshot& s)
{
    const VehicleState& vs = s.state;
    json j = {
        {"vehicle",   v.number()},
        {"sysid",     v.sysid()},
        {"autopilot", firmware_profile(vs).name()},
        {"type",      type_name(vs.type)},
        {"armed",     vs.armed},
        {"mode",      mode_name(vs)},
        {"stale",     v.stale()},
    };
    if (vs.has_global_pos)
        j["position"] = { {"lat", vs.lat}, {"lon", vs.lon},
                          {"alt_rel_m", vs.alt_rel}, {"alt_asl_m", vs.alt_asl} };
    if (vs.has_home)
        j["home"] = { {"lat", vs.home_lat}, {"lon", vs.home_lon},
                      {"alt_asl_m", vs.home_alt_asl} };
    if (vs.has_attitude)
        j["attitude_deg"] = { {"roll", vs.roll}, {"pitch", vs.pitch}, {"yaw", vs.yaw} };
    if (vs.has_vfr)
        j["motion"] = { {"airspeed_ms", vs.airspeed}, {"groundspeed_ms", vs.groundspeed},
                        {"climb_ms", vs.climb}, {"heading_deg", vs.heading},
                        {"throttle_pct", vs.throttle} };
    if (vs.has_gps_raw)
        j["gps"] = { {"fix_type", vs.gps_fix_type}, {"satellites", vs.satellites_vis},
                     {"hdop", vs.hdop}, {"vdop", vs.vdop} };
    if (vs.has_sys_status) {
        j["battery"] = { {"voltage_v", vs.voltage_V}, {"current_a", vs.current_A} };
        if (vs.battery_remaining >= 0) j["battery"]["remaining_pct"] = vs.battery_remaining;
    }
    if (vs.has_ekf_status)
        j["estimator"] = { {"units", firmware_profile(vs).estimator_units()},
                           {"velocity", vs.ekf_velocity_variance},
                           {"pos_horiz", vs.ekf_pos_horiz_variance},
                           {"pos_vert", vs.ekf_pos_vert_variance},
                           {"compass", vs.ekf_compass_variance} };
    j["selectable_modes"] = selectable_modes(vs);
    return j;
}

bool param_is_integer(uint8_t type)
{
    return type != MAV_PARAM_TYPE_REAL32 && type != MAV_PARAM_TYPE_REAL64;
}

json param_value(const ParamEntry& p)
{
    if (param_is_integer(p.type)) return (long long)std::llround(p.value);
    return p.value;
}

json mission_item_json(const MissionItem& m)
{
    return { {"seq", m.seq}, {"command", m.command}, {"frame", m.frame},
             {"lat", m.lat}, {"lon", m.lon}, {"alt_m", m.alt},
             {"params", { m.param1, m.param2, m.param3, m.param4 }} };
}

// Great-circle distance, for telling the operator how far a goto goes.
double distance_m(double lat1, double lon1, double lat2, double lon2)
{
    constexpr double R = 6371000.0, D2R = 3.14159265358979323846 / 180.0;
    const double dlat = (lat2 - lat1) * D2R, dlon = (lon2 - lon1) * D2R;
    const double a = std::sin(dlat / 2) * std::sin(dlat / 2) +
                     std::cos(lat1 * D2R) * std::cos(lat2 * D2R) *
                     std::sin(dlon / 2) * std::sin(dlon / 2);
    return 2.0 * R * std::asin(std::sqrt((std::min)(1.0, a)));
}

// A mode the operator could pick, matched by `match` against its name and its
// MAV_STANDARD_MODE (0 for the fallback tables, which carry none).
struct ModeChoice {
    uint32_t    custom   = 0;
    uint8_t     standard = 0;
    std::string name;
};

bool find_mode(const VehicleState& vs,
               const std::function<bool(const std::string&, uint8_t)>& match,
               ModeChoice& out)
{
    if (!vs.available_modes.empty()) {
        for (const auto& m : vs.available_modes)
            if (m.user_selectable() && match(m.name, m.standard_mode)) {
                out = { m.custom_mode, m.standard_mode, m.name };
                return true;
            }
        return false;
    }
    for (const auto& m : firmware_profile(vs).fallback_modes(vs.type))
        if (match(m.label, 0)) {
            out = { m.custom_mode, 0, m.label };
            return true;
        }
    return false;
}

// Mission commands an assistant may name rather than number.
const std::pair<const char*, uint16_t> MISSION_COMMANDS[] = {
    { "waypoint",     16 },   // MAV_CMD_NAV_WAYPOINT
    { "loiter_unlim", 17 },
    { "loiter_turns", 18 },
    { "loiter_time",  19 },
    { "rtl",          20 },
    { "land",         21 },
    { "takeoff",      22 },
    { "change_speed", 178 },  // MAV_CMD_DO_CHANGE_SPEED
};

// Upper bound on one upload, well past what any autopilot here stores, so a
// runaway list is refused rather than streamed at the vehicle for minutes.
constexpr size_t MAX_MISSION_ITEMS = 1000;

// How long an action's caller waits on the vehicle after it has been sent.
constexpr std::chrono::milliseconds ACK_WAIT{3500};
constexpr std::chrono::seconds      UPLOAD_WAIT{20};
constexpr std::chrono::seconds      DOWNLOAD_WAIT{15};

// Reads go into the event log at most this often per tool and vehicle, with a
// count of the ones in between. Several agents polling telemetry each second
// would otherwise fill the log in minutes and push out the vehicles' own
// messages — the prearm failure the operator needed to see. Actions, and any
// read that fails, are logged every time.
constexpr std::chrono::seconds READ_LOG_INTERVAL{30};
constexpr size_t               EVENTS_MAX = 1000;   // undrained; the UI takes them each frame

// A claim nobody has used for this long lapses, so an agent that crashed or
// was stopped cannot hold a vehicle forever. Using it — an action carrying
// the token, or claiming again — starts the clock over.
constexpr std::chrono::minutes CLAIM_IDLE{15};

// What counts as too close, between a path an agent asks for and another
// armed vehicle or the point one is heading to. Generous next to GPS error,
// which on its own is a few metres each way.
constexpr double SEPARATION_H_M  = 10.0;
constexpr double SEPARATION_V_M  = 5.0;
constexpr double PATH_SAMPLE_M   = 2.0;     // how finely a path is walked
constexpr int    PATH_SAMPLES_MAX = 500;

// ── Tool table ───────────────────────────────────────────────────────────────

const json VEHICLE_ARG = {
    {"type", "integer"},
    {"description", "Which vehicle: its number as list_vehicles and the GCS show it, e.g. 2 "
                    "for \"(2)\". This is not the MAVLink system ID. Leave it out to use the "
                    "vehicle the operator has selected on screen."},
};

const json REASON_ARG = {
    {"type", "string"},
    {"description", "One plain sentence saying why you are doing this, e.g. \"Moving clear "
                    "of the tree line before the survey\". The operator reads it in the "
                    "approval dialog, and in the event log for actions that run without asking."},
};

const json NUMBER = { {"type", "number"} };

json schema(json props = json::object(), json required = json::array())
{
    json s = { {"type", "object"}, {"properties", std::move(props)} };
    if (!required.empty()) s["required"] = std::move(required);
    return s;
}

// What every agent is told about this GCS, before the operator's own rules.
// Sent at initialize and returned by get_guidelines, so hosts that drop the
// former still have a way to it.
const char* const BUILTIN_INSTRUCTIONS =
    "Free Body Problem is a ground control station (GCS): the program a human operator uses "
    "to watch and command drones and other uncrewed vehicles running ArduPilot or PX4. The "
    "vehicles may be real aircraft with people nearby, so act carefully and only do what "
    "you were asked.\n"
    "- Reading is always allowed and changes nothing: list_vehicles, get_telemetry, "
    "get_params, get_mission, get_messages, list_plugins.\n"
    "- Actions send commands to a vehicle. Each action's description says whether it runs "
    "straight away or waits (up to 45 s) for the operator to click ALLOW. Always pass a "
    "short `reason`; the operator sees it.\n"
    "- Vehicles are picked by the number the GCS shows, e.g. 2 for \"(2)\", not by MAVLink "
    "system ID. Get it from list_vehicles.\n"
    "- Altitudes are in metres above the home point (usually where the vehicle was armed) "
    "unless a tool says otherwise. Distances are in metres.\n"
    "- A command being accepted is not the same as it having happened. Check with "
    "get_telemetry afterwards, and read get_messages when something is refused.\n"
    "- If you are unsure where a vehicle is, call get_telemetry instead of asking or "
    "guessing. For moves like \"50 m north\", give goto north_m/east_m and let the GCS do "
    "the arithmetic.\n"
    "- When several agents share the fleet, claim_vehicle reserves a vehicle for you.\n"
    "get_guidelines returns this text, the operator's own rules, and what is currently "
    "allowed.";

json read_tools()
{
    const json ro = { {"readOnlyHint", true} };
    return json::array({
        { {"name", "get_guidelines"},
          {"description", "Call this first. Explains how this ground control station works, "
                          "the operator's rules for agents (follow them), and which actions "
                          "are turned on and whether each one needs the operator's approval."},
          {"inputSchema", schema()},
          {"annotations", ro} },

        { {"name", "list_vehicles"},
          {"description", "Lists every vehicle the GCS is connected to. For each: the number "
                          "to use as `vehicle` in other tools, its type (copter, plane, ...), "
                          "whether its motors are armed, its current flight mode, how it is "
                          "connected, which agent has claimed it, and which one the operator "
                          "has selected. Start here to find out which vehicles exist."},
          {"inputSchema", schema()},
          {"annotations", ro} },

        { {"name", "get_telemetry"},
          {"description", "Everything the GCS currently knows about one vehicle: where it is "
                          "(latitude, longitude, altitude above home and above sea level), "
                          "where home is, which way it is pointing, how fast it is moving, GPS "
                          "quality, battery, whether it is armed, its flight mode and the modes "
                          "it can switch to. Use it to find a vehicle's position, and to check "
                          "that a command had the effect you wanted."},
          {"inputSchema", schema({ {"vehicle", VEHICLE_ARG} })},
          {"annotations", ro} },

        { {"name", "get_params"},
          {"description", "Reads the vehicle's settings (parameters), e.g. RTL_ALT, the "
                          "height ArduPilot climbs to before returning home. Give exact names, "
                          "or a prefix such as \"BATT_\" for every battery setting. At most "
                          "200 are returned at once."},
          {"inputSchema", schema({
              {"vehicle", VEHICLE_ARG},
              {"names",  { {"type", "array"}, {"items", { {"type", "string"} }},
                           {"description", "Exact parameter names, e.g. [\"RTL_ALT\", \"WPNAV_SPEED\"]."} }},
              {"prefix", { {"type", "string"},
                           {"description", "Every parameter whose name starts with this, e.g. \"BATT_\"."} }},
          })},
          {"annotations", ro} },

        { {"name", "get_mission"},
          {"description", "The vehicle's mission (the list of waypoints and commands it "
                          "flies in Auto/Mission mode) and its geofence (the area it must "
                          "stay inside). By default this is the GCS's copy; set download to "
                          "true to read the mission from the vehicle itself first (takes up "
                          "to 15 s). Do that before changing a plan."},
          {"inputSchema", schema({ {"vehicle", VEHICLE_ARG},
                                   {"download", { {"type", "boolean"},
                                                  {"description", "Read the mission from the vehicle first. Default false."} }} })},
          {"annotations", ro} },

        { {"name", "get_messages"},
          {"description", "Recent text messages the vehicle itself sent, oldest first: "
                          "why it refused to arm (\"PreArm: ...\"), warnings, errors, mode "
                          "changes. Read this when a command is refused or something looks "
                          "wrong."},
          {"inputSchema", schema({
              {"vehicle", VEHICLE_ARG},
              {"limit", { {"type", "integer"}, {"minimum", 1}, {"maximum", 100},
                          {"description", "How many of the most recent messages to return."} }},
          })},
          {"annotations", ro} },

        { {"name", "list_plugins"},
          {"description", "The operator's custom plugin buttons in the GCS, each with its "
                          "slot number and what it does. run_plugin presses one by slot."},
          {"inputSchema", schema()},
          {"annotations", ro} },
    });
}

const json CLAIM_ARG = {
    {"type", "string"},
    {"description", "The token claim_vehicle gave you. Needed only for a vehicle that is "
                    "claimed; leave it out otherwise."},
};

// Reserving vehicles, for several agents sharing one fleet. Not actions — they
// send nothing to a vehicle — so no setting gates them beyond actions being on.
json claim_tools()
{
    return json::array({
        { {"name", "claim_vehicle"},
          {"description", "Reserves a vehicle for you, so other agents cannot command it while "
                          "you work. Only needed when several agents share the fleet. Returns "
                          "a token: pass it as `claim` in every action on that vehicle. The "
                          "claim ends with release_vehicle, or after 15 minutes without use. "
                          "It never stops the operator."},
          {"inputSchema", schema({ {"vehicle", VEHICLE_ARG},
                                   {"label", { {"type", "string"},
                                               {"description", "Who you are, e.g. \"survey-NE\". "
                                                               "Shown to the operator and to other agents."} }} },
                                 { "label" })},
          {"annotations", { {"readOnlyHint", false}, {"destructiveHint", false} }} },
        { {"name", "release_vehicle"},
          {"description", "Ends your claim on a vehicle, so other agents may command it again. "
                          "Call it when you are done."},
          {"inputSchema", schema({ {"vehicle", VEHICLE_ARG}, {"claim", CLAIM_ARG} },
                                 { "claim" })},
          {"annotations", { {"readOnlyHint", false}, {"destructiveHint", false} }} },
    });
}

// The input schema of each action tool, keyed by name. Descriptions come from
// mcp_actions() plus what the policy says about asking — see tools/list.
json action_schema(const std::string& tool)
{
    const json v = VEHICLE_ARG, r = REASON_ARG;
    if (tool == "arm" || tool == "disarm" || tool == "return_to_launch" ||
        tool == "start_mission" || tool == "pause_mission")
        return schema({ {"vehicle", v}, {"reason", r} });
    if (tool == "takeoff")
        return schema({ {"vehicle", v},
                        {"alt_m", { {"type", "number"}, {"exclusiveMinimum", 0}, {"maximum", 1000},
                                    {"description", "Height to climb to, in metres above the "
                                                    "home point, e.g. 10."} }},
                        {"reason", r} },
                      { "alt_m" });
    if (tool == "goto")
        return schema({ {"vehicle", v},
                        {"lat", { {"type", "number"}, {"description", "Target latitude in degrees, e.g. "
                                                                     "47.397742. Give together with lon."} }},
                        {"lon", { {"type", "number"}, {"description", "Target longitude in degrees. Give "
                                                                     "together with lat."} }},
                        {"north_m", { {"type", "number"},
                                      {"description", "Instead of lat/lon: metres north of where the "
                                                      "vehicle is now (negative is south). The GCS "
                                                      "works out the point."} }},
                        {"east_m", { {"type", "number"},
                                     {"description", "Instead of lat/lon: metres east of where the "
                                                     "vehicle is now (negative is west)."} }},
                        {"alt_m", { {"type", "number"}, {"description", "Target height in metres above the "
                                                                       "home point."} }},
                        {"up_m", { {"type", "number"},
                                   {"description", "Instead of alt_m: metres above its current "
                                                   "altitude (negative is down). Give neither to "
                                                   "hold the current altitude."} }},
                        {"reason", r} });
    if (tool == "set_mode")
        return schema({ {"vehicle", v},
                        {"mode", { {"type", "string"},
                                   {"description", "The mode's name, e.g. \"Guided\", \"Loiter\", "
                                                   "\"RTL\". get_telemetry lists the ones this "
                                                   "vehicle accepts."} }},
                        {"reason", r} },
                      { "mode" });
    if (tool == "set_param")
        return schema({ {"vehicle", v},
                        {"name", { {"type", "string"},
                                   {"description", "Exact parameter name, e.g. \"RTL_ALT\"."} }},
                        {"value", { {"type", "number"},
                                    {"description", "The new value, in the parameter's own units. "
                                                    "Read it first with get_params."} }},
                        {"reason", r} },
                      { "name", "value" });
    if (tool == "run_plugin")
        return schema({ {"vehicle", v},
                        {"slot", { {"type", "integer"}, {"minimum", 0},
                                   {"description", "The plugin's slot number from list_plugins."} }},
                        {"reason", r} },
                      { "slot" });
    if (tool == "upload_mission") {
        std::string names;
        for (const auto& [n, id] : MISSION_COMMANDS) names += (names.empty() ? "" : ", ") + std::string(n);
        const json item = {
            {"type", "object"},
            {"properties", {
                {"command", { {"type", {"string", "integer"}},
                              {"description", "What this step does. One of: " + names +
                                              " (waypoint flies to the point; takeoff climbs "
                                              "to alt_m; land lands at the point, or where it "
                                              "is; rtl flies home). Advanced: any MAV_CMD number."} }},
                {"lat", { {"type", "number"}, {"description", "Latitude in degrees."} }},
                {"lon", { {"type", "number"}, {"description", "Longitude in degrees."} }},
                {"alt_m", { {"type", "number"},
                            {"description", "Height in metres above the home point (unless "
                                            "`frame` says otherwise)."} }},
                {"frame", { {"type", "integer"},
                            {"description", "Advanced: MAV_FRAME number. Leave it out: the "
                                            "default 3 means alt_m is above home."} }},
                {"params", { {"type", "array"}, {"items", NUMBER}, {"maxItems", 4},
                             {"description", "Advanced: the command's param1 to param4, "
                                             "e.g. [5] on loiter_time to wait 5 s."} }},
            }},
            {"required", { "command" }},
        };
        return schema({ {"vehicle", v},
                        {"items", { {"type", "array"}, {"items", item}, {"minItems", 1},
                                    {"description", "The steps in order. A typical copter plan: "
                                                    "takeoff, some waypoints, then rtl or land."} }},
                        {"reason", r} },
                      { "items" });
    }
    return schema();
}

} // namespace

const std::vector<McpActionInfo>& mcp_actions()
{
    static const std::vector<McpActionInfo> actions = {
        { "arm",              "Arm",              "Starts the motors, so the vehicle is ready to take off. The vehicle runs its own safety checks first and may refuse; get_messages says why." },
        { "disarm",           "Disarm",           "Stops the motors. Use it after landing; ArduPilot refuses while the vehicle is flying." },
        { "takeoff",          "Takeoff",          "Climbs straight up to a height and holds there. The vehicle must be armed first, and on ArduPilot be in Guided mode." },
        { "goto",             "Go to position",   "Flies to a point and holds there. Give either latitude/longitude, or a move in metres from where the vehicle is now (north_m, east_m, up_m). On ArduPilot it must be in Guided mode." },
        { "set_mode",         "Change mode",      "Switches the flight mode, e.g. Guided (follows GCS commands), Loiter (holds position), Land, RTL (flies home). Only modes the vehicle accepts." },
        { "start_mission",    "Start mission",    "Switches to Auto (Mission on PX4), so the vehicle flies the mission it has stored." },
        { "pause_mission",    "Pause mission",    "Stops following the mission and holds position where it is (Hold mode, else Guided). start_mission resumes." },
        { "upload_mission",   "Upload mission",   "Replaces the vehicle's stored mission with a new list of steps (waypoints, takeoff, land, return home). Does not start it." },
        { "return_to_launch", "Return to launch", "Flies back to the home point (usually where it was armed) and lands there, or loiters, as the vehicle is set up to do." },
        { "set_param",        "Write parameter",  "Changes one of the vehicle's settings (parameters). Some take effect at once and can change how it flies." },
        { "run_plugin",       "Run plugin",       "Presses one of the operator's custom plugin buttons, for this vehicle. What it does depends on the plugin; see list_plugins." },
    };
    return actions;
}

// ── Server ───────────────────────────────────────────────────────────────────

struct McpServer::Impl {
    httplib::Server svr;
};

struct McpServer::Pending {
    // Running is set on the UI thread before the action starts, so a waiter
    // that times out in that instant knows not to report "nothing was sent".
    enum class State { Waiting, Running, Done, Refused, Expired };

    McpApproval                   info;
    std::string                   rpc_key;      // session + JSON-RPC id, for cancellation
    bool                          ask = true;   // false: pump() runs it unasked
    std::function<std::string()>  action;
    State                         state = State::Waiting;
    std::string                   result;
};

McpServer::McpServer(Fleet& fleet) : fleet_(fleet) {}

McpServer::~McpServer() { stop(); }

bool McpServer::start(const McpConfig& cfg)
{
    stop();
    cfg_  = cfg;
    impl_ = std::make_unique<Impl>();
    httplib::Server& svr = impl_->svr;

    svr.Post("/mcp", [this](const httplib::Request& req, httplib::Response& res) {
        if (!origin_ok(req))              { res.status = 403; return; }
        if (!authorised(req, cfg_.token)) { res.status = 401; return; }
        int status = 200;
        std::string session = req.get_header_value("Mcp-Session-Id");
        const std::string in_session = session;
        const std::string out = handle(req.body, session, status);
        res.status = status;
        if (session != in_session) res.set_header("Mcp-Session-Id", session);
        if (!out.empty()) res.set_content(out, "application/json");
    });

    // No server-to-client stream and no sessions to end: the spec's answer to
    // both is 405.
    auto not_offered = [](const httplib::Request&, httplib::Response& res) {
        res.status = 405;
        res.set_header("Allow", "POST");
    };
    svr.Get("/mcp", not_offered);
    svr.Delete("/mcp", not_offered);

    if (!svr.bind_to_port("127.0.0.1", cfg_.port)) {
        gcs_log("mcp: cannot listen on 127.0.0.1:%d \xe2\x80\x94 port in use?", cfg_.port);
        impl_.reset();
        return false;
    }

    running_ = true;
    thread_  = std::thread([this] { impl_->svr.listen_after_bind(); });
    gcs_log("mcp: listening on http://127.0.0.1:%d/mcp", cfg_.port);
    return true;
}

void McpServer::stop()
{
    if (!running_.exchange(false)) return;

    // Turn down whatever is still queued first. Those requests are parked on
    // httplib's worker threads, and stopping the server joins its workers —
    // so they have to be let go before that, not after.
    {
        std::lock_guard<std::mutex> lk(queue_mtx_);
        for (auto& p : queue_) {
            p->state  = Pending::State::Refused;
            p->result = "The GCS was closed before this ran. Nothing was sent.";
        }
        queue_.clear();
    }
    queue_cv_.notify_all();

    impl_->svr.stop();
    if (thread_.joinable()) thread_.join();
    impl_.reset();
    gcs_log("mcp: stopped");
}

void McpServer::set_policy(McpPolicy p)
{
    std::lock_guard<std::mutex> lk(policy_mtx_);
    policy_ = std::move(p);
}

McpPolicy McpServer::policy() const
{
    std::lock_guard<std::mutex> lk(policy_mtx_);
    return policy_;
}

std::vector<McpClaim> McpServer::claims() const
{
    const auto now = std::chrono::steady_clock::now();
    std::vector<McpClaim> out;
    std::lock_guard<std::mutex> lk(coord_mtx_);
    for (const auto& [key, c] : claims_) {
        if (now - c.used >= CLAIM_IDLE) continue;
        out.push_back({ c.number, c.label,
                        std::chrono::duration<double>(now - c.used).count() });
    }
    return out;
}

void McpServer::release_all_claims()
{
    {
        std::lock_guard<std::mutex> lk(coord_mtx_);
        claims_.clear();
    }
    gcs_log("mcp: operator released every vehicle claim");
    event(5, "operator released every vehicle claim");
}

void McpServer::event(uint8_t severity, std::string text)
{
    std::lock_guard<std::mutex> lk(events_mtx_);
    events_.push_back({ severity, std::move(text) });
    while (events_.size() > EVENTS_MAX) events_.pop_front();
}

void McpServer::event_read(const std::string& what)
{
    const auto now = std::chrono::steady_clock::now();
    int count = 0;
    double span = 0;
    {
        std::lock_guard<std::mutex> lk(events_mtx_);
        ReadTally& t = read_tally_[what];
        ++t.count;
        const bool first = t.count == 1 && t.last.time_since_epoch().count() == 0;
        if (!first && now - t.last < READ_LOG_INTERVAL) return;
        count = t.count;
        span  = first ? 0 : std::chrono::duration<double>(now - t.last).count();
        t.last  = now;
        t.count = 0;
    }
    if (count <= 1) event(6, what);
    else            event(6, what + " \xc3\x97" + std::to_string(count) + " in " +
                             std::to_string((int)span) + " s");
}

std::vector<McpEvent> McpServer::take_events()
{
    std::lock_guard<std::mutex> lk(events_mtx_);
    std::vector<McpEvent> out(events_.begin(), events_.end());
    events_.clear();
    return out;
}

void McpServer::finish(const std::shared_ptr<Pending>& p)
{
    std::string result;
    try {
        result = p->action();
    } catch (const std::exception& e) {
        result = std::string("Failed: ") + e.what();
    } catch (...) {
        result = "Failed.";
    }
    {
        std::lock_guard<std::mutex> lk(queue_mtx_);
        p->result = std::move(result);
        p->state  = Pending::State::Done;
    }
    queue_cv_.notify_all();
}

void McpServer::pump()
{
    std::vector<std::shared_ptr<Pending>> run;
    {
        std::lock_guard<std::mutex> lk(queue_mtx_);
        for (auto it = queue_.begin(); it != queue_.end();) {
            if ((*it)->ask) { ++it; continue; }
            (*it)->state = Pending::State::Running;
            run.push_back(*it);
            it = queue_.erase(it);
        }
    }
    for (const auto& p : run) finish(p);
}

bool McpServer::pending_approval(McpApproval& out) const
{
    std::lock_guard<std::mutex> lk(queue_mtx_);
    for (const auto& p : queue_)
        if (p->ask) { out = p->info; return true; }
    return false;
}

void McpServer::resolve_approval(uint64_t id, bool approve)
{
    std::shared_ptr<Pending> p;
    {
        std::lock_guard<std::mutex> lk(queue_mtx_);
        auto it = std::find_if(queue_.begin(), queue_.end(),
                               [id](const auto& q) { return q->info.id == id; });
        if (it == queue_.end()) return;              // expired while the dialog was up
        p = *it;
        queue_.erase(it);
        p->state = approve ? Pending::State::Running : Pending::State::Refused;
    }
    gcs_log("mcp: %s %s", approve ? "approved" : "refused", p->info.summary.c_str());
    event(approve ? 5 : 3, std::string(approve ? "operator approved: " : "operator refused: ") +
                           p->info.summary);

    if (approve) finish(p);
    else         queue_cv_.notify_all();
}

void McpServer::cancel_request(const std::string& rpc_key)
{
    std::shared_ptr<Pending> p;
    {
        std::lock_guard<std::mutex> lk(queue_mtx_);
        auto it = std::find_if(queue_.begin(), queue_.end(),
                               [&](const auto& q) { return q->rpc_key == rpc_key; });
        if (it == queue_.end()) return;              // already ran, or never queued
        p = *it;
        queue_.erase(it);
        p->state  = Pending::State::Refused;
        p->result = "Cancelled by the client before it ran. Nothing was sent.";
    }
    gcs_log("mcp: cancelled by the client: %s", p->info.summary.c_str());
    event(3, "cancelled by the agent before it ran: " + p->info.summary);
    queue_cv_.notify_all();
}

std::string McpServer::request_action(const std::string& tool, const std::string& rpc_key,
                                      std::chrono::steady_clock::time_point deadline,
                                      McpApproval info, std::function<std::string()> action,
                                      bool& ok)
{
    auto p = std::make_shared<Pending>();
    p->action  = std::move(action);
    p->rpc_key = rpc_key;
    {
        std::lock_guard<std::mutex> lk(policy_mtx_);
        auto it = policy_.auto_allow.find(tool);
        p->ask = !(it != policy_.auto_allow.end() && it->second);
    }

    std::unique_lock<std::mutex> lk(queue_mtx_);
    if (!running_) { ok = false; return "The GCS is shutting its MCP server down."; }
    info.id = next_id_++;
    p->info = std::move(info);
    queue_.push_back(p);
    if (p->ask)
        gcs_log("mcp: awaiting approval: %s \xe2\x86\x92 %s",
                p->info.summary.c_str(), p->info.target.c_str());
    else
        gcs_log("mcp: allowed in settings: %s \xe2\x86\x92 %s%s%s",
                p->info.summary.c_str(), p->info.target.c_str(),
                p->info.reason.empty() ? "" : " \xe2\x80\x94 ", p->info.reason.c_str());
    {
        // "(2) sys1", without the link — the log line has to fit a sidebar.
        const std::string& t = p->info.target;
        const std::string who = t.substr(0, t.find(" on "));
        event(p->ask ? 4 : 5,
              std::string(p->ask ? "awaiting approval: " : "allowed in settings: ") +
              p->info.summary + " \xe2\x86\x92 " + who +
              (p->info.reason.empty() ? "" : " \xe2\x80\x94 \"" + p->info.reason + "\"") +
              (p->info.warning.empty() ? "" : " \xe2\x80\x94 SEPARATION: " + p->info.warning));
    }

    using S = Pending::State;
    const auto give_up = (std::min)(deadline, std::chrono::steady_clock::now() + APPROVAL_TIMEOUT);
    const bool answered = queue_cv_.wait_until(lk, give_up,
                                               [&] { return p->state != S::Waiting; });
    if (!answered) {
        p->state = S::Expired;
        queue_.erase(std::remove(queue_.begin(), queue_.end(), p), queue_.end());
        gcs_log("mcp: expired: %s", p->info.summary.c_str());
        event(3, std::string(p->ask ? "operator did not answer in time: " : "not run in time: ") +
                 p->info.summary);
        ok = false;
        return p->ask ? "The operator did not answer in time. Nothing was sent."
                      : "The GCS did not get to this in time. Nothing was sent.";
    }
    // Under way: the action is quick (it queues a frame), so wait it out.
    queue_cv_.wait(lk, [&] { return p->state != S::Running; });

    if (p->state == S::Refused) {
        ok = false;
        return p->result.empty() ? "The operator refused this. Nothing was sent." : p->result;
    }
    ok = true;
    return p->result;
}

// ── Tools ────────────────────────────────────────────────────────────────────

// The tool bodies, kept apart from the transport. A friend of McpServer for the
// fleet, the policy and the action queue.
struct McpTools {
    McpServer&  s;
    std::string rpc_key;    // this call's session + id, for cancellation

    // Every wait inside one call shares CALL_BUDGET, counted from its arrival.
    const std::chrono::steady_clock::time_point deadline =
        std::chrono::steady_clock::now() + CALL_BUDGET;

    std::chrono::steady_clock::time_point until(std::chrono::steady_clock::duration d) const
    { return (std::min)(deadline, std::chrono::steady_clock::now() + d); }

    // The vehicle a call means: by its displayed number, or the selected one.
    std::shared_ptr<Vehicle> vehicle(const json& args, std::string& err) const
    {
        if (args.contains("vehicle") && !args["vehicle"].is_null()) {
            if (!args["vehicle"].is_number_integer()) { err = "vehicle must be an integer"; return nullptr; }
            const uint32_t n = args["vehicle"].get<uint32_t>();
            for (const auto& v : s.fleet_.vehicles())
                if (v->number() == n) return v;
            err = "No vehicle (" + std::to_string(n) + "). Call list_vehicles.";
            return nullptr;
        }
        auto v = s.fleet_.active();
        if (!v) err = "No vehicle is connected.";
        return v;
    }

    static uint64_t key(const Vehicle& v)
    {
        return (static_cast<uint64_t>(v.link_id()) << 8) | v.sysid();
    }

    // May this call act on `v`? Yes when it is unclaimed, its claim has
    // lapsed, or the call carries the claim's token — which also refreshes it.
    bool claim_allows(const Vehicle& v, const json& args, std::string& err) const
    {
        const std::string token = args.value("claim", std::string());
        const auto now = std::chrono::steady_clock::now();
        std::lock_guard<std::mutex> lk(s.coord_mtx_);
        auto it = s.claims_.find(key(v));
        if (it == s.claims_.end()) return true;
        if (now - it->second.used >= CLAIM_IDLE) { s.claims_.erase(it); return true; }
        if (!token.empty() && token == it->second.token) { it->second.used = now; return true; }
        err = "Vehicle (" + std::to_string(v.number()) + ") is claimed by '" + it->second.label +
              "'. Only that claim's token can command it" +
              (token.empty() ? "; none was given." : "; the one given is not it.");
        return false;
    }

    // Who holds `v`, for list_vehicles: their label, or null.
    json claimed_by(const Vehicle& v) const
    {
        std::lock_guard<std::mutex> lk(s.coord_mtx_);
        auto it = s.claims_.find(key(v));
        if (it == s.claims_.end() ||
            std::chrono::steady_clock::now() - it->second.used >= CLAIM_IDLE) return nullptr;
        return it->second.label;
    }

    // Would flying `self` straight from where it is to (lat, lon, amsl) bring
    // it too close to another armed vehicle, or to the point another vehicle
    // in Guided was last sent to? Returns the concerns, or "" for none.
    //
    // Walks the path in PATH_SAMPLE_M steps and compares each point with the
    // others' positions — a straight line, which is what Guided flies, and
    // the others as they are now, not where they will be.
    std::string separation_check(const Vehicle& self, const VehicleState& vs,
                                 double to_lat, double to_lon, double to_amsl) const
    {
        if (!vs.has_global_pos) return {};
        const double from_lat = vs.lat, from_lon = vs.lon, from_amsl = vs.alt_asl;
        const double h_len = distance_m(from_lat, from_lon, to_lat, to_lon);
        const double len   = std::sqrt(h_len * h_len + (to_amsl - from_amsl) * (to_amsl - from_amsl));
        const int    n     = (std::max)(1, (std::min)(PATH_SAMPLES_MAX, (int)std::ceil(len / PATH_SAMPLE_M)));

        struct Point { double lat, lon, amsl; std::string what; };
        std::vector<Point> others;
        std::map<uint64_t, McpServer::Target> targets;
        {
            std::lock_guard<std::mutex> lk(s.coord_mtx_);
            targets = s.targets_;
        }
        for (const auto& o : s.fleet_.vehicles()) {
            if (o->id() == self.id()) continue;
            VehicleSnapshot snap;
            o->snapshot(snap);
            const VehicleState& os = snap.state;
            if (!os.armed || !os.has_global_pos) continue;
            const std::string name = "vehicle (" + std::to_string(o->number()) + ")";
            others.push_back({ os.lat, os.lon, os.alt_asl, name });
            auto t = targets.find(key(*o));
            if (t != targets.end() && iequals(mode_name(os), "Guided"))
                others.push_back({ t->second.lat, t->second.lon, t->second.amsl,
                                   "the point " + name + " is heading to" });
        }

        std::string out;
        for (const Point& p : others) {
            double best_h = 1e9, best_v = 1e9;
            for (int i = 0; i <= n; ++i) {
                const double f = (double)i / n;
                const double h = distance_m(from_lat + f * (to_lat - from_lat),
                                            from_lon + f * (to_lon - from_lon), p.lat, p.lon);
                const double v = std::fabs(from_amsl + f * (to_amsl - from_amsl) - p.amsl);
                if (h < SEPARATION_H_M && v < SEPARATION_V_M && h + v < best_h + best_v) {
                    best_h = h; best_v = v;
                }
            }
            if (best_h > 1e8) continue;
            char line[192];
            std::snprintf(line, sizeof line, "%sThis path passes %.0f m horizontally and %.0f m "
                          "vertically from %s.", out.empty() ? "" : " ", best_h, best_v, p.what.c_str());
            out += line;
        }
        return out;
    }

    std::string describe(const Vehicle& v) const
    {
        for (const auto& c : s.fleet_.chips())
            if (c.id == v.id())
                return "(" + std::to_string(v.number()) + ") sys" +
                       std::to_string(v.sysid()) + " on " + c.link_name;
        return "(" + std::to_string(v.number()) + ") sys" + std::to_string(v.sysid());
    }

    static std::string reason(const json& args)
    {
        return args.contains("reason") && args["reason"].is_string()
             ? args["reason"].get<std::string>() : std::string();
    }

    static bool number_arg(const json& args, const char* key, double& out)
    {
        if (!args.contains(key) || !args[key].is_number()) return false;
        out = args[key].get<double>();
        return std::isfinite(out);
    }

    // What the vehicle said to the COMMAND_LONG just sent, read off the
    // sender's ACK tracking: accepted, refused, or nothing within ACK_WAIT.
    std::string await_ack(MavlinkSender& sender, uint16_t cmd,
                          const std::string& what, const char* if_refused) const
    {
        const auto give_up = until(ACK_WAIT);
        while (std::chrono::steady_clock::now() < give_up) {
            switch (sender.query_flash(cmd)) {
            case CmdFlashState::Accepted:
                return what + ": accepted by the vehicle.";
            case CmdFlashState::Rejected:
                return what + ": REFUSED by the vehicle. " + if_refused;
            case CmdFlashState::Normal:
                return what + ": sent, no acknowledgement came back.";
            case CmdFlashState::Pending:
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
        }
        return what + ": sent, no acknowledgement in time.";
    }

    // ── Reads ────────────────────────────────────────────────────────────────

    json get_guidelines() const
    {
        const McpPolicy pol = s.policy();
        json allowed = json::array(), asks = json::array();
        if (pol.actions_enabled)
            for (const McpActionInfo& a : mcp_actions()) {
                auto it = pol.auto_allow.find(a.tool);
                (it != pol.auto_allow.end() && it->second ? allowed : asks).push_back(a.tool);
            }
        json out = {
            {"about", BUILTIN_INSTRUCTIONS},
            {"operator_rules", pol.instructions.empty() ? json(nullptr) : json(pol.instructions)},
            {"actions_enabled", pol.actions_enabled},
        };
        if (pol.actions_enabled) {
            out["runs_without_asking"]   = allowed;
            out["needs_operator_approval"] = asks;
        }
        return tool_json(out);
    }

    json list_vehicles() const
    {
        const VehicleId active = s.fleet_.active_id();
        json out = json::array();
        for (const auto& c : s.fleet_.chips()) {
            json row = { {"vehicle", c.number}, {"sysid", c.id.sysid},
                         {"link", c.link_name}, {"armed", c.armed},
                         {"stale", c.stale}, {"selected", c.id == active} };
            for (const auto& v : s.fleet_.vehicles()) {
                if (v->id() != c.id) continue;
                VehicleSnapshot snap;
                v->snapshot(snap);
                row["type"]      = type_name(snap.state.type);
                row["autopilot"] = firmware_profile(snap.state).name();
                row["mode"]      = mode_name(snap.state);
                row["claimed_by"] = claimed_by(*v);
                break;
            }
            out.push_back(std::move(row));
        }
        return tool_json(out);
    }

    json get_telemetry(const json& args) const
    {
        std::string err;
        auto v = vehicle(args, err);
        if (!v) return tool_text(err, true);
        VehicleSnapshot snap;
        v->snapshot(snap);
        return tool_json(telemetry_json(*v, snap));
    }

    json get_params(const json& args) const
    {
        std::string err;
        auto v = vehicle(args, err);
        if (!v) return tool_text(err, true);
        VehicleSnapshot snap;
        v->snapshot(snap);
        if (snap.params.empty())
            return tool_text("No parameters fetched yet. The operator can fetch them "
                             "from the PARAMS tab.", true);

        json out = json::object();
        if (args.contains("names") && args["names"].is_array()) {
            for (const auto& n : args["names"]) {
                if (!n.is_string()) continue;
                auto it = snap.params.find(n.get<std::string>());
                out[n.get<std::string>()] = it == snap.params.end()
                                          ? json(nullptr) : param_value(it->second);
            }
        } else {
            const std::string prefix = args.value("prefix", std::string());
            std::vector<const ParamEntry*> hits;
            for (const auto& [name, p] : snap.params)
                if (name.compare(0, prefix.size(), prefix) == 0) hits.push_back(&p);
            std::sort(hits.begin(), hits.end(), [](auto a, auto b) {
                return std::strcmp(a->param_id, b->param_id) < 0;
            });
            if (hits.size() > MAX_PARAMS_RETURNED) hits.resize(MAX_PARAMS_RETURNED);
            for (const ParamEntry* p : hits) out[p->param_id] = param_value(*p);
        }
        return tool_json(out);
    }

    json get_mission(const json& args) const
    {
        std::string err;
        auto v = vehicle(args, err);
        if (!v) return tool_text(err, true);
        VehicleSnapshot snap;
        v->snapshot(snap);

        // The same request the MISSION tab's download sends; the vehicle thread
        // drives the rest of the transfer, and mission_generation ticks once it
        // is whole. Only reads, so no permission is asked for.
        std::string note;
        if (args.value("download", false)) {
            const uint32_t before = snap.state.mission_generation;
            v->sender().request_mission_list(v->sysid(), v->compid());
            const auto give_up = until(DOWNLOAD_WAIT);
            for (;;) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                v->snapshot(snap);
                if (snap.state.mission_generation != before) break;
                if (std::chrono::steady_clock::now() >= give_up) {
                    note = "Download did not finish in time; this is what the GCS held before.";
                    break;
                }
            }
        }

        const VehicleState& vs = snap.state;
        json out = { {"has_mission", vs.has_mission}, {"has_fence", vs.has_fence} };
        json items = json::array(), fence = json::array();
        for (const auto& m : vs.mission) items.push_back(mission_item_json(m));
        for (const auto& m : vs.fence)   fence.push_back(mission_item_json(m));
        out["mission"] = std::move(items);
        out["fence"]   = std::move(fence);
        if (!note.empty()) out["note"] = note;
        return tool_json(out);
    }

    json get_messages(const json& args) const
    {
        std::string err;
        auto v = vehicle(args, err);
        if (!v) return tool_text(err, true);
        VehicleSnapshot snap;
        v->snapshot(snap);
        size_t limit = MAX_MESSAGES_RETURNED;
        if (args.contains("limit") && args["limit"].is_number_integer())
            limit = (std::min)(limit, (size_t)(std::max)(1, args["limit"].get<int>()));
        const auto& t = snap.status_texts;
        json out = json::array();
        for (size_t i = t.size() > limit ? t.size() - limit : 0; i < t.size(); ++i)
            out.push_back({ {"severity", t[i].severity}, {"text", t[i].text} });
        return tool_json(out);
    }

    json list_plugins() const
    {
        // The slot table is the registry's and is only renamed on the UI thread;
        // reading the strings here can race a rename. Harmless for a listing —
        // at worst a stale label — but not something to build on.
        json out = json::array();
        const auto& slots = gcs_plugins();
        for (size_t i = 0; i < slots.size(); ++i)
            out.push_back({ {"slot", i}, {"label", slots[i].label},
                            {"description", slots[i].tooltip} });
        return tool_json(out);
    }

    // ── Actions ──────────────────────────────────────────────────────────────

    // Queues `action` under `tool`'s policy and waits for it to run. On success
    // and with `ack_cmd` set, goes on to wait for the vehicle's COMMAND_ACK.
    // A claimed vehicle is refused here, before anything is queued, unless the
    // call carries its token. `warning` rides along to the operator's dialog,
    // the log and the result; it never stops the action.
    json act(const std::string& tool, const std::shared_ptr<Vehicle>& v, std::string summary,
             const json& args, std::function<std::string()> action,
             uint16_t ack_cmd = 0, const char* if_refused = "",
             const std::string& warning = {})
    {
        std::string err;
        if (!claim_allows(*v, args, err)) return tool_text(err, true);

        McpApproval info;
        info.target  = describe(*v);
        info.reason  = reason(args);
        info.summary = summary;
        info.warning = warning;
        if (!warning.empty())
            gcs_log("mcp: separation warning for %s: %s", summary.c_str(), warning.c_str());
        bool ok = false;
        std::string text = s.request_action(tool, rpc_key, deadline, std::move(info),
                                            std::move(action), ok);
        const std::string note = warning.empty() ? "" : "\nSEPARATION WARNING: " + warning;
        if (!ok) return tool_text(text, true);
        if (ack_cmd == 0) return tool_text(text + note);
        text = await_ack(v->sender(), ack_cmd, summary, if_refused);
        return tool_text(text + note, text.find("REFUSED") != std::string::npos);
    }

    json claim_vehicle(const json& args)
    {
        std::string err;
        auto v = vehicle(args, err);
        if (!v) return tool_text(err, true);
        const std::string label = args.value("label", std::string());
        if (label.empty()) return tool_text("label is required: say who you are", true);

        const auto now = std::chrono::steady_clock::now();
        std::string token;
        {
            std::lock_guard<std::mutex> lk(s.coord_mtx_);
            auto it = s.claims_.find(key(*v));
            if (it != s.claims_.end() && now - it->second.used < CLAIM_IDLE &&
                args.value("claim", std::string()) != it->second.token)
                return tool_text("Vehicle (" + std::to_string(v->number()) + ") is already "
                                 "claimed by '" + it->second.label + "'.", true);
            token = mcp_generate_token();
            s.claims_[key(*v)] = { token, label, v->number(), now };
        }
        gcs_log("mcp: (%u) claimed by '%s'", (unsigned)v->number(), label.c_str());
        return tool_json({ {"vehicle", v->number()}, {"claim", token},
                           {"note", "Pass this as `claim` with every action on this vehicle. "
                                    "It lapses after 15 minutes without use."} });
    }

    json release_vehicle(const json& args)
    {
        std::string err;
        auto v = vehicle(args, err);
        if (!v) return tool_text(err, true);
        std::lock_guard<std::mutex> lk(s.coord_mtx_);
        auto it = s.claims_.find(key(*v));
        if (it == s.claims_.end()) return tool_text("Not claimed.");
        if (args.value("claim", std::string()) != it->second.token)
            return tool_text("That is not the claim's token.", true);
        gcs_log("mcp: (%u) released by '%s'", (unsigned)v->number(), it->second.label.c_str());
        s.claims_.erase(it);
        return tool_text("Released.");
    }

    // Puts a vehicle in a mode, through the same encoding the mode grid uses.
    json switch_mode(const std::string& tool, const std::shared_ptr<Vehicle>& v,
                     const VehicleState& vs, const ModeChoice& m,
                     const std::string& summary, const json& args)
    {
        // Profiles are immortal singletons, so the pointer outlives the wait.
        const FirmwareProfile* prof = &firmware_profile(vs);
        const uint16_t cmd = prof->encode_set_mode(m.custom, m.standard).command;
        const uint8_t sys = v->sysid(), comp = v->compid();
        return act(tool, v, summary, args, [v, sys, comp, prof, m] {
            send_set_mode(v->sender(), sys, comp, *prof, m.custom, m.standard);
            return std::string("sent");
        }, cmd, "Check get_messages for why.");
    }

    json mode_not_found(const VehicleState& vs, const std::string& what) const
    {
        return tool_text(what + " Choose one of " + selectable_modes(vs).dump() +
                         (vs.available_modes.empty() || vs.modes_complete ? ""
                          : " (the mode list is still arriving; try again shortly)"),
                         true);
    }

    json set_mode(const json& args)
    {
        std::string err;
        auto v = vehicle(args, err);
        if (!v) return tool_text(err, true);
        if (!args.contains("mode") || !args["mode"].is_string())
            return tool_text("mode is required", true);
        const std::string want = args["mode"].get<std::string>();

        VehicleSnapshot snap;
        v->snapshot(snap);
        ModeChoice m;
        if (!find_mode(snap.state, [&](const std::string& n, uint8_t) { return iequals(n, want); }, m))
            return mode_not_found(snap.state, "Not a selectable mode for this vehicle: " + want + ".");
        return switch_mode("set_mode", v, snap.state, m, "SET MODE \xe2\x86\x92 " + m.name, args);
    }

    json start_mission(const json& args)
    {
        std::string err;
        auto v = vehicle(args, err);
        if (!v) return tool_text(err, true);
        VehicleSnapshot snap;
        v->snapshot(snap);
        ModeChoice m;
        if (!find_mode(snap.state, [](const std::string& n, uint8_t std_mode) {
                return std_mode == MAV_STANDARD_MODE_MISSION ||
                       iequals(n, "Auto") || iequals(n, "Mission");
            }, m))
            return mode_not_found(snap.state, "This vehicle lists no mission mode.");
        return switch_mode("start_mission", v, snap.state, m,
                           "START MISSION (mode " + m.name + ")", args);
    }

    json pause_mission(const json& args)
    {
        std::string err;
        auto v = vehicle(args, err);
        if (!v) return tool_text(err, true);
        VehicleSnapshot snap;
        v->snapshot(snap);
        // A mode that holds position on its own, in order of preference. Not
        // Loiter first: ArduCopter's Loiter takes its climb rate from the
        // throttle stick, and with nobody on the sticks that stick is at the
        // bottom — the copter descends and lands. Seen in SITL. Hold (PX4,
        // Rover) and Guided (ArduCopter, ArduPlane) stay put without a pilot.
        ModeChoice m;
        bool found = false;
        for (const char* want : { "Hold", "Guided", "Loiter" }) {
            if (find_mode(snap.state, [want](const std::string& n, uint8_t) {
                    return iequals(n, want);
                }, m)) { found = true; break; }
        }
        if (!found)
            return mode_not_found(snap.state, "This vehicle lists no Hold, Guided or Loiter "
                                              "mode; use set_mode.");
        return switch_mode("pause_mission", v, snap.state, m,
                           "PAUSE MISSION (mode " + m.name + ")", args);
    }

    json arm(const json& args)
    {
        std::string err;
        auto v = vehicle(args, err);
        if (!v) return tool_text(err, true);
        VehicleSnapshot snap;
        v->snapshot(snap);
        if (snap.state.armed) return tool_text("Already armed. Nothing was sent.");
        const uint8_t sys = v->sysid(), comp = v->compid();
        return act("arm", v, "ARM", args, [v, sys, comp] {
            v->sender().arm(sys, comp);
            return std::string("sent");
        }, MAV_CMD_COMPONENT_ARM_DISARM,
           "Prearm checks probably failed; get_messages says which.");
    }

    json disarm(const json& args)
    {
        std::string err;
        auto v = vehicle(args, err);
        if (!v) return tool_text(err, true);
        VehicleSnapshot snap;
        v->snapshot(snap);
        if (!snap.state.armed) return tool_text("Already disarmed. Nothing was sent.");
        const uint8_t sys = v->sysid(), comp = v->compid();
        return act("disarm", v, "DISARM", args, [v, sys, comp] {
            v->sender().disarm(sys, comp);
            return std::string("sent");
        }, MAV_CMD_COMPONENT_ARM_DISARM,
           "Autopilots refuse to disarm while flying; land first.");
    }

    json takeoff(const json& args)
    {
        std::string err;
        auto v = vehicle(args, err);
        if (!v) return tool_text(err, true);
        double alt = 0;
        if (!number_arg(args, "alt_m", alt) || alt <= 0.0 || alt > 1000.0)
            return tool_text("alt_m must be above 0 and at most 1000 (metres above home)", true);

        VehicleSnapshot snap;
        v->snapshot(snap);
        const VehicleState& vs = snap.state;
        // Straight up: from here to the same point, alt metres above home.
        const std::string warning = separation_check(*v, vs, vs.lat, vs.lon,
                                                     vs.alt_asl - vs.alt_rel + alt);

        char summary[64];
        std::snprintf(summary, sizeof summary, "TAKEOFF to %.1f m", alt);
        const uint8_t sys = v->sysid(), comp = v->compid();
        return act("takeoff", v, summary, args, [v, sys, comp, alt] {
            v->sender().takeoff(sys, comp, (float)alt);
            return std::string("sent");
        }, MAV_CMD_NAV_TAKEOFF,
           "It must be armed, and ArduPilot takes off only in Guided.", warning);
    }

    json go_to(const json& args)
    {
        std::string err;
        auto v = vehicle(args, err);
        if (!v) return tool_text(err, true);

        // Where to, in one of two forms: absolute lat/lon, or an offset in
        // metres from where the vehicle is now. The offset is for agents that
        // think in "100 m north" — the GCS does the arithmetic, not the model.
        double lat = 0, lon = 0, north = 0, east = 0;
        const bool has_lat = number_arg(args, "lat", lat), has_lon = number_arg(args, "lon", lon);
        const bool has_n = number_arg(args, "north_m", north), has_e = number_arg(args, "east_m", east);
        const bool absolute = has_lat || has_lon, offset = has_n || has_e;
        if (absolute && offset)
            return tool_text("Give lat/lon or north_m/east_m, not both", true);
        if (!absolute && !offset)
            return tool_text("Give lat and lon, or north_m and/or east_m", true);
        if (absolute && !(has_lat && has_lon))
            return tool_text("lat and lon go together", true);
        double alt = 0, up = 0;
        const bool has_alt = number_arg(args, "alt_m", alt), has_up = number_arg(args, "up_m", up);
        if (has_alt && has_up)
            return tool_text("Give alt_m or up_m, not both", true);

        VehicleSnapshot snap;
        v->snapshot(snap);
        const VehicleState& vs = snap.state;
        if ((offset || !has_alt) && !vs.has_global_pos)
            return tool_text("The vehicle has no position yet, so an offset or its current "
                             "altitude cannot be worked out. Give lat, lon and alt_m.", true);
        if (offset) {
            // Flat-earth about the vehicle: well inside a metre over the
            // few kilometres a goto covers.
            constexpr double R = 6371000.0, R2D = 180.0 / 3.14159265358979323846;
            lat = vs.lat + north / R * R2D;
            lon = vs.lon + east / (R * std::cos(vs.lat / R2D)) * R2D;
            if (lon > 180)  lon -= 360;
            if (lon < -180) lon += 360;
        }
        if (lat < -90 || lat > 90 || lon < -180 || lon > 180)
            return tool_text("lat/lon out of range", true);
        if (!has_alt) alt = vs.alt_rel + up;
        // The same gate the map's "go here" uses: on ArduPilot a target outside
        // Guided is dropped without a word, so say so instead of sending it.
        const Capability cap = firmware_profile(vs).position_target(vs);
        if (!cap.supported)
            return tool_text(std::string("Cannot send a position target now: ") +
                             (cap.reason ? cap.reason : "unsupported") +
                             ". On ArduPilot, set_mode Guided first.", true);

        char summary[160];
        if (offset)
            std::snprintf(summary, sizeof summary, "GO TO %.0f m %s, %.0f m %s → %.6f, %.6f at %.1f m",
                          std::fabs(north), north < 0 ? "S" : "N",
                          std::fabs(east),  east  < 0 ? "W" : "E", lat, lon, alt);
        else if (vs.has_global_pos)
            std::snprintf(summary, sizeof summary, "GO TO %.6f, %.6f at %.1f m (%.0f m away)",
                          lat, lon, alt, distance_m(vs.lat, vs.lon, lat, lon));
        else
            std::snprintf(summary, sizeof summary, "GO TO %.6f, %.6f at %.1f m", lat, lon, alt);

        // alt_m is above this vehicle's home; the others are compared in AMSL.
        const double to_amsl = vs.alt_asl - vs.alt_rel + alt;
        const std::string warning = separation_check(*v, vs, lat, lon, to_amsl);

        const FirmwareProfile* prof = &firmware_profile(vs);
        const bool acks = prof->goto_method() == FirmwareProfile::GotoMethod::Reposition;
        const uint8_t sys = v->sysid(), comp = v->compid();
        McpServer* srv = &s;
        const uint64_t k = key(*v);
        return act("goto", v, summary, args, [v, sys, comp, prof, lat, lon, alt, srv, k, to_amsl] {
            send_goto(v->sender(), sys, comp, *prof, lat, lon, (float)alt);
            {
                std::lock_guard<std::mutex> lk(srv->coord_mtx_);
                srv->targets_[k] = { lat, lon, to_amsl };
            }
            return std::string("Sent. Position targets are not acknowledged; watch "
                               "get_telemetry for the vehicle moving.");
        }, acks ? MAV_CMD_DO_REPOSITION : 0, "Check its mode and get_messages.", warning);
    }

    json upload_mission(const json& args)
    {
        std::string err;
        auto v = vehicle(args, err);
        if (!v) return tool_text(err, true);
        if (!args.contains("items") || !args["items"].is_array() || args["items"].empty())
            return tool_text("items must be a non-empty array", true);
        if (args["items"].size() > MAX_MISSION_ITEMS)
            return tool_text("At most 1000 items", true);

        std::vector<MissionItem> plan;
        for (size_t i = 0; i < args["items"].size(); ++i) {
            const json& j = args["items"][i];
            const std::string at = "item " + std::to_string(i) + ": ";
            if (!j.is_object()) return tool_text(at + "not an object", true);

            MissionItem m{};
            const json& c = j.contains("command") ? j["command"] : json();
            if (c.is_number_integer()) {
                m.command = c.get<uint16_t>();
            } else if (c.is_string()) {
                bool known = false;
                for (const auto& [n, id] : MISSION_COMMANDS)
                    if (iequals(n, c.get<std::string>())) { m.command = id; known = true; }
                if (!known) return tool_text(at + "unknown command " + c.get<std::string>(), true);
            } else {
                return tool_text(at + "command is required", true);
            }

            const bool needs_pos = m.command >= 16 && m.command <= 19;   // waypoint, loiters
            double d = 0;
            if (number_arg(j, "lat", d)) m.lat = d; else if (needs_pos) return tool_text(at + "lat is required", true);
            if (number_arg(j, "lon", d)) m.lon = d; else if (needs_pos) return tool_text(at + "lon is required", true);
            if (m.lat < -90 || m.lat > 90 || m.lon < -180 || m.lon > 180)
                return tool_text(at + "lat/lon out of range", true);
            if (number_arg(j, "alt_m", d)) m.alt = (float)d;
            m.frame = j.value("frame", 3);
            if (j.contains("params") && j["params"].is_array()) {
                float* p[4] = { &m.param1, &m.param2, &m.param3, &m.param4 };
                for (size_t k = 0; k < 4 && k < j["params"].size(); ++k)
                    if (j["params"][k].is_number()) *p[k] = j["params"][k].get<float>();
            }
            plan.push_back(m);
        }

        VehicleSnapshot snap;
        v->snapshot(snap);
        if (v->sender().upload_active())
            return tool_text("An upload to this vehicle is already in progress.", true);
        const std::vector<MissionItem> wire = mission_for_wire(snap.state, plan);
        const size_t n = plan.size();

        const uint8_t sys = v->sysid(), comp = v->compid();
        json r = act("upload_mission", v,
                     "UPLOAD MISSION: " + std::to_string(n) + " item" + (n == 1 ? "" : "s"),
                     args, [v, sys, comp, wire] {
                         v->sender().start_upload(sys, comp, wire);
                         return std::string("started");
                     });
        if (r.value("isError", false)) return r;

        // The vehicle pulls the items one by one; the vehicle thread clears the
        // stored upload once MISSION_ACK lands, whichever way it went.
        const auto give_up = until(UPLOAD_WAIT);
        while (v->sender().upload_active()) {
            if (std::chrono::steady_clock::now() >= give_up)
                return tool_text("Upload started but the vehicle has not finished it yet. "
                                 "The MISSION tab shows its progress.", true);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        v->snapshot(snap);
        if (snap.state.upload_status == VehicleState::UploadStatus::Accepted)
            return tool_text("Mission accepted by the vehicle: " + std::to_string(n) + " items.");
        return tool_text("Mission upload FAILED (MAV_MISSION_RESULT " +
                         std::to_string(snap.state.upload_ack_result) + ").", true);
    }

    json set_param(const json& args)
    {
        std::string err;
        auto v = vehicle(args, err);
        if (!v) return tool_text(err, true);
        double value = 0;
        if (!args.contains("name") || !args["name"].is_string() || !number_arg(args, "value", value))
            return tool_text("name (string) and value (finite number) are required", true);
        const std::string name = args["name"].get<std::string>();

        VehicleSnapshot snap;
        v->snapshot(snap);
        auto it = snap.params.find(name);
        if (it == snap.params.end())
            return tool_text("Unknown parameter " + name + ", or the table has not "
                             "been fetched yet.", true);
        const ParamEntry p = it->second;
        if (param_is_integer(p.type) && value != std::floor(value))
            return tool_text(name + " is an integer parameter", true);

        char summary[128];
        std::snprintf(summary, sizeof summary, "SET %s: %g \xe2\x86\x92 %g",
                      p.param_id, p.value, value);
        const uint8_t sys = v->sysid(), comp = v->compid();
        return act("set_param", v, summary, args, [v, sys, comp, p, value] {
            v->sender().set_param(sys, comp, p.param_id, (float)value, p.type);
            return std::string("Sent. The vehicle echoes the stored value; read it back "
                               "with get_params to confirm.");
        });
    }

    json return_to_launch(const json& args)
    {
        std::string err;
        auto v = vehicle(args, err);
        if (!v) return tool_text(err, true);
        const uint8_t sys = v->sysid(), comp = v->compid();
        return act("return_to_launch", v, "RETURN TO LAUNCH", args, [v, sys, comp] {
            v->sender().return_to_launch(sys, comp);
            return std::string("sent");
        }, MAV_CMD_NAV_RETURN_TO_LAUNCH, "Check get_messages for why.");
    }

    json run_plugin(const json& args)
    {
        std::string err;
        auto v = vehicle(args, err);
        if (!v) return tool_text(err, true);
        if (!args.contains("slot") || !args["slot"].is_number_integer())
            return tool_text("slot is required", true);
        const int slot = args["slot"].get<int>();
        const auto& slots = gcs_plugins();
        if (slot < 0 || (size_t)slot >= slots.size())
            return tool_text("No plugin in slot " + std::to_string(slot), true);
        if (!s.plugin_runner_)
            return tool_text("Plugins cannot be run from here in this build.", true);

        McpServer::PluginRunner run = s.plugin_runner_;
        return act("run_plugin", v, "PLUGIN " + slots[slot].label, args, [v, slot, run] {
            return run(slot, *v) ? std::string("Plugin ran; see the GCS log for its output.")
                                 : std::string("Plugin slot vanished.");
        });
    }

    json call(const std::string& name, const json& args)
    {
        if (name == "get_guidelines") return get_guidelines();
        if (name == "list_vehicles")  return list_vehicles();
        if (name == "get_telemetry")  return get_telemetry(args);
        if (name == "get_params")     return get_params(args);
        if (name == "get_mission")    return get_mission(args);
        if (name == "get_messages")   return get_messages(args);
        if (name == "list_plugins")   return list_plugins();

        // Checked per call, not when the tool list was sent: the operator can
        // switch actions off in SETTINGS under a client that already has it.
        if (!s.policy().actions_enabled)
            return tool_text("Actions are switched off in the GCS settings.", true);
        if (name == "claim_vehicle")    return claim_vehicle(args);
        if (name == "release_vehicle")  return release_vehicle(args);
        if (name == "arm")              return arm(args);
        if (name == "disarm")           return disarm(args);
        if (name == "takeoff")          return takeoff(args);
        if (name == "goto")             return go_to(args);
        if (name == "set_mode")         return set_mode(args);
        if (name == "start_mission")    return start_mission(args);
        if (name == "pause_mission")    return pause_mission(args);
        if (name == "upload_mission")   return upload_mission(args);
        if (name == "return_to_launch") return return_to_launch(args);
        if (name == "set_param")        return set_param(args);
        if (name == "run_plugin")       return run_plugin(args);
        return tool_text("Unknown tool: " + name, true);
    }
};

// ── JSON-RPC dispatch ────────────────────────────────────────────────────────

std::string McpServer::handle(const std::string& body, std::string& session, int& http_status)
{
    json msg = json::parse(body, nullptr, false);
    if (msg.is_discarded())
        return rpc_error(nullptr, RPC_PARSE_ERROR, "Parse error").dump();
    if (!msg.is_object() || !msg.contains("method") || !msg["method"].is_string())
        return rpc_error(nullptr, RPC_INVALID_REQUEST,
                         "Expected one JSON-RPC request object").dump();

    const std::string method = msg["method"].get<std::string>();
    const json        params = msg.value("params", json::object());

    // Request ids are only unique per client, so a cancellation is matched on
    // the session as well: one client cannot withdraw another's request 5.
    auto rpc_key = [&](const json& id) { return session + "/" + id.dump(); };

    // A notification (no id) or a response from the client: nothing to send
    // back. 202 is the spec's answer for both.
    if (!msg.contains("id")) {
        if (method == "notifications/cancelled" && params.contains("requestId"))
            cancel_request(rpc_key(params["requestId"]));
        http_status = 202;
        return {};
    }

    const json id = msg["id"];

    if (method == "initialize") {
        session = mcp_generate_token();
        {
            const json ci = params.value("clientInfo", json::object());
            const std::string name = ci.is_object() ? ci.value("name", std::string("unnamed client")) : "unnamed client";
            const std::string ver  = ci.is_object() ? ci.value("version", std::string()) : "";
            event(6, "agent connected: " + name + (ver.empty() ? "" : " " + ver));
        }
        std::string version = PROTOCOL_VERSIONS[0];
        const std::string asked = params.value("protocolVersion", std::string());
        for (const char* v : PROTOCOL_VERSIONS)
            if (asked == v) version = v;
        return rpc_result(id, {
            {"protocolVersion", version},
            {"capabilities", { {"tools", json::object()} }},
            {"serverInfo", { {"name", "freebodyproblem"}, {"version", "0.3"} }},
            {"instructions", [&] {
                const std::string rules = policy().instructions;
                return rules.empty() ? std::string(BUILTIN_INSTRUCTIONS)
                                     : std::string(BUILTIN_INSTRUCTIONS) +
                                       "\n\nThe operator's rules for agents:\n" + rules;
            }()},
        }).dump();
    }

    if (method == "ping") return rpc_result(id, json::object()).dump();

    if (method == "tools/list") {
        json tools = read_tools();
        const McpPolicy pol = policy();
        if (pol.actions_enabled) {
            for (const McpActionInfo& a : mcp_actions()) {
                auto it = pol.auto_allow.find(a.tool);
                const bool unasked = it != pol.auto_allow.end() && it->second;
                tools.push_back({
                    {"name", a.tool},
                    {"description", std::string(a.hint) +
                        (unasked ? " Runs immediately: the operator allowed it in settings."
                                 : " Waits up to 45 s for the operator to approve it on screen.")},
                    {"inputSchema", [&] {
                        json sch = action_schema(a.tool);
                        sch["properties"]["claim"] = CLAIM_ARG;
                        return sch;
                    }()},
                    {"annotations", { {"readOnlyHint", false}, {"destructiveHint", true} }},
                });
            }
        }
        if (pol.actions_enabled)
            for (auto& t : claim_tools()) tools.push_back(std::move(t));
        return rpc_result(id, { {"tools", std::move(tools)} }).dump();
    }

    if (method == "tools/call") {
        if (!params.contains("name") || !params["name"].is_string())
            return rpc_error(id, RPC_INVALID_PARAMS, "tools/call needs a name").dump();
        const std::string name = params["name"].get<std::string>();
        const json args = params.contains("arguments") && params["arguments"].is_object()
                        ? params["arguments"] : json::object();
        // Claim tokens are credentials between agents; the log shows that one
        // was passed, not what it was.
        json shown = args;
        if (shown.contains("claim")) shown["claim"] = "\xe2\x80\xa6";
        gcs_log("mcp: %s %s", name.c_str(), shown.dump().c_str());
        McpTools tools{*this, rpc_key(id)};
        const json result = tools.call(name, args);

        // Into the event log. Which vehicle: the one named, or the selected
        // one the call will have used.
        std::string who;
        if (args.contains("vehicle") && args["vehicle"].is_number_integer())
            who = " (" + std::to_string(args["vehicle"].get<long long>()) + ")";
        else if (auto v = fleet_.active())
            who = " (" + std::to_string(v->number()) + ")";
        if (name == "list_vehicles" || name == "list_plugins" || name == "get_guidelines")
            who.clear();

        const bool failed = result.value("isError", false);
        std::string text = result["content"][0].value("text", std::string());
        // A reply that is data, not a sentence, is not copied into the log —
        // claim_vehicle's carries the claim token, which must stay out of it.
        if (!failed && !text.empty() && (text[0] == '{' || text[0] == '['))
            text = name == "claim_vehicle" ? "claimed as '" + args.value("label", std::string()) + "'"
                                           : "done";
        text = text.substr(0, text.find('\n'));
        if (text.size() > 160) text = text.substr(0, 157) + "...";

        const bool is_read = name.rfind("get_", 0) == 0 || name.rfind("list_", 0) == 0;
        if (is_read && !failed) {
            event_read(name + who);
        } else {
            // Red for what the vehicle turned down or that broke; orange for
            // what was refused before it got that far; accent for done.
            const bool vehicle_said_no = text.find("REFUSED by the vehicle") != std::string::npos ||
                                         text.find("FAILED") != std::string::npos;
            event(!failed ? 5 : vehicle_said_no ? 2 : 3, name + who + ": " + text);
        }
        return rpc_result(id, result).dump();
    }

    return rpc_error(id, RPC_METHOD_NOT_FOUND, "Method not found: " + method).dump();
}

std::string mcp_generate_token()
{
    std::random_device rd;
    char out[33];
    for (int i = 0; i < 16; ++i)
        std::snprintf(out + i * 2, 3, "%02x", (unsigned)(rd() & 0xFF));
    return std::string(out, 32);
}
