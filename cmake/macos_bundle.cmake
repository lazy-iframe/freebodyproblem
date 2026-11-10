# Makes freebodyproblem.app self-contained. Run at install time (and so by
# CPack) after the bundle's own files are in place; see CMakeLists.txt.
#
# In:  GCS_APP            the installed .app
#      GCS_GST_FRAMEWORK  GStreamer.framework to copy in, or empty for none
#      GCS_GST_PLUGINS    the plugins to keep, by name ("coreelements", ...)
#      GCS_ARCH           the one architecture to keep, or empty for all
#
# The bundle the Windows installer is to the GStreamer MSVC runtime, this is to
# the framework: a copy of the whole thing, so plugins, the plugin scanner and
# GIO's modules all come along and keep the layout GStreamer finds them by.
# Anything else the executable links from outside the system — OpenSSL from
# Homebrew, if the framework did not provide it — is copied beside it in
# Contents/Frameworks and the references rewritten. Last, the bundle is
# re-signed ad hoc: every edit above voids the linker's signature, and Apple
# Silicon refuses to run unsigned code at all.

if(NOT EXISTS "${GCS_APP}/Contents/MacOS/freebodyproblem")
    message(FATAL_ERROR "macos_bundle: no executable in ${GCS_APP}")
endif()

set(_exe "${GCS_APP}/Contents/MacOS/freebodyproblem")
set(_fw_dir "${GCS_APP}/Contents/Frameworks")
file(MAKE_DIRECTORY "${_fw_dir}")

function(_run)
    execute_process(COMMAND ${ARGN} RESULT_VARIABLE rc
                    OUTPUT_VARIABLE out ERROR_VARIABLE err)
    if(NOT rc EQUAL 0)
        string(REPLACE ";" " " cmd "${ARGN}")
        message(FATAL_ERROR "macos_bundle: '${cmd}' failed (${rc}):\n${out}${err}")
    endif()
endfunction()

# The install names a Mach-O file links against, own id excluded.
function(_deps file out_var)
    execute_process(COMMAND otool -L "${file}" OUTPUT_VARIABLE out)
    execute_process(COMMAND otool -D "${file}" OUTPUT_VARIABLE id_out)
    string(REGEX REPLACE "^[^\n]*\n" "" id_out "${id_out}")
    string(STRIP "${id_out}" self_id)

    string(REPLACE "\n" ";" lines "${out}")
    list(REMOVE_AT lines 0)                      # "<file>:"
    set(deps "")
    foreach(l IN LISTS lines)
        string(STRIP "${l}" l)
        # Blank, or the "<file> (architecture arm64):" header otool prints
        # for each slice of a universal binary.
        if(l STREQUAL "" OR l MATCHES ":$")
            continue()
        endif()
        string(REGEX REPLACE " \\(compatibility.*$" "" d "${l}")
        if(NOT d STREQUAL self_id)
            list(APPEND deps "${d}")
        endif()
    endforeach()
    set(${out_var} "${deps}" PARENT_SCOPE)
endfunction()

function(_rpaths file out_var)
    execute_process(COMMAND otool -l "${file}" OUTPUT_VARIABLE out)
    string(REGEX MATCHALL "path [^\n]* \\(offset" hits "${out}")
    set(r "")
    foreach(h IN LISTS hits)
        string(REGEX REPLACE "^path (.*) \\(offset$" "\\1" p "${h}")
        list(APPEND r "${p}")
    endforeach()
    set(${out_var} "${r}" PARENT_SCOPE)
endfunction()

# ── 1. GStreamer.framework ────────────────────────────────────────────────────
#
# rsync rather than file(COPY): a framework is mostly symlinks (Versions/Current,
# the top-level aliases), and they have to stay symlinks. What is left out is
# for building against it — headers, static and libtool archives, pkg-config
# files, documentation — and what this app never loads: the command-line
# tools, the Python bindings, translations and the rest of share/. Step 3
# trims the plugins and libraries further.
if(GCS_GST_FRAMEWORK)
    message(STATUS "macos_bundle: copying ${GCS_GST_FRAMEWORK}")
    _run(rsync -a --delete
         --exclude=Headers --exclude=include
         --exclude=*.a --exclude=*.la
         --exclude=pkgconfig --exclude=aclocal
         --exclude=gtk-doc --exclude=doc --exclude=man
         --exclude=Versions/1.0/bin
         --exclude=Versions/1.0/share
         --exclude=Versions/1.0/lib/python*
         --exclude=Versions/1.0/lib/gio
         --exclude=Versions/1.0/lib/girepository-1.0
         --exclude=Versions/1.0/lib/glib-2.0
         --exclude=Versions/1.0/lib/cmake
         --exclude=Commands
         "${GCS_GST_FRAMEWORK}/" "${_fw_dir}/GStreamer.framework/")
endif()

# ── 2. The executable's references ────────────────────────────────────────────

