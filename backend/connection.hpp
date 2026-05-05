// Free Body Problem: A modern, simple and minimalist Ground Control Station
// Copyright (C) 2026  Azhar Tanweer
// Contact: azhar.tanweer404@gmail.com

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.


#pragma once
#include <string>
#include <vector>

enum class ConnType   { UDP, TCP, Serial };
enum class LinkStatus { Idle, Connecting, Connected, Timeout, Error };

struct SerialPortInfo {
    std::string device;       // e.g. "/dev/ttyUSB0"
    std::string description;  // e.g. "FTDI USB Serial Device"
};

// Enumerate available serial ports.
// Linux: scans /dev/ttyUSB*, /dev/ttyACM*, real /dev/ttyS* (with sysfs device node).
// Windows: returns COM ports (stub for now).
std::vector<SerialPortInfo> list_serial_ports();
