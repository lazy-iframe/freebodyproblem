<p align="center">
  <img src="screens/splash.svg" alt="Free Body Problem — ground control station" width="520">
</p>

<p align="center">
  <a href="https://github.com/lazy-iframe/freebodyproblem/actions/workflows/ci.yml"><img src="https://img.shields.io/github/actions/workflow/status/lazy-iframe/freebodyproblem/ci.yml?branch=main&style=flat-square&label=integration&labelColor=36441E" alt="integration"></a>
  <a href="https://github.com/lazy-iframe/freebodyproblem/actions/workflows/cd.yml"><img src="https://img.shields.io/github/actions/workflow/status/lazy-iframe/freebodyproblem/cd.yml?style=flat-square&label=build&labelColor=36441E" alt="build"></a>
  <a href="https://github.com/lazy-iframe/freebodyproblem/releases/latest"><img src="https://img.shields.io/github/v/release/lazy-iframe/freebodyproblem?style=flat-square&label=release&labelColor=36441E&color=ECB94E" alt="release"></a>
  <a href="https://github.com/lazy-iframe/freebodyproblem/releases"><img src="https://img.shields.io/badge/changelog-release%20notes-6E7B55?style=flat-square&labelColor=36441E" alt="changelog"></a>
  <a href="https://github.com/lazy-iframe/freebodyproblem#readme"><img src="https://img.shields.io/github/v/tag/lazy-iframe/freebodyproblem?style=flat-square&label=docs&labelColor=36441E&color=97EA89" alt="docs"></a>
</p>

A modern, fast ground control station for ArduPilot and PX4 autopilots. Built for operators who know what they're doing. The development is in its early stages.

## Philosophy

This GCS is designed for UAV professionals and enthusiasts already familiar with MAVLink, ArduPilot/PX4 ecosystems, and autonomous systems. It prioritizes speed, responsiveness, and direct access to vehicle internals over guided workflows and safety checks. If you need a beginner-friendly interface, try something like QGroundControl which is much more user-friendly. Only the features which are actively needed during a flight will be prioritized. (firmware upload, log analysis, etc. not planned as features)

## Features

> Only tested with Ardupilot for now. PX4 support pending.

![screen capture](screens/cap0.png)

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
- **Multiple transport layers**: UDP, TCP, Serial (Linux and Windows)
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

### Plugins
- **User C++ functions on the rail**: a column of square buttons down the right of the centre view, each one a function you write in [`plugins/`](plugins/README.md)
- **Full vehicle access**: telemetry snapshot, the MAVLink sender, and any `MAV_CMD` via `command_long`
- **Camera surface**: start/stop the feed and save the current frame to PNG, for payload workflows the UI does not model
- **Clickable video**: click a point or drag a box on the feed to run your own handler, with coordinates in frame space — wired to payload track-point/track-rectangle out of the box
- **No build wiring**: new `.cpp` files in `plugins/` are globbed by CMake; buttons can rename themselves at runtime
- **Blue-accented rail**: the app's olive panels with blue on the seams, captions and title where the rest of the UI goes amber, so user code is never mistaken for vehicle chrome; an engaged plugin holds the same red as the ARMED annunciator

### UI/UX
- **Dear ImGui interface**: immediate-mode GUI with low latency
- **Theme support**: Tactical (default), Retro Amber and Matrix built-ins, plus customizable color schemes
- **Application log**: bottom-bar console with MAVLink events and system messages
- **Splash screen**: dismissible startup overlay


## Install

