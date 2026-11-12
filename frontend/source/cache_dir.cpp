/**************************************************************************
 * Copyright (C) 2026  Azhar Tanweer
 * Contact: azhar.tanweer404@gmail.com
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 **************************************************************************/


#include "frontend/source/cache_dir.hpp"

#include <cstdlib>
#include <filesystem>

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <shlobj.h>
#endif

std::string app_cache_dir(const char* name)
{
#ifdef _WIN32
    char buf[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, buf)))
        return std::string(buf) + "\\freebodyproblem\\" + name;
    return std::string(".\\") + name + "_cache";
#else
    const char* xdg = getenv("XDG_CACHE_HOME");
    if (xdg && *xdg)
        return std::string(xdg) + "/freebodyproblem/" + name;
    const char* home = getenv("HOME");
    if (home)
        return std::string(home) + "/.cache/freebodyproblem/" + name;
    return std::string("./.") + name + "_cache";
#endif
}

std::string app_default_save_dir()
{
#ifdef _WIN32
    char buf[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_PERSONAL, nullptr, 0, buf)))
        return std::string(buf) + "\\freebodyproblem";
    return ".";
#else
    const char* home = getenv("HOME");
#  ifdef __APPLE__
    if (home) return std::string(home) + "/Documents/freebodyproblem";
#  else
    const char* xdg = getenv("XDG_DATA_HOME");
    if (xdg && *xdg) return std::string(xdg) + "/freebodyproblem";
    if (home) return std::string(home) + "/.local/share/freebodyproblem";
#  endif
    return ".";
#endif
}

static std::string s_save_dir;

void save_dir_set(const std::string& chosen)
{
    s_save_dir = chosen.empty() ? app_default_save_dir() : chosen;
}

std::string save_dir(const char* sub)
{
    if (s_save_dir.empty()) s_save_dir = app_default_save_dir();
    return sub ? (std::filesystem::path(s_save_dir) / sub).string() : s_save_dir;
}
