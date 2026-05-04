# Free Body Problem

A modern, fast ground control station for ArduPilot and PX4 autopilots. Built for operators who know what they're doing. The development is in its early stages.

## Philosophy

This GCS is designed for UAV professionals and enthusiasts already familiar with MAVLink, ArduPilot/PX4 ecosystems, and autonomous systems. It prioritizes speed, responsiveness, and direct access to vehicle internals over guided workflows and safety checks. If you need a beginner-friendly interface, try something like QGroundControl which is much more user-friendly. Only the features which are actively needed during a flight will be prioritized. (firmware upload, log analysis, etc. not planned as features)

## Features

> Only tested with Ardupilot for now. PX4 support pending.

### Core Telemetry
- **Real-time vehicle state**: attitude, GPS, altitude, speed, battery, EKF health
- **MAVLink inspector**: live message stream viewer with field-level decoding for all message types
- **Parameter management**: fetch, search, edit, and write parameters with metadata tooltips
- **Flight modes**: quick mode switching with visual feedback (ArduCopter/Plane/Rover/Sub)
- **EKF status**: variance bars for velocity, horizontal/vertical position, compass, terrain, and airspeed
- **Status text log**: vehicle-generated messages with severity color-coding

### Mission Planning
- **Interactive map view**: slippy map with OpenStreetMap tile support (configurable tile server)
- **Mission upload/download**: create, edit, and upload waypoints with visual feedback
- **Point-and-click waypoint editing**: click map to place waypoints during mission planning
- **Real-time vehicle tracking**: aircraft position and heading overlay on map

### Connection
- **Multiple transport layers**: UDP, TCP, Serial (POSIX)
- **Auto-discovery**: serial port enumeration with device descriptions
- **Configurable baud rates**: 9600 to 921600
- **Automatic telemetry rate configuration**: requests optimal message rates on connect
- **Connection timeout handling**: 10-second timeout with clear status indicators

### Video Streaming
- **GStreamer integration**: RTSP and UDP video streams
- **Zero-copy frame pipeline**: efficient RGB decoding for real-time display
- **Configurable stream URLs**: support for standard video sources

### Auxiliary Functions
- **Servo/Aux control**: configure and trigger auxiliary functions
- **Arming/disarming**: direct vehicle control with command ACK feedback
- **Motor interlock**: safety interlock control for helicopters

### UI/UX
- **Dear ImGui interface**: immediate-mode GUI with low latency
- **Theme support**: includes Dracula theme and customizable color schemes
- **Application log**: bottom-bar console with MAVLink events and system messages
- **Splash screen**: dismissible startup overlay


## Building

### Dependencies
- **CMake** ≥ 3.13
- **C++17 compiler** (GCC, Clang, MSVC)
- **OpenGL** 3.3+
- **GLFW** 3.4 (auto-fetched if not system-installed)
- **GStreamer** 1.0 (with gstreamer-app)
- **OpenSSL** (for HTTPS tile fetching)
- **Python 3** (build-time only, for MAVLink code generation)

### Ubuntu/Debian
```bash
sudo apt install cmake g++ libglfw3-dev libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev libssl-dev python3 \
    gstreamer1.0-plugins-{base,good,bad,ugly} gstreamer1.0-libav
```

### Build Steps
```bash
# Clone with submodules (includes MAVLink definitions and Dear ImGui)
git clone --recursive https://github.com/your-username/freebodyproblem.git
cd freebodyproblem

# Configure and build
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

# Run
./build/freebodyproblem
```

#### Build Options
- **MAVLINK_DIALECT**: Default `ardupilotmega`. Supports all the dialects that MAVLink main repo does.
- **MAVLINK_VERSION**: Default `2.0` (MAVLink 2.0 wire protocol)

```bash
cmake -B build -DMAVLINK_DIALECT=common -DMAVLINK_VERSION=2.0
```

## Usage

### Quick Start
1. Launch `freebodyproblem`
2. Click any key to dismiss splash screen
3. Select connection type in left sidebar (Serial/TCP/UDP)
4. Configure transport parameters:
   - **Serial**: select port from dropdown, set baud rate
   - **TCP**: enter IP and port (e.g., `192.168.1.100:5760`)
   - **UDP**: bind address and port (default `0.0.0.0:14550`)
5. Click **CONNECT**
6. Telemetry will auto-configure and display within seconds

### Parameter Workflow
1. Navigate to **PARAMETERS** tab
2. Click **FETCH ALL** to download full parameter set
3. Use search box to filter (e.g., `WPNAV`, `EK2_`)
4. Edit values in-place with spinners
5. Click **WRITE** to commit changes to vehicle

### Mission Planning
1. Navigate to **MISSION** tab in left sidebar
2. Click **REQUEST** to download current mission from vehicle
3. Click **ADD WAYPOINT** to append a new point
4. Click the **pin icon** next to a waypoint to enable map pick mode
5. Click on the map to set waypoint coordinates
6. Adjust altitude, command type (NAV_WAYPOINT, NAV_LOITER_UNLIM, etc.)
7. Click **UPLOAD** to send mission to vehicle
8. Click **CLEAR** to erase vehicle mission (zero waypoints)

