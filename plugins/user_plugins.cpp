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


// ── User plugins ──────────────────────────────────────────────────────────────
//
// This file is yours. Each GCS_PLUGIN block below is one square button on the
// rail down the right of the centre column, top to bottom in the order they
// appear here.
//
// The first three are working examples of the camera surface and the fifth
// shows a button that names itself from its own state; the rest are empty slots
// to fill in. Delete what you do not want — a slot with no GCS_PLUGIN block
// simply has no button.
//
// At the bottom are the video-event handlers, which are not buttons at all:
// they run when the operator clicks or drags a box on the feed itself. See
// README.md in this folder.

#include "plugin_api.hpp"

#include <cstdio>
#include <ctime>

// MAV_CMD values used below. Full list: MAVLink common.xml.
static constexpr uint16_t CMD_DO_DIGICAM_CONTROL   = 203;
static constexpr uint16_t CMD_DO_MOUNT_CONTROL     = 205;
static constexpr uint16_t CMD_CAMERA_TRACK_POINT   = 2004;
static constexpr uint16_t CMD_CAMERA_TRACK_RECT    = 2005;
static constexpr uint16_t CMD_CAMERA_STOP_TRACKING = 2010;

// ── FUNCTION 1 — save the frame on screen ────────────────────────────────────

GCS_PLUGIN(function1, "F1", "FUNCTION 1", "Save the current video frame as a PNG")
{
    if (!ctx.camera.streaming) {
        gcs_log("snapshot: no video");
        return;
    }

    // Timestamped so repeated presses never overwrite the last capture.
    char path[64];
    const std::time_t t  = std::time(nullptr);
    std::tm           tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::snprintf(path, sizeof(path), "snapshot_%04d%02d%02d_%02d%02d%02d.png",
                  tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                  tm.tm_hour, tm.tm_min, tm.tm_sec);

    if (gcs_camera_snapshot(path))
        gcs_log("snapshot saved: %s (%dx%d)", path,
                ctx.camera.frame_w, ctx.camera.frame_h);
}

// ── FUNCTION 2 — trigger the camera shutter ──────────────────────────────────

GCS_PLUGIN(function2, "F2", "FUNCTION 2", "Trigger the payload camera shutter")
{
    if (!ctx.connected) {
        gcs_log("shutter: no link");
        return;
    }

    // DO_DIGICAM_CONTROL: param5 = 1 asks for one shot.
    ctx.sender->command_long(ctx.tsys, ctx.tcomp, CMD_DO_DIGICAM_CONTROL,
                             0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f);
    gcs_log("camera shutter triggered");
}

// ── FUNCTION 3 — point the gimbal straight down ──────────────────────────────

GCS_PLUGIN(function3, "F3", "FUNCTION 3", "Point the gimbal nadir (-90 deg pitch)")
{
    if (!ctx.connected) {
        gcs_log("gimbal: no link");
        return;
    }

    // DO_MOUNT_CONTROL: pitch, roll, yaw in degrees; param7 = MAV_MOUNT_MODE,
    // 2 = MAVLINK_TARGETING (obey the angles in this message).
    ctx.sender->command_long(ctx.tsys, ctx.tcomp, CMD_DO_MOUNT_CONTROL,
                             -90.f, 0.f, 0.f, 0.f, 0.f, 0.f, 2.f);
    gcs_log("gimbal → nadir");
}

// ── FUNCTION 4 ───────────────────────────────────────────────────────────────

GCS_PLUGIN(function4, "F4", "FUNCTION 4", "Unassigned — edit plugins/user_plugins.cpp")
{
    // ctx.vehicle carries this frame's telemetry: ctx.vehicle->alt_rel,
    // ->groundspeed, ->armed, ->custom_mode, and so on.
    gcs_log("function 4 (alt %.1f m)", (double)ctx.vehicle->alt_rel);
}

// ── FUNCTION 5 — a button that renames itself ────────────────────────────────

// State the caption reflects. File-scope so the startup hook below can read it.
static bool s_payload_on = false;

GCS_PLUGIN(function5, "OFF", "PAYLOAD POW", "Toggle payload power")
{
    // The caption under a button is not fixed at registration: gcs_plugin_set_
    // label/tag/tooltip change it from inside the plugin, which is how a button
    // shows its state rather than only its name. Marking it engaged holds the
    // plate in the ARMED red for as long as the payload is on.
    s_payload_on = !s_payload_on;

    gcs_plugin_set_tag    (ctx, s_payload_on ? "ON" : "OFF");
    gcs_plugin_set_engaged(ctx, s_payload_on);
    gcs_log("payload power → %s", s_payload_on ? "on" : "off");
}