set(_system_re "^(/System/|/usr/lib/)")
set(_gst_prefix "${GCS_GST_FRAMEWORK}/")
set(_queue "${_exe}")
set(_done "")

while(_queue)
    list(GET _queue 0 _file)
    list(REMOVE_AT _queue 0)
    list(APPEND _done "${_file}")

    _deps("${_file}" _file_deps)
    foreach(_d IN LISTS _file_deps)
        if(_d MATCHES "${_system_re}" OR _d MATCHES "^@")
            continue()   # the OS's own, or already bundle-relative
        endif()

        string(FIND "${_d}" "${_gst_prefix}" _at)
        if(GCS_GST_FRAMEWORK AND _at EQUAL 0)
            # Into the framework copied above, at the same place inside it.
            string(LENGTH "${_gst_prefix}" _n)
            string(SUBSTRING "${_d}" ${_n} -1 _rel)
            _run(install_name_tool -change "${_d}"
                 "@executable_path/../Frameworks/GStreamer.framework/${_rel}" "${_file}")
            continue()
        endif()

        # Anything else from outside the system: copied in flat, renamed to
        # its bundle path, and its own references followed in turn.
        get_filename_component(_real "${_d}" REALPATH)
        get_filename_component(_name "${_d}" NAME)
        set(_copy "${_fw_dir}/${_name}")
        if(NOT EXISTS "${_copy}")
            message(STATUS "macos_bundle: bundling ${_d}")
            file(COPY "${_real}" DESTINATION "${_fw_dir}")
            get_filename_component(_real_name "${_real}" NAME)
            if(NOT _real_name STREQUAL _name)
                file(RENAME "${_fw_dir}/${_real_name}" "${_copy}")
            endif()
            _run(chmod u+w "${_copy}")
            _run(install_name_tool -id "@executable_path/../Frameworks/${_name}" "${_copy}")
            list(APPEND _queue "${_copy}")
        endif()
        _run(install_name_tool -change "${_d}"
             "@executable_path/../Frameworks/${_name}" "${_file}")
    endforeach()
endwhile()

# @rpath references — how the framework's libraries name themselves — resolve
# through the executable's rpaths. Each is checked against the copied framework
# and the rpath that finds it added; absolute rpaths left over from the build
# machine are dropped, so nothing outside the bundle is ever searched.
_rpaths("${_exe}" _have)
foreach(_r IN LISTS _have)
    if(_r MATCHES "^/")
        _run(install_name_tool -delete_rpath "${_r}" "${_exe}")
        list(REMOVE_ITEM _have "${_r}")
    endif()
endforeach()

_deps("${_exe}" _exe_deps)
foreach(_d IN LISTS _exe_deps)
    if(NOT _d MATCHES "^@rpath/")
        continue()
    endif()
    string(REGEX REPLACE "^@rpath/" "" _rel "${_d}")
    set(_found "")
    foreach(_base "" "GStreamer.framework" "GStreamer.framework/Versions/1.0"
                  "GStreamer.framework/Versions/1.0/lib")
        if(_base STREQUAL "")
            set(_dir "${_fw_dir}")
            set(_rp  "@executable_path/../Frameworks")
        else()
            set(_dir "${_fw_dir}/${_base}")
            set(_rp  "@executable_path/../Frameworks/${_base}")
        endif()
        if(EXISTS "${_dir}/${_rel}")
            set(_found "${_rp}")
            break()
        endif()
    endforeach()
    if(NOT _found)
        message(FATAL_ERROR "macos_bundle: ${_d} is not in the bundle")
    endif()
    if(NOT _found IN_LIST _have)
        _run(install_name_tool -add_rpath "${_found}" "${_exe}")
        list(APPEND _have "${_found}")
    endif()
endforeach()

