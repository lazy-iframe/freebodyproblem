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


#pragma once

// ── GCS plugin API ────────────────────────────────────────────────────────────
//
// A plugin is one C++ function bound to one square button on the rail down the
// right of the centre column. Drop a .cpp file in this folder, write
// GCS_PLUGIN(...) around a function body, rebuild — the button appears.
// Nothing else to register.
//
//   #include "plugin_api.hpp"
//
//   GCS_PLUGIN(shutter, "SHT", "SHUTTER", "Trigger the camera shutter once")
//   {
//       if (!ctx.connected) { gcs_log("shutter: no link"); return; }
//       ctx.sender->command_long(ctx.tsys, ctx.tcomp, 203,   // DO_DIGICAM_CONTROL
//                                0, 0, 0, 0, 1, 0, 0);
//       gcs_log("shutter fired");
//   }
//
// Buttons appear in registration order: declaration order within one file, and
// across files the order the linker ran their static initialisers — which
// follows the CMake glob, i.e. filename order. Prefix filenames (10_cam.cpp,
// 20_gimbal.cpp) if you care about the exact arrangement.
//
// Threading: every plugin runs on the UI thread, inside the frame where the
// button was clicked. That makes ctx safe to read without locking and makes the
// sender calls safe to issue — but a plugin that blocks freezes the GCS. Long
// work belongs on a thread the plugin starts and owns.
//
// Failure: an exception escaping a plugin is caught and logged rather than
// taken down the process, so a broken plugin costs you one button, not a
// flight. It is still your job not to throw.
//
// Not everything is a button: GCS_ON_VIDEO_CLICK and GCS_ON_VIDEO_BOX below
// bind functions to gestures on the video feed instead.

#include <cstdint>
#include <string>
#include <vector>

#include "../backend/mavlink_parser.hpp"   // VehicleState
#include "../backend/mavlink_sender.hpp"   // MavlinkSender
#include "../frontend/app_log.hpp"         // gcs_log

// ── What a plugin gets to see ────────────────────────────────────────────────

// The live camera feed, as the centre view currently has it.
struct PluginCamera {
    bool        streaming;   // pipeline is up and delivering frames
    const char* url;         // configured source ("" when never set)
    int         frame_w;     // last decoded frame size, 0 when not streaming
    int         frame_h;
};

// Everything a plugin is handed when its button is pressed. Valid only for the
// duration of the call — copy anything you intend to keep.
struct PluginContext {
    const VehicleState* vehicle;     // never null; telemetry snapshot for this frame
    MavlinkSender*      sender;      // never null; queues commands to the vehicle
    bool                connected;   // a heartbeat has been seen; tsys/tcomp are real
    uint8_t             tsys;        // target system id  (1 when not connected)
    uint8_t             tcomp;       // target component id
    PluginCamera        camera;
    int                 slot;        // which button is running; see gcs_plugin_set_*
                                     // (-1 for video events, which have no button)
};

using PluginFn = void (*)(const PluginContext& ctx);

// ── Camera helpers ───────────────────────────────────────────────────────────
//
// Implemented by the centre view. Call these from the UI thread only, which is
// where plugins already run.

// Point the feed at `url` (rtsp://… or udp://host:port) and start the pipeline.
// Replaces whatever is playing.
void gcs_camera_start(const char* url);

// Tear the pipeline down and release the texture.
void gcs_camera_stop();

// Write the frame currently on screen to `path` as a PNG. Returns false when
// nothing is streaming or the file could not be written (reason is logged).
bool gcs_camera_snapshot(const char* path);

// ── Video events ─────────────────────────────────────────────────────────────
//
// The feed is clickable. A click on the frame runs every GCS_ON_VIDEO_CLICK
// handler; a click dragged into a box runs every GCS_ON_VIDEO_BOX handler
// instead. Both are ordinary functions in this folder, same as the buttons:
//
//   GCS_ON_VIDEO_CLICK(track_point)
//   {
//       if (!ctx.connected) return;
//       // MAV_CMD_CAMERA_TRACK_POINT — the payload takes normalised coords
//       ctx.sender->command_long(ctx.tsys, ctx.tcomp, 2004,
//                                click.at.u, click.at.v, 0.f, 0, 0, 0, 0);
//   }
//
// Coordinates come two ways, because payloads disagree about which they want:
// `u`/`v` run 0..1 across the frame (0,0 at the top-left), and `x`/`y` are the
// same point in decoded-frame pixels. Neither is in window or screen space —
// the letterboxing around the frame is already accounted for, so the numbers
// mean the same thing whatever the window is doing.
//
// Only fires while the feed is playing, and only inside the picture itself.
// The pixel-vs-box decision is by drag distance: a press and release within a
// few pixels is a click, anything further is a box, so an unsteady hand does
// not turn a click into a one-pixel selection.

