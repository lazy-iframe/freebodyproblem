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

// Two-way state for GO HERE: the map's right-click menu, and the guided
// position target it asks for.
//
// The map draws and reads the gesture; it does not hold a sender. So the
// fields below split in two — what center_view tells the map about the
// vehicle, and what the map hands back for center_view to put on the wire.
// Same shape as MissionPickState next door, for the same reason.
struct GotoTargetState {
    // ── In: what the vehicle can be asked for ────────────────────────────────

    // True while a vehicle is connected and in GUIDED. The menu item is drawn
    // either way — an operator who right-clicks wants to know *why* they
    // cannot go there — but only sends when this is set.
    bool  guided    = false;
    bool  connected = false;

    // Vehicle altitude above home, metres. Seeds the altitude field the first
    // time the menu opens, so the default is "stay at this height".
    float current_alt_m = 0.0f;

    // ── Out: the target the operator asked for ───────────────────────────────

    // Set by map_view when GO is pressed; cleared by center_view once sent.
    bool   requested = false;
    double req_lat   = 0.0;
    double req_lon   = 0.0;
    float  req_alt_m = 0.0f;

    // ── The target now standing ──────────────────────────────────────────────
    // Written by center_view after a send, drawn by map_view until the vehicle
    // changes, so the operator can see where they sent it.
    bool   has_target  = false;
    double target_lat  = 0.0;
    double target_lon  = 0.0;
    float  target_alt_m = 0.0f;
};
