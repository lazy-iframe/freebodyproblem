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
- **connection.cpp**: Serial port enumeration (Linux/Windows)
- **link.cpp**: One transport — UDP socket, TCP connection or serial port — and the platform detail behind opening, reading and writing it
- **fleet.cpp**: Every link and vehicle, the link read threads, and the sysid demultiplexing that decides which vehicle a message belongs to
- **vehicle.cpp**: One vehicle and the thread that talks to it — stream rates, the mode list, clock sync, the parameter fetch and its retransmits, and the snapshot the UI reads
- **mavlink_framer.hpp**: Byte stream to whole messages, with the reassembly buffer held per link rather than in the library's per-channel globals, so links can frame concurrently
- **mavlink_parser.cpp**: MAVLink message decoder with per-ID stats tracking; one per vehicle, and it accepts only that vehicle's system
- **mavlink_sender.cpp**: Command queue with ACK tracking and retransmit logic; one per vehicle, each keeping its own outgoing sequence counter, so there is no cap on how many vehicles the fleet holds
- **mcp_server.cpp**: the built-in MCP server — Streamable HTTP on loopback, bearer token, the read and action tools, the approval queue the UI thread drains, vehicle claims between agents, and the separation check on takeoff and goto paths
- **geofence.cpp**: uploaded geofence zones — polygons and circles, inclusion and exclusion — to and from the fence list's mission items
- **firmware_profile.cpp**: what differs between flight stacks in one place — which message carries estimator health (ArduPilot's `EKF_STATUS_REPORT` vs the portable `ESTIMATOR_STATUS`), each stack's built-in mode table for vehicles that publish no `AVAILABLE_MODES`, and how a mode number is written on the wire. PX4 splits main and sub mode across two command parameters where ArduPilot puts one flat number in the first; sending ArduPilot's shape at PX4 is rejected
- **rc_calibration.cpp** / **rc_binding.cpp**: RC endpoint measurement and the stick/mode/aux parameter tables, per stack
- **accel_calibration.cpp** / **gyro_calibration.cpp** / **mag_calibration.cpp**: the GCS half of each calibration — link-free state machines fed messages and a clock
- **sensor_inventory.cpp**: compass/accel/gyro devices decoded out of the parameter table (device-ID packing, chip names)
- **geodesic_grid.cpp**: ArduPilot's 80-section coverage grid, for drawing what a compass calibration has and has not seen

## Frontend (`frontend/`)
- **main.cpp**: GLFW/OpenGL event loop, the app log, and the per-frame snapshot of whichever vehicle is selected
- **settings.cpp**: JSON-based persistent configuration (tile server, themes, window state)
- **param_file.cpp**: `.params` reader/writer (Mission Planner / QGroundControl format)
- **audio.cpp**: synthesised cue tones and accelerating progress ticks — lock-free voice pool mixed on the miniaudio callback
- **widgets/**: Modular UI components (topbar, sidebars, map, video, telemetry panels)
  - **layout_rules.hpp**: The screen layout as plain arithmetic, free of ImGui — sidebar widths in glyphs, the narrow cutoff, and the stacked or side-by-side split of MAP + VIDEO and of the SWARM map. **layout.hpp** feeds it the live window and the measured width of a glyph
  - **goto_target.hpp** / **mission_pick.hpp**: The two-way state the map is driven through — a right-clicked position target and a waypoint being picked. The map never holds a sender; it reports what the operator asked for and the panel that owns the vehicle sends it
  - **vehicle_ui_state.hpp**: Panel state that belongs to a vehicle rather than a panel — a calibration in progress, staged parameter edits, the mission being planned — keyed by vehicle instead of held in a file-scope static, and dropped when that vehicle goes
  - **sidebar_left/**: Tab-based left panel (connection, flight, params, themes, mission, MAVLink, radio, sensors)
  - **map_view.cpp**: Multi-threaded tile fetcher with OpenGL texture upload, and everything drawn over it — the fleet's aircraft symbols and labels, per-vehicle trails, home pads, the mission overlay and the GO HERE menu
  - **video_player.cpp**: GStreamer pipeline wrapper with RGB frame extraction
  - **plugin_rail.cpp**: Button column driving the user plugins in `plugins/`
  - **mcp_approval.cpp**: The ASSISTANT REQUEST dialog an agent's action waits on
  - **mavlink_display_generated.cpp**: Auto-generated message field decoders (from MAVLink XML)

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
├── frontend/             # ImGui UI, widgets, rendering
│   └── widgets/          # Reusable UI components
│       └── sidebar_left/ # Per-tab left sidebar modules
├── plugins/              # User C++ functions bound to the button rail
├── adapter/              # freebodyproblem-mcp: stdio bridge for MCP hosts
├── scripts/              # Python code generators
├── third_party/          # Git submodules (imgui, mavlink)
└── CMakeLists.txt        # Build configuration
```