struct VideoPoint {
    float u, v;   // 0..1 across the frame, from the top-left
    int   x, y;   // the same point in decoded-frame pixels
};

struct VideoClick {
    VideoPoint at;
    int        frame_w, frame_h;   // decoded frame size the pixels refer to
};

struct VideoBox {
    VideoPoint min, max;           // normalised so min is the top-left corner
    int        frame_w, frame_h;

    int   px_w() const { return max.x - min.x; }
    int   px_h() const { return max.y - min.y; }
    float u_w()  const { return max.u - min.u; }
    float v_h()  const { return max.v - min.v; }
};

using PluginVideoClickFn = void (*)(const PluginContext& ctx, const VideoClick& click);
using PluginVideoBoxFn   = void (*)(const PluginContext& ctx, const VideoBox& box);

// Register at static-init time. Use the macros below rather than these.
struct VideoClickRegistrar { VideoClickRegistrar(const char* name, PluginVideoClickFn fn); };
struct VideoBoxRegistrar   { VideoBoxRegistrar(const char* name, PluginVideoBoxFn fn); };

// Every registered handler runs, in registration order — a click is not
// consumed by the first one to answer.
#define GCS_ON_VIDEO_CLICK(ident)                                              \
    static void ident(const PluginContext& ctx, const VideoClick& click);      \
    static const VideoClickRegistrar ident##_gcs_vclick(#ident, &ident);       \
    static void ident(const PluginContext& ctx, const VideoClick& click)

#define GCS_ON_VIDEO_BOX(ident)                                                \
    static void ident(const PluginContext& ctx, const VideoBox& box);          \
    static const VideoBoxRegistrar ident##_gcs_vbox(#ident, &ident);           \
    static void ident(const PluginContext& ctx, const VideoBox& box)

// Called by the centre view when the operator clicks or drags on the feed; a
// plugin has no reason to call them. Handlers that throw are logged by name and
// the rest still run.
void gcs_video_dispatch_click(const PluginContext& ctx, const VideoClick& click);
void gcs_video_dispatch_box(const PluginContext& ctx, const VideoBox& box);

// Whether anybody is listening. The feed only swallows clicks, and only draws
// the selection rubber band, when something is there to receive them.
bool gcs_video_has_click_handlers();
bool gcs_video_has_box_handlers();

// ── Registration ─────────────────────────────────────────────────────────────

struct PluginSlot {
    std::string tag;       // 1–3 characters, drawn inside the square button
    std::string label;     // caption under the button
    std::string tooltip;   // hover text; empty for none
    bool        engaged;   // draw the plate in the ARMED red
    PluginFn    fn;
};

// The registered slots, in registration order. Used by the rail; a plugin has
// no reason to call it.
const std::vector<PluginSlot>& gcs_plugins();

// Handed to a startup hook — see GCS_PLUGIN_INIT below.
struct PluginInit {
    int slot;   // the button this hook belongs to
};

// ── Renaming a button ────────────────────────────────────────────────────────
//
// The macro's `tag` and `label` are what the button starts up as. These change
// it afterwards, so a button can carry state — "REC" flipping to "STOP", a
// preset number, the mode it just selected:
//
//   GCS_PLUGIN(record, "REC", "RECORD", "Start / stop recording")
//   {
//       static bool rolling = false;
//       rolling = !rolling;
//       gcs_plugin_set_label(ctx, rolling ? "RECORDING" : "RECORD");
//       gcs_plugin_set_tag  (ctx, rolling ? "\xe2\x96\xa0" : "REC");
//   }
//
// The change persists until something changes it again; it is not written to
// disk, so the macro's names come back on the next start. All of these are
// UI-thread only — call them from a plugin or its init hook, not from a worker
// the plugin spawned.

// Rename the button that is running (ctx.slot), or the one a startup hook
// belongs to (init.slot).
void gcs_plugin_set_label(const PluginContext& ctx, const char* label);
void gcs_plugin_set_label(const PluginInit& init, const char* label);

