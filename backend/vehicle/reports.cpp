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
#include "backend/vehicle/reports.hpp"

#include <mavlink/ardupilotmega/mavlink.h>

#include "backend/mavlink/timesync.hpp"
#include "frontend/source/app_log.hpp"   // gcs_log — thread-safe, see the header
#include "frontend/source/audio.hpp"     // gcs_tone
#include "lib/mav_result.hpp"
#include "lib/text.hpp"

void VehicleReports::tick(const VehicleState& ps, const std::deque<StatusText>& sts)
{
    const unsigned tsys = sysid_;

    if (!logged_timesync_ && ps.has_timesync) {
        logged_timesync_ = true;
        const int64_t up_s = (timesync_monotonic_ns() + ps.time_offset_ns)
                             / 1000000000;
        gcs_log("[sys%u] clock synced \xe2\x80\x94 up %s", tsys, text_hms(up_s).c_str());
    }

    // New vehicle STATUSTEXT messages, to the bottom bar.
    static const char* const kSev[] = {
        "EMRG","ALRT","CRIT","ERR","WARN","NOTE","INFO","DEBUG"
    };
    for (size_t i = last_status_count_; i < sts.size(); ++i)
        gcs_log("[sys%u/%s] %s", tsys, kSev[sts[i].severity & 7], sts[i].text);
    last_status_count_ = sts.size();

    if (ps.has_heartbeat && !logged_heartbeat_) {
        logged_heartbeat_ = true;
        gcs_log("[sys%u] heartbeat: sys=%d comp=%d", tsys,
                (int)ps.sysid, (int)ps.compid);
    }

    // The name comes from the banner when there is one (vehicle/kind.hpp),
    // which the connect burst asks for alongside the version and which lands a
    // moment after it. A firmware too old to answer that request gets its name
    // from the HEARTBEAT type, after the wait.
    if (ps.has_fw_info && !logged_fw_info_) {
        constexpr auto BANNER_WAIT = std::chrono::seconds(3);
        const auto now = std::chrono::steady_clock::now();
        if (fw_info_at_ == std::chrono::steady_clock::time_point{}) fw_info_at_ = now;
        const bool named = ps.banner_kind != VehicleKind::Unknown ||
                           ps.autopilot != MAV_AUTOPILOT_ARDUPILOTMEGA ||
                           now - fw_info_at_ >= BANNER_WAIT;
        if (named) {
            logged_fw_info_ = true;
            gcs_log("[sys%u] firmware: %s %s (%.8s)", tsys, vehicle_firmware_name(ps),
                    ps.fw_version, ps.fw_hash);
        }
    }
}

void VehicleReports::command_ack(const CommandAck& ack, bool tones) const
{
    const char* cname = nullptr;
    switch (ack.command) {
    case 400: cname = "arm/disarm";   break;
    case  22: cname = "takeoff";      break;
    case  20: cname = "RTL";          break;
    case 176: cname = "mode change";  break;
    // DO_SET_STANDARD_MODE — the other way a mode gets set, and the
    // only one that reaches PX4's standard modes.
    case 262: cname = "mode change";  break;
    case 218: cname = "aux function"; break;
    case 192: cname = "reposition";   break;
    default:  break;
    }
    // Tone only for commands with a name above, i.e. the ones a person pressed
    // a button for. The rate and capability requests this GCS fires on connect
    // ACK too, and a burst of beeps at every connect teaches the operator to
    // ignore the sound. Gated on this being the vehicle on screen for the same
    // reason, one fleet-sized step up.
    if (!cname) return;
    if (tones)
        gcs_tone(ack.result == 0 ? GcsTone::Success : GcsTone::Failure);

    gcs_log("[sys%u] ack: %s \xe2\x86\x92 %s", (unsigned)sysid_, cname,
            mav_result_text(ack.result));
}

void VehicleReports::param_ext_ack(const ParamExtAck& pa, bool tones) const
{
    const char* verdict = (pa.result == PARAM_ACK_ACCEPTED)          ? "accepted"
                        : (pa.result == PARAM_ACK_VALUE_UNSUPPORTED) ? "value unsupported"
                        : (pa.result == PARAM_ACK_FAILED)            ? "failed"
                        : (pa.result == PARAM_ACK_IN_PROGRESS)       ? "in progress"
                                                                     : "rejected";
    gcs_log("[sys%u] param_ext: %s \xe2\x86\x92 %s", (unsigned)sysid_, pa.param_id, verdict);
    if (pa.result != PARAM_ACK_IN_PROGRESS && tones)
        gcs_tone(pa.result == PARAM_ACK_ACCEPTED ? GcsTone::Success
                                                 : GcsTone::Failure);
}

void VehicleReports::mission(bool fence, const MissionWatch::Actions& a,
                             const MissionWatch::Progress& p, const VehicleState& ps,
                             bool tones) const
{
    const unsigned tsys = sysid_;
    const char*    what = fence ? "fence" : "mission";

    if (a.loaded) {
        if (fence)
            gcs_log("[sys%u] fence loaded: %u items", tsys, (unsigned)ps.fence.size());
        else
            gcs_log("[sys%u] mission loaded: %u items%s", tsys,
                    (unsigned)ps.mission.size(),
                    ps.has_mission_home ? " (+ home)" : "");
    }
    if (a.stalled)
        gcs_log("[sys%u] %s download stalled at item %u of %u", tsys, what,
                (unsigned)p.received, (unsigned)p.count);

    if (a.upload == MissionWatch::Actions::Upload::Accepted) {
        gcs_log("[sys%u] %s upload accepted", tsys, what);
        if (tones) gcs_tone(GcsTone::Success);
    } else if (a.upload == MissionWatch::Actions::Upload::Failed) {
        gcs_log("[sys%u] %s upload failed (result=%u)", tsys, what,
                (unsigned)(fence ? ps.fence_upload_ack_result : ps.upload_ack_result));
        if (tones) gcs_tone(GcsTone::Failure);
    }
}
