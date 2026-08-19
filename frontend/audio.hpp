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

// ── Audio cues ────────────────────────────────────────────────────────────────
//
// Short synthesised tones, mixed on a miniaudio device. Nothing is loaded from
// disk: the tones are generated from a couple of frequency segments each, so
// there are no assets to install and no sample-rate assumptions to get wrong.
//
// Every entry point is safe to call from any thread and safe to call when audio
// never started — a machine with no sound device (headless, over SSH, a locked
// down container) logs one line at startup and every call after that is a no-op.
// A ground station must not care whether it has speakers.

#include <cstdint>

enum class GcsTone {
    Success,   // rising two-tone: a command the vehicle accepted
    Failure,   // falling two-tone: a rejection, a lost link, a failed write
    Armed,     // short single beep, repeated while the vehicle is armed
};

// Opens the default playback device. Returns false when there is none, which is
// not an error worth stopping for. Safe to call twice.
bool audio_init();

// Stops the device and frees it. Call before the process exits.
void audio_shutdown();

// Queue one tone. Overlapping tones mix; when every voice is busy the request
// is dropped rather than queued, because a cue that arrives late is worse than
// one that never played.
void gcs_tone(GcsTone tone);

// GcsTone::Armed has no timer behind it on purpose: the caller plays it every
// fifth HEARTBEAT while the vehicle is armed, so the cadence is the vehicle's
// pulse rather than the ground station's clock. A link that dies mid-flight
// goes quiet, which is the more useful thing to hear.

// Master switches, persisted in settings.json. Volume is linear 0..1.
void  audio_set_enabled(bool on);
bool  audio_enabled();
void  audio_set_volume(float v);
float audio_volume();
