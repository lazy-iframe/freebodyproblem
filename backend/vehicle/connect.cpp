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
#include "backend/vehicle/connect.hpp"

#include "backend/vehicle/firmware_profile.hpp"

const char* send_connect_burst(MavlinkSender& sender, uint8_t tsys, uint8_t tcomp,
                               const VehicleState& vs)
{
    const FirmwareProfile& prof = firmware_profile(vs.autopilot);

    sender.request_message_interval(tsys, tcomp,  30, 100000); // ATTITUDE            @ 10 Hz
    sender.request_message_interval(tsys, tcomp,  74, 100000); // VFR_HUD             @ 10 Hz
    sender.request_message_interval(tsys, tcomp,  33, 100000); // GLOBAL_POSITION_INT @ 10 Hz
    sender.request_message_interval(tsys, tcomp,   1, 500000); // SYS_STATUS          @  2 Hz
    sender.request_message_interval(tsys, tcomp,  24, 500000); // GPS_RAW_INT         @  2 Hz
    // Estimator health @ 5 Hz — EKF_STATUS_REPORT (#193) on ArduPilot,
    // ESTIMATOR_STATUS (#230) on PX4, which has no #193 to give.
    sender.request_message_interval(tsys, tcomp, prof.estimator_msgid(), 200000);
    // HOME_POSITION at 0.5 Hz. A vehicle announces home when it sets or moves
    // it, but home is usually already set by the time a GCS connects — and that
    // announcement is long gone. Streaming it slowly costs nothing and means the
    // map has home from the first couple of seconds, however late the operator
    // joined.
    sender.request_message_interval(tsys, tcomp, 242, 2000000); // HOME_POSITION       @0.5 Hz
    // RC_CHANNELS at 10 Hz. Fast enough that a calibration sweep catches a
    // stick's true stop rather than wherever it happened to be sampled, and that
    // the RC panel's bars track the sticks instead of stepping after them.
    sender.request_message_interval(tsys, tcomp,  65, 100000); // RC_CHANNELS         @ 10 Hz

    // What the vehicle is; VersionProbe and ModeListWalk take it from here.
    sender.request_message(tsys, tcomp, MAVLINK_MSG_ID_AUTOPILOT_VERSION);
    sender.request_available_modes(tsys, tcomp);
    // Which ArduPilot firmware, which the HEARTBEAT type cannot always say.
    if (vs.autopilot == MAV_AUTOPILOT_ARDUPILOTMEGA)
        sender.request_banner(tsys, tcomp);

    // The parameter-set geofence, so the map can draw its cylinder without a
    // full parameter fetch. Later changes arrive by themselves: a vehicle echoes
    // every parameter it is sent.
    static const char* const AP_FENCE[]  = { "FENCE_ENABLE", "FENCE_TYPE",
        "FENCE_RADIUS", "FENCE_ALT_MAX", "FENCE_ALT_MIN", "FENCE_ACTION" };
    static const char* const PX4_FENCE[] = { "GF_ACTION",
        "GF_MAX_HOR_DIST", "GF_MAX_VER_DIST" };
    if (vs.autopilot == MAV_AUTOPILOT_ARDUPILOTMEGA)
        for (const char* id : AP_FENCE)  sender.request_param_read(tsys, tcomp, id);
    else if (vs.autopilot == MAV_AUTOPILOT_PX4)
        for (const char* id : PX4_FENCE) sender.request_param_read(tsys, tcomp, id);

    return prof.name();
}
