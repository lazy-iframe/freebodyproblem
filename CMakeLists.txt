cmake_minimum_required(VERSION 3.13)

# The oldest macOS the .app runs on, and the LSMinimumSystemVersion in
# assets/Info.plist.in. Has to be set before project(); ignored elsewhere.
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")

project(freebodyproblem VERSION 0.8.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── MAVLink options ────────────────────────────────────────────────────────────
set(MAVLINK_DIALECT "ardupilotmega" CACHE STRING "MAVLink message dialect")
set(MAVLINK_VERSION "2.0"   CACHE STRING "MAVLink wire-protocol version (1.0 or 2.0)")

# ── Locate or generate MAVLink headers ────────────────────────────────────────
#
# Priority:
#   1. System-installed MAVLink found via find_package
#   2. Headers already generated in the local install prefix (previous build)
#   3. Generate from the submodule at build time (first build / clean build)
#
# The generation step mirrors the official instructions:
#   cmake -Bbuild -H. -DCMAKE_INSTALL_PREFIX=install \
#         -DMAVLINK_DIALECT=common -DMAVLINK_VERSION=2.0
#   cmake --build build --target install

find_package(MAVLink QUIET)

if(MAVLink_FOUND)
    message(STATUS "MAVLink: using system installation (${MAVLink_DIR})")

    add_library(mavlink_headers INTERFACE)
    target_link_libraries(mavlink_headers INTERFACE MAVLink::mavlink)

else()
    set(_MAVLINK_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/mavlink-install")
    set(_MAVLINK_SENTINEL
        "${_MAVLINK_INSTALL_PREFIX}/include/mavlink/${MAVLINK_DIALECT}/mavlink.h")

    if(EXISTS "${_MAVLINK_SENTINEL}")
        message(STATUS "MAVLink: using pre-generated headers in ${_MAVLINK_INSTALL_PREFIX}/include")

        add_library(mavlink_headers INTERFACE)
        target_include_directories(mavlink_headers INTERFACE
            "${_MAVLINK_INSTALL_PREFIX}/include")

    else()
        message(STATUS "MAVLink: headers not found – will generate from submodule at build time")

        # Ensure pymavlink nested submodule is initialized (required for header generation)
        find_package(Git QUIET REQUIRED)
        execute_process(
            COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/third_party/mavlink"
            RESULT_VARIABLE _GIT_SUBMODULE_RESULT
            OUTPUT_QUIET
        )
        if(NOT _GIT_SUBMODULE_RESULT EQUAL 0)
            message(FATAL_ERROR
                "Failed to initialize pymavlink submodule. "
                "Run: git submodule update --init --recursive")
        endif()

        include(ExternalProject)

        ExternalProject_Add(mavlink_generate
            SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/mavlink"
            BINARY_DIR "${CMAKE_BINARY_DIR}/mavlink-build"
            INSTALL_DIR "${_MAVLINK_INSTALL_PREFIX}"
            CMAKE_ARGS
                -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
                -DMAVLINK_DIALECT=${MAVLINK_DIALECT}
                -DMAVLINK_VERSION=${MAVLINK_VERSION}
            # Re-runs the install step (which includes header generation) only
            # when the ExternalProject stamp is missing. To force regeneration
            # after changing a dialect XML, delete:
            #   ${CMAKE_BINARY_DIR}/mavlink-build/CMakeFiles/mavlink_generate-complete
            BUILD_ALWAYS FALSE
        )

        add_library(mavlink_headers INTERFACE)
        target_include_directories(mavlink_headers INTERFACE
            "${_MAVLINK_INSTALL_PREFIX}/include")
        add_dependencies(mavlink_headers mavlink_generate)

    endif()
endif()

# ── Third-party: stb (header-only, no CMakeLists) ─────────────────────────────

include(FetchContent)

FetchContent_Declare(stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        master
)
FetchContent_GetProperties(stb)
if(NOT stb_POPULATED)
    FetchContent_Populate(stb)
endif()
# stb_SOURCE_DIR is now set; consumers add it to their include paths below.

# ── macOS: GStreamer.framework ────────────────────────────────────────────────
#
# The macOS counterpart of the Windows MSVC SDK: the official framework from
# gstreamer.freedesktop.org, which is built to be relocatable and is copied
# whole into the .app at install time (cmake/macos_bundle.cmake), so the disk
# image runs on a Mac with nothing else installed. Homebrew's GStreamer is not
# used for a release: its libraries hard-code /opt/homebrew paths and are built
# for whatever macOS the build machine runs.
#
# Without the framework the build still works, against Homebrew via
# pkg-config, but the bundle is then only for the machine that built it.
if(APPLE)
    set(GSTREAMER_FRAMEWORK "/Library/Frameworks/GStreamer.framework"
        CACHE PATH "GStreamer.framework to build against and bundle")
    set(_gst_fw_root "${GSTREAMER_FRAMEWORK}/Versions/1.0")
    if(EXISTS "${_gst_fw_root}/lib/pkgconfig/gstreamer-1.0.pc")
        set(GCS_BUNDLE_GSTREAMER ON)
        # Searched ahead of Homebrew's .pc files, so a Mac with both gets the
        # framework.
        set(ENV{PKG_CONFIG_PATH} "${_gst_fw_root}/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
        message(STATUS "GStreamer: using ${GSTREAMER_FRAMEWORK} (bundled into the .app)")
        # The framework carries OpenSSL for its own plugins; using that copy
        # means one libssl in the bundle rather than two.
        if(NOT OPENSSL_ROOT_DIR AND EXISTS "${_gst_fw_root}/include/openssl/ssl.h")
            set(OPENSSL_ROOT_DIR "${_gst_fw_root}")
            message(STATUS "OpenSSL: using the copy in GStreamer.framework")
        endif()
    else()
        message(STATUS "GStreamer: no framework at ${GSTREAMER_FRAMEWORK}; "
                       "using pkg-config (the .app will not be self-contained)")
    endif()
endif()

# ── Third-party: cpp-httplib (header-only HTTPS client) ───────────────────────
# Verify the tag at https://github.com/yhirose/cpp-httplib/releases if the
# build fails to fetch.

find_package(OpenSSL REQUIRED)

set(HTTPLIB_REQUIRE_OPENSSL ON  CACHE BOOL "" FORCE)
set(HTTPLIB_USE_OPENSSL_IF_AVAILABLE ON CACHE BOOL "" FORCE)
if(APPLE)
    # httplib links Brotli when it finds it, and on a Mac with Homebrew it does.
    # Homebrew builds for the machine's own macOS, so that copy would raise the
    # bundle's minimum to whatever the build machine runs (26 on CI). Tile and
    # elevation servers answer gzip or uncompressed just the same.
    set(HTTPLIB_USE_BROTLI_IF_AVAILABLE OFF CACHE BOOL "" FORCE)
endif()
# Vendored dependencies must not contribute their own install rules — otherwise
# the package ships their headers, static libs and CMake config, and
# /usr/include/GLFW/glfw3.h collides with the system libglfw3-dev package.
set(HTTPLIB_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG        v0.18.3
)
FetchContent_MakeAvailable(httplib)

# ── Third-party: nlohmann/json (header-only JSON) ─────────────────────────────

set(JSON_Install OFF CACHE BOOL "" FORCE)  # see HTTPLIB_INSTALL above

FetchContent_Declare(json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)
FetchContent_MakeAvailable(json)

# ── Third-party: miniaudio (header-only audio playback) ───────────────────────
#
# Header-only, and it dlopens its backends (ALSA / PulseAudio / JACK on Linux,
# WASAPI on Windows) at runtime — so it adds no link-time dependency and nothing
# to the package's Depends. A machine with no sound server simply fails to open
# a device, which frontend/source/audio.cpp treats as "no tones" rather than an error.

FetchContent_Declare(miniaudio
    GIT_REPOSITORY https://github.com/mackron/miniaudio.git
    GIT_TAG        0.11.21
)
FetchContent_GetProperties(miniaudio)
if(NOT miniaudio_POPULATED)
    FetchContent_Populate(miniaudio)
endif()
# miniaudio_SOURCE_DIR is now set; consumers add it to their include paths below.

# ── Frontend (Dear ImGui + GLFW + OpenGL) ─────────────────────────────────────

# 3.4 is a hard floor, not a preference. GLFW_WAYLAND_APP_ID arrived in 3.4,
# and on Wayland it is the only way a window can say which application it
# belongs to — there is no protocol for a client to hand the compositor icon
# pixels, so the app id is what resolves to the installed .desktop entry and
# therefore to the icon. Built against 3.3 the hint does not exist, the code
# guarding it compiles to nothing, and the result is a window the desktop
# reports as having no wmclass and cannot track: no icon, "unknown" in the
# dash. That is what shipped in 0.4.2 through 0.5.3, because CI installs
# libglfw3-dev and the runner's is 3.3.
#
# Asking for the version here means an older system GLFW is declined and the
# vendored 3.4 below is built instead, rather than quietly losing the feature.
#
# On macOS the vendored copy is always built, and linked statically: it is one
# less library to carry in the bundle, and a Homebrew GLFW would tie the .app
# to the build machine's macOS version.
if(NOT APPLE)
    find_package(glfw3 3.4 QUIET)
endif()
if(glfw3_FOUND)
    message(STATUS "GLFW: using system installation (${glfw3_VERSION})")
else()
    message(STATUS "GLFW: system copy missing or older than 3.4 - building 3.4")
endif()
if(NOT glfw3_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG        3.4
    )
    set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)  # see HTTPLIB_INSTALL above
    FetchContent_MakeAvailable(glfw)
endif()

find_package(OpenGL REQUIRED)

# ── MAVLink display code generation ───────────────────────────────────────────

find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(MAVLINK_XML_DIR    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/mavlink/message_definitions/v1.0")
set(MAVLINK_GEN_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/scripts/gen_mavlink_display.py")
set(MAVLINK_DISP_CPP   "${CMAKE_CURRENT_SOURCE_DIR}/frontend/components/sidebar_left/mavlink_display_generated.cpp")
set(MAVLINK_DISP_HPP   "${CMAKE_CURRENT_SOURCE_DIR}/frontend/components/sidebar_left/mavlink_display_generated.hpp")
set(MAVLINK_CAT_CPP    "${CMAKE_CURRENT_SOURCE_DIR}/frontend/components/sidebar_left/mavlink_field_catalog_generated.cpp")
set(MAVLINK_CAT_HPP    "${CMAKE_CURRENT_SOURCE_DIR}/frontend/components/sidebar_left/mavlink_field_catalog_generated.hpp")

add_custom_command(
    OUTPUT  "${MAVLINK_DISP_CPP}" "${MAVLINK_DISP_HPP}"
            "${MAVLINK_CAT_CPP}"  "${MAVLINK_CAT_HPP}"
    COMMAND "${Python3_EXECUTABLE}" "${MAVLINK_GEN_SCRIPT}"
            "${MAVLINK_XML_DIR}" "${MAVLINK_DISP_CPP}" "${MAVLINK_DISP_HPP}"
    DEPENDS "${MAVLINK_GEN_SCRIPT}"
            "${MAVLINK_XML_DIR}/ardupilotmega.xml"
            "${MAVLINK_XML_DIR}/common.xml"
            "${MAVLINK_XML_DIR}/minimal.xml"
            "${MAVLINK_XML_DIR}/standard.xml"
    COMMENT "Generating MAVLink display decoder + field catalog (${MAVLINK_XML_DIR})"
    VERBATIM
)

# ── GStreamer (video streaming) ────────────────────────────────────────────────
#
# On Unix, pkg-config is authoritative. On Windows it is not usable: the MSVC
# GStreamer SDK ships .pc files with a non-relocatable baked-in prefix, and the
# common pkg-config build for Windows (pkgconfiglite, pkg-config 0.28) predates
# --define-prefix. Locate the SDK directly there instead.
#
# Both branches populate GST_INCLUDE_DIRS / GST_LIBRARIES so consumers below are
# identical across platforms.

if(WIN32)
    # Set by the official GStreamer MSVC installer.
    if(DEFINED ENV{GSTREAMER_1_0_ROOT_MSVC_X86_64})
        file(TO_CMAKE_PATH "$ENV{GSTREAMER_1_0_ROOT_MSVC_X86_64}" _GST_ROOT_DEFAULT)
    else()
        set(_GST_ROOT_DEFAULT "C:/gstreamer/1.0/msvc_x86_64")
    endif()
    set(GSTREAMER_ROOT "${_GST_ROOT_DEFAULT}" CACHE PATH
        "Root of the GStreamer MSVC SDK")

    find_path(GST_MAIN_INCLUDE_DIR gst/gst.h
        HINTS "${GSTREAMER_ROOT}/include/gstreamer-1.0" NO_DEFAULT_PATH)
    find_path(GST_GLIB_INCLUDE_DIR glib.h
        HINTS "${GSTREAMER_ROOT}/include/glib-2.0" NO_DEFAULT_PATH)
    # glibconfig.h is generated per-ABI and lives under lib/, not include/.
    find_path(GST_GLIBCONFIG_INCLUDE_DIR glibconfig.h
        HINTS "${GSTREAMER_ROOT}/lib/glib-2.0/include" NO_DEFAULT_PATH)

    if(NOT GST_MAIN_INCLUDE_DIR OR NOT GST_GLIB_INCLUDE_DIR
       OR NOT GST_GLIBCONFIG_INCLUDE_DIR)
        message(FATAL_ERROR
            "GStreamer development headers not found under '${GSTREAMER_ROOT}'.\n"
            "Install the GStreamer MSVC *devel* package, or pass "
            "-DGSTREAMER_ROOT=<path to msvc_x86_64>.")
    endif()

    set(GST_INCLUDE_DIRS
        "${GST_MAIN_INCLUDE_DIR}"
        "${GST_GLIB_INCLUDE_DIR}"
        "${GST_GLIBCONFIG_INCLUDE_DIR}"
    )

    set(GST_LIBRARIES "")
    foreach(_gst_lib gstreamer-1.0 gstapp-1.0 gstbase-1.0 gobject-2.0 glib-2.0)
        find_library(GST_LIB_${_gst_lib} NAMES ${_gst_lib}
            HINTS "${GSTREAMER_ROOT}/lib" NO_DEFAULT_PATH)
        if(NOT GST_LIB_${_gst_lib})
            # List what is actually there, so a naming/ABI mismatch in the SDK is
            # visible from the build log instead of costing another CI round-trip.
            file(GLOB _gst_available "${GSTREAMER_ROOT}/lib/*.lib")
            string(REPLACE ";" "\n  " _gst_available "${_gst_available}")
            message(FATAL_ERROR
                "GStreamer library '${_gst_lib}' not found under "
                "'${GSTREAMER_ROOT}/lib'.\n"
                "Import libraries present:\n  ${_gst_available}")
        endif()
        list(APPEND GST_LIBRARIES "${GST_LIB_${_gst_lib}}")
    endforeach()

    message(STATUS "GStreamer: using MSVC SDK at ${GSTREAMER_ROOT}")

else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GST REQUIRED
        gstreamer-1.0
        gstreamer-app-1.0
    )
    # pkg-config's bare library names (gstreamer-1.0, glib-2.0) only link when
    # their directory is on the linker's default path. It is on Linux; on macOS
    # Homebrew's /opt/homebrew/lib is not. The full paths work on both, but are
    # only swapped in where needed.
    if(APPLE)
        set(GST_LIBRARIES ${GST_LINK_LIBRARIES})
    endif()
endif()

set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui")

add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)
target_link_libraries(imgui PUBLIC glfw OpenGL::GL)
if(APPLE)
    # Apple deprecated OpenGL in 10.14 and warns on every call; it still works,
    # up to 4.1 core, which is more than the 3.3 this app asks for.
    target_compile_definitions(imgui PUBLIC GL_SILENCE_DEPRECATION)
endif()

# ── Third-party: ImGuiFileDialog (in-app file browser) ────────────────────────
#
# Renders through the same ImGui draw list as everything else in this app —
# no native GTK/Win32 dialog, so it needs nothing added to the .deb's Depends
# and it inherits the active theme's colors the same way every other panel
# does (apply_global_theme() writes ImGuiStyle::Colors once; this just reads
# them like any other ImGui widget).
set(IGFD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/ImGuiFileDialog")

add_library(imguifiledialog STATIC
    ${IGFD_DIR}/ImGuiFileDialog.cpp
)
target_include_directories(imguifiledialog PUBLIC ${IGFD_DIR})
target_link_libraries(imguifiledialog PUBLIC imgui)
# std::filesystem over the bundled dirent.h shim — C++17 is already required
# project-wide, so this trades a vendored compatibility header for the real
# thing rather than adding a dependency.
target_compile_definitions(imguifiledialog PRIVATE USE_STD_FILESYSTEM)
if(WIN32)
    target_compile_definitions(imguifiledialog PRIVATE NOMINMAX)
endif()

# User plugins — every .cpp in plugins/ is a set of buttons on the rail beside
# the video. Globbed on purpose: a user adding a plugin should not have to edit
# this file. CONFIGURE_DEPENDS makes the build re-glob when the folder changes,
# so a new file is picked up by `cmake --build` without a manual re-configure.
file(GLOB GCS_PLUGIN_SOURCES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/plugins/*.cpp")
list(LENGTH GCS_PLUGIN_SOURCES GCS_PLUGIN_COUNT)
message(STATUS "Plugins: ${GCS_PLUGIN_COUNT} source file(s) in plugins/")

# The executable's own icon, for Explorer and the task bar. Compiled in as a
# Windows resource; the window icon is set from the embedded PNGs on every
# platform regardless.
if(WIN32)
    enable_language(RC)
    set(GCS_WIN_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/assets/freebodyproblem.rc")
endif()

# On macOS the app is a .app bundle, with its icon converted from the same PNGs
# the window and the Linux desktop entry use. iconutil ships with macOS.
if(APPLE)
    set(GCS_ICNS "${CMAKE_CURRENT_BINARY_DIR}/freebodyproblem.icns")
    set(_iconset "${CMAKE_CURRENT_BINARY_DIR}/freebodyproblem.iconset")
    set(_icon_dir "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons")
    # iconutil's names: each size at 1x, and at 2x from the next size up.
    add_custom_command(
        OUTPUT  "${GCS_ICNS}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${_iconset}"
        COMMAND ${CMAKE_COMMAND} -E copy "${_icon_dir}/freebodyproblem-16.png"  "${_iconset}/icon_16x16.png"
        COMMAND ${CMAKE_COMMAND} -E copy "${_icon_dir}/freebodyproblem-32.png"  "${_iconset}/icon_16x16@2x.png"
        COMMAND ${CMAKE_COMMAND} -E copy "${_icon_dir}/freebodyproblem-32.png"  "${_iconset}/icon_32x32.png"
        COMMAND ${CMAKE_COMMAND} -E copy "${_icon_dir}/freebodyproblem-64.png"  "${_iconset}/icon_32x32@2x.png"
        COMMAND ${CMAKE_COMMAND} -E copy "${_icon_dir}/freebodyproblem-128.png" "${_iconset}/icon_128x128.png"
        COMMAND ${CMAKE_COMMAND} -E copy "${_icon_dir}/freebodyproblem-256.png" "${_iconset}/icon_128x128@2x.png"
        COMMAND ${CMAKE_COMMAND} -E copy "${_icon_dir}/freebodyproblem-256.png" "${_iconset}/icon_256x256.png"
        COMMAND ${CMAKE_COMMAND} -E copy "${_icon_dir}/freebodyproblem-512.png" "${_iconset}/icon_256x256@2x.png"
        COMMAND ${CMAKE_COMMAND} -E copy "${_icon_dir}/freebodyproblem-512.png" "${_iconset}/icon_512x512.png"
        COMMAND iconutil -c icns -o "${GCS_ICNS}" "${_iconset}"
        DEPENDS "${_icon_dir}/freebodyproblem-512.png"
        VERBATIM)
    set_source_files_properties("${GCS_ICNS}" PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources)
    set(GCS_MAC_RESOURCES "${GCS_ICNS}")
endif()

add_executable(freebodyproblem MACOSX_BUNDLE
    ${GCS_PLUGIN_SOURCES}
    ${GCS_WIN_RESOURCES}
    ${GCS_MAC_RESOURCES}
    frontend/components/others/plugin_rail.cpp
    frontend/components/others/mcp_approval.cpp
    lib/geo.cpp
    lib/mav_result.cpp
    lib/text.cpp
    lib/tile_fetch.cpp
    backend/mavlink/sender/sender.cpp
    backend/mavlink/sender/commands.cpp
    backend/mavlink/sender/params.cpp
    backend/mavlink/sender/mission.cpp
    backend/vehicle/vehicle.cpp
    backend/vehicle/param_fetch.cpp
    backend/vehicle/identity.cpp
    backend/vehicle/kind.cpp
    backend/vehicle/stream_rates.cpp
    backend/vehicle/connect.cpp
    backend/vehicle/reports.cpp
    backend/link/link.cpp
    backend/mavlink/signing.cpp
    backend/link/fleet.cpp
    backend/link/connection.cpp
    backend/calibration/rc.cpp
    backend/input/joystick_control.cpp
    backend/calibration/accel.cpp
    backend/calibration/gyro.cpp
    backend/calibration/mag.cpp
    backend/vehicle/sensor_inventory.cpp
    backend/calibration/geodesic_grid.cpp
    backend/mavlink/timesync.cpp
    backend/input/rc_binding.cpp
    backend/vehicle/firmware_profile.cpp
    backend/mission/geofence.cpp
    backend/mission/download.cpp
    backend/mission/watch.cpp
    backend/mcp/mcp_server.cpp
    frontend/main.cpp
    frontend/source/settings.cpp
    frontend/source/param_file.cpp
    frontend/source/cache_dir.cpp
    frontend/source/terrain.cpp
    frontend/source/terrain_clearance.cpp
    frontend/source/mission_cmds.cpp
    frontend/source/audio.cpp
    frontend/source/joystick.cpp
    frontend/source/tile_source.cpp
    frontend/elements/topbar.cpp
    frontend/elements/swarm_view.cpp
    frontend/elements/sidebar_left.cpp
    frontend/components/sidebar_left/tab_connection.cpp
    frontend/components/sidebar_left/tab_flight.cpp
    frontend/components/sidebar_left/tab_params.cpp
    frontend/components/sidebar_left/params/info.cpp
    frontend/components/sidebar_left/params/editors.cpp
    frontend/components/sidebar_left/params/file.cpp
    frontend/components/sidebar_left/tab_themes.cpp
    frontend/components/sidebar_left/tab_mission.cpp
    frontend/components/sidebar_left/mission/fields.cpp
    frontend/components/sidebar_left/mission/add.cpp
    frontend/components/sidebar_left/mission/fence.cpp
    frontend/components/sidebar_left/mission/list.cpp
    frontend/components/sidebar_left/tab_mavlink.cpp
    frontend/components/sidebar_left/tab_rc.cpp
    frontend/components/sidebar_left/rc/helpers.cpp
    frontend/components/sidebar_left/rc/radio.cpp
    frontend/components/sidebar_left/rc/radio_binding.cpp
    frontend/components/sidebar_left/rc/joystick.cpp
    frontend/components/sidebar_left/rc/joystick_buttons.cpp
    frontend/components/sidebar_left/rc/joystick_virtuals.cpp
    frontend/components/sidebar_left/rc/joystick_output.cpp
    frontend/components/sidebar_left/tab_sensors.cpp
    frontend/components/sidebar_left/sensors/common.cpp
    frontend/components/sidebar_left/sensors/accel.cpp
    frontend/components/sidebar_left/sensors/gyro.cpp
    frontend/components/sidebar_left/sensors/mag.cpp
    frontend/components/sidebar_left/sensors/coverage_sphere.cpp
    frontend/components/sidebar_left/mavlink_display_generated.cpp
    frontend/components/sidebar_left/mavlink_field_catalog_generated.cpp
    frontend/components/others/sidebar_themes.cpp
    frontend/components/others/crt_overlay.cpp
    frontend/elements/center_view.cpp
    frontend/components/others/splash_screen.cpp
    frontend/source/video_player.cpp
    frontend/components/map/map_view.cpp
    frontend/components/map/map2d/input.cpp
    frontend/components/map/map2d/menus.cpp
    frontend/components/map/map2d/chrome.cpp
    frontend/components/map/map2d/fences.cpp
    frontend/components/map/map2d/layers.cpp
    frontend/components/map/tiles.cpp
    frontend/components/map/tracks.cpp
    frontend/components/map/symbols.cpp
    frontend/components/map/overlay.cpp
    frontend/components/map/map_view_3d.cpp
    frontend/components/map/map3d/gl.cpp
    frontend/components/map/map3d/airframes.cpp
    frontend/components/map/map3d/heights.cpp
    frontend/components/map/map3d/craft.cpp
    frontend/components/map/map3d/scene.cpp
    frontend/components/map/map3d/overlays.cpp
    frontend/components/map/map3d/pick.cpp
    frontend/components/map/map3d/bands.cpp
    frontend/components/map/map3d/edit.cpp
    frontend/source/gl3.cpp
    frontend/source/stb_impl.cpp
    frontend/source/app_icon.cpp
    frontend/elements/sidebar_right.cpp
    frontend/components/sidebar_right/attitude_ball.cpp
    frontend/components/sidebar_right/hud_parts.cpp
    frontend/components/sidebar_right/hud.cpp
    frontend/components/sidebar_right/event_log.cpp
    frontend/components/sidebar_right/event_log_view.cpp
    frontend/components/sidebar_right/telemetry_grid.cpp
    frontend/components/others/ui_kit.cpp
    frontend/components/others/file_dialog.cpp
    frontend/source/vehicle_ui_state.cpp
    frontend/source/param_meta.cpp
    frontend/source/param_meta_arducopter.cpp
    frontend/source/param_meta_arduplane.cpp
    frontend/source/param_meta_px4.cpp
    backend/mavlink/parser/parser.cpp
    backend/mavlink/parser/telemetry.cpp
    backend/mavlink/parser/params.cpp
    backend/mavlink/parser/modes.cpp
    backend/mavlink/parser/commands.cpp
    backend/mavlink/parser/mission.cpp
)
# Project includes are rooted at the repository: #include "backend/vehicle/vehicle.hpp",
# never a ../ path, so a file can move without its includes being rewritten.
target_include_directories(freebodyproblem PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${stb_SOURCE_DIR}
    ${miniaudio_SOURCE_DIR}
    ${GST_INCLUDE_DIRS}
)
# miniaudio dlopens its backend and uses threads, so it needs dl and pthread
# explicitly rather than by luck of what another dependency happened to pull in.
find_package(Threads REQUIRED)

target_link_libraries(freebodyproblem PRIVATE
    mavlink_headers
    imgui
    imguifiledialog
    httplib::httplib
    nlohmann_json::nlohmann_json
    OpenSSL::SSL
    OpenSSL::Crypto
    Threads::Threads
    ${CMAKE_DL_LIBS}
    ${GST_LIBRARIES}
)
target_compile_definitions(freebodyproblem PRIVATE
    # Streams a ground station can hear at once, per sysid, compid and link. New ones
    # past this are refused, and a swarm has many.
    MAVLINK_MAX_SIGNING_STREAMS=256
    GCS_VERSION="${PROJECT_VERSION}"
    GCS_FONT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/third_party/fonts/GeistMono-Regular.ttf"
    GCS_FONT_BOLD_PATH="${CMAKE_CURRENT_SOURCE_DIR}/third_party/fonts/GeistMono-Bold.ttf"
    GCS_FONT_FALLBACK_PATH="${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/misc/fonts/Roboto-Medium.ttf"
    GCS_ICON_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets/icons"
)
if(WIN32)
    # setupapi: serial port enumeration in backend/link/connection.cpp
    target_link_libraries(freebodyproblem PRIVATE ws2_32 setupapi)
    target_compile_definitions(freebodyproblem PRIVATE
        # <windows.h> defines min/max as macros, which breaks every std::min /
        # std::max call site that sees it transitively.
        # (WIN32_LEAN_AND_MEAN is deliberately not set here — the two files that
        # include <windows.h> define it themselves, and defining it on the
        # command line as well would be a C4005 mismatched redefinition.)
        NOMINMAX
        # Silences C4996 on strncpy/getenv. The POSIX spellings are correct and
        # portable; the _s variants are MSVC-only.
        _CRT_SECURE_NO_WARNINGS
        # M_PI and friends: <cmath> leaves them out on MSVC without this, and
        # the drawing code uses them everywhere.
        _USE_MATH_DEFINES
    )
endif()

if(APPLE)
    # The title bar's colour (frontend/source/macos_window.mm). Objective-C++
    # by flag rather than enable_language(OBJCXX), which needs CMake 3.16.
    target_sources(freebodyproblem PRIVATE frontend/source/macos_window.mm)
    set_source_files_properties(frontend/source/macos_window.mm PROPERTIES
        COMPILE_FLAGS "-x objective-c++ -fobjc-arc")
    # IOKit and CoreFoundation: serial port enumeration in backend/link/connection.cpp.
    # Cocoa: the title bar.
    target_link_libraries(freebodyproblem PRIVATE
        "-framework IOKit" "-framework CoreFoundation" "-framework Cocoa")
    set_target_properties(freebodyproblem PROPERTIES
        # Where the bundled libraries are looked up once installed. The build
        # tree keeps CMake's build rpath, pointing at the originals.
        INSTALL_RPATH "@executable_path/../Frameworks"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/assets/Info.plist.in"
        MACOSX_BUNDLE_BUNDLE_NAME         "freebodyproblem"
        MACOSX_BUNDLE_GUI_IDENTIFIER      "tech.freebodyproblem.gcs"
        MACOSX_BUNDLE_ICON_FILE           "freebodyproblem.icns"
        MACOSX_BUNDLE_BUNDLE_VERSION      "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}")
endif()

# ── MCP stdio adapter ─────────────────────────────────────────────────────────
#
# freebodyproblem-mcp: a small program an MCP host launches over stdio, which
# forwards each message to the GCS's built-in MCP server on 127.0.0.1. Kept
# lean on purpose — no GUI, no video, and httplib's header alone rather than
# the httplib::httplib target, which would drag OpenSSL in for a loopback-only
# HTTP client.
add_executable(freebodyproblem-mcp adapter/freebodyproblem_mcp.cpp)
target_include_directories(freebodyproblem-mcp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${httplib_SOURCE_DIR})
target_link_libraries(freebodyproblem-mcp PRIVATE nlohmann_json::nlohmann_json Threads::Threads)
if(WIN32)
    target_link_libraries(freebodyproblem-mcp PRIVATE ws2_32)
    target_compile_definitions(freebodyproblem-mcp PRIVATE NOMINMAX _CRT_SECURE_NO_WARNINGS)
endif()
if(APPLE)
    # The GCS looks for the adapter beside its own executable, which on macOS
    # is inside the bundle — in the build tree as much as in the installed app.
    set_target_properties(freebodyproblem-mcp PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/freebodyproblem.app/Contents/MacOS")
endif()

# ── Install ───────────────────────────────────────────────────────────────────
#
# Layout is prefix-relative and matches what find_asset() in frontend/main.cpp
# looks for: the binary in bin/, runtime assets in share/freebodyproblem/.
# GCS_FONT_PATH remains a build-time absolute path, but it is now only the last
# resort for running straight out of the build tree.

include(GNUInstallDirs)

if(APPLE)
    # Everything inside the bundle, so dragging the .app anywhere moves it all:
    # the adapter beside the GCS in Contents/MacOS, assets in Contents/Resources
    # (the "../Resources" candidate in find_asset()).
    set(GCS_DATA_DIR "freebodyproblem.app/Contents/Resources")
    install(TARGETS freebodyproblem BUNDLE DESTINATION .)
    install(TARGETS freebodyproblem-mcp
            RUNTIME DESTINATION freebodyproblem.app/Contents/MacOS)
else()
    set(GCS_DATA_DIR "share/freebodyproblem")
    install(TARGETS freebodyproblem freebodyproblem-mcp
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

install(FILES
            "${CMAKE_CURRENT_SOURCE_DIR}/third_party/fonts/GeistMono-Regular.ttf"
            "${CMAKE_CURRENT_SOURCE_DIR}/third_party/fonts/GeistMono-Bold.ttf"
            "${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/misc/fonts/Roboto-Medium.ttf"
        DESTINATION ${GCS_DATA_DIR}/fonts)

# The window icon, found at runtime the same way the fonts are.
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/"
        DESTINATION ${GCS_DATA_DIR}/icons
        FILES_MATCHING PATTERN "freebodyproblem-*.png")

# Desktop integration — what a desktop environment reads off disk rather than
# from the running window: the menu entry, and the icon shown beside it.
if(UNIX AND NOT APPLE)
    foreach(_icon_size 16 24 32 48 64 128 256 512)
        install(FILES
                    "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/freebodyproblem-${_icon_size}.png"
                DESTINATION
                    "${CMAKE_INSTALL_DATADIR}/icons/hicolor/${_icon_size}x${_icon_size}/apps"
                RENAME freebodyproblem.png)
    endforeach()

    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/assets/freebodyproblem.desktop"
            DESTINATION "${CMAKE_INSTALL_DATADIR}/applications")
endif()

if(APPLE)
    # Runs after everything above has been installed into the bundle: copies
    # GStreamer.framework and any other non-system library in, points the
    # executable at the copies, and re-signs. CPack runs it too, so the disk
    # image carries the result.
    if(GCS_BUNDLE_GSTREAMER)
        set(_bundle_fw "${GSTREAMER_FRAMEWORK}")
    else()
        set(_bundle_fw "")
    endif()

    # The GStreamer plugins the bundle keeps; the rest of the framework's are
    # left out, with every library only they needed. Enough for the two
    # pipelines in frontend/source/video_player.cpp and what decodebin picks
    # for an RTSP camera's H.264, H.265 or MJPEG. A pipeline that gains an
    # element needs its plugin here, or it plays in a build-tree run and fails
    # in the installed app. ("gst-inspect-1.0 <element>" names the plugin.)
    set(GCS_GST_PLUGINS
        coreelements        # queue, capsfilter, typefind
        app                 # appsink
        playback            # decodebin
        typefindfunctions   # decodebin's stream type detection
        videoconvertscale   # videoconvert
        udp                 # udpsrc
        rtsp                # rtspsrc
        rtp                 # rtph264depay and the other depayloaders
        rtpmanager          # rtpbin and its jitter buffer, inside rtspsrc
        videoparsersbad     # h264parse, h265parse
        libav               # avdec_h264, avdec_h265
        applemedia          # vtdec: VideoToolbox, the hardware decoder
        jpeg                # jpegdec, for MJPEG cameras
    )

    # One architecture, the one the executable is built for. The framework is
    # universal; carrying the other half of it would double the bundle for
    # code nothing can load. A universal build (two CMAKE_OSX_ARCHITECTURES)
    # keeps both.
    list(LENGTH CMAKE_OSX_ARCHITECTURES _n_arch)
    if(_n_arch EQUAL 1)
        set(_bundle_arch "${CMAKE_OSX_ARCHITECTURES}")
    elseif(_n_arch EQUAL 0)
        set(_bundle_arch "${CMAKE_HOST_SYSTEM_PROCESSOR}")
    else()
        set(_bundle_arch "")
    endif()

    install(CODE "
        set(GCS_APP \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/freebodyproblem.app\")
        set(GCS_GST_FRAMEWORK \"${_bundle_fw}\")
        set(GCS_GST_PLUGINS \"${GCS_GST_PLUGINS}\")
        set(GCS_ARCH \"${_bundle_arch}\")
    ")
    install(SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/cmake/macos_bundle.cmake")
endif()

if(WIN32)
    # These roots arrive on the command line as native paths ("C:\Program
    # Files\OpenSSL"). Globbed results keep the backslashes, and CMake then
    # fails to parse its own generated cmake_install.cmake ("Invalid character
    # escape '\P'"), so normalise to forward slashes before any use below.
    #
    # Deliberately not file(TO_CMAKE_PATH): that treats its input as a search
    # path, and on a Unix host it splits the drive letter on ':' and yields
    # "C;/Program Files/...". A plain replace does the one thing wanted and
    # behaves identically on every host.
    string(REPLACE "\\" "/" _gst_root "${GSTREAMER_ROOT}")
    string(REPLACE "\\" "/" _ssl_root "${OPENSSL_ROOT_DIR}")

    # Windows has no system package manager to satisfy these, so the installer
    # carries the full runtime. GStreamer locates its plugins relative to the
    # directory holding gstreamer-1.0-0.dll, so the SDK's bin/ + lib/ split has
    # to be preserved exactly.
    install(DIRECTORY "${_gst_root}/bin/"
            DESTINATION ${CMAKE_INSTALL_BINDIR}
            FILES_MATCHING PATTERN "*.dll")

    install(DIRECTORY "${_gst_root}/lib/gstreamer-1.0/"
            DESTINATION lib/gstreamer-1.0
            FILES_MATCHING PATTERN "*.dll")

    # Only the two runtime libraries. The rest of OpenSSL's bin/ is optional
    # providers and test engines — ossltest, p_test, dasync, padlock — which
    # have no business in a release installer.
    file(GLOB _openssl_dlls
        "${_ssl_root}/bin/libssl-*.dll"
        "${_ssl_root}/bin/libcrypto-*.dll")
    if(_openssl_dlls)
        install(FILES ${_openssl_dlls} DESTINATION ${CMAKE_INSTALL_BINDIR})
    else()
        # Not fatal: a static OpenSSL build legitimately has no DLLs to ship.
        message(WARNING
            "No OpenSSL runtime DLLs under ${_ssl_root}/bin — if OpenSSL was "
            "linked dynamically, the installed app will fail to start.")
    endif()

    # msvcp140.dll and friends, so the installer works on a machine with no
    # Visual Studio redistributable present.
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ${CMAKE_INSTALL_BINDIR})
    include(InstallRequiredSystemLibraries)
endif()

# ── Packaging ─────────────────────────────────────────────────────────────────

set(CPACK_PACKAGE_NAME              "freebodyproblem")
set(CPACK_PACKAGE_VENDOR            "Azhar Tanweer")
set(CPACK_PACKAGE_VERSION           "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "Ground control station for ArduPilot and PX4 autopilots")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "freebodyproblem")
set(CPACK_RESOURCE_FILE_LICENSE     "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_CONTACT           "azhar.tanweer404@gmail.com")

if(WIN32)
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_NSIS_PACKAGE_NAME     "freebodyproblem")
    set(CPACK_NSIS_DISPLAY_NAME     "freebodyproblem")
    set(CPACK_NSIS_INSTALLED_ICON_NAME "${CMAKE_INSTALL_BINDIR}/freebodyproblem.exe")
    set(CPACK_NSIS_MUI_ICON   "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/freebodyproblem.ico")
    set(CPACK_NSIS_MUI_UNIICON "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/freebodyproblem.ico")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_MODIFY_PATH OFF)
    set(CPACK_PACKAGE_EXECUTABLES   "freebodyproblem;freebodyproblem")
    set(CPACK_NSIS_CREATE_ICONS_EXTRA
        "CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\freebodyproblem.lnk' '$INSTDIR\\\\${CMAKE_INSTALL_BINDIR}\\\\freebodyproblem.exe'")
    set(CPACK_NSIS_DELETE_ICONS_EXTRA
        "Delete '$SMPROGRAMS\\\\$START_MENU\\\\freebodyproblem.lnk'")

elseif(APPLE)
    # A disk image holding the .app and a link to /Applications to drag it to.
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_DMG_VOLUME_NAME "freebodyproblem")
    set(CPACK_PACKAGE_ICON "${GCS_ICNS}")
    set(CPACK_DMG_FORMAT "UDZO")
    # No licence click-through before the image mounts: an Apache licence does
    # not need to be agreed to. (OFF is CMake's default; stated so it stays so.)
    set(CPACK_DMG_SLA_USE_RESOURCE_FILE_LICENSE OFF)

elseif(UNIX)
    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Azhar Tanweer <azhar.tanweer404@gmail.com>")
    set(CPACK_DEBIAN_PACKAGE_SECTION    "science")
    set(CPACK_DEBIAN_FILE_NAME          "DEB-DEFAULT")
    # Derive Depends from the actual ELF linkage rather than a hand-kept list,
    # which silently rots as dependencies change. Needs dpkg-dev at package time.
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS  ON)
    # GStreamer plugins and the audio backends are dlopen'd at runtime, so
    # shlibdeps cannot see them: the binary has no NEEDED entry for any of them.
    # Recommends rather than Depends because the application degrades rather
    # than fails without them — no plugins means no video, no sound library
    # means no cue tones, and neither stops a flight.
    #
    # libasound2 was renamed libasound2t64 by Ubuntu's 64-bit time_t
    # transition; the alternation covers both sides of it.
    set(CPACK_DEBIAN_PACKAGE_RECOMMENDS
        "gstreamer1.0-plugins-base, gstreamer1.0-plugins-good, gstreamer1.0-plugins-bad, gstreamer1.0-libav, libasound2t64 | libasound2, libpulse0")
endif()

include(CPack)
