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


#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

enum class ConnType   { UDP, TCP, Serial };
enum class LinkStatus { Idle, Connecting, Connected, Timeout, Error };

struct SerialPortInfo {
    std::string device;       // e.g. "/dev/ttyUSB0", or "COM3" on Windows
    std::string description;  // e.g. "FTDI USB Serial Device"
};

// Enumerate available serial ports.
// Linux:   scans /dev/ttyUSB*, /dev/ttyACM*, real /dev/ttyS* (with sysfs device node).
// Windows: enumerates the GUID_DEVCLASS_PORTS setup class, skipping LPT ports.
std::vector<SerialPortInfo> list_serial_ports();

// ── Serial transport ──────────────────────────────────────────────────────────
//
// A serial port is a file descriptor on POSIX and a HANDLE on Windows; intptr_t
// holds either. Keeping the platform types out of the header means the link
// thread does not need <termios.h> or <windows.h>.

using SerialHandle = std::intptr_t;

constexpr SerialHandle SERIAL_INVALID = -1;

// Open and configure `device` at `baud` — 8N1, raw, no flow control.
// Returns SERIAL_INVALID on failure.
SerialHandle serial_open(const char* device, int baud);

// Read with a ~200 ms timeout, matching the link thread's poll interval.
// Returns bytes read, 0 on timeout, or -1 on error.
int serial_read(SerialHandle h, void* buf, std::size_t len);

// Returns bytes written, or -1 on error.
int serial_write(SerialHandle h, const void* buf, std::size_t len);

void serial_close(SerialHandle h);
