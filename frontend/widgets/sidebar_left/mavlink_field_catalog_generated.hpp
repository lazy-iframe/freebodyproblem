// AUTO-GENERATED — do not edit manually.
// Regenerate: re-run CMake or call gen_mavlink_display.py directly.
#pragma once
#include <cstdint>
#include <cstddef>
#include <mavlink/ardupilotmega/mavlink.h>

// Describes one numeric-readable field in a MAVLink message.
struct MavlinkFieldDesc {
    uint32_t    msg_id;
    const char* msg_name;   // e.g. "VFR_HUD"
    const char* field_name; // e.g. "airspeed" or "controls[0]"
    const char* label;      // e.g. "airspeed (m/s)"
};

// Flat catalog of all numeric scalar fields, sorted by msg_id.
extern const MavlinkFieldDesc kMavlinkFieldCatalog[];
extern const size_t           kMavlinkFieldCatalogCount;

// Read a single numeric field value from a decoded raw message.
// Returns true and sets *out_value on success.
bool mavlink_field_read(uint32_t msg_id, const char* field_name,
                        const mavlink_message_t& raw, double* out_value);
