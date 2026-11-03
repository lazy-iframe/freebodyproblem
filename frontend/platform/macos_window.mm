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

#include "macos_window.hpp"

#import <Cocoa/Cocoa.h>

#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

void macos_match_titlebar(GLFWwindow* window, float r, float g, float b)
{
    static float last[3] = { -1.0f, -1.0f, -1.0f };
    if (last[0] == r && last[1] == g && last[2] == b) return;
    last[0] = r; last[1] = g; last[2] = b;

    NSWindow* w = glfwGetCocoaWindow(window);
    if (!w) return;

    // Transparent, so the window's background shows through it; with no
    // title text, so it is a plain strip above the top bar.
    w.titlebarAppearsTransparent = YES;
    w.titleVisibility            = NSWindowTitleHidden;
    w.backgroundColor = [NSColor colorWithSRGBRed:r green:g blue:b alpha:1.0];

    // Dark appearance on a dark bar, so the traffic lights' unfocused grey and
    // any title text stay visible; light on a light theme.
    const float luma = 0.2126f * r + 0.7152f * g + 0.0722f * b;
    w.appearance = [NSAppearance appearanceNamed:
        luma < 0.5f ? NSAppearanceNameDarkAqua : NSAppearanceNameAqua];
}
