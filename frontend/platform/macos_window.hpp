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

// The macOS window's title bar, matched to the app's own top bar.
//
// Elsewhere the window is undecorated and the top bar is the only chrome. On
// macOS it cannot be: AppKit will not zoom, resize or take full screen a
// borderless window, so the window there keeps its title bar — and with it
// the traffic lights, the green button's full screen and ordinary resizing —
// and this makes that bar transparent over the top bar's colour, so the two
// read as one strip. The content is not extended under the bar: the bar takes
// the mouse for dragging the window, which would leave the top bar's own
// buttons beneath it dead.

#ifdef __APPLE__
struct GLFWwindow;

// Sets the title bar's colour to (r, g, b), in sRGB 0..1, and the light or dark
// appearance that keeps its buttons and title readable on it. Cheap to call
// every frame: nothing is done unless the colour has changed, so a theme
// switch follows on the next frame.
void macos_match_titlebar(GLFWwindow* window, float r, float g, float b);
#endif
