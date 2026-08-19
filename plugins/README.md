# Plugins

Every square button on the rail down the right of the centre column is one C++
function in this folder. Write the function, rebuild, press the button.

```cpp
#include "plugin_api.hpp"

GCS_PLUGIN(shutter, "SHT", "SHUTTER", "Trigger the camera shutter once")
{
    if (!ctx.connected) { gcs_log("shutter: no link"); return; }
    ctx.sender->command_long(ctx.tsys, ctx.tcomp, 203,   // DO_DIGICAM_CONTROL
                             0, 0, 0, 0, 1, 0, 0);
    gcs_log("shutter fired");
}
```

`GCS_PLUGIN(ident, tag, label, tooltip)` takes a unique C++ identifier, the 1–3
characters drawn inside the square, the caption under it, and the hover text
(`nullptr` for none). The body is an ordinary function with one implicit
argument, `ctx`. The tag and label are what the button starts up as; both can be
changed later — see [Naming the button](#naming-the-button).

New `.cpp` files here are picked up by CMake on the next configure — no source
list to edit. `plugin_api.hpp` and `plugin_registry.cpp` are the framework;
everything else in this folder is yours. `user_plugins.cpp` ships with six
example slots (`FUNCTION 1`…`FUNCTION 6`), the first three of them working
camera examples.

## What `ctx` gives you

| Field | What it holds |
| --- | --- |
| `ctx.vehicle` | `const VehicleState*` — this frame's telemetry: `alt_rel`, `groundspeed`, `armed`, `custom_mode`, `lat`/`lon`, battery, EKF, … |
| `ctx.sender` | `MavlinkSender*` — queues commands to the vehicle |
| `ctx.connected` | a heartbeat has been seen; `tsys`/`tcomp` are real |
| `ctx.tsys`, `ctx.tcomp` | target system / component id |
| `ctx.camera` | `streaming`, `url`, `frame_w`, `frame_h` for the live feed |
| `ctx.slot` | this button's position on the rail; used by the rename and engaged calls |

`ctx` is valid only for the duration of the call — copy anything you keep.

## Sending to the vehicle

`ctx.sender` exposes the named commands the GCS itself uses (`arm`, `takeoff`,
`set_mode`, `do_set_servo`, `set_param`, …) plus the generic escape hatch:

```cpp
ctx.sender->command_long(ctx.tsys, ctx.tcomp, cmd, p1, p2, p3, p4, p5, p6, p7);
```

which sends any `MAV_CMD` as a `COMMAND_LONG`. See
`backend/mavlink_sender.hpp` for the full list.

## Naming the button

The macro's `label` is the caption under the square and `tag` is what is drawn
inside it. Neither is fixed at registration — a plugin can rename its own button
so it shows what it will do next rather than what it is called:

```cpp
GCS_PLUGIN(record, "REC", "RECORD", "Start / stop recording")
{
    static bool rolling = false;
    rolling = !rolling;
    gcs_plugin_set_label(ctx, rolling ? "RECORDING" : "RECORD");
    gcs_plugin_set_tag  (ctx, rolling ? "STOP"      : "REC");
}
```

`gcs_plugin_set_label`, `gcs_plugin_set_tag` and `gcs_plugin_set_tooltip` act on
the button being pressed. The `*_at(slot, …)` forms address any button by its
position on the rail (0-based, registration order) — one plugin can relabel
another. Changes last until something changes them again; nothing is saved to
disk, so the macro's names are what you get on the next start.

Captions are clipped to the rail width — roughly 11 characters — and the tooltip
carries the full text. A tag longer than about three characters overflows its
plate.

### Setting the opening name

The macro's `tag` and `label` are literals, so they can only tell the truth
about state known at compile time. A button whose caption reflects something
decided at startup needs `GCS_PLUGIN_INIT`, which runs once before the first
frame:

```cpp
static bool s_power_on = false;

GCS_PLUGIN(payload, "OFF", "PAYLOAD POWER", "Toggle payload power")
{
    s_power_on = !s_power_on;
    gcs_plugin_set_tag(ctx, s_power_on ? "ON" : "OFF");
}

GCS_PLUGIN_INIT(payload)
{
    gcs_plugin_set_tag(init, s_power_on ? "ON" : "OFF");
}
```

The hook names the plugin it belongs to and may sit above or below it. Hooks run
in rail order, on the UI thread, after the link and video stack are up and
before any frame is drawn — so a hook can also open a device or start a worker,
not only rename its button. There is no matching shutdown hook. A hook naming a
plugin that does not exist is reported in the log rather than ignored.

## Showing that a plugin is live

A button rests as the app's usual dark well, its seam and caption picking up the
rail's blue instead of amber. Pressing one flashes the plate red for ~350 ms so
a momentary plugin visibly fires, and a plugin that is *doing* something can
hold that red for as long as it runs:

```cpp
GCS_PLUGIN(record, "REC", "RECORD", "Start / stop recording")
{
    static bool rolling = false;
    rolling = !rolling;
    gcs_plugin_set_engaged(ctx, rolling);
}
```

It is the same red as the ARMED annunciator, deliberately — on this UI red means
something is live right now. Nothing infers the state: the button is engaged
only while a plugin says so, so set it back to `false` when the work stops.
`gcs_plugin_set_engaged_at(slot, …)` and `gcs_plugin_engaged_at(slot)` reach
other buttons, and the `PluginInit` overload sets the opening state.

## Video events

Not every plugin is a button. A click on the video feed, or a click dragged into
a box, runs handlers you write the same way:

```cpp
GCS_ON_VIDEO_CLICK(track_point)
{
    if (!ctx.connected) return;
    // MAV_CMD_CAMERA_TRACK_POINT wants normalised coordinates
    ctx.sender->command_long(ctx.tsys, ctx.tcomp, 2004,
                             click.at.u, click.at.v, 0.05f, 0, 0, 0, 0);
}

GCS_ON_VIDEO_BOX(track_rectangle)
{
    if (!ctx.connected) return;
    // MAV_CMD_CAMERA_TRACK_RECTANGLE wants two corners, already sorted for you
    ctx.sender->command_long(ctx.tsys, ctx.tcomp, 2005,
                             box.min.u, box.min.v, box.max.u, box.max.v, 0, 0, 0);
}
```

Both get the usual `ctx` plus the gesture:

| | |
| --- | --- |
| `click.at` | `u`, `v` (0..1 across the frame, from the top-left) and `x`, `y` (decoded-frame pixels) |
| `box.min`, `box.max` | the same two forms for each corner, sorted so `min` is the top-left whichever way the drag went |
| `frame_w`, `frame_h` | the decoded frame size those pixels refer to |
| `box.px_w()`, `px_h()`, `u_w()`, `v_h()` | box size in pixels and in normalised units |

- **Coordinates are in the frame, never the window.** The letterbox bars around
  the picture are already accounted for, so a handler's numbers mean the same
  thing at any window size, and clicks on the bars do not fire at all.
- **Click or box is decided by drag distance** — under ~5 px is a click. A firm
  click never turns into a one-pixel selection.
- **Only while the feed is playing**, and `ctx.slot` is `-1`: a video event has
  no button, so the rename and engaged calls have nothing to act on. Pair the
  gesture with a rail button (as `STOP TRACK` does) if you need one.
- **Handlers stack.** Every registered handler runs for the same gesture, in
  registration order — the first to answer does not consume it. One throwing is
  logged by name and the others still run.
- The rubber band drawn during a drag only appears if some `GCS_ON_VIDEO_BOX`
  handler exists; with nobody listening the feed ignores clicks entirely.

## Camera helpers

```cpp
void gcs_camera_start(const char* url);      // rtsp://… or udp://host:port
void gcs_camera_stop();
bool gcs_camera_snapshot(const char* path);  // current frame → PNG
```

## Rules

- **Button order** is registration order: declaration order within a file, and
  across files the order their static initialisers ran, which follows the CMake
  glob — filename order. Prefix filenames (`10_camera.cpp`, `20_gimbal.cpp`) if
  the exact arrangement matters.
- **Plugins run on the UI thread**, inside the frame where the button was
  clicked. That is what makes `ctx` safe to read without locking and the sender
  calls safe to issue — but anything that blocks freezes the GCS. Long work
  belongs on a thread your plugin starts and owns; don't touch `ctx` from it.
- **Exceptions** escaping a plugin are caught and logged, not fatal. A broken
  plugin costs you one button, not a flight. Don't rely on it.
- **`gcs_log(fmt, …)`** writes to the bottom-bar log and is `printf`-checked.
