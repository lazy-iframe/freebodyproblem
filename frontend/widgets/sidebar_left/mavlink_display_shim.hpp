// Free Body Problem: A modern, simple and minimalist Ground Control Station
// Copyright (C) 2026  Azhar Tanweer

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
// ImGui helpers used exclusively by mavlink_display_generated.cpp.
// Not part of the public API.

#include "imgui.h"
#include "../sidebar_themes.hpp"
#include <cstdarg>
#include <cstdio>

static inline void display_kv(const char* key, const char* val)
{
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(ImVec4(0.50f, 0.50f, 0.50f, 1.0f), "%s", key);
    ImGui::SameLine(0, 4);
    ImGui::TextColored(col_data(), "%s", val);
}

static inline void display_kvf(const char* key, const char* fmt, ...)
{
    char buf[128];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    display_kv(key, buf);
}
