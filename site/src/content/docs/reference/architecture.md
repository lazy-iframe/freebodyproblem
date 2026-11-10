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
  - **vehicle_ui_state.hpp**: Panel state that belongs to a vehicle rather than a panel — a calibration in progress, staged parameter edits, the mission being planned — keyed by vehicle instead of held in a file-scope static, and dropped when that vehicle goes
  - **video_player.cpp**: GStreamer pipeline wrapper with RGB frame extraction
- **components/**: Widgets drawn inside a screen division
  - **map_view.cpp**: Multi-threaded tile fetcher with OpenGL texture upload, and everything drawn over it — the fleet's aircraft symbols and labels, per-vehicle trails, home pads, the mission overlay and the GO HERE menu
  - **plugin_rail.cpp**: Button column driving the user plugins in `plugins/`
  - **mcp_approval.cpp**: The ASSISTANT REQUEST dialog an agent's action waits on
  - **sidebar_left/**: The left panel's tabs (connection, flight, params, themes, mission, MAVLink, radio, sensors)
    - **mavlink_display_generated.cpp**: Auto-generated message field decoders (from MAVLink XML)
- **elements/**: The major screen divisions — topbar, left sidebar, right sidebar, centre view, SWARM view

## Plugins (`plugins/`)
- **plugin_api.hpp**: What a plugin sees and how it registers — buttons, startup hooks, video-gesture handlers
- **plugin_registry.cpp**: Registration lists, startup pass, and dispatch
- **user_plugins.cpp**: Example slots, yours to edit — see [plugins/README.md](https://github.com/lazy-iframe/freebodyproblem/blob/main/plugins/README.md)

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
│   ├── mavlink/          # Wire protocol: framer, signing, timesync
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
│   ├── components/       # Reusable widgets: map, plugin rail, UI kit, dialogs
│   │   └── sidebar_left/ # Per-tab left sidebar modules
│   └── elements/         # Major screen divisions: topbar, sidebars, centre view
├── plugins/              # User C++ functions bound to the button rail
├── adapter/              # freebodyproblem-mcp: stdio bridge for MCP hosts
├── scripts/              # Python code generators
├── third_party/          # Git submodules (imgui, mavlink)
└── CMakeLists.txt        # Build configuration
```
