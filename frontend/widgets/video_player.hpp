// Free Body Problem: A modern, simple and minimalist Ground Control Station
// Copyright (C) 2026  Azhar Tanweer
// Contact: azhar.tanweer404@gmail.com

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
