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