Prebuilt packages are attached to each release on the
[Releases page](https://github.com/lazy-iframe/freebodyproblem/releases).
If you only want to run the application, use these — building from source is
not required.

### Linux (Ubuntu)

**1. Refresh the package index.** The `.deb` does not vendor its runtime
libraries; apt pulls them from your configured repositories, and it can only do
that against a current index. On a fresh install this step is not optional:

```bash
sudo apt update
```

**2. Install the package.** Pass the path with a leading `./` so apt treats it
as a file rather than a package name:

```bash
sudo apt install ./freebodyproblem_0.1.0_amd64.deb
```

This reads the package's `Depends` and installs everything needed — GStreamer,
OpenSSL, GLib, OpenGL, and the C/C++ runtimes. There is no separate list of
libraries to install beforehand.

Do **not** use `dpkg -i`: it installs the package without resolving
dependencies and leaves it unconfigured. If you already did, recover with:

```bash
sudo apt --fix-broken install
```

**3. Run it.** The binary lands in `/usr/bin`, so it is already on your `PATH`:

```bash
freebodyproblem
```

Remove it later with `sudo apt remove freebodyproblem`.

#### Distribution compatibility

The dependency list is generated at build time from the machine that produced
the package, so it inherits that machine's library versions as a *minimum*.
Check what a given `.deb` actually requires before installing:

```bash
dpkg-deb --info freebodyproblem_0.1.0_amd64.deb | grep Depends
```

Two consequences worth knowing:

- **Older Ubuntu releases may refuse it.** If apt reports something like
  `libc6 (>= 2.43) but 2.39-0ubuntu8.3 is to be installed`, your release is
  older than the build machine. Build from source instead — there is no way to
  satisfy a newer glibc on an older system.
- **Debian is not covered.** The package depends on `libssl3t64` and
  `libglib2.0-0t64`, names introduced by Ubuntu's 64-bit `time_t` transition.
  Debian calls these `libssl3` and `libglib2.0-0`, so the dependencies cannot
  be satisfied there. Build from source on Debian.

#### Video streaming plugins

GStreamer loads its codecs at runtime, so they cannot be detected as hard
dependencies. They are listed under `Recommends` and apt installs them by
default. If you run apt with `--no-install-recommends`, add them yourself or
video will fail to start:

```bash
sudo apt install gstreamer1.0-plugins-{base,good,bad} gstreamer1.0-libav
```

#### Graphics driver

The UI needs an OpenGL 3.3+ core profile. Verify with `glxinfo` (from
`mesa-utils`):

```bash
glxinfo | grep "OpenGL core profile version"
```

Anything below 3.3 — common on bare VMs with no GPU passthrough — will fail at
window creation.

### Windows

**Nothing needs installing first.** The installer is self-contained: it bundles
the GStreamer runtime and its plugins, the OpenSSL libraries, and the Visual C++
runtime. Download `freebodyproblem-<version>-win64.exe` and run it.

It installs to `C:\Program Files\freebodyproblem` by default and adds a Start
Menu entry. Uninstall from **Settings → Apps**, or with the uninstaller in the
install directory.

Two things to expect:

- The installer is unsigned, so SmartScreen warns on first run. Choose
  **More info → Run anyway**.
- As on Linux, the UI needs an OpenGL 3.3+ driver. A fresh VM running on the
  Microsoft Basic Display Adapter does not provide one — install your GPU
  vendor's driver first.

---

## Building from Source

Only needed for development, or to run on a platform without a prebuilt package.

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
git clone --recursive https://github.com/lazy-iframe/freebodyproblem.git
cd freebodyproblem

# Configure and build
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

# Run
./build/freebodyproblem
```

Running straight out of the build tree works: fonts are located next to the
executable, then in the installed prefix, then in the source tree.

#### Build Options
- **MAVLINK_DIALECT**: Default `ardupilotmega`. Supports all the dialects that MAVLink main repo does.
- **MAVLINK_VERSION**: Default `2.0` (MAVLink 2.0 wire protocol)

```bash
cmake -B build -DMAVLINK_DIALECT=common -DMAVLINK_VERSION=2.0
```

### Building the Packages

Packaging is handled by CPack, which ships with CMake. The generator is selected per platform, so no `-G` argument is needed.

```bash
cmake --build build --target package
```

The result lands in `build/` — `freebodyproblem_<version>_amd64.deb` on Linux,
`freebodyproblem-<version>-win64.exe` on Windows.

Extra tooling per platform:

| Platform | Generator | Requires                                                                                         |
|----------|-----------|--------------------------------------------------------------------------------------------------|
| Linux    | `DEB`     | `dpkg-dev` (for `dpkg-shlibdeps`, which derives the dependency list)                             |
| Windows  | `NSIS`    | [NSIS](https://nsis.sourceforge.io/) (`choco install nsis`) and the GStreamer MSVC **devel** SDK |

Inspect a built `.deb` with:

```bash
dpkg-deb --info build/freebodyproblem_0.1.0_amd64.deb      # metadata and dependencies
dpkg-deb --contents build/freebodyproblem_0.1.0_amd64.deb  # file listing
```

### Releasing

Pushing a `v*` tag builds both packages and publishes them to a
[GitHub Release](https://github.com/lazy-iframe/freebodyproblem/releases)
(`.github/workflows/cd.yml`).

The version in `CMakeLists.txt` is the single source of truth — CPack stamps it
into the package filenames and metadata — so it must be bumped *before* tagging:

```bash
# 1. Bump the version
#    CMakeLists.txt:  project(freebodyproblem VERSION 0.2.0 LANGUAGES CXX)
git commit -am "release 0.2.0"
git push

# 2. Tag and push
git tag v0.2.0
git push origin v0.2.0
```

The pipeline verifies the tag against `project(... VERSION ...)` before building
anything, and fails within seconds if they disagree:

```text
Tag v0.2.0 does not match project() VERSION 0.1.0 in CMakeLists.txt.
```

To recover from that, bump the version, commit, then move the tag:

```bash
git tag -d v0.2.0
git push origin :refs/tags/v0.2.0
git tag v0.2.0 && git push origin v0.2.0
```

#### Prereleases

A hyphen in the tag marks the release as a prerelease on GitHub. The suffix is
ignored when matching against `CMakeLists.txt`, so `v0.2.0-rc1` is valid while
the project version reads `0.2.0`:

```bash
git tag v0.2.0-rc1 && git push origin v0.2.0-rc1
```

This is the safest way to exercise the release pipeline end to end without
publishing a headline version.

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
2. Select **Tactical** (default), **Retro Amber** or **Matrix**
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
├── plugins/              # User C++ functions bound to the button rail
├── scripts/              # Python code generators
├── third_party/          # Git submodules (imgui, mavlink)
└── CMakeLists.txt        # Build configuration
```

## Configuration

Settings are auto-saved to a per-user location, not the working directory:

| Platform | Path                                                                                           |
|----------|------------------------------------------------------------------------------------------------|
| Linux    | `$XDG_CONFIG_HOME/freebodyproblem/settings.json`, or `~/.config/freebodyproblem/settings.json` |
| Windows  | `%APPDATA%\freebodyproblem\settings.json`                                                      |

If neither environment variable is set, it falls back to
`./freebodyproblem_settings.json` in the working directory.

Map tiles are cached separately, under
`$XDG_CACHE_HOME`/`~/.cache/freebodyproblem/tiles` on Linux and
`%LOCALAPPDATA%\freebodyproblem\tiles` on Windows. Deleting that directory is
safe — tiles are refetched on demand.

Settings contents:
- **Active theme**: Selected color scheme
- **Custom themes**: User-defined color palettes
- **Tile server**: URL template and attribution string
- **Video URL**: Last used stream source

Example `settings.json`:
```json
{
  "active_theme": "Tactical",
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
- **OS support**: Linux and Windows are built and packaged by CI. macOS is not currently built or tested.
- **No telemetry replay**: Live connections only; no `.tlog` or `.bin` file playback
- **No geofence editor**: Geofence/rally point management not implemented yet

## Future Plans

- Implementations of "Console", "Radio" and "ESC" tabs which are for Mavlink Console, Radio Controller calibration and connection management, and ESC configuration along with motor test respectively.
- Multi-Vehicle Support
- PX4 Support
- macOS Support
- Video AI features

## Contributing

This is a personal project built for speed and clarity over feature completeness (only the features needed **during** the flight should be implemented). PRs welcome for:
- Bug fixes
- Performance improvements
- Additional MAVLink message handlers
- macOS support
- Telemetry graph overlays
- Other features which are essential during a flight

Please keep contributions focused on the core philosophy: fast, direct, minimal abstraction.
