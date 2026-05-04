// AUTO-GENERATED — do not edit manually.
// Regenerate: re-run CMake or call gen_mavlink_display.py directly.
#pragma once
#include <cstdint>
#include <mavlink/ardupilotmega/mavlink.h>

// Returns the human-readable name for a MAVLink message ID, or nullptr.
const char* mavlink_display_name(uint32_t id);

// Renders all decoded fields for the given raw message as ImGui kv rows.
// Must be called inside an active ImGui window or child.
void mavlink_display_draw(uint32_t id, const mavlink_message_t& raw);