// Change what is drawn inside the square. Keep it to ~3 characters — a longer
// tag is drawn as-is and will overflow the plate.
void gcs_plugin_set_tag(const PluginContext& ctx, const char* tag);
void gcs_plugin_set_tag(const PluginInit& init, const char* tag);

// Change the hover text. nullptr or "" removes it.
void gcs_plugin_set_tooltip(const PluginContext& ctx, const char* tooltip);
void gcs_plugin_set_tooltip(const PluginInit& init, const char* tooltip);

// Same three, addressing any button by its position on the rail (0-based, in
// registration order) rather than the running one. Out-of-range is ignored.
void gcs_plugin_set_label_at(int slot, const char* label);
void gcs_plugin_set_tag_at(int slot, const char* tag);
void gcs_plugin_set_tooltip_at(int slot, const char* tooltip);

// ── Engaged state ────────────────────────────────────────────────────────────
//
// An engaged button holds the same red as the ARMED annunciator, so a glance at
// the rail says which plugins are currently doing something — recording,
// holding a payload on, streaming. Every press also flashes that red briefly,
// so a momentary plugin still reads as having fired.
//
//   GCS_PLUGIN(record, "REC", "RECORD", "Start / stop recording")
//   {
//       static bool rolling = false;
//       rolling = !rolling;
//       gcs_plugin_set_engaged(ctx, rolling);
//   }
//
// Nothing infers this: a button is engaged only while a plugin says it is. Set
// it back to false when the work stops, or the rail will claim it is still
// running. Like the names, it is not saved to disk — see GCS_PLUGIN_INIT for
// setting it at startup.
void gcs_plugin_set_engaged(const PluginContext& ctx, bool engaged);
void gcs_plugin_set_engaged(const PluginInit& init, bool engaged);
void gcs_plugin_set_engaged_at(int slot, bool engaged);

// Whether that button is currently engaged. Out-of-range reads as false.
bool gcs_plugin_engaged_at(int slot);

// Registers at static-init time. Use GCS_PLUGIN rather than this directly.
struct PluginRegistrar {
    PluginRegistrar(const char* tag, const char* label,
                    const char* tooltip, PluginFn fn);
};

// Define a plugin and bind it to a button in one go. `ident` is a C++ function
// name and must be unique within the file; `tag` is what fits inside the square.
#define GCS_PLUGIN(ident, tag, label, tooltip)                                 \
    static void ident(const PluginContext& ctx);                               \
    static const PluginRegistrar ident##_gcs_registrar((tag), (label),         \
                                                       (tooltip), &ident);     \
    static void ident(const PluginContext& ctx)

// ── Startup hooks ────────────────────────────────────────────────────────────
//
// A plugin whose caption reflects state has a chicken-and-egg problem: the
// macro's names are literals fixed at compile time, but the state may only be
// known at startup — read from a file, derived from a default, whatever the
// plugin decides. GCS_PLUGIN_INIT runs once before the first frame, which is
// where that opening state gets applied:
//
//   static bool s_power_on = false;
//
//   GCS_PLUGIN(payload, "OFF", "PAYLOAD POWER", "Toggle payload power")
//   {
//       s_power_on = !s_power_on;
//       gcs_plugin_set_tag(ctx, s_power_on ? "ON" : "OFF");
//   }
//
//   GCS_PLUGIN_INIT(payload)
//   {
//       gcs_plugin_set_tag(init, s_power_on ? "ON" : "OFF");
//   }
//
// The hook names the plugin it belongs to and may sit above or below it. Hooks
// run in slot order, on the UI thread, after the link and video stack are up
// and before any frame is drawn — so a hook may also open a device or start a
// worker, not only rename its button. There is no matching shutdown hook.
using PluginInitFn = void (*)(const PluginInit& init);

// Registers a startup hook. Use GCS_PLUGIN_INIT rather than this directly.
struct PluginInitRegistrar {
    PluginInitRegistrar(PluginFn owner, PluginInitFn init);
};

#define GCS_PLUGIN_INIT(ident)                                                 \
    static void ident(const PluginContext& ctx);                               \
    static void ident##_gcs_init(const PluginInit& init);                      \
    static const PluginInitRegistrar ident##_gcs_init_registrar(&ident,        \
                                                    &ident##_gcs_init);        \
    static void ident##_gcs_init(const PluginInit& init)

// Runs every registered startup hook. Called once by the app before the first
// frame; a plugin has no reason to call it.
void gcs_plugins_init();
