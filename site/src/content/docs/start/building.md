---
title: Building from source
description: Build Free Body Problem and its packages yourself.
---

Only needed for development, or to run on a platform without a prebuilt package.

## Dependencies
- **CMake** ≥ 3.13
- **C++17 compiler** (GCC, Clang, MSVC)
- **OpenGL** 3.3+
- **GLFW** 3.4 (auto-fetched if not system-installed)
- **GStreamer** 1.0 (with gstreamer-app)
- **OpenSSL** (for HTTPS tile fetching)
- **Python 3** (build-time only, for MAVLink code generation)

## Ubuntu/Debian
```bash
sudo apt install cmake g++ libglfw3-dev libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev libssl-dev python3 \
    gstreamer1.0-plugins-{base,good,bad,ugly} gstreamer1.0-libav
```

## macOS
```bash
xcode-select --install
brew install cmake ninja pkg-config python
```

Then install GStreamer.framework, both the **runtime** and the **development**
`.pkg`, from [gstreamer.freedesktop.org](https://gstreamer.freedesktop.org/download/)
(CI uses 1.26). The build finds it in `/Library/Frameworks`, takes OpenSSL from it
too, and copies what it needs into the `.app` at packaging time.

Homebrew's `gstreamer` also builds, with
`-DOPENSSL_ROOT_DIR=$(brew --prefix openssl@3)`, but the result only runs on
machines with the same Homebrew libraries, so it is for development only.

## Build Steps
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

On macOS the executable is inside the bundle:
`./build/freebodyproblem.app/Contents/MacOS/freebodyproblem`.

Running straight out of the build tree works: fonts are located next to the
executable, then in the installed prefix, then in the source tree.

### Build Options
- **MAVLINK_DIALECT**: Default `ardupilotmega`. Supports all the dialects that MAVLink main repo does.
- **MAVLINK_VERSION**: Default `2.0` (MAVLink 2.0 wire protocol)

```bash
cmake -B build -DMAVLINK_DIALECT=common -DMAVLINK_VERSION=2.0
```

## Building the Packages

Packaging is handled by CPack, which ships with CMake. The generator is selected per platform, so no `-G` argument is needed.

```bash
cmake --build build --target package
```

The result lands in `build/` — `freebodyproblem_<version>_amd64.deb` on Linux,
`freebodyproblem-<version>-win64.exe` on Windows,
`freebodyproblem-<version>-Darwin.dmg` on macOS.

Extra tooling per platform:

| Platform | Generator | Requires                                                                                         |
|----------|-----------|--------------------------------------------------------------------------------------------------|
| Linux    | `DEB`     | `dpkg-dev` (for `dpkg-shlibdeps`, which derives the dependency list)                             |
| Windows  | `NSIS`    | [NSIS](https://nsis.sourceforge.io/) (`choco install nsis`) and the GStreamer MSVC **devel** SDK |
| macOS    | `DragNDrop` | GStreamer.framework, runtime and development packages, for a self-contained bundle (see [macOS](#macos)) |

Inspect a built `.deb` with:

```bash
dpkg-deb --info build/freebodyproblem_0.1.0_amd64.deb      # metadata and dependencies
dpkg-deb --contents build/freebodyproblem_0.1.0_amd64.deb  # file listing
```
