---
title: Architecture
description: How the backend and frontend fit together.
---

The process runs one thread per link and one thread per vehicle. A link reads
bytes and frames them; framing has to happen there because a message's sysid is
not knowable until it is framed. Everything after that — the message dispatch,
the parameter table, the mission state and the protocol timers — belongs to one
vehicle and runs on that vehicle's own thread. Sending goes the other way: a
vehicle queues frames in its own sender, and the link, which owns the socket or
the serial handle, drains them.

```
  socket / serial ──▶ link thread          one per connection
                       read, frame, demux on sysid
                              │
                              ▼  inbox
                     vehicle thread        one per vehicle
                       dispatch, protocol timers, snapshot
                              │
                              ▼  snapshot
                        UI thread          draws the selected vehicle
```

## Backend (`backend/`)
- **mavlink/**: MAVLink wire protocol: framing, decoding, sending, signing, clock sync. **Every message received is decoded by `MavlinkParser` and every message sent is packed by `MavlinkSender`**; nothing else in the GCS touches the wire
  - **framer.hpp**: Byte stream to whole messages, with the reassembly buffer held per link rather than in the library's per-channel globals, so links can frame concurrently
  - **parser/**: MAVLink message decoder with per-ID stats tracking; one per vehicle, and it accepts only that vehicle's system. `parser.cpp` dispatches each message to a handler in the file for its family: `telemetry.cpp`, `params.cpp`, `modes.cpp`, `commands.cpp`, `mission.cpp`
  - **tlog.cpp**: The telemetry log — each vehicle's frames, both ways, copied into memory as they pass and written to its `.tlog` by one background thread, so no link thread waits on a disk
  - **sender/**: Command queue with ACK tracking and retransmit logic; one per vehicle, each keeping its own outgoing sequence counter, so there is no cap on how many vehicles the fleet holds. `sender.cpp` holds the queue, transport, signing, ACK feedback and retries; `commands.cpp`, `params.cpp` and `mission.cpp` the messages themselves
- **mission/**: The mission protocol's logic, link-free: fed decoded messages, it says what the vehicle loop should send
  - **types.hpp**: `MissionItem` and `FenceParams`
  - **download.cpp**: one list's download — items taken in order, duplicates dropped, the home slot split off
  - **watch.cpp**: keeping a list's transfers moving — stalled downloads asked again then given up, completed ones acknowledged, upload results reported
  - **geofence.cpp**: uploaded geofence zones — polygons and circles, inclusion and exclusion — to and from the fence list's mission items
- **link/**: Transports and the fleet that demultiplexes them
  - **connection.cpp**: Serial port enumeration (Linux/Windows/macOS)
  - **link.cpp**: One transport — UDP socket, TCP connection or serial port — and the platform detail behind opening, reading and writing it
  - **fleet.cpp**: Every link and vehicle, the link read threads, and the sysid demultiplexing that decides which vehicle a message belongs to
- **vehicle/**: One vehicle, and what is known about its flight stack and sensors
  - **state.hpp**: `VehicleState`, everything known about one vehicle, as the parser fills it in
  - **vehicle.cpp**: One vehicle and the thread that talks to it — stream rates, the mode list, clock sync, the parameter fetch and its retransmits, and the snapshot the UI reads
    - The protocol loop's conversations are link-free classes it drives, each saying what is due: **connect.cpp** (the connect burst), **identity.cpp** (mode-list walk, firmware-version probe), **stream_rates.cpp** (rate watchdog), **param_fetch.cpp** (bulk parameter fetch: stall retransmit, give-up, publish edges), **reports.cpp** (log lines and tones)
  - **kind.cpp**: which ArduPilot firmware a vehicle runs (Copter, Plane, Rover, Sub, Blimp, AntennaTracker), the one answer mode numbers, the RCn_OPTION table and the parameter reference all go by. The HEARTBEAT type cannot always say — a Copter BiCopter reports a tailsitter's type, and Plane reports whatever `Q_MAV_TYPE` is set to — so the banner ArduPilot sends on `MAV_CMD_DO_SEND_BANNER` (asked for in the connect burst) settles it once heard
  - **firmware_profile.cpp**: what differs between flight stacks in one place — which message carries estimator health (ArduPilot's `EKF_STATUS_REPORT` vs the portable `ESTIMATOR_STATUS`), each stack's built-in mode table for vehicles that publish no `AVAILABLE_MODES`, and how a mode number is written on the wire. PX4 splits main and sub mode across two command parameters where ArduPilot puts one flat number in the first; sending ArduPilot's shape at PX4 is rejected
  - **sensor_inventory.cpp**: compass/accel/gyro devices decoded out of the parameter table (device-ID packing, chip names)
- **calibration/**: Link-free calibration state machines
  - **rc.cpp**: RC endpoint measurement
  - **accel.cpp** / **gyro.cpp** / **mag.cpp**: the GCS half of each calibration — link-free state machines fed messages and a clock
  - **geodesic_grid.cpp**: ArduPilot's 80-section coverage grid, for drawing what a compass calibration has and has not seen
- **input/**: Operator input mapped to vehicle controls
  - **joystick_control.cpp**: joystick axes to `MANUAL_CONTROL` and `RC_CHANNELS_OVERRIDE` values, button triggers and virtual channels; holds no device and no link. `frontend/source/joystick.cpp` reads the device and runs the send loop
  - **rc_binding.cpp**: the stick/mode/aux parameter tables, per stack
- **mcp/**: The built-in MCP server
  - **mcp_server.cpp**: the built-in MCP server — Streamable HTTP on loopback, bearer token, the read and action tools, the approval queue the UI thread drains, vehicle claims between agents, and the separation check on takeoff and goto paths

## Frontend (`frontend/`)
- **main.cpp**: GLFW/OpenGL event loop, the app log, and the per-frame snapshot of whichever vehicle is selected
- **source/**: Support code that draws nothing
  - **settings.cpp**: JSON-based persistent configuration (tile server, themes, window state)
  - **param_file.cpp**: `.params` reader/writer (Mission Planner / QGroundControl format)
  - **audio.cpp**: synthesised cue tones and accelerating progress ticks — lock-free voice pool mixed on the miniaudio callback
  - **layout_rules.hpp**: The screen layout as plain arithmetic, free of ImGui — sidebar widths in glyphs, the narrow cutoff, and the stacked or side-by-side split of MAP + VIDEO and of the SWARM map. **layout.hpp** feeds it the live window and the measured width of a glyph
  - **goto_target.hpp** / **mission_pick.hpp**: The two-way state the map is driven through — a right-clicked position target and a waypoint being picked. The map never holds a sender; it reports what the operator asked for and the panel that owns the vehicle sends it
  - **mission_cmds.cpp**: The mission commands the editor knows by name — what each is called, what its parameters mean, and whether it has a position and a height
  - **vehicle_ui_state.hpp**: Panel state that belongs to a vehicle rather than a panel — a calibration in progress, staged parameter edits, the mission being planned — keyed by vehicle instead of held in a file-scope static, and dropped when that vehicle goes
  - **video_player.cpp**: GStreamer pipeline wrapper with RGB frame extraction
- **components/**: Widgets drawn inside a screen division
  - **map/**: The map, flat and 3D, and what the two views share
    - **tiles.cpp**: multi-threaded tile fetcher, disk cache and OpenGL texture cache — the imagery both views draw
    - **tracks.cpp** / **symbols.cpp** / **overlay.cpp**: per-vehicle trails; home, vehicle and aircraft symbols; the tags and lines drawn over either view
    - **map_view.cpp**: the map window — `draw_map_view()` in outline, one call per step, and the switch to 3D
    - **map2d/**: the flat map's steps, sharing map2d_internal.hpp and a per-frame `MapFrame`
      - **input.cpp**: pan and zoom, a pick's click, EDIT and FENCE EDIT drags, the GO HERE and CLEAR clicks — called in a fixed order, which decides who gets a press
      - **layers.cpp** / **fences.cpp**: what is drawn over the tiles, bottom to top; the geofences
      - **menus.cpp** / **chrome.cpp**: the GO HERE and CLEAR menus; the control column, A/C block, scale bar, north pointer and attribution
    - **map_view_3d.cpp**: the 3D view's entry points: the camera, its input, and the frame put together from the parts in map3d/
    - **map3d/**: the 3D view's parts, sharing map3d_internal.hpp
      - **gl.cpp** / **airframes.cpp**: shaders, GL objects and the offscreen target; the airframe meshes and their texture
      - **heights.cpp** / **scene.cpp**: terrain height textures; the tile quadtree, imagery and drawing the frame
      - **craft.cpp** / **overlays.cpp**: aircraft poses and models; trails, mission, drop lines and gizmos drawn over the frame
      - **bands.cpp**: the mission path and geofence walls cast onto the terrain
      - **pick.cpp** / **edit.cpp**: placing mission items and GO HERE points in 3D; moving waypoints in EDIT
  - **others/**: Widgets that belong to no one screen area
    - **ui_kit.cpp**: The shared drawing kit — buttons, readout tiles, panel headers, tab headings, the popup frame and dialog style every panel draws with
    - **sidebar_themes.cpp**: The SETTINGS tab's panel — themes and colours, and the app's settings
    - **plugin_rail.cpp**: Button column driving the user plugins in `plugins/`
    - **mcp_approval.cpp**: The ASSISTANT REQUEST dialog an agent's action waits on
    - **file_dialog.cpp**: The app's file browser (over ImGuiFileDialog), for any panel that reads or writes a file
    - **splash_screen.cpp** / **crt_overlay.cpp**: The start-up splash; the optional CRT treatment (scanlines in the plates, darkened edges)
  - **sidebar_left/**: The left panel's tabs (connection, flight, params, themes, mission, MAVLink, radio, sensors)
    - **rc/**: the RADIO tab's sections — the radio's monitor, calibration and bindings, and the joystick's axes, buttons, virtual channels and output; `tab_rc.cpp` lays them out
    - **params/**: the PARAMS tab's parts — the value editors (enum list, bitmask checkboxes, number field), the parameter info popup, and loading and saving .params files; `tab_params.cpp` keeps the list, staging and WRITE ALL
    - **sensors/**: the SENSORS tab's calibration sections — accelerometer (six positions), gyroscope and compass (with its coverage sphere) — and the sensor inventory they share; `tab_sensors.cpp` keeps the pump that feeds the calibrations and lays the sections out
    - **mission/**: the MISSION tab's parts — the waypoint list, an opened item's fields, the add grid, and the whole GEOFENCE view; `tab_mission.cpp` keeps the editor's sync with the vehicle, map-pick handling, EDIT, the terrain summary and upload
  - **sidebar_right/**: The right panel's sections — the HUD (`hud.cpp`, `hud_parts.cpp`), the classic attitude ball, the fleet event log (`event_log.cpp` takes lines in, `event_log_view.cpp` draws them) and the telemetry grid; the SWARM view and the fullscreen map's overlay draw them too. `elements/sidebar_right.cpp` lays them out
    - **mavlink_display_generated.cpp**: Auto-generated message field decoders (from MAVLink XML)
- **elements/**: The major screen divisions — topbar, left sidebar, right sidebar, centre view, SWARM view

## Plugins (`plugins/`)
- **plugin_api.hpp**: What a plugin sees and how it registers — buttons, startup hooks, video-gesture handlers
- **plugin_registry.cpp**: Registration lists, startup pass, and dispatch
- **user_plugins.cpp**: Example slots, yours to edit — see [plugins/README.md](https://github.com/lazy-iframe/freebodyproblem/blob/main/plugins/README.md)

## Shared code (`lib/`)
Code that more than one part of the app needs and that belongs to none of them. It depends on nothing in `backend/` or `frontend/`, so either may include it; a helper used in only one place stays where it is used.
- **geo.cpp**: Positions on the earth — exact WGS84 distance (Vincenty), flat-earth distance and north/east offset for the short spans the map and the mission editor work over, and Web Mercator in world fractions and in tiles
- **mav_result.cpp**: `MAV_RESULT` as its enum name for protocol traces, and as plain words for the log and the calibration panels
- **text.cpp**: Case changes, comparing and searching without regard to case, and a duration as `hh:mm:ss`
- **tile_fetch.cpp**: What the map imagery and the terrain heights share in fetching z/x/y tiles: splitting and filling the URL template, the on-disk cache layout, reading and saving cached tiles, and the HTTP client setup

## Code Generation
- **scripts/gen_mavlink_display.py**: Parses MAVLink XML definitions to generate C++ field decoders for all message types

## Libraries
- **Dear ImGui** (third_party/imgui): Immediate-mode GUI
- **ImGuiFileDialog** (third_party/ImGuiFileDialog): In-app file browser for the parameter-file path — renders through the same draw list as everything else, so it needs no native GTK/Win32 dialog and inherits the active theme
- **MAVLink** (third_party/mavlink): Auto-generated C headers (ardupilotmega dialect)
- **stb_image / stb_image_write** (FetchContent): PNG/JPG decoding for map tiles, PNG encoding for video snapshots
- **miniaudio** (FetchContent): header-only audio playback for the cue tones
- **cpp-httplib** (FetchContent): HTTPS client for tile server requests, the MCP server, and the adapter's loopback client
- **nlohmann/json** (FetchContent): JSON parsing for settings and firmware manifests

## Project Structure
```
freebodyproblem/
├── backend/              # Links, vehicles, MAVLink parsing, command queue
│   ├── mavlink/          # Wire protocol: framer, signing, timesync, telemetry log
│   │   ├── parser/       # Every message received is decoded here
│   │   └── sender/       # Every message sent is packed here
│   ├── mission/          # Mission protocol logic: download, watch, geofence
│   ├── link/             # Transports and the fleet
│   ├── vehicle/          # Vehicle and its protocol loop's conversations; firmware profile, sensors
│   ├── calibration/      # Accel, gyro, mag, RC calibration
│   ├── input/            # Joystick and RC mapping
│   └── mcp/              # Built-in MCP server
├── frontend/             # ImGui UI, rendering; main.cpp is the entry point
│   ├── source/           # Non-drawing support: settings, terrain, tiles, theme, layout
│   ├── components/       # Widgets drawn inside a screen division
│   │   ├── map/          # The map, flat and 3D
│   │   ├── sidebar_left/ # Per-tab left sidebar modules
│   │   ├── sidebar_right/ # HUD, event log, telemetry grid
│   │   └── others/       # UI kit, theme editor, plugin rail, dialogs, splash
│   └── elements/         # Major screen divisions: topbar, sidebars, centre view
├── lib/                  # Shared code used across backend and frontend: geo, MAV_RESULT, text, tile fetching
├── plugins/              # User C++ functions bound to the button rail
├── adapter/              # freebodyproblem-mcp: stdio bridge for MCP hosts
├── scripts/              # Python code generators
├── third_party/          # Git submodules (imgui, mavlink)
└── CMakeLists.txt        # Build configuration
```
