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


#include "cache_dir.hpp"

#include <cstdlib>

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <shlobj.h>
#else
#  include <sys/stat.h>
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

void mkdirs_for_file(const std::string& filepath)
{
    size_t sep = filepath.rfind('/');
#ifdef _WIN32
    if (sep == std::string::npos) sep = filepath.rfind('\\');
#endif
    if (sep == std::string::npos) return;
    std::string dir = filepath.substr(0, sep);
    for (size_t i = 1; i <= dir.size(); ++i) {
        if (i == dir.size() || dir[i] == '/' || dir[i] == '\\') {
            std::string sub = dir.substr(0, i);
#ifdef _WIN32
            CreateDirectoryA(sub.c_str(), nullptr);
#else
            mkdir(sub.c_str(), 0755);
#endif
        }
    }
}
