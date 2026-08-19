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
#include "../../backend/mavlink_parser.hpp"

class MavlinkSender;

// The column of square user-function buttons beside the video feed. One button
// per plugin registered in plugins/ ; pressing one runs that plugin on this
// thread. Drawn by the centre view, which owns the rectangle beside the video.
//
// `floating` says the rail is over the picture rather than beside it — the
// fullscreen case — and softens the panel so the feed still reads underneath.
void draw_plugin_rail(const VehicleState& vs, MavlinkSender* sender,
                      float x, float y, float w, float h,
                      bool floating = false);