### MAVLink Inspector
1. Navigate to **MAVLINK** tab
2. View message ID, name, and receive rate in scrollable table
3. Click a message row to decode all fields in the detail panel below
4. Use the **REQUEST** panel to configure message intervals (MAVLink 2 only):
   - Enter message ID (e.g., `33` for GLOBAL_POSITION_INT)
   - Set rate in Hz (e.g., `10`)
   - Click **SEND** to apply

### Video Streaming
1. Navigate to the **VIDEO** area
2. Enter stream URL:
   - RTSP: `rtsp://192.168.1.100:8554/stream`
   - UDP: `udp://0.0.0.0:5600`
3. Click **START STREAM**
4. Video will render in center view

### Themes
1. Navigate to **SETTINGS** tab
2. Select **Dracula** or **Retro Amber** (default)
3. Create custom themes by editing color values
4. Themes persist across sessions via `settings.json`

## Architecture

### Backend (`backend/`)
- **connection.cpp**: Serial port enumeration (Linux/Windows)
- **mavlink_parser.cpp**: Stateless MAVLink message decoder with per-ID stats tracking
- **mavlink_sender.cpp**: Command queue with ACK tracking and retransmit logic

### Frontend (`frontend/`)
- **main.cpp**: GLFW/OpenGL event loop, link thread management, MAVLink I/O
- **settings.cpp**: JSON-based persistent configuration (tile server, themes, window state)
- **widgets/**: Modular UI components (topbar, sidebars, map, video, telemetry panels)
  - **sidebar_left/**: Tab-based left panel (connection, flight, params, themes, mission, MAVLink)
  - **map_view.cpp**: Multi-threaded tile fetcher with OpenGL texture upload
  - **video_player.cpp**: GStreamer pipeline wrapper with RGB frame extraction
  - **mavlink_display_generated.cpp**: Auto-generated message field decoders (from MAVLink XML)

### Code Generation
- **scripts/gen_mavlink_display.py**: Parses MAVLink XML definitions to generate C++ field decoders for all message types

### Libraries
- **Dear ImGui** (third_party/imgui): Immediate-mode GUI
- **MAVLink** (third_party/mavlink): Auto-generated C headers (ardupilotmega dialect)
- **stb_image** (FetchContent): PNG/JPG decoding for map tiles
- **cpp-httplib** (FetchContent): HTTPS client for tile server requests
- **nlohmann/json** (FetchContent): JSON parsing for settings and firmware manifests

## Project Structure
```
freebodyproblem/
├── backend/              # MAVLink I/O, parsing, command queue
├── frontend/             # ImGui UI, widgets, rendering
│   └── widgets/          # Reusable UI components
│       └── sidebar_left/ # Per-tab left sidebar modules
├── scripts/              # Python code generators
├── third_party/          # Git submodules (imgui, mavlink)
└── CMakeLists.txt        # Build configuration
```

## Configuration

Settings are auto-saved to `settings.json` in the working directory:
- **Active theme**: Selected color scheme
- **Custom themes**: User-defined color palettes
- **Tile server**: URL template and attribution string
- **Video URL**: Last used stream source

Example `settings.json`:
```json
{
  "active_theme": "Dracula",
  "tile_url": "https://tile.openstreetmap.org/{z}/{x}/{y}.png",
  "tile_attribution": "© OpenStreetMap contributors",
  "video_url": "udp://0.0.0.0:5600",
  "themes": {
    "Custom": {
      "bg": [0.05, 0.05, 0.08],
      "accent": [0.2, 0.8, 0.9],
      ...
    }
  }
}
```

## Performance Notes

- **Zero-copy MAVLink parsing**: Messages are decoded in-place from UDP datagrams
- **Threaded tile fetching**: Map tiles load asynchronously without blocking the render loop
- **Exponential moving average rates**: Message rate display decays gracefully when telemetry stops
- **Minimal allocations**: Telemetry state is copied once per frame; no per-message heap allocations
- **Parameter retransmit logic**: Automatically re-requests dropped PARAM_VALUE packets after 2s stall

## Limitations

- **No firmware upload**: Use ArduPilot's `uploader.py` or Mission Planner for bootloader operations
- **No log download**: Access DataFlash logs via MAVLink file transfer in other tools
- **OS support**: Tested on POSIX only (Linux/macOS), although can be build on windows in theory. Some features such as serial connection would have to be updated accordingly to support windows.
- **No telemetry replay**: Live connections only; no `.tlog` or `.bin` file playback
- **No geofence editor**: Geofence/rally point management not implemented yet

## Future Plans

- Implementations of "Console", "Radio" and "ESC" tabs which are for Mavlink Console, Radio Controller calibration and connection management, and ESC configuration along with motor test respectively.
- Multi-Vehicle Support
- PX4 Support
- Windows Support
- Video AI features

## Contributing

This is a personal project built for speed and clarity over feature completeness (only the features needed **during** the flight should be implemented). PRs welcome for:
- Bug fixes
- Performance improvements
- Additional MAVLink message handlers
- Windows support
- Telemetry graph overlays
- Other features which are essential during a flight

Please keep contributions focused on the core philosophy: fast, direct, minimal abstraction.
