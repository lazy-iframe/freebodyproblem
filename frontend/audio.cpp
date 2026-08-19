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


#include "audio.hpp"
#include "app_log.hpp"

// Playback only, and only what these tones need: no decoders, no resource
// manager, no recording. Trims both the build and the surface that can fail.
#define MA_NO_DECODING
#define MA_NO_ENCODING
#define MA_NO_GENERATION
#define MA_NO_RESOURCE_MANAGER
#define MA_NO_NODE_GRAPH
#define MA_API static
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include <atomic>
#include <cmath>

namespace {

constexpr int   SAMPLE_RATE = 48000;
constexpr int   MAX_VOICES  = 8;
constexpr int   MAX_SEGS    = 4;

// One beat, in the sense GRUB's tune format means it: GRUB_INIT_TUNE leads with
// a tempo in beats per minute and every note length is a count of these. At the
// 480 the tones below are written for, a beat is 125 ms.
constexpr float BEAT_S = 60.0f / 480.0f;

// Attack and release ramps, kept short. A tone gated on and off at full
// amplitude clicks at both ends, and through a modern output that click reads
// as a fault rather than as character — but a long decay would turn a PC
// speaker note into a chime, which is the opposite of the point.
constexpr float RAMP_ATTACK_S  = 0.004f;
constexpr float RAMP_RELEASE_S = 0.004f;

// The PC speaker could only slam a cone between two positions, which is why
// every BIOS and bootloader tune sounds the way it does. A sine at the same
// frequency is the same note and the wrong instrument — and the square's
// harmonics are also what carry it out of a small laptop speaker.
enum class Wave { Sine, Square };

struct Segment { float freq; float secs; };

struct ToneDef {
    Segment seg[MAX_SEGS];
    int     count;
    float   gain;
    Wave    wave;
};

// Written in the idiom of GRUB_INIT_TUNE="480 440 1" — square wave, 440 Hz
// root, whole notes at 125 ms. Shapes, not just pitches: over a running motor
// the ear catches "went up" and "went down" long after it stops resolving which
// note it was.
//
// Success rises a fifth, the same interval a bootloader tune uses to sound
// finished:  480 440 1 660 1
const ToneDef TONE_SUCCESS {
    { { 440.0f, BEAT_S }, { 660.0f, BEAT_S } }, 2, 0.26f, Wave::Square };
// Three notes falling, which no bootloader plays on a good boot and which is
// unmistakable against the two-note success:  480 660 1 440 1 330 1
// Louder than success, because a rejection matters more than an ack. Nothing
// here drops below 330 Hz — small speakers roll off hard under ~300 Hz, and an
// earlier 420 -> 260 Hz version of this could not be heard at all.
const ToneDef TONE_FAILURE {
    { { 660.0f, BEAT_S }, { 440.0f, BEAT_S }, { 330.0f, BEAT_S } },
    3, 0.30f, Wave::Square };
// GRUB_INIT_TUNE="480 440 1" exactly: one 440 Hz note, one beat long.
//
// Deliberately the quietest thing the app plays — roughly a third of the
// amplitude of success and under a third of failure, about 10 dB down on both.
// It is not an alert: it repeats for the whole armed period and has to sit
// under a conversation and under the aircraft, present enough that its absence
// registers without ever competing with a tone that means something happened.
// Anything loud enough to notice every time is loud enough to resent by the
// end of a flight.
const ToneDef TONE_ARMED {
    { { 440.0f, BEAT_S } }, 1, 0.09f, Wave::Square };

const ToneDef& tone_def(GcsTone t)
{
    switch (t) {
    case GcsTone::Success: return TONE_SUCCESS;
    case GcsTone::Failure: return TONE_FAILURE;
    default:               return TONE_ARMED;
    }
}

// ── Voices ───────────────────────────────────────────────────────────────────
//
// Fixed pool, claimed without a lock. The device callback runs on a real-time
// thread that must never wait on a UI thread or a link thread, so a voice moves
// Free -> Filling -> Active by atomic compare-exchange on the producer side and
// back to Free by the callback alone. Nothing else touches `state`.

enum VoiceState : int { VOICE_FREE = 0, VOICE_FILLING = 1, VOICE_ACTIVE = 2 };

struct Voice {
    std::atomic<int> state { VOICE_FREE };

