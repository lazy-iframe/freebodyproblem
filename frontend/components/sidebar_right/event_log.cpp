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

// Right sidebar: the fleet event log — what goes into it.

#define _USE_MATH_DEFINES
#include <cmath>
#include "frontend/elements/sidebar_right.hpp"
#include "frontend/components/sidebar_right/sidebar_internal.hpp"
#include "frontend/elements/center_view.hpp"
#include "frontend/source/vehicle_ui_state.hpp"
#include "frontend/source/layout.hpp"
#include "frontend/source/theme.hpp"
#include "frontend/components/others/ui_kit.hpp"
#include "imgui.h"
#include "frontend/components/sidebar_left/mavlink_field_catalog_generated.hpp"
#include "frontend/source/settings.hpp"
#include "lib/text.hpp"
#include <vector>
#include <cstdio>
#include <cstring>
#include <limits>
#include <cfloat>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <deque>
#include <cctype>
#include <ctime>
#include <string>

namespace right_sidebar {

// Bounded for the same reason the per-vehicle deque behind it is: a talkative
// airframe must not grow this without limit over a long flight. Deeper than one
// vehicle's 200, since several vehicles now share it.
static constexpr size_t FLEET_LOG_MAX = 800;

std::deque<FleetLogEntry> g_fleet_log;

// Which vehicles have actually said something. The "[2]" prefix is only drawn
// once this holds more than one — with a single aircraft talking there is
// nothing to tell apart, and the prefix would be noise on every line.
std::unordered_set<uint32_t> g_fleet_log_numbers;

// What the log has taken in from each vehicle so far. Its own map rather than a
// VehicleUiState: event_log_vehicle_gone() has to read it for a vehicle that is
// no longer bound to anything, and erases it itself once it has.
struct FleetLogCursor {
    uint32_t ingested        = 0;      // statustext_total already taken in
    uint32_t reboots         = 0;      // VehicleState::reboot_count already marked
    uint32_t reboot_requests = 0;      // MavlinkSender::reboot_requests() already marked
    bool     lost            = false;  // a LinkLost marker is standing for it
};
static std::unordered_map<VehicleId, FleetLogCursor, VehicleIdHash> g_fleet_log_cursors;

static std::string wall_stamp()
{
    const std::time_t t = std::time(nullptr);
    std::tm            lt{};
#ifdef _WIN32
    localtime_s(&lt, &t);
#else
    localtime_r(&t, &lt);
#endif
    return text_hms(lt.tm_hour * 3600 + lt.tm_min * 60 + lt.tm_sec);
}

static void fleet_log_trim()
{
    while (g_fleet_log.size() > FLEET_LOG_MAX) g_fleet_log.pop_front();
}

// Every entry gets a number of its own on the way in. A selection is held as
// the numbers of the lines at its two ends rather than their positions: the
// log drops lines off its front as it fills and a reboot marker can land in
// its middle, and either would slide a selection held by position onto lines
// the operator never chose.
static uint64_t g_fleet_log_seq = 0;

static void fleet_log_add(FleetLogEntry e)
{
    e.seq = ++g_fleet_log_seq;
    g_fleet_log.push_back(std::move(e));
    fleet_log_trim();
}

static FleetLogEntry fleet_log_marker(FleetLogKind kind, VehicleId vid,
                                      uint32_t number, const char* what)
{
    char text[64];
    snprintf(text, sizeof(text), "VEHICLE %u %s", (unsigned)number, what);
    return FleetLogEntry{ kind, vid, number, 0, 0, wall_stamp(), text };
}

// The reboot line goes where the old boot's messages end, which is not always
// the end of the log: the reboot is noticed only when the new boot's clock is
// first read, and by then its opening STATUSTEXTs may already be in. So the
// marker goes before the first of this vehicle's messages numbered past
// `mark`, and takes that message's stamp so the clock never runs backwards.
static void fleet_log_insert_reboot(VehicleId vid, uint32_t number, uint32_t mark)
{
    FleetLogEntry m = fleet_log_marker(FleetLogKind::Rebooted, vid, number, "REBOOTED");

    size_t at = g_fleet_log.size();
    for (size_t i = g_fleet_log.size(); i-- > 0; ) {
        const FleetLogEntry& e = g_fleet_log[i];
        if (e.vid != vid || e.kind != FleetLogKind::Message) continue;
        if (e.ordinal <= mark) break;
        at = i;
    }
    if (at < g_fleet_log.size()) m.stamp = g_fleet_log[at].stamp;
    m.seq = ++g_fleet_log_seq;
    g_fleet_log.insert(g_fleet_log.begin() + (std::ptrdiff_t)at, std::move(m));
    fleet_log_trim();
}

// The vehicle tag, "[2]", in the accent colour and thickened.
//
// Drawn twice a fraction of a pixel apart rather than in a bold face: the app
// ships Regular weights only, and at 13 px this reads as bold without a second
// font in the atlas. Submitted as an ImGui item so it sits in the line's normal
// left-to-right flow with the stamp and the message.

} // namespace right_sidebar

