cmake_minimum_required(VERSION 3.13)
project(freebodyproblem CXX)

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

FetchContent_Declare(httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG        v0.18.3
)
FetchContent_MakeAvailable(httplib)

# ── Third-party: nlohmann/json (header-only JSON) ─────────────────────────────

FetchContent_Declare(json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)
FetchContent_MakeAvailable(json)

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
find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED
    gstreamer-1.0
    gstreamer-app-1.0
)

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

add_executable(freebodyproblem
    backend/mavlink_sender.cpp
    backend/connection.cpp
    frontend/main.cpp
    frontend/settings.cpp
    frontend/widgets/topbar.cpp
    frontend/widgets/sidebar_left.cpp
    frontend/widgets/sidebar_left/tab_connection.cpp
    frontend/widgets/sidebar_left/tab_flight.cpp
    frontend/widgets/sidebar_left/tab_params.cpp
    frontend/widgets/sidebar_left/tab_themes.cpp
    frontend/widgets/sidebar_left/tab_mission.cpp
    frontend/widgets/sidebar_left/tab_mavlink.cpp
    frontend/widgets/sidebar_left/mavlink_display_generated.cpp
    frontend/widgets/sidebar_left/mavlink_field_catalog_generated.cpp
    frontend/widgets/sidebar_themes.cpp
    frontend/widgets/center_view.cpp
    frontend/widgets/splash_screen.cpp
    frontend/widgets/video_player.cpp
    frontend/widgets/map_view.cpp
    frontend/widgets/stb_impl.cpp
    frontend/widgets/sidebar_right.cpp
    backend/mavlink_parser.cpp
)
target_include_directories(freebodyproblem PRIVATE
    ${stb_SOURCE_DIR}
    ${GST_INCLUDE_DIRS}
)
target_link_libraries(freebodyproblem PRIVATE
    mavlink_headers
    imgui
    httplib::httplib
    nlohmann_json::nlohmann_json
    OpenSSL::SSL
    OpenSSL::Crypto
    ${GST_LIBRARIES}
)
target_compile_definitions(freebodyproblem PRIVATE
    GCS_FONT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/misc/fonts/Roboto-Medium.ttf"
)
if(WIN32)
    target_link_libraries(freebodyproblem PRIVATE ws2_32)
endif()