    ToneDef def;
    int     seg    = 0;       // segment being played
    float   phase  = 0.0f;    // radians, carried across segments so they join
    float   t_seg  = 0.0f;    // seconds into this segment
    float   t_tone = 0.0f;    // seconds into the whole tone
    float   total  = 0.0f;    // total tone length, for the release ramp
};

Voice            g_voices[MAX_VOICES];
ma_device        g_device;
std::atomic<bool> g_running { false };
std::atomic<bool> g_enabled { true };
std::atomic<float> g_volume { 0.7f };

// ── Mixing ───────────────────────────────────────────────────────────────────

float voice_sample(Voice& v, float dt)
{
    const Segment& s = v.def.seg[v.seg];

    // Envelope: ramp in at the start of the tone, out at its end. Between
    // segments the amplitude is held, so a two-tone cue reads as one gesture.
    float env = 1.0f;
    if (v.t_tone < RAMP_ATTACK_S)
        env = v.t_tone / RAMP_ATTACK_S;
    else if (v.t_tone > v.total - RAMP_RELEASE_S)
        env = (v.total - v.t_tone) / RAMP_RELEASE_S;
    if (env < 0.0f) env = 0.0f;
    if (env > 1.0f) env = 1.0f;

    // Square is taken as the sign of the sine rather than from a separate
    // counter, so both waves share one phase accumulator and one wrap.
    const float raw = (v.def.wave == Wave::Square)
                    ? (std::sin(v.phase) >= 0.0f ? 1.0f : -1.0f)
                    : std::sin(v.phase);
    const float out = raw * env * v.def.gain;

    v.phase += 2.0f * 3.14159265358979f * s.freq * dt;
    if (v.phase > 2.0f * 3.14159265358979f)
        v.phase -= 2.0f * 3.14159265358979f;

    v.t_seg  += dt;
    v.t_tone += dt;
    if (v.t_seg >= s.secs) {
        v.t_seg = 0.0f;
        ++v.seg;
    }
    return out;
}

void data_callback(ma_device* dev, void* out, const void* in, ma_uint32 frames)
{
    (void)dev; (void)in;

    float*      buf = static_cast<float*>(out);
    const float dt  = 1.0f / (float)SAMPLE_RATE;
    const float vol = g_volume.load(std::memory_order_relaxed);

    for (ma_uint32 i = 0; i < frames; ++i) buf[i] = 0.0f;

    for (Voice& v : g_voices) {
        if (v.state.load(std::memory_order_acquire) != VOICE_ACTIVE) continue;

        for (ma_uint32 i = 0; i < frames; ++i) {
            if (v.seg >= v.def.count) break;
            buf[i] += voice_sample(v, dt);
        }

        if (v.seg >= v.def.count)
            v.state.store(VOICE_FREE, std::memory_order_release);
    }

    // One master gain at the end, then clip: eight voices at full tilt would
    // wrap around and turn a cue into a crack.
    for (ma_uint32 i = 0; i < frames; ++i) {
        float s = buf[i] * vol;
        if (s >  1.0f) s =  1.0f;
        if (s < -1.0f) s = -1.0f;
        buf[i] = s;
    }
}

} // namespace

// ── Public API ────────────────────────────────────────────────────────────────

bool audio_init()
{
    if (g_running.load()) return true;

    ma_device_config cfg  = ma_device_config_init(ma_device_type_playback);
    cfg.playback.format   = ma_format_f32;
    cfg.playback.channels = 1;
    cfg.sampleRate        = SAMPLE_RATE;
    cfg.dataCallback      = data_callback;

    if (ma_device_init(nullptr, &cfg, &g_device) != MA_SUCCESS) {
        gcs_log("audio: no playback device \xe2\x80\x94 tones disabled");
        return false;
    }
    if (ma_device_start(&g_device) != MA_SUCCESS) {
        gcs_log("audio: could not start playback \xe2\x80\x94 tones disabled");
        ma_device_uninit(&g_device);
        return false;
    }

    g_running.store(true);
    return true;
}

void audio_shutdown()
{
    if (!g_running.exchange(false)) return;
    ma_device_uninit(&g_device);
}

void gcs_tone(GcsTone tone)
{
    if (!g_running.load(std::memory_order_acquire)) return;
    if (!g_enabled.load(std::memory_order_relaxed)) return;

    for (Voice& v : g_voices) {
        int expected = VOICE_FREE;
        if (!v.state.compare_exchange_strong(expected, VOICE_FILLING,
                                             std::memory_order_acq_rel))
            continue;

        v.def    = tone_def(tone);
        v.seg    = 0;
        v.phase  = 0.0f;
        v.t_seg  = 0.0f;
        v.t_tone = 0.0f;
        v.total  = 0.0f;
        for (int i = 0; i < v.def.count; ++i) v.total += v.def.seg[i].secs;

        v.state.store(VOICE_ACTIVE, std::memory_order_release);
        return;
    }
    // Every voice busy: drop it. A cue queued behind three others would play
    // against whatever the operator is doing by then.
}

void audio_set_enabled(bool on) { g_enabled.store(on); }
bool audio_enabled()            { return g_enabled.load(); }

void audio_set_volume(float v)
{
    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;
    g_volume.store(v);
}

float audio_volume() { return g_volume.load(); }
