# Makes freebodyproblem.app self-contained. Run at install time (and so by
# CPack) after the bundle's own files are in place; see CMakeLists.txt.
#
# In:  GCS_APP            the installed .app
#      GCS_GST_FRAMEWORK  GStreamer.framework to copy in, or empty for none
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
        if(l STREQUAL "")
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
# only for building against it — headers, static and libtool archives,
# pkg-config files, documentation — and the command-line tools.
if(GCS_GST_FRAMEWORK)
    message(STATUS "macos_bundle: copying ${GCS_GST_FRAMEWORK}")
    _run(rsync -a --delete
         --exclude=Headers --exclude=include
         --exclude=*.a --exclude=*.la
         --exclude=pkgconfig --exclude=aclocal
         --exclude=gtk-doc --exclude=doc --exclude=man
         --exclude=Versions/1.0/bin
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

# ── 3. Signature ──────────────────────────────────────────────────────────────
#
# Ad hoc ("-"): enough for the code to run, not a Developer ID. --deep because
# the framework's own signature no longer matches its contents once the build
# files are left out of the copy.
_run(codesign --force --deep --sign - "${GCS_APP}")
message(STATUS "macos_bundle: ${GCS_APP} is self-contained and signed")