# ── 3. Plugins and the libraries they need ────────────────────────────────────
#
# The framework is the whole of GStreamer — every plugin set, every codec
# library behind them — and almost none of it is used: the video panel builds
# two pipelines (frontend/source/video_player.cpp). Only the plugins named in
# GCS_GST_PLUGINS are kept, then only the libraries something kept still links
# to, found by following references out from the executable, those plugins
# and the plugin scanner. Everything else in lib/ goes.
if(GCS_GST_FRAMEWORK)
    set(_root    "${_fw_dir}/GStreamer.framework/Versions/1.0")
    set(_lib     "${_root}/lib")
    set(_plugdir "${_lib}/gstreamer-1.0")

    # Plugins: keep the list, fail on a name that is not there — a typo here
    # would otherwise surface as a stream that will not play.
    set(_roots "${_exe}")
    foreach(_p IN LISTS GCS_GST_PLUGINS)
        set(_f "${_plugdir}/libgst${_p}.dylib")
        if(NOT EXISTS "${_f}")
            message(FATAL_ERROR "macos_bundle: plugin '${_p}' is not in ${GCS_GST_FRAMEWORK}")
        endif()
        list(APPEND _roots "${_f}")
    endforeach()
    file(GLOB _plugins LIST_DIRECTORIES true "${_plugdir}/*")
    set(_n_plug 0)
    foreach(_f IN LISTS _plugins)
        get_filename_component(_name "${_f}" NAME)
        string(REGEX REPLACE "^libgst(.*)\\.dylib$" "\\1" _p "${_name}")
        if(NOT _p IN_LIST GCS_GST_PLUGINS OR _p STREQUAL _name)
            file(REMOVE_RECURSE "${_f}")
            math(EXPR _n_plug "${_n_plug} + 1")
        endif()
    endforeach()

    # The plugin scanner is the one helper GStreamer runs; the rest of
    # libexec (the PTP helper, gst-validate's tools) goes.
    set(_scanner "${_root}/libexec/gstreamer-1.0/gst-plugin-scanner")
    if(EXISTS "${_scanner}")
        list(APPEND _roots "${_scanner}")
        file(GLOB _libexec LIST_DIRECTORIES true "${_root}/libexec/*" "${_root}/libexec/gstreamer-1.0/*")
        foreach(_f IN LISTS _libexec)
            if(NOT _f STREQUAL "${_root}/libexec/gstreamer-1.0" AND NOT _f STREQUAL _scanner)
                file(REMOVE_RECURSE "${_f}")
            endif()
        endforeach()
    endif()

    # Libraries: everything reachable from the roots, by file name — the
    # framework names its libraries @rpath/libX.dylib, @rpath/lib/libX.dylib or
    # by absolute path depending on the library, but always by the file name
    # that is in lib/. A symlink's target is kept along with it.
    set(_keep "")
    set(_seen "")
    set(_work "${_roots}")
    while(_work)
        list(GET _work 0 _f)
        list(REMOVE_AT _work 0)
        _deps("${_f}" _fd)
        foreach(_d IN LISTS _fd)
            if(_d MATCHES "${_system_re}")
                continue()
            endif()
            get_filename_component(_b "${_d}" NAME)
            if(NOT EXISTS "${_lib}/${_b}")
                continue()   # outside the framework: step 2's business
            endif()
            get_filename_component(_real "${_lib}/${_b}" REALPATH)
            get_filename_component(_rb "${_real}" NAME)
            list(APPEND _keep "${_b}" "${_rb}")
            if(NOT _real IN_LIST _seen)
                list(APPEND _seen "${_real}")
                list(APPEND _work "${_real}")
            endif()
        endforeach()
    endwhile()
    list(REMOVE_DUPLICATES _keep)

    file(GLOB _libs LIST_DIRECTORIES true "${_lib}/*")
    set(_n_lib 0)
    foreach(_f IN LISTS _libs)
        get_filename_component(_name "${_f}" NAME)
        if(_name STREQUAL "gstreamer-1.0" OR _name IN_LIST _keep)
            continue()
        endif()
        file(REMOVE_RECURSE "${_f}")
        math(EXPR _n_lib "${_n_lib} + 1")
    endforeach()
    list(LENGTH _keep _n_keep)
    message(STATUS "macos_bundle: removed ${_n_plug} plugins and ${_n_lib} "
                   "library files; kept ${_n_keep} library names")
endif()

# ── 4. One architecture ───────────────────────────────────────────────────────
#
# The framework is universal — every library carries an x86_64 and an arm64
# copy of its code — while the executable is built for one. The other half of
# every file in the bundle is dead weight, so it is cut out with lipo.
if(GCS_ARCH)
    file(GLOB_RECURSE _all LIST_DIRECTORIES false "${_fw_dir}/*")
    set(_n_thin 0)
    foreach(_f IN LISTS _all)
        if(IS_SYMLINK "${_f}")
            continue()
        endif()
        execute_process(COMMAND lipo -archs "${_f}"
                        OUTPUT_VARIABLE _archs ERROR_QUIET RESULT_VARIABLE _rc)
        if(NOT _rc EQUAL 0)
            continue()   # not Mach-O
        endif()
        string(STRIP "${_archs}" _archs)
        string(REPLACE " " ";" _archs "${_archs}")
        list(LENGTH _archs _na)
        if(_na LESS 2)
            continue()
        endif()
        if(NOT GCS_ARCH IN_LIST _archs)
            message(FATAL_ERROR "macos_bundle: ${_f} has no ${GCS_ARCH} code")
        endif()
        _run(chmod u+w "${_f}")
        _run(lipo -thin "${GCS_ARCH}" "${_f}" -output "${_f}")
        math(EXPR _n_thin "${_n_thin} + 1")
    endforeach()
    message(STATUS "macos_bundle: thinned ${_n_thin} files to ${GCS_ARCH}")
endif()

# ── 5. Signature ──────────────────────────────────────────────────────────────
#
# Ad hoc ("-"): enough for the code to run, not a Developer ID. --deep because
# the framework's own signature no longer matches its contents once the build
# files are left out of the copy.
_run(codesign --force --deep --sign - "${GCS_APP}")
message(STATUS "macos_bundle: ${GCS_APP} is self-contained and signed")
