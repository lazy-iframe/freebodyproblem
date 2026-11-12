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

// lib/text.hpp
//
// Small text helpers: changing case, comparing and searching without regard
// to it, and a duration as hh:mm:ss. ASCII only — what MAVLink carries in its
// names and status text, and what the UI labels are written in.

#pragma once

#include <cstdint>
#include <string>
#include <string_view>

std::string text_lower(std::string_view s);
std::string text_upper(std::string_view s);

// Equal, ignoring case.
bool text_iequals(std::string_view a, std::string_view b);

// `needle` somewhere in `haystack`, ignoring case. An empty needle is found
// everywhere, as a search box with nothing typed in it matches every row.
bool text_icontains(std::string_view haystack, std::string_view needle);

// A count of seconds as "hh:mm:ss". Hours go past 24 rather than wrap, as an
// uptime should; a negative count reads as zero.
std::string text_hms(int64_t seconds);
