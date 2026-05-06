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


#include "connection.hpp"

#ifndef _WIN32
#include <dirent.h>
#include <sys/stat.h>
#include <cstdlib>   // realpath
#include <fstream>
#include <algorithm>
#endif

// ── helpers ───────────────────────────────────────────────────────────────────

#ifndef _WIN32
static std::string read_sysfs_attr(const std::string& path)
{
    std::ifstream f(path);
    if (!f) return {};
    std::string s;
    std::getline(f, s);
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' '))
        s.pop_back();
    return s;
}
#endif

// ── list_serial_ports ─────────────────────────────────────────────────────────

std::vector<SerialPortInfo> list_serial_ports()
{
    std::vector<SerialPortInfo> result;

#ifndef _WIN32
    const std::string tty_class = "/sys/class/tty";

    DIR* dir = opendir(tty_class.c_str());
    if (!dir) return result;

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        const std::string name = entry->d_name;
        if (name == "." || name == "..") continue;

        const bool is_usb = name.rfind("ttyUSB", 0) == 0;
        const bool is_acm = name.rfind("ttyACM", 0) == 0;
        const bool is_s   = name.rfind("ttyS",   0) == 0;

        if (!is_usb && !is_acm && !is_s) continue;

        // For ttyS, require a sysfs device symlink (otherwise it is a virtual UART)
        const std::string dev_link = tty_class + "/" + name + "/device";
        struct stat st{};
        if (is_s && stat(dev_link.c_str(), &st) != 0)
            continue;

        // Require the /dev node to exist
        const std::string dev_path = "/dev/" + name;
        if (stat(dev_path.c_str(), &st) != 0) continue;

        // Build human-readable description from USB sysfs attributes
        std::string desc;
        if (is_usb || is_acm) {
            char real[PATH_MAX] = {};
            if (realpath(dev_link.c_str(), real)) {
                // dev_link resolves to the tty sysfs node; its parent is the USB interface,
                // its grandparent is the USB device that holds manufacturer/product strings.
                std::string iface = real;
                auto slash = iface.rfind('/');
                std::string usb_dev = (slash != std::string::npos)
                                      ? iface.substr(0, slash) : iface;

                const std::string mfr  = read_sysfs_attr(usb_dev + "/manufacturer");
                const std::string prod = read_sysfs_attr(usb_dev + "/product");

                if (!mfr.empty() && !prod.empty())
                    desc = mfr + " " + prod;
                else if (!prod.empty())
                    desc = prod;
                else if (!mfr.empty())
                    desc = mfr;
            }
        }
        if (desc.empty()) desc = name;

        result.push_back({ dev_path, desc });
    }
    closedir(dir);

    // Sort: ttyUSB first, ttyACM second, ttyS last; then alphabetically
    std::sort(result.begin(), result.end(), [](const SerialPortInfo& a, const SerialPortInfo& b) {
        auto pri = [](const std::string& d) {
            if (d.find("ttyUSB") != std::string::npos) return 0;
            if (d.find("ttyACM") != std::string::npos) return 1;
            return 2;
        };
        int pa = pri(a.device), pb = pri(b.device);
        if (pa != pb) return pa < pb;
        return a.device < b.device;
    });
#endif

    return result;
}
