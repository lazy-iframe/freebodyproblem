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

// MavlinkParser: what the vehicle says it is — its mode list, firmware version, components.
// Every MAVLink message received is decoded by MavlinkParser; handle() in
// parser.cpp dispatches here.

#include "backend/mavlink/parser/parser.hpp"
#include "backend/vehicle/firmware_profile.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "backend/mavlink/signing.hpp"
#include "frontend/source/app_log.hpp"   // gcs_log — thread-safe, see the header

namespace {

// The name of a MAV_STANDARD_MODE, for a vehicle that gave a mode number
// without a string.
//
// Not invention: a flight stack publishing a standard mode is entitled to leave
// mode_name empty precisely because the number already says what the mode is,
// and PX4 does exactly that — of the 28 modes it advertises, the standard ones
// arrive nameless. Without this they reach the UI as "?" buttons.
const char* standard_mode_name(uint8_t standard_mode)
{
    switch (standard_mode) {
    case 1: return "Position Hold";
    case 2: return "Orbit";
    case 3: return "Cruise";
    case 4: return "Altitude Hold";
    case 5: return "Safe Recovery";
    case 6: return "Mission";
    case 7: return "Land";
    case 8: return "Takeoff";
    default: return nullptr;   // 0 is NON_STANDARD: the vehicle owed us a name
    }
}

} // namespace

void MavlinkParser::on_available_modes(const mavlink_message_t& msg)
{
    mavlink_available_modes_t am;
    mavlink_msg_available_modes_decode(&msg, &am);

    // mode_index is 1-based and is the protocol's identity for a mode, so
    // it is the dedup key: the link thread re-requests indices that went
    // missing, and those replies must replace rather than duplicate.
    state_.modes_expected = am.number_modes;

    // mode_name is not guaranteed NUL-terminated when it fills the field.
    char name[sizeof(am.mode_name) + 1] = {};
    std::memcpy(name, am.mode_name, sizeof(am.mode_name));

    FlightModeInfo info;
    info.mode_index    = am.mode_index;
    info.custom_mode   = am.custom_mode;
    info.standard_mode = am.standard_mode;
    info.properties    = am.properties;
    info.name          = name;

    // A standard mode may arrive with no name of its own, the number being
    // the name. Fill it from the enum so every consumer — the mode grid,
    // the topbar chip, the RC slot editor — has something to print.
    if (info.name.empty()) {
        if (const char* std_name = standard_mode_name(info.standard_mode))
            info.name = std_name;
    }

    auto it = std::find_if(state_.available_modes.begin(),
                           state_.available_modes.end(),
                           [&](const FlightModeInfo& m) {
                               return m.mode_index == info.mode_index;
                           });
    const bool replaced = (it != state_.available_modes.end());
    if (replaced) {
        *it = info;
    } else {
        // Kept in mode_index order: replies arrive in request order, which
        // is not necessarily ascending after a retry sweep, and the UI
        // renders the list as-is.
        auto pos = std::lower_bound(state_.available_modes.begin(),
                                    state_.available_modes.end(),
                                    info.mode_index,
                                    [](const FlightModeInfo& m, uint8_t idx) {
                                        return m.mode_index < idx;
                                    });
        state_.available_modes.insert(pos, info);
    }

    state_.modes_complete =
        (state_.modes_expected > 0) &&
        (state_.available_modes.size() >= state_.modes_expected);

    // DEBUG: dumps every field the vehicle reported. "replace" means this
    // custom_mode was already in the list — a vehicle that sends distinct
    // modes all carrying the same custom_mode would collapse to one entry.
    std::fprintf(stderr,
                 "[modes] rx AVAILABLE_MODES from %u/%u  idx=%u/%u  "
                 "std=%u custom=%u props=0x%X  name=\"%s\"  (%s, have %zu)%s\n",
                 msg.sysid, msg.compid,
                 am.mode_index, am.number_modes,
                 am.standard_mode, am.custom_mode, am.properties, name,
                 replaced ? "replace" : "new",
                 state_.available_modes.size(),
                 state_.modes_complete ? "  COMPLETE" : "");
}

void MavlinkParser::on_available_modes_monitor(const mavlink_message_t& msg)
{
    mavlink_available_modes_monitor_t mm;
    mavlink_msg_available_modes_monitor_decode(&msg, &mm);

    // First sighting only records the sequence; a later change means the
    // vehicle's mode list has been rebuilt (frame change, reboot) and what
    // we hold is stale.
    const bool changed = state_.has_modes_seq && mm.seq != state_.modes_seq;
    if (changed) {
        state_.available_modes.clear();
        state_.modes_expected = 0;
        state_.modes_complete = false;
        state_.modes_dirty    = true;
    }
    // DEBUG: only on change / first sighting — the vehicle streams this
    // one periodically and printing every copy would drown the log.
    if (changed || !state_.has_modes_seq)
        std::fprintf(stderr, "[modes] rx AVAILABLE_MODES_MONITOR seq=%u%s\n",
                     mm.seq, changed ? "  (CHANGED - list cleared)" : "  (first)");
    state_.modes_seq     = mm.seq;
    state_.has_modes_seq = true;
}

void MavlinkParser::on_autopilot_version(const mavlink_message_t& msg)
{
    mavlink_autopilot_version_t av;
    mavlink_msg_autopilot_version_decode(&msg, &av);
    // Version packed as major(bits 31:24) minor(23:16) patch(15:8) type(7:0)
    const uint8_t major = (av.flight_sw_version >> 24) & 0xFF;
    const uint8_t minor = (av.flight_sw_version >> 16) & 0xFF;
    const uint8_t patch = (av.flight_sw_version >>  8) & 0xFF;
    snprintf(state_.fw_version, sizeof(state_.fw_version),
             "%u.%u.%u", major, minor, patch);
    // flight_custom_version: 8 bytes → 16 hex chars
    snprintf(state_.fw_hash, sizeof(state_.fw_hash),
             "%02x%02x%02x%02x%02x%02x%02x%02x",
             av.flight_custom_version[0], av.flight_custom_version[1],
             av.flight_custom_version[2], av.flight_custom_version[3],
             av.flight_custom_version[4], av.flight_custom_version[5],
             av.flight_custom_version[6], av.flight_custom_version[7]);
    state_.vendor_id     = av.vendor_id;
    state_.product_id    = av.product_id;
    state_.board_version = av.board_version;
    state_.capabilities  = av.capabilities;
    state_.board_uid     = av.uid;
    state_.has_fw_info = true;
}

void MavlinkParser::on_component_information(const mavlink_message_t& msg)
{
    mavlink_component_information_t ci;
    mavlink_msg_component_information_decode(&msg, &ci);
    // The URI is 100 chars and need not be terminated when it fills the
    // field, so the copy leaves the last byte of ours alone.
    std::memcpy(state_.comp_info_uri, ci.general_metadata_uri, 100);
    state_.comp_info_uri[100] = '\0';
    state_.comp_info_compid   = msg.compid;
    state_.has_comp_info      = true;
}
