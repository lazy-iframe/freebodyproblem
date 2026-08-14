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

#include <algorithm>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#include <setupapi.h>
#include <devguid.h>
#include <cstdio>    // sscanf
#else
#include <dirent.h>
#include <sys/stat.h>
#include <cstdlib>   // realpath
#include <fstream>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
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

static speed_t baud_to_speed(int baud)
{
    switch (baud) {
    case 9600:   return B9600;
    case 19200:  return B19200;
    case 38400:  return B38400;
    case 57600:  return B57600;
    case 115200: return B115200;
    case 230400: return B230400;
    case 460800: return B460800;
    case 921600: return B921600;
    default:     return B57600;
    }
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

#else  // _WIN32

    // The ports setup class covers both COM and LPT; the friendly name carries
    // the human-readable string ("USB Serial Port (COM3)") while the device's
    // registry key holds the bare port name we actually need to open.
    HDEVINFO devs = SetupDiGetClassDevsA(&GUID_DEVCLASS_PORTS, nullptr, nullptr,
                                         DIGCF_PRESENT);
    if (devs == INVALID_HANDLE_VALUE) return result;

    SP_DEVINFO_DATA info{};
    info.cbSize = sizeof(info);

    for (DWORD i = 0; SetupDiEnumDeviceInfo(devs, i, &info); ++i) {
        char port[64] = {};

        HKEY key = SetupDiOpenDevRegKey(devs, &info, DICS_FLAG_GLOBAL, 0,
                                        DIREG_DEV, KEY_READ);
        if (key == INVALID_HANDLE_VALUE) continue;

        DWORD type = 0, size = sizeof(port);
        const LONG rc = RegQueryValueExA(key, "PortName", nullptr, &type,
                                         reinterpret_cast<LPBYTE>(port), &size);
        RegCloseKey(key);

        if (rc != ERROR_SUCCESS || type != REG_SZ) continue;
        if (strncmp(port, "COM", 3) != 0) continue;   // skip LPT and friends

        char friendly[256] = {};
        if (!SetupDiGetDeviceRegistryPropertyA(
                devs, &info, SPDRP_FRIENDLYNAME, nullptr,
                reinterpret_cast<PBYTE>(friendly), sizeof(friendly), nullptr)) {
            // Fall back to the generic description, then to the port itself.
            if (!SetupDiGetDeviceRegistryPropertyA(
                    devs, &info, SPDRP_DEVICEDESC, nullptr,
                    reinterpret_cast<PBYTE>(friendly), sizeof(friendly), nullptr)) {
                strncpy(friendly, port, sizeof(friendly) - 1);
            }
        }

        result.push_back({ port, friendly });
    }

    SetupDiDestroyDeviceInfoList(devs);

    // Sort numerically so COM10 follows COM9 rather than COM1.
    std::sort(result.begin(), result.end(),
              [](const SerialPortInfo& a, const SerialPortInfo& b) {
        int na = 0, nb = 0;
        if (sscanf(a.device.c_str(), "COM%d", &na) == 1 &&
            sscanf(b.device.c_str(), "COM%d", &nb) == 1)
            return na < nb;
        return a.device < b.device;
    });
#endif

    return result;
}

// ── Serial transport ──────────────────────────────────────────────────────────

SerialHandle serial_open(const char* device, int baud)
{
#ifdef _WIN32
    // The \\.\ prefix is required for COM10 and above, and is harmless below it.
    std::string path = device;
    if (path.rfind("\\\\.\\", 0) != 0) path = "\\\\.\\" + path;

    HANDLE h = CreateFileA(path.c_str(), GENERIC_READ | GENERIC_WRITE,
                           0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (h == INVALID_HANDLE_VALUE) return SERIAL_INVALID;

    DCB dcb{};
    dcb.DCBlength = sizeof(dcb);
    if (!GetCommState(h, &dcb)) { CloseHandle(h); return SERIAL_INVALID; }

    dcb.BaudRate        = static_cast<DWORD>(baud);
    dcb.ByteSize        = 8;
    dcb.Parity          = NOPARITY;
    dcb.StopBits        = ONESTOPBIT;
    dcb.fBinary         = TRUE;
    dcb.fParity         = FALSE;
    dcb.fOutxCtsFlow    = FALSE;
    dcb.fOutxDsrFlow    = FALSE;
    dcb.fDtrControl     = DTR_CONTROL_ENABLE;
    dcb.fRtsControl     = RTS_CONTROL_ENABLE;
    dcb.fOutX           = FALSE;
    dcb.fInX            = FALSE;
    dcb.fAbortOnError   = FALSE;
    if (!SetCommState(h, &dcb)) { CloseHandle(h); return SERIAL_INVALID; }

    // Return as soon as any byte is available, or after 200 ms with none —
    // the equivalent of the POSIX VMIN=0 / VTIME=2 setting below.
    COMMTIMEOUTS to{};
    to.ReadIntervalTimeout         = MAXDWORD;
    to.ReadTotalTimeoutMultiplier  = MAXDWORD;
    to.ReadTotalTimeoutConstant    = 200;
    to.WriteTotalTimeoutMultiplier = 0;
    to.WriteTotalTimeoutConstant   = 1000;
    if (!SetCommTimeouts(h, &to)) { CloseHandle(h); return SERIAL_INVALID; }

    PurgeComm(h, PURGE_RXCLEAR | PURGE_TXCLEAR);
    return reinterpret_cast<SerialHandle>(h);

#else
    const int fd = open(device, O_RDWR | O_NOCTTY);
    if (fd < 0) return SERIAL_INVALID;

    termios tio{};
    tcgetattr(fd, &tio);
    cfmakeraw(&tio);
    const speed_t spd = baud_to_speed(baud);
    cfsetispeed(&tio, spd);
    cfsetospeed(&tio, spd);
    tio.c_cc[VTIME] = 2; // 0.2 s read timeout
    tio.c_cc[VMIN]  = 0;
    tcsetattr(fd, TCSANOW, &tio);

    return static_cast<SerialHandle>(fd);
#endif
}

int serial_read(SerialHandle h, void* buf, std::size_t len)
{
    if (h == SERIAL_INVALID) return -1;
#ifdef _WIN32
    DWORD got = 0;
    if (!ReadFile(reinterpret_cast<HANDLE>(h), buf, static_cast<DWORD>(len),
                  &got, nullptr))
        return -1;
    return static_cast<int>(got);
#else
    return static_cast<int>(read(static_cast<int>(h), buf, len));
#endif
}

int serial_write(SerialHandle h, const void* buf, std::size_t len)
{
    if (h == SERIAL_INVALID) return -1;
#ifdef _WIN32
    DWORD put = 0;
    if (!WriteFile(reinterpret_cast<HANDLE>(h), buf, static_cast<DWORD>(len),
                   &put, nullptr))
        return -1;
    return static_cast<int>(put);
#else
    return static_cast<int>(write(static_cast<int>(h), buf, len));
#endif
}

void serial_close(SerialHandle h)
{
    if (h == SERIAL_INVALID) return;
#ifdef _WIN32
    CloseHandle(reinterpret_cast<HANDLE>(h));
#else
    close(static_cast<int>(h));
#endif
}
