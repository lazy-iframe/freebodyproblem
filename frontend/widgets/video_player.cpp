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


#include "video_player.hpp"

#include <cstdio>
#include <cstring>
#include <string>

// ── gst_init / gst_deinit wrappers ───────────────────────────────────────────

void video_player_init()
{
    gst_init(nullptr, nullptr);
}

void video_player_shutdown()
{
    gst_deinit();
}

// ── appsink new-sample callback ───────────────────────────────────────────────
//
// Runs in GStreamer's streaming thread.  Copies the decoded RGB frame into
// the VideoPlayer's double-buffer under the frame_mtx lock.

static GstFlowReturn on_new_sample(GstElement* sink, VideoPlayer* vp)
{
    GstSample* sample = gst_app_sink_pull_sample(GST_APP_SINK(sink));
    if (!sample)
        return GST_FLOW_ERROR;

    GstBuffer* buf  = gst_sample_get_buffer(sample);
    GstCaps*   caps = gst_sample_get_caps(sample);

    int w = 0, h = 0;
    if (caps) {
        const GstStructure* s = gst_caps_get_structure(caps, 0);
        gst_structure_get_int(s, "width",  &w);
        gst_structure_get_int(s, "height", &h);
    }

    GstMapInfo map;
    if (w > 0 && h > 0 && gst_buffer_map(buf, &map, GST_MAP_READ)) {
        // Build new buffer outside the lock, then swap in O(1)
        std::vector<uint8_t> tmp(map.data, map.data + map.size);
        gst_buffer_unmap(buf, &map);

        std::lock_guard<std::mutex> lk(vp->frame_mtx);
        std::swap(vp->frame_data, tmp);
        vp->frame_w   = w;
        vp->frame_h   = h;
        vp->new_frame = true;
        // Transition Connecting → Playing on first frame
        if (vp->state.load(std::memory_order_relaxed) == VideoState::Connecting)
            vp->state.store(VideoState::Playing, std::memory_order_release);
    }

    gst_sample_unref(sample);
    return GST_FLOW_OK;
}

// ── Pipeline builder ──────────────────────────────────────────────────────────
//
// Common appsink tail: raw RGB, no sync (low-latency), drop old frames, keep
// at most one buffered sample so the UI always gets the freshest frame.

static constexpr const char* kSinkTail =
    "videoconvert ! video/x-raw,format=RGB"
    " ! appsink name=videosink sync=false drop=true max-buffers=1";

static std::string build_pipeline(const char* url)
{
    std::string u = url;

    // ── RTSP ─────────────────────────────────────────────────────────────────
    // rtspsrc latency=0 is aggressive but suitable for a LAN drone link.
    // Use latency=100 or higher over WAN if frames stutter.
    if (u.rfind("rtsp://", 0) == 0) {
        return
            "rtspsrc location=" + u + " latency=0 ! "
            "decodebin ! "
            + kSinkTail;
    }

    // ── UDP RTP H.264 ─────────────────────────────────────────────────────────
    // Standard ArduPilot / PX4 companion-video stream (default port 5600).
    // URL format: udp://0.0.0.0:5600
    if (u.rfind("udp://", 0) == 0) {
        return
            "udpsrc uri=" + u + " "
            "! application/x-rtp,clock-rate=90000,encoding-name=H264,payload=96 "
            "! rtph264depay ! h264parse "
            "! avdec_h264 max-threads=1 "
            "! " + kSinkTail;
    }

    return {};
}

// ── Public API ────────────────────────────────────────────────────────────────

void video_player_start(VideoPlayer& vp)
{
    // Clean up any previous pipeline first
    video_player_stop(vp);

    vp.error_msg[0] = '\0';
    vp.state.store(VideoState::Connecting, std::memory_order_release);

    const std::string pipe_str = build_pipeline(vp.url);
    if (pipe_str.empty()) {
        std::snprintf(vp.error_msg, sizeof(vp.error_msg),
                      "Unsupported scheme — use rtsp:// or udp://");
        vp.state.store(VideoState::Error, std::memory_order_release);
        return;
    }

    GError* err = nullptr;
    vp.pipeline = gst_parse_launch(pipe_str.c_str(), &err);
    if (!vp.pipeline || err) {
        std::snprintf(vp.error_msg, sizeof(vp.error_msg),
                      "%s", err ? err->message : "gst_parse_launch failed");
        if (err) g_error_free(err);
        if (vp.pipeline) { gst_object_unref(vp.pipeline); vp.pipeline = nullptr; }
        vp.state.store(VideoState::Error, std::memory_order_release);
        return;
    }

    vp.appsink = gst_bin_get_by_name(GST_BIN(vp.pipeline), "videosink");
    if (!vp.appsink) {
        std::snprintf(vp.error_msg, sizeof(vp.error_msg),
                      "internal: appsink element not found");
        gst_object_unref(vp.pipeline);
        vp.pipeline = nullptr;
        vp.state.store(VideoState::Error, std::memory_order_release);
        return;
    }

    g_object_set(vp.appsink, "emit-signals", TRUE, nullptr);
    g_signal_connect(vp.appsink, "new-sample",
                     G_CALLBACK(on_new_sample), &vp);

    if (gst_element_set_state(vp.pipeline, GST_STATE_PLAYING)
            == GST_STATE_CHANGE_FAILURE) {
        std::snprintf(vp.error_msg, sizeof(vp.error_msg),
                      "Failed to set pipeline to PLAYING");
        gst_object_unref(vp.appsink);
        gst_object_unref(vp.pipeline);
        vp.appsink  = nullptr;
        vp.pipeline = nullptr;
        vp.state.store(VideoState::Error, std::memory_order_release);
    }
}

void video_player_stop(VideoPlayer& vp)
{
    if (vp.pipeline) {
        gst_element_set_state(vp.pipeline, GST_STATE_NULL);
        gst_object_unref(vp.pipeline);
        vp.pipeline = nullptr;
    }
    if (vp.appsink) {
        gst_object_unref(vp.appsink);
        vp.appsink = nullptr;
    }
    {
        std::lock_guard<std::mutex> lk(vp.frame_mtx);
        vp.frame_data.clear();
        vp.frame_w   = 0;
        vp.frame_h   = 0;
        vp.new_frame = false;
    }
    vp.state.store(VideoState::Idle, std::memory_order_release);
}

void video_player_poll(VideoPlayer& vp)
{
    if (!vp.pipeline) return;

    GstBus* bus = gst_element_get_bus(vp.pipeline);
    if (!bus) return;

    GstMessage* msg;
    while ((msg = gst_bus_pop_filtered(bus,
                      static_cast<GstMessageType>(
                          GST_MESSAGE_ERROR | GST_MESSAGE_EOS))))
    {
        switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_ERROR: {
            GError* gerr  = nullptr;
            gchar*  debug = nullptr;
            gst_message_parse_error(msg, &gerr, &debug);
            std::snprintf(vp.error_msg, sizeof(vp.error_msg),
                          "%s", gerr ? gerr->message : "unknown error");
            if (gerr)  g_error_free(gerr);
            if (debug) g_free(debug);
            vp.state.store(VideoState::Error, std::memory_order_release);
            break;
        }
        case GST_MESSAGE_EOS:
            std::snprintf(vp.error_msg, sizeof(vp.error_msg), "stream ended (EOS)");
            vp.state.store(VideoState::Error, std::memory_order_release);
            break;
        default:
            break;
        }
        gst_message_unref(msg);
    }

    gst_object_unref(bus);
}
