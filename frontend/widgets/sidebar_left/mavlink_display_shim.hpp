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