// The macro's tag and label are literals, so they only tell the truth about
// state that is known at compile time. This runs once before the first frame,
// which is where a caption derived from anything else gets applied — read the
// real state from the payload, a config file, a default, and set it here.
GCS_PLUGIN_INIT(function5)
{
    gcs_plugin_set_tag    (init, s_payload_on ? "ON" : "OFF");
    gcs_plugin_set_engaged(init, s_payload_on);
}

// ── FUNCTION 6 ───────────────────────────────────────────────────────────────

GCS_PLUGIN(function6, "F6", "FUNCTION 6", "Unassigned — edit plugins/user_plugins.cpp")
{
    // ctx.sender->command_long(ctx.tsys, ctx.tcomp, <MAV_CMD>, p1…p7) sends any
    // command the autopilot or payload understands.
    gcs_log("function 6");
}

GCS_PLUGIN(function7, "F7", "FUNCTION 7", "Unassigned — edit plugins/user_plugins.cpp")
{
    // ctx.sender->command_long(ctx.tsys, ctx.tcomp, <MAV_CMD>, p1…p7) sends any
    // command the autopilot or payload understands.
    gcs_log("function 7");
}

GCS_PLUGIN(function8, "F8", "FUNCTION 8", "Unassigned — edit plugins/user_plugins.cpp")
{
    // ctx.sender->command_long(ctx.tsys, ctx.tcomp, <MAV_CMD>, p1…p7) sends any
    // command the autopilot or payload understands.
    gcs_log("function 8");
}

GCS_PLUGIN(function9, "F9", "FUNCTION 9", "Unassigned — edit plugins/user_plugins.cpp")
{
    // ctx.sender->command_long(ctx.tsys, ctx.tcomp, <MAV_CMD>, p1…p7) sends any
    // command the autopilot or payload understands.
    gcs_log("function 9");
}

// ── Video events ─────────────────────────────────────────────────────────────
//
// These two are not buttons. They run when the operator interacts with the feed
// itself: a click on the picture, or a click dragged into a box. Both hand over
// coordinates in the frame — u/v normalised 0..1, x/y in decoded pixels — never
// in window space, so the letterboxing around the picture is already dealt with.

// Click a target: ask the payload to track that point.
GCS_ON_VIDEO_CLICK(track_point)
{
    gcs_log("video click: %d,%d px  (%.3f, %.3f)",
            click.at.x, click.at.y, (double)click.at.u, (double)click.at.v);

    if (!ctx.connected) return;

    // MAV_CMD_CAMERA_TRACK_POINT takes normalised coordinates and a radius,
    // which is why the event carries both forms of the same point.
    ctx.sender->command_long(ctx.tsys, ctx.tcomp, CMD_CAMERA_TRACK_POINT,
                             click.at.u, click.at.v, 0.05f, 0.f, 0.f, 0.f, 0.f);
}

// Drag a box: ask the payload to track that region instead. A box smaller than
// a few pixels is treated as a click by the centre view, so anything arriving
// here was deliberate.
GCS_ON_VIDEO_BOX(track_rectangle)
{
    gcs_log("video box: %dx%d px at %d,%d",
            box.px_w(), box.px_h(), box.min.x, box.min.y);

    if (!ctx.connected) return;

    // MAV_CMD_CAMERA_TRACK_RECTANGLE: top-left and bottom-right, normalised.
    // The corners are pre-sorted, so a box dragged right-to-left works too.
    ctx.sender->command_long(ctx.tsys, ctx.tcomp, CMD_CAMERA_TRACK_RECT,
                             box.min.u, box.min.v, box.max.u, box.max.v,
                             0.f, 0.f, 0.f);
}

// Handlers stack rather than compete — every registered one runs for the same
// gesture, so a second listener can log, record or overlay without touching the
// one above. Delete this block if one handler is all you want.
GCS_ON_VIDEO_BOX(box_area_check)
{
    const float frac = (box.u_w() * box.v_h()) * 100.0f;
    if (frac > 60.0f)
        gcs_log("selection covers %.0f%% of frame \xe2\x80\x94 tracker may lose it",
                (double)frac);
}

// ── FUNCTION 9 (stop tracking) ───────────────────────────────────────────────
//
// Whatever starts tracking needs something that stops it, and a video event has
// nowhere to put a button — so the rail carries the other half of the pair.
GCS_PLUGIN(track_stop, "TRK", "STOP TRACK", "Stop payload tracking")
{
    if (!ctx.connected) {
        gcs_log("tracking: no link");
        return;
    }
    ctx.sender->command_long(ctx.tsys, ctx.tcomp, CMD_CAMERA_STOP_TRACKING,
                             0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f);
    gcs_log("payload tracking stopped");
}