using namespace right_sidebar;

void event_log_pump(VehicleId id, uint32_t number,
                    const VehicleState& vs,
                    const std::vector<StatusText>& status_texts,
                    bool online, uint32_t reboot_requests)
{
    FleetLogCursor& cur = g_fleet_log_cursors[id];

    // The operator asked for a restart. Marked at once, not when the vehicle
    // acts on it: the request is the operator's reference point, and a vehicle
    // that never comes back would otherwise leave no trace of having been asked.
    if (reboot_requests != cur.reboot_requests) {
        cur.reboot_requests = reboot_requests;
        fleet_log_add(fleet_log_marker(FleetLogKind::RebootRequested, id,
                                       number, "REBOOT REQUESTED"));
    }

    // Counted rather than diffed against the list. The vehicle keeps only its
    // last 200 messages and drops the oldest to make room, so the list's length
    // stops being a count of anything the moment it fills up; statustext_total
    // is the number the vehicle has ever sent and only goes up.
    const uint32_t total = vs.statustext_total;
    if (total < cur.ingested) cur.ingested = 0;   // cannot happen; never underflow
    if (total != cur.ingested) {
        // Anything older than what the vehicle still holds is gone for good —
        // this takes what is left of the backlog and accepts the gap.
        const size_t n_new = std::min<size_t>(total - cur.ingested, status_texts.size());
        const std::string stamp = wall_stamp();

        uint32_t ordinal = total - (uint32_t)n_new;
        for (size_t i = status_texts.size() - n_new; i < status_texts.size(); ++i) {
            fleet_log_add(FleetLogEntry{ FleetLogKind::Message, id, number,
                                         ++ordinal, status_texts[i].severity,
                                         stamp, status_texts[i].text });
        }
        if (n_new > 0) g_fleet_log_numbers.insert(number);
        cur.ingested = total;
    }

    // After the messages, so the ones the new boot has already sent are in the
    // log for the marker to be placed ahead of.
    if (vs.reboot_count != cur.reboots) {
        cur.reboots = vs.reboot_count;
        fleet_log_insert_reboot(id, number, vs.reboot_statustext_mark);
    }

    // Once per outage, not once per frame of it. Heard from again, it is
    // re-armed — the line then separates what it said before the gap from what
    // it says after.
    if (!online && !cur.lost) {
        cur.lost = true;
        fleet_log_add(fleet_log_marker(FleetLogKind::LinkLost, id,
                                       number, "CONNECTION LOST"));
    } else if (online) {
        cur.lost = false;
    }
}

void event_log_assistant(uint8_t severity, const std::string& text)
{
    fleet_log_add(FleetLogEntry{ FleetLogKind::Assistant, VehicleId{}, 0, 0,
                                 severity, wall_stamp(), text });
}

void event_log_vehicle_gone(VehicleId id, uint32_t number)
{
    auto it = g_fleet_log_cursors.find(id);
    // A vehicle whose loss is already marked needs no second line for being
    // removed — dropping a dead link is housekeeping, not news.
    if (it == g_fleet_log_cursors.end() || !it->second.lost)
        fleet_log_add(fleet_log_marker(FleetLogKind::LinkLost, id,
                                       number, "DISCONNECTED"));
    if (it != g_fleet_log_cursors.end()) g_fleet_log_cursors.erase(it);
}
