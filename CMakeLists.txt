cmake_minimum_required(VERSION 3.13)
project(freebodyproblem VERSION 0.3.1 LANGUAGES CXX)

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

# ── Third-party: cpp-httplib (header-only HTTPS client) ───────────────────────
# Verify the tag at https://github.com/yhirose/cpp-httplib/releases if the
# build fails to fetch.

find_package(OpenSSL REQUIRED)

set(HTTPLIB_REQUIRE_OPENSSL ON  CACHE BOOL "" FORCE)
set(HTTPLIB_USE_OPENSSL_IF_AVAILABLE ON CACHE BOOL "" FORCE)
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
# a device, which frontend/audio.cpp treats as "no tones" rather than an error.

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

find_package(glfw3 QUIET)
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
set(MAVLINK_DISP_CPP   "${CMAKE_CURRENT_SOURCE_DIR}/frontend/widgets/sidebar_left/mavlink_display_generated.cpp")
set(MAVLINK_DISP_HPP   "${CMAKE_CURRENT_SOURCE_DIR}/frontend/widgets/sidebar_left/mavlink_display_generated.hpp")
set(MAVLINK_CAT_CPP    "${CMAKE_CURRENT_SOURCE_DIR}/frontend/widgets/sidebar_left/mavlink_field_catalog_generated.cpp")
set(MAVLINK_CAT_HPP    "${CMAKE_CURRENT_SOURCE_DIR}/frontend/widgets/sidebar_left/mavlink_field_catalog_generated.hpp")

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

# User plugins — every .cpp in plugins/ is a set of buttons on the rail beside
# the video. Globbed on purpose: a user adding a plugin should not have to edit
# this file. CONFIGURE_DEPENDS makes the build re-glob when the folder changes,
# so a new file is picked up by `cmake --build` without a manual re-configure.
file(GLOB GCS_PLUGIN_SOURCES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/plugins/*.cpp")
list(LENGTH GCS_PLUGIN_SOURCES GCS_PLUGIN_COUNT)
message(STATUS "Plugins: ${GCS_PLUGIN_COUNT} source file(s) in plugins/")

add_executable(freebodyproblem
    ${GCS_PLUGIN_SOURCES}
    frontend/widgets/plugin_rail.cpp
    backend/mavlink_sender.cpp
    backend/connection.cpp
    backend/rc_calibration.cpp
    backend/rc_binding.cpp
    frontend/main.cpp
    frontend/settings.cpp
    frontend/param_file.cpp
    frontend/audio.cpp
    frontend/widgets/topbar.cpp
    frontend/widgets/sidebar_left.cpp
    frontend/widgets/sidebar_left/tab_connection.cpp
    frontend/widgets/sidebar_left/tab_flight.cpp
    frontend/widgets/sidebar_left/tab_params.cpp
    frontend/widgets/sidebar_left/tab_themes.cpp
    frontend/widgets/sidebar_left/tab_mission.cpp
    frontend/widgets/sidebar_left/tab_mavlink.cpp
    frontend/widgets/sidebar_left/tab_rc.cpp
    frontend/widgets/sidebar_left/mavlink_display_generated.cpp
    frontend/widgets/sidebar_left/mavlink_field_catalog_generated.cpp
    frontend/widgets/sidebar_themes.cpp
    frontend/widgets/center_view.cpp
    frontend/widgets/splash_screen.cpp
    frontend/widgets/video_player.cpp
    frontend/widgets/map_view.cpp
    frontend/widgets/stb_impl.cpp
    frontend/widgets/sidebar_right.cpp
    frontend/widgets/ui_kit.cpp
    backend/mavlink_parser.cpp
)
target_include_directories(freebodyproblem PRIVATE
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
    httplib::httplib
    nlohmann_json::nlohmann_json
    OpenSSL::SSL
    OpenSSL::Crypto
    Threads::Threads
    ${CMAKE_DL_LIBS}
    ${GST_LIBRARIES}
)
target_compile_definitions(freebodyproblem PRIVATE
    GCS_FONT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/third_party/fonts/GeistMono-Regular.ttf"
    GCS_FONT_FALLBACK_PATH="${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/misc/fonts/Roboto-Medium.ttf"
)
if(WIN32)
    # setupapi: serial port enumeration in backend/connection.cpp
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
    )
endif()

# ── Install ───────────────────────────────────────────────────────────────────
#
# Layout is prefix-relative and matches what find_asset() in frontend/main.cpp
# looks for: the binary in bin/, runtime assets in share/freebodyproblem/.
# GCS_FONT_PATH remains a build-time absolute path, but it is now only the last
# resort for running straight out of the build tree.

include(GNUInstallDirs)

install(TARGETS freebodyproblem
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES
            "${CMAKE_CURRENT_SOURCE_DIR}/third_party/fonts/GeistMono-Regular.ttf"
            "${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/misc/fonts/Roboto-Medium.ttf"
        DESTINATION share/freebodyproblem/fonts)

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
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_MODIFY_PATH OFF)
    set(CPACK_PACKAGE_EXECUTABLES   "freebodyproblem;freebodyproblem")
    set(CPACK_NSIS_CREATE_ICONS_EXTRA
        "CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\freebodyproblem.lnk' '$INSTDIR\\\\${CMAKE_INSTALL_BINDIR}\\\\freebodyproblem.exe'")
    set(CPACK_NSIS_DELETE_ICONS_EXTRA
        "Delete '$SMPROGRAMS\\\\$START_MENU\\\\freebodyproblem.lnk'")

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
