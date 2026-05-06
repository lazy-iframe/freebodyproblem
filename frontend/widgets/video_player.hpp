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

#include <atomic>
#include <cstdint>
#include <mutex>
#include <vector>

#include <gst/gst.h>
#include <gst/app/gstappsink.h>

enum class VideoState { Idle, Connecting, Playing, Error };

// ── VideoPlayer ───────────────────────────────────────────────────────────────
//
// Owns a GStreamer pipeline and exposes the latest decoded frame as raw RGB
// bytes. Thread-safety contract:
//   • pipeline / appsink  — UI thread only (start / stop / poll)
//   • state               — atomic; readable from any thread
//   • frame_data/w/h/new  — guarded by frame_mtx
//   • url / error_msg     — UI thread only
//
struct VideoPlayer {
    std::atomic<VideoState> state { VideoState::Idle };
    char url[512]       = {};
    char error_msg[256] = {};

    GstElement* pipeline = nullptr;
    GstElement* appsink  = nullptr;

    std::mutex           frame_mtx;
    std::vector<uint8_t> frame_data;   // latest RGB frame
    int  frame_w   = 0;
    int  frame_h   = 0;
    bool new_frame = false;
};

// Call once at startup before any VideoPlayer is used.
void video_player_init();

// Build the GStreamer pipeline from vp.url and transition to Connecting.
// Supported URL schemes:  rtsp://...   udp://host:port
void video_player_start(VideoPlayer& vp);

// Stop the pipeline and reset to Idle.  Safe to call from the UI thread.
void video_player_stop(VideoPlayer& vp);

// Poll the GStreamer bus for error / EOS events.  Call once per frame.
void video_player_poll(VideoPlayer& vp);

// Call once on application exit.
void video_player_shutdown();
