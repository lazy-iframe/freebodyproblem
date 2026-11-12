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

// frontend/source/coord_format.hpp
//
// A position as the operator reads it: unsigned degrees with the hemisphere
// after them, "46.59350N 7.90910E". The top bar, the map's A/C block and the
// GO HERE menu all show it this way. (What is copied to the clipboard is the
// signed decimal form other tools paste, and is not this.)

#pragma once

#include <cmath>
#include <cstdio>
#include <string>

// `decimals` places; `sep` between latitude and longitude.
inline std::string format_lat_lon(double lat, double lon, int decimals, const char* sep = " ")
{
    char buf[80];
    std::snprintf(buf, sizeof(buf), "%.*f%c%s%.*f%c",
                  decimals, std::fabs(lat), lat < 0 ? 'S' : 'N', sep,
                  decimals, std::fabs(lon), lon < 0 ? 'W' : 'E');
    return buf;
}
