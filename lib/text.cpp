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

// lib/text.cpp

#include "lib/text.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>

static char lower_char(char c) { return (char)std::tolower((unsigned char)c); }
static char upper_char(char c) { return (char)std::toupper((unsigned char)c); }

std::string text_lower(std::string_view s)
{
    std::string out(s);
    for (char& c : out) c = lower_char(c);
    return out;
}

std::string text_upper(std::string_view s)
{
    std::string out(s);
    for (char& c : out) c = upper_char(c);
    return out;
}

bool text_iequals(std::string_view a, std::string_view b)
{
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i)
        if (lower_char(a[i]) != lower_char(b[i])) return false;
    return true;
}

bool text_icontains(std::string_view haystack, std::string_view needle)
{
    return std::search(haystack.begin(), haystack.end(), needle.begin(), needle.end(),
                       [](char x, char y) { return lower_char(x) == lower_char(y); })
           != haystack.end() || needle.empty();
}

std::string text_hms(int64_t seconds)
{
    if (seconds < 0) seconds = 0;
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%02lld:%02d:%02d", (long long)(seconds / 3600),
                  (int)((seconds / 60) % 60), (int)(seconds % 60));
    return buf;
}
