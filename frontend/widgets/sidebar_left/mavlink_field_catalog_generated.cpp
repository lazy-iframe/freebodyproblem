// AUTO-GENERATED — do not edit manually.
// Regenerate: re-run CMake or call gen_mavlink_display.py directly.
#include "mavlink_field_catalog_generated.hpp"
#include <cstring>

const MavlinkFieldDesc kMavlinkFieldCatalog[] = {
    { 0, "HEARTBEAT", "type", "type" },
    { 0, "HEARTBEAT", "autopilot", "autopilot" },
    { 0, "HEARTBEAT", "base_mode", "base_mode" },
    { 0, "HEARTBEAT", "custom_mode", "custom_mode" },
    { 0, "HEARTBEAT", "system_status", "system_status" },
    { 1, "SYS_STATUS", "onboard_control_sensors_present", "onboard_control_sensors_present" },
    { 1, "SYS_STATUS", "onboard_control_sensors_enabled", "onboard_control_sensors_enabled" },
    { 1, "SYS_STATUS", "onboard_control_sensors_health", "onboard_control_sensors_health" },
    { 1, "SYS_STATUS", "load", "load (d%)" },
    { 1, "SYS_STATUS", "voltage_battery", "voltage_battery (mV)" },
    { 1, "SYS_STATUS", "current_battery", "current_battery (cA)" },
    { 1, "SYS_STATUS", "battery_remaining", "battery_remaining (%)" },
    { 1, "SYS_STATUS", "drop_rate_comm", "drop_rate_comm (c%)" },
    { 1, "SYS_STATUS", "errors_comm", "errors_comm" },
    { 1, "SYS_STATUS", "errors_count1", "errors_count1" },
    { 1, "SYS_STATUS", "errors_count2", "errors_count2" },
    { 1, "SYS_STATUS", "errors_count3", "errors_count3" },
    { 1, "SYS_STATUS", "errors_count4", "errors_count4" },
    { 1, "SYS_STATUS", "onboard_control_sensors_present_extended", "onboard_control_sensors_present_extended" },
    { 1, "SYS_STATUS", "onboard_control_sensors_enabled_extended", "onboard_control_sensors_enabled_extended" },
    { 1, "SYS_STATUS", "onboard_control_sensors_health_extended", "onboard_control_sensors_health_extended" },
    { 2, "SYSTEM_TIME", "time_unix_usec", "time_unix_usec (us)" },
    { 2, "SYSTEM_TIME", "time_boot_ms", "time_boot_ms (ms)" },
    { 4, "PING", "time_usec", "time_usec (us)" },
    { 4, "PING", "seq", "seq" },
    { 4, "PING", "target_system", "target_system" },
    { 4, "PING", "target_component", "target_component" },
    { 8, "LINK_NODE_STATUS", "timestamp", "timestamp (ms)" },
    { 8, "LINK_NODE_STATUS", "tx_buf", "tx_buf (%)" },
    { 8, "LINK_NODE_STATUS", "rx_buf", "rx_buf (%)" },
    { 8, "LINK_NODE_STATUS", "tx_rate", "tx_rate (bytes/s)" },
    { 8, "LINK_NODE_STATUS", "rx_rate", "rx_rate (bytes/s)" },
    { 8, "LINK_NODE_STATUS", "rx_parse_err", "rx_parse_err (bytes)" },
    { 8, "LINK_NODE_STATUS", "tx_overflows", "tx_overflows (bytes)" },
    { 8, "LINK_NODE_STATUS", "rx_overflows", "rx_overflows (bytes)" },
    { 8, "LINK_NODE_STATUS", "messages_sent", "messages_sent" },
    { 8, "LINK_NODE_STATUS", "messages_received", "messages_received" },
    { 8, "LINK_NODE_STATUS", "messages_lost", "messages_lost" },
    { 22, "PARAM_VALUE", "param_value", "param_value" },
    { 22, "PARAM_VALUE", "param_type", "param_type" },
    { 22, "PARAM_VALUE", "param_count", "param_count" },
    { 22, "PARAM_VALUE", "param_index", "param_index" },
    { 24, "GPS_RAW_INT", "time_usec", "time_usec (us)" },
    { 24, "GPS_RAW_INT", "fix_type", "fix_type" },
    { 24, "GPS_RAW_INT", "lat", "lat (degE7)" },
    { 24, "GPS_RAW_INT", "lon", "lon (degE7)" },
    { 24, "GPS_RAW_INT", "alt", "alt (mm)" },
    { 24, "GPS_RAW_INT", "eph", "eph" },
    { 24, "GPS_RAW_INT", "epv", "epv" },
    { 24, "GPS_RAW_INT", "vel", "vel (cm/s)" },
    { 24, "GPS_RAW_INT", "cog", "cog (cdeg)" },
    { 24, "GPS_RAW_INT", "satellites_visible", "satellites_visible" },
    { 24, "GPS_RAW_INT", "alt_ellipsoid", "alt_ellipsoid (mm)" },
    { 24, "GPS_RAW_INT", "h_acc", "h_acc (mm)" },
    { 24, "GPS_RAW_INT", "v_acc", "v_acc (mm)" },
    { 24, "GPS_RAW_INT", "vel_acc", "vel_acc (mm/s)" },
    { 24, "GPS_RAW_INT", "hdg_acc", "hdg_acc (degE5)" },
    { 24, "GPS_RAW_INT", "yaw", "yaw (cdeg)" },
    { 25, "GPS_STATUS", "satellites_visible", "satellites_visible" },
    { 26, "SCALED_IMU", "time_boot_ms", "time_boot_ms (ms)" },
    { 26, "SCALED_IMU", "xacc", "xacc (mG)" },
    { 26, "SCALED_IMU", "yacc", "yacc (mG)" },
    { 26, "SCALED_IMU", "zacc", "zacc (mG)" },
    { 26, "SCALED_IMU", "xgyro", "xgyro (mrad/s)" },
    { 26, "SCALED_IMU", "ygyro", "ygyro (mrad/s)" },
    { 26, "SCALED_IMU", "zgyro", "zgyro (mrad/s)" },
    { 26, "SCALED_IMU", "xmag", "xmag (mgauss)" },
    { 26, "SCALED_IMU", "ymag", "ymag (mgauss)" },
    { 26, "SCALED_IMU", "zmag", "zmag (mgauss)" },
    { 26, "SCALED_IMU", "temperature", "temperature (cdegC)" },
    { 27, "RAW_IMU", "time_usec", "time_usec (us)" },
    { 27, "RAW_IMU", "xacc", "xacc" },
    { 27, "RAW_IMU", "yacc", "yacc" },
    { 27, "RAW_IMU", "zacc", "zacc" },
    { 27, "RAW_IMU", "xgyro", "xgyro" },
    { 27, "RAW_IMU", "ygyro", "ygyro" },
    { 27, "RAW_IMU", "zgyro", "zgyro" },
    { 27, "RAW_IMU", "xmag", "xmag" },
    { 27, "RAW_IMU", "ymag", "ymag" },
    { 27, "RAW_IMU", "zmag", "zmag" },
    { 27, "RAW_IMU", "id", "id" },
    { 27, "RAW_IMU", "temperature", "temperature (cdegC)" },
    { 28, "RAW_PRESSURE", "time_usec", "time_usec (us)" },
    { 28, "RAW_PRESSURE", "press_abs", "press_abs" },
    { 28, "RAW_PRESSURE", "press_diff1", "press_diff1" },
    { 28, "RAW_PRESSURE", "press_diff2", "press_diff2" },
    { 28, "RAW_PRESSURE", "temperature", "temperature" },
    { 29, "SCALED_PRESSURE", "time_boot_ms", "time_boot_ms (ms)" },
    { 29, "SCALED_PRESSURE", "press_abs", "press_abs (hPa)" },
    { 29, "SCALED_PRESSURE", "press_diff", "press_diff (hPa)" },
    { 29, "SCALED_PRESSURE", "temperature", "temperature (cdegC)" },
    { 29, "SCALED_PRESSURE", "temperature_press_diff", "temperature_press_diff (cdegC)" },
    { 30, "ATTITUDE", "time_boot_ms", "time_boot_ms (ms)" },
    { 30, "ATTITUDE", "roll", "roll (rad)" },
    { 30, "ATTITUDE", "pitch", "pitch (rad)" },
    { 30, "ATTITUDE", "yaw", "yaw (rad)" },
    { 30, "ATTITUDE", "rollspeed", "rollspeed (rad/s)" },
    { 30, "ATTITUDE", "pitchspeed", "pitchspeed (rad/s)" },
    { 30, "ATTITUDE", "yawspeed", "yawspeed (rad/s)" },
    { 31, "ATTITUDE_QUATERNION", "time_boot_ms", "time_boot_ms (ms)" },
    { 31, "ATTITUDE_QUATERNION", "q1", "q1" },
    { 31, "ATTITUDE_QUATERNION", "q2", "q2" },
    { 31, "ATTITUDE_QUATERNION", "q3", "q3" },
    { 31, "ATTITUDE_QUATERNION", "q4", "q4" },
    { 31, "ATTITUDE_QUATERNION", "rollspeed", "rollspeed (rad/s)" },
    { 31, "ATTITUDE_QUATERNION", "pitchspeed", "pitchspeed (rad/s)" },
    { 31, "ATTITUDE_QUATERNION", "yawspeed", "yawspeed (rad/s)" },
    { 31, "ATTITUDE_QUATERNION", "repr_offset_q[0]", "repr_offset_q[0]" },
    { 31, "ATTITUDE_QUATERNION", "repr_offset_q[1]", "repr_offset_q[1]" },
    { 31, "ATTITUDE_QUATERNION", "repr_offset_q[2]", "repr_offset_q[2]" },
    { 31, "ATTITUDE_QUATERNION", "repr_offset_q[3]", "repr_offset_q[3]" },
    { 32, "LOCAL_POSITION_NED", "time_boot_ms", "time_boot_ms (ms)" },
    { 32, "LOCAL_POSITION_NED", "x", "x (m)" },
    { 32, "LOCAL_POSITION_NED", "y", "y (m)" },
    { 32, "LOCAL_POSITION_NED", "z", "z (m)" },
    { 32, "LOCAL_POSITION_NED", "vx", "vx (m/s)" },
    { 32, "LOCAL_POSITION_NED", "vy", "vy (m/s)" },
    { 32, "LOCAL_POSITION_NED", "vz", "vz (m/s)" },
    { 33, "GLOBAL_POSITION_INT", "time_boot_ms", "time_boot_ms (ms)" },
    { 33, "GLOBAL_POSITION_INT", "lat", "lat (degE7)" },
    { 33, "GLOBAL_POSITION_INT", "lon", "lon (degE7)" },
    { 33, "GLOBAL_POSITION_INT", "alt", "alt (mm)" },
    { 33, "GLOBAL_POSITION_INT", "relative_alt", "relative_alt (mm)" },
    { 33, "GLOBAL_POSITION_INT", "vx", "vx (cm/s)" },
    { 33, "GLOBAL_POSITION_INT", "vy", "vy (cm/s)" },
    { 33, "GLOBAL_POSITION_INT", "vz", "vz (cm/s)" },
    { 33, "GLOBAL_POSITION_INT", "hdg", "hdg (cdeg)" },
    { 34, "RC_CHANNELS_SCALED", "time_boot_ms", "time_boot_ms (ms)" },
    { 34, "RC_CHANNELS_SCALED", "port", "port" },
    { 34, "RC_CHANNELS_SCALED", "chan1_scaled", "chan1_scaled" },
    { 34, "RC_CHANNELS_SCALED", "chan2_scaled", "chan2_scaled" },
    { 34, "RC_CHANNELS_SCALED", "chan3_scaled", "chan3_scaled" },
    { 34, "RC_CHANNELS_SCALED", "chan4_scaled", "chan4_scaled" },
    { 34, "RC_CHANNELS_SCALED", "chan5_scaled", "chan5_scaled" },
    { 34, "RC_CHANNELS_SCALED", "chan6_scaled", "chan6_scaled" },
    { 34, "RC_CHANNELS_SCALED", "chan7_scaled", "chan7_scaled" },
    { 34, "RC_CHANNELS_SCALED", "chan8_scaled", "chan8_scaled" },
    { 34, "RC_CHANNELS_SCALED", "rssi", "rssi" },
    { 35, "RC_CHANNELS_RAW", "time_boot_ms", "time_boot_ms (ms)" },
    { 35, "RC_CHANNELS_RAW", "port", "port" },
    { 35, "RC_CHANNELS_RAW", "chan1_raw", "chan1_raw (us)" },
    { 35, "RC_CHANNELS_RAW", "chan2_raw", "chan2_raw (us)" },
    { 35, "RC_CHANNELS_RAW", "chan3_raw", "chan3_raw (us)" },
    { 35, "RC_CHANNELS_RAW", "chan4_raw", "chan4_raw (us)" },
    { 35, "RC_CHANNELS_RAW", "chan5_raw", "chan5_raw (us)" },
    { 35, "RC_CHANNELS_RAW", "chan6_raw", "chan6_raw (us)" },
    { 35, "RC_CHANNELS_RAW", "chan7_raw", "chan7_raw (us)" },
    { 35, "RC_CHANNELS_RAW", "chan8_raw", "chan8_raw (us)" },
    { 35, "RC_CHANNELS_RAW", "rssi", "rssi" },
    { 36, "SERVO_OUTPUT_RAW", "time_usec", "time_usec (us)" },
    { 36, "SERVO_OUTPUT_RAW", "port", "port" },
    { 36, "SERVO_OUTPUT_RAW", "servo1_raw", "servo1_raw (us)" },
    { 36, "SERVO_OUTPUT_RAW", "servo2_raw", "servo2_raw (us)" },
    { 36, "SERVO_OUTPUT_RAW", "servo3_raw", "servo3_raw (us)" },
    { 36, "SERVO_OUTPUT_RAW", "servo4_raw", "servo4_raw (us)" },
    { 36, "SERVO_OUTPUT_RAW", "servo5_raw", "servo5_raw (us)" },
    { 36, "SERVO_OUTPUT_RAW", "servo6_raw", "servo6_raw (us)" },
    { 36, "SERVO_OUTPUT_RAW", "servo7_raw", "servo7_raw (us)" },
    { 36, "SERVO_OUTPUT_RAW", "servo8_raw", "servo8_raw (us)" },
    { 36, "SERVO_OUTPUT_RAW", "servo9_raw", "servo9_raw (us)" },
    { 36, "SERVO_OUTPUT_RAW", "servo10_raw", "servo10_raw (us)" },
    { 36, "SERVO_OUTPUT_RAW", "servo11_raw", "servo11_raw (us)" },
    { 36, "SERVO_OUTPUT_RAW", "servo12_raw", "servo12_raw (us)" },
    { 36, "SERVO_OUTPUT_RAW", "servo13_raw", "servo13_raw (us)" },
    { 36, "SERVO_OUTPUT_RAW", "servo14_raw", "servo14_raw (us)" },
    { 36, "SERVO_OUTPUT_RAW", "servo15_raw", "servo15_raw (us)" },
    { 36, "SERVO_OUTPUT_RAW", "servo16_raw", "servo16_raw (us)" },
    { 42, "MISSION_CURRENT", "seq", "seq" },
    { 42, "MISSION_CURRENT", "total", "total" },
    { 42, "MISSION_CURRENT", "mission_state", "mission_state" },
    { 42, "MISSION_CURRENT", "mission_mode", "mission_mode" },
    { 42, "MISSION_CURRENT", "mission_id", "mission_id" },
    { 42, "MISSION_CURRENT", "fence_id", "fence_id" },
    { 42, "MISSION_CURRENT", "rally_points_id", "rally_points_id" },
    { 44, "MISSION_COUNT", "target_system", "target_system" },
    { 44, "MISSION_COUNT", "target_component", "target_component" },
    { 44, "MISSION_COUNT", "count", "count" },
    { 44, "MISSION_COUNT", "mission_type", "mission_type" },
    { 44, "MISSION_COUNT", "opaque_id", "opaque_id" },
    { 46, "MISSION_ITEM_REACHED", "seq", "seq" },
    { 47, "MISSION_ACK", "target_system", "target_system" },
    { 47, "MISSION_ACK", "target_component", "target_component" },
    { 47, "MISSION_ACK", "type", "type" },
    { 47, "MISSION_ACK", "mission_type", "mission_type" },
    { 47, "MISSION_ACK", "opaque_id", "opaque_id" },
    { 49, "GPS_GLOBAL_ORIGIN", "latitude", "latitude (degE7)" },
    { 49, "GPS_GLOBAL_ORIGIN", "longitude", "longitude (degE7)" },
    { 49, "GPS_GLOBAL_ORIGIN", "altitude", "altitude (mm)" },
    { 49, "GPS_GLOBAL_ORIGIN", "time_usec", "time_usec (us)" },
    { 55, "SAFETY_ALLOWED_AREA", "frame", "frame" },
    { 55, "SAFETY_ALLOWED_AREA", "p1x", "p1x (m)" },
    { 55, "SAFETY_ALLOWED_AREA", "p1y", "p1y (m)" },
    { 55, "SAFETY_ALLOWED_AREA", "p1z", "p1z (m)" },
    { 55, "SAFETY_ALLOWED_AREA", "p2x", "p2x (m)" },
    { 55, "SAFETY_ALLOWED_AREA", "p2y", "p2y (m)" },
    { 55, "SAFETY_ALLOWED_AREA", "p2z", "p2z (m)" },
    { 61, "ATTITUDE_QUATERNION_COV", "time_usec", "time_usec (us)" },
    { 61, "ATTITUDE_QUATERNION_COV", "q[0]", "q[0]" },
    { 61, "ATTITUDE_QUATERNION_COV", "q[1]", "q[1]" },
    { 61, "ATTITUDE_QUATERNION_COV", "q[2]", "q[2]" },
    { 61, "ATTITUDE_QUATERNION_COV", "q[3]", "q[3]" },
    { 61, "ATTITUDE_QUATERNION_COV", "rollspeed", "rollspeed (rad/s)" },
    { 61, "ATTITUDE_QUATERNION_COV", "pitchspeed", "pitchspeed (rad/s)" },
    { 61, "ATTITUDE_QUATERNION_COV", "yawspeed", "yawspeed (rad/s)" },
    { 62, "NAV_CONTROLLER_OUTPUT", "nav_roll", "nav_roll (deg)" },
    { 62, "NAV_CONTROLLER_OUTPUT", "nav_pitch", "nav_pitch (deg)" },
    { 62, "NAV_CONTROLLER_OUTPUT", "nav_bearing", "nav_bearing (deg)" },
    { 62, "NAV_CONTROLLER_OUTPUT", "target_bearing", "target_bearing (deg)" },
    { 62, "NAV_CONTROLLER_OUTPUT", "wp_dist", "wp_dist (m)" },
    { 62, "NAV_CONTROLLER_OUTPUT", "alt_error", "alt_error (m)" },
    { 62, "NAV_CONTROLLER_OUTPUT", "aspd_error", "aspd_error (m/s)" },
    { 62, "NAV_CONTROLLER_OUTPUT", "xtrack_error", "xtrack_error (m)" },
    { 63, "GLOBAL_POSITION_INT_COV", "time_usec", "time_usec (us)" },
    { 63, "GLOBAL_POSITION_INT_COV", "estimator_type", "estimator_type" },
    { 63, "GLOBAL_POSITION_INT_COV", "lat", "lat (degE7)" },
    { 63, "GLOBAL_POSITION_INT_COV", "lon", "lon (degE7)" },
    { 63, "GLOBAL_POSITION_INT_COV", "alt", "alt (mm)" },
    { 63, "GLOBAL_POSITION_INT_COV", "relative_alt", "relative_alt (mm)" },
    { 63, "GLOBAL_POSITION_INT_COV", "vx", "vx (m/s)" },
    { 63, "GLOBAL_POSITION_INT_COV", "vy", "vy (m/s)" },
    { 63, "GLOBAL_POSITION_INT_COV", "vz", "vz (m/s)" },
    { 64, "LOCAL_POSITION_NED_COV", "time_usec", "time_usec (us)" },
    { 64, "LOCAL_POSITION_NED_COV", "estimator_type", "estimator_type" },
    { 64, "LOCAL_POSITION_NED_COV", "x", "x (m)" },
    { 64, "LOCAL_POSITION_NED_COV", "y", "y (m)" },
    { 64, "LOCAL_POSITION_NED_COV", "z", "z (m)" },
    { 64, "LOCAL_POSITION_NED_COV", "vx", "vx (m/s)" },
    { 64, "LOCAL_POSITION_NED_COV", "vy", "vy (m/s)" },
    { 64, "LOCAL_POSITION_NED_COV", "vz", "vz (m/s)" },
    { 64, "LOCAL_POSITION_NED_COV", "ax", "ax (m/s/s)" },
    { 64, "LOCAL_POSITION_NED_COV", "ay", "ay (m/s/s)" },
    { 64, "LOCAL_POSITION_NED_COV", "az", "az (m/s/s)" },
    { 65, "RC_CHANNELS", "time_boot_ms", "time_boot_ms (ms)" },
    { 65, "RC_CHANNELS", "chancount", "chancount" },
    { 65, "RC_CHANNELS", "chan1_raw", "chan1_raw (us)" },
    { 65, "RC_CHANNELS", "chan2_raw", "chan2_raw (us)" },
    { 65, "RC_CHANNELS", "chan3_raw", "chan3_raw (us)" },
    { 65, "RC_CHANNELS", "chan4_raw", "chan4_raw (us)" },
    { 65, "RC_CHANNELS", "chan5_raw", "chan5_raw (us)" },
    { 65, "RC_CHANNELS", "chan6_raw", "chan6_raw (us)" },
    { 65, "RC_CHANNELS", "chan7_raw", "chan7_raw (us)" },
    { 65, "RC_CHANNELS", "chan8_raw", "chan8_raw (us)" },
    { 65, "RC_CHANNELS", "chan9_raw", "chan9_raw (us)" },
    { 65, "RC_CHANNELS", "chan10_raw", "chan10_raw (us)" },
    { 65, "RC_CHANNELS", "chan11_raw", "chan11_raw (us)" },
    { 65, "RC_CHANNELS", "chan12_raw", "chan12_raw (us)" },
    { 65, "RC_CHANNELS", "chan13_raw", "chan13_raw (us)" },
    { 65, "RC_CHANNELS", "chan14_raw", "chan14_raw (us)" },
    { 65, "RC_CHANNELS", "chan15_raw", "chan15_raw (us)" },
    { 65, "RC_CHANNELS", "chan16_raw", "chan16_raw (us)" },
    { 65, "RC_CHANNELS", "chan17_raw", "chan17_raw (us)" },
    { 65, "RC_CHANNELS", "chan18_raw", "chan18_raw (us)" },
    { 65, "RC_CHANNELS", "rssi", "rssi" },
    { 67, "DATA_STREAM", "stream_id", "stream_id" },
    { 67, "DATA_STREAM", "message_rate", "message_rate (Hz)" },
    { 67, "DATA_STREAM", "on_off", "on_off" },
    { 74, "VFR_HUD", "airspeed", "airspeed (m/s)" },
    { 74, "VFR_HUD", "groundspeed", "groundspeed (m/s)" },
    { 74, "VFR_HUD", "heading", "heading (deg)" },
    { 74, "VFR_HUD", "throttle", "throttle (%)" },
    { 74, "VFR_HUD", "alt", "alt (m)" },
    { 74, "VFR_HUD", "climb", "climb (m/s)" },
    { 77, "COMMAND_ACK", "command", "command" },
    { 77, "COMMAND_ACK", "result", "result" },
    { 77, "COMMAND_ACK", "progress", "progress (%)" },
    { 77, "COMMAND_ACK", "result_param2", "result_param2" },
    { 77, "COMMAND_ACK", "target_system", "target_system" },
    { 77, "COMMAND_ACK", "target_component", "target_component" },
    { 83, "ATTITUDE_TARGET", "time_boot_ms", "time_boot_ms (ms)" },
    { 83, "ATTITUDE_TARGET", "type_mask", "type_mask" },
    { 83, "ATTITUDE_TARGET", "q[0]", "q[0]" },
    { 83, "ATTITUDE_TARGET", "q[1]", "q[1]" },
    { 83, "ATTITUDE_TARGET", "q[2]", "q[2]" },
    { 83, "ATTITUDE_TARGET", "q[3]", "q[3]" },
    { 83, "ATTITUDE_TARGET", "body_roll_rate", "body_roll_rate (rad/s)" },
    { 83, "ATTITUDE_TARGET", "body_pitch_rate", "body_pitch_rate (rad/s)" },
    { 83, "ATTITUDE_TARGET", "body_yaw_rate", "body_yaw_rate (rad/s)" },
    { 83, "ATTITUDE_TARGET", "thrust", "thrust" },
    { 85, "POSITION_TARGET_LOCAL_NED", "time_boot_ms", "time_boot_ms (ms)" },
    { 85, "POSITION_TARGET_LOCAL_NED", "coordinate_frame", "coordinate_frame" },
    { 85, "POSITION_TARGET_LOCAL_NED", "type_mask", "type_mask" },
    { 85, "POSITION_TARGET_LOCAL_NED", "x", "x (m)" },
    { 85, "POSITION_TARGET_LOCAL_NED", "y", "y (m)" },
    { 85, "POSITION_TARGET_LOCAL_NED", "z", "z (m)" },
    { 85, "POSITION_TARGET_LOCAL_NED", "vx", "vx (m/s)" },
    { 85, "POSITION_TARGET_LOCAL_NED", "vy", "vy (m/s)" },
    { 85, "POSITION_TARGET_LOCAL_NED", "vz", "vz (m/s)" },
    { 85, "POSITION_TARGET_LOCAL_NED", "afx", "afx (m/s/s)" },
    { 85, "POSITION_TARGET_LOCAL_NED", "afy", "afy (m/s/s)" },
    { 85, "POSITION_TARGET_LOCAL_NED", "afz", "afz (m/s/s)" },
    { 85, "POSITION_TARGET_LOCAL_NED", "yaw", "yaw (rad)" },
    { 85, "POSITION_TARGET_LOCAL_NED", "yaw_rate", "yaw_rate (rad/s)" },
    { 87, "POSITION_TARGET_GLOBAL_INT", "time_boot_ms", "time_boot_ms (ms)" },
    { 87, "POSITION_TARGET_GLOBAL_INT", "coordinate_frame", "coordinate_frame" },
    { 87, "POSITION_TARGET_GLOBAL_INT", "type_mask", "type_mask" },
    { 87, "POSITION_TARGET_GLOBAL_INT", "lat_int", "lat_int (degE7)" },
    { 87, "POSITION_TARGET_GLOBAL_INT", "lon_int", "lon_int (degE7)" },
    { 87, "POSITION_TARGET_GLOBAL_INT", "alt", "alt (m)" },
    { 87, "POSITION_TARGET_GLOBAL_INT", "vx", "vx (m/s)" },
    { 87, "POSITION_TARGET_GLOBAL_INT", "vy", "vy (m/s)" },
    { 87, "POSITION_TARGET_GLOBAL_INT", "vz", "vz (m/s)" },
    { 87, "POSITION_TARGET_GLOBAL_INT", "afx", "afx (m/s/s)" },
    { 87, "POSITION_TARGET_GLOBAL_INT", "afy", "afy (m/s/s)" },
    { 87, "POSITION_TARGET_GLOBAL_INT", "afz", "afz (m/s/s)" },
    { 87, "POSITION_TARGET_GLOBAL_INT", "yaw", "yaw (rad)" },
    { 87, "POSITION_TARGET_GLOBAL_INT", "yaw_rate", "yaw_rate (rad/s)" },
    { 89, "LOCAL_POSITION_NED_SYSTEM_GLOBAL_OFFSET", "time_boot_ms", "time_boot_ms (ms)" },
    { 89, "LOCAL_POSITION_NED_SYSTEM_GLOBAL_OFFSET", "x", "x (m)" },
    { 89, "LOCAL_POSITION_NED_SYSTEM_GLOBAL_OFFSET", "y", "y (m)" },
    { 89, "LOCAL_POSITION_NED_SYSTEM_GLOBAL_OFFSET", "z", "z (m)" },
    { 89, "LOCAL_POSITION_NED_SYSTEM_GLOBAL_OFFSET", "roll", "roll (rad)" },
    { 89, "LOCAL_POSITION_NED_SYSTEM_GLOBAL_OFFSET", "pitch", "pitch (rad)" },
    { 89, "LOCAL_POSITION_NED_SYSTEM_GLOBAL_OFFSET", "yaw", "yaw (rad)" },
    { 100, "OPTICAL_FLOW", "time_usec", "time_usec (us)" },
    { 100, "OPTICAL_FLOW", "sensor_id", "sensor_id" },
    { 100, "OPTICAL_FLOW", "flow_x", "flow_x (dpix)" },
    { 100, "OPTICAL_FLOW", "flow_y", "flow_y (dpix)" },
    { 100, "OPTICAL_FLOW", "flow_comp_m_x", "flow_comp_m_x (m/s)" },
    { 100, "OPTICAL_FLOW", "flow_comp_m_y", "flow_comp_m_y (m/s)" },
    { 100, "OPTICAL_FLOW", "quality", "quality" },
    { 100, "OPTICAL_FLOW", "ground_distance", "ground_distance (m)" },
    { 100, "OPTICAL_FLOW", "flow_rate_x", "flow_rate_x (rad/s)" },
    { 100, "OPTICAL_FLOW", "flow_rate_y", "flow_rate_y (rad/s)" },
    { 101, "GLOBAL_VISION_POSITION_ESTIMATE", "usec", "usec (us)" },
    { 101, "GLOBAL_VISION_POSITION_ESTIMATE", "x", "x (m)" },
    { 101, "GLOBAL_VISION_POSITION_ESTIMATE", "y", "y (m)" },
    { 101, "GLOBAL_VISION_POSITION_ESTIMATE", "z", "z (m)" },
    { 101, "GLOBAL_VISION_POSITION_ESTIMATE", "roll", "roll (rad)" },
    { 101, "GLOBAL_VISION_POSITION_ESTIMATE", "pitch", "pitch (rad)" },
    { 101, "GLOBAL_VISION_POSITION_ESTIMATE", "yaw", "yaw (rad)" },
    { 101, "GLOBAL_VISION_POSITION_ESTIMATE", "reset_counter", "reset_counter" },
    { 102, "VISION_POSITION_ESTIMATE", "usec", "usec (us)" },
    { 102, "VISION_POSITION_ESTIMATE", "x", "x (m)" },
    { 102, "VISION_POSITION_ESTIMATE", "y", "y (m)" },
    { 102, "VISION_POSITION_ESTIMATE", "z", "z (m)" },
    { 102, "VISION_POSITION_ESTIMATE", "roll", "roll (rad)" },
    { 102, "VISION_POSITION_ESTIMATE", "pitch", "pitch (rad)" },
    { 102, "VISION_POSITION_ESTIMATE", "yaw", "yaw (rad)" },
    { 102, "VISION_POSITION_ESTIMATE", "reset_counter", "reset_counter" },
    { 103, "VISION_SPEED_ESTIMATE", "usec", "usec (us)" },
    { 103, "VISION_SPEED_ESTIMATE", "x", "x (m/s)" },
    { 103, "VISION_SPEED_ESTIMATE", "y", "y (m/s)" },
    { 103, "VISION_SPEED_ESTIMATE", "z", "z (m/s)" },
    { 103, "VISION_SPEED_ESTIMATE", "reset_counter", "reset_counter" },
    { 104, "VICON_POSITION_ESTIMATE", "usec", "usec (us)" },
    { 104, "VICON_POSITION_ESTIMATE", "x", "x (m)" },
    { 104, "VICON_POSITION_ESTIMATE", "y", "y (m)" },
    { 104, "VICON_POSITION_ESTIMATE", "z", "z (m)" },
    { 104, "VICON_POSITION_ESTIMATE", "roll", "roll (rad)" },
    { 104, "VICON_POSITION_ESTIMATE", "pitch", "pitch (rad)" },
    { 104, "VICON_POSITION_ESTIMATE", "yaw", "yaw (rad)" },
    { 105, "HIGHRES_IMU", "time_usec", "time_usec (us)" },
    { 105, "HIGHRES_IMU", "xacc", "xacc (m/s/s)" },
    { 105, "HIGHRES_IMU", "yacc", "yacc (m/s/s)" },
    { 105, "HIGHRES_IMU", "zacc", "zacc (m/s/s)" },
    { 105, "HIGHRES_IMU", "xgyro", "xgyro (rad/s)" },
    { 105, "HIGHRES_IMU", "ygyro", "ygyro (rad/s)" },
    { 105, "HIGHRES_IMU", "zgyro", "zgyro (rad/s)" },
    { 105, "HIGHRES_IMU", "xmag", "xmag (gauss)" },
    { 105, "HIGHRES_IMU", "ymag", "ymag (gauss)" },
    { 105, "HIGHRES_IMU", "zmag", "zmag (gauss)" },
    { 105, "HIGHRES_IMU", "abs_pressure", "abs_pressure (hPa)" },
    { 105, "HIGHRES_IMU", "diff_pressure", "diff_pressure (hPa)" },
    { 105, "HIGHRES_IMU", "pressure_alt", "pressure_alt" },
    { 105, "HIGHRES_IMU", "temperature", "temperature (degC)" },
    { 105, "HIGHRES_IMU", "fields_updated", "fields_updated" },
    { 105, "HIGHRES_IMU", "id", "id" },
    { 106, "OPTICAL_FLOW_RAD", "time_usec", "time_usec (us)" },
    { 106, "OPTICAL_FLOW_RAD", "sensor_id", "sensor_id" },
    { 106, "OPTICAL_FLOW_RAD", "integration_time_us", "integration_time_us (us)" },
    { 106, "OPTICAL_FLOW_RAD", "integrated_x", "integrated_x (rad)" },
    { 106, "OPTICAL_FLOW_RAD", "integrated_y", "integrated_y (rad)" },
    { 106, "OPTICAL_FLOW_RAD", "integrated_xgyro", "integrated_xgyro (rad)" },
    { 106, "OPTICAL_FLOW_RAD", "integrated_ygyro", "integrated_ygyro (rad)" },
    { 106, "OPTICAL_FLOW_RAD", "integrated_zgyro", "integrated_zgyro (rad)" },
    { 106, "OPTICAL_FLOW_RAD", "temperature", "temperature (cdegC)" },
    { 106, "OPTICAL_FLOW_RAD", "quality", "quality" },
    { 106, "OPTICAL_FLOW_RAD", "time_delta_distance_us", "time_delta_distance_us (us)" },
    { 106, "OPTICAL_FLOW_RAD", "distance", "distance (m)" },
    { 109, "RADIO_STATUS", "rssi", "rssi" },
    { 109, "RADIO_STATUS", "remrssi", "remrssi" },
    { 109, "RADIO_STATUS", "txbuf", "txbuf (%)" },
    { 109, "RADIO_STATUS", "noise", "noise" },
    { 109, "RADIO_STATUS", "remnoise", "remnoise" },
    { 109, "RADIO_STATUS", "rxerrors", "rxerrors" },
    { 109, "RADIO_STATUS", "fixed", "fixed" },
    { 110, "FILE_TRANSFER_PROTOCOL", "target_network", "target_network" },
    { 110, "FILE_TRANSFER_PROTOCOL", "target_system", "target_system" },
    { 110, "FILE_TRANSFER_PROTOCOL", "target_component", "target_component" },
    { 111, "TIMESYNC", "tc1", "tc1 (ns)" },
    { 111, "TIMESYNC", "ts1", "ts1 (ns)" },
    { 111, "TIMESYNC", "target_system", "target_system" },
    { 111, "TIMESYNC", "target_component", "target_component" },
    { 112, "CAMERA_TRIGGER", "time_usec", "time_usec (us)" },
    { 112, "CAMERA_TRIGGER", "seq", "seq" },
    { 116, "SCALED_IMU2", "time_boot_ms", "time_boot_ms (ms)" },
    { 116, "SCALED_IMU2", "xacc", "xacc (mG)" },
    { 116, "SCALED_IMU2", "yacc", "yacc (mG)" },
    { 116, "SCALED_IMU2", "zacc", "zacc (mG)" },
    { 116, "SCALED_IMU2", "xgyro", "xgyro (mrad/s)" },
    { 116, "SCALED_IMU2", "ygyro", "ygyro (mrad/s)" },
    { 116, "SCALED_IMU2", "zgyro", "zgyro (mrad/s)" },
    { 116, "SCALED_IMU2", "xmag", "xmag (mgauss)" },
    { 116, "SCALED_IMU2", "ymag", "ymag (mgauss)" },
    { 116, "SCALED_IMU2", "zmag", "zmag (mgauss)" },
    { 116, "SCALED_IMU2", "temperature", "temperature (cdegC)" },
    { 118, "LOG_ENTRY", "id", "id" },
    { 118, "LOG_ENTRY", "num_logs", "num_logs" },
    { 118, "LOG_ENTRY", "last_log_num", "last_log_num" },
    { 118, "LOG_ENTRY", "time_utc", "time_utc (s)" },
    { 118, "LOG_ENTRY", "size", "size (bytes)" },
    { 120, "LOG_DATA", "id", "id" },
    { 120, "LOG_DATA", "ofs", "ofs" },
    { 120, "LOG_DATA", "count", "count (bytes)" },
    { 124, "GPS2_RAW", "time_usec", "time_usec (us)" },
    { 124, "GPS2_RAW", "fix_type", "fix_type" },
    { 124, "GPS2_RAW", "lat", "lat (degE7)" },
    { 124, "GPS2_RAW", "lon", "lon (degE7)" },
    { 124, "GPS2_RAW", "alt", "alt (mm)" },
    { 124, "GPS2_RAW", "eph", "eph" },
    { 124, "GPS2_RAW", "epv", "epv" },
    { 124, "GPS2_RAW", "vel", "vel (cm/s)" },
    { 124, "GPS2_RAW", "cog", "cog (cdeg)" },
    { 124, "GPS2_RAW", "satellites_visible", "satellites_visible" },
    { 124, "GPS2_RAW", "dgps_numch", "dgps_numch" },
    { 124, "GPS2_RAW", "dgps_age", "dgps_age (ms)" },
    { 124, "GPS2_RAW", "yaw", "yaw (cdeg)" },
    { 124, "GPS2_RAW", "alt_ellipsoid", "alt_ellipsoid (mm)" },
    { 124, "GPS2_RAW", "h_acc", "h_acc (mm)" },
    { 124, "GPS2_RAW", "v_acc", "v_acc (mm)" },
    { 124, "GPS2_RAW", "vel_acc", "vel_acc (mm/s)" },
    { 124, "GPS2_RAW", "hdg_acc", "hdg_acc (degE5)" },
    { 125, "POWER_STATUS", "Vcc", "Vcc (mV)" },
    { 125, "POWER_STATUS", "Vservo", "Vservo (mV)" },
    { 125, "POWER_STATUS", "flags", "flags" },
    { 127, "GPS_RTK", "time_last_baseline_ms", "time_last_baseline_ms (ms)" },
    { 127, "GPS_RTK", "rtk_receiver_id", "rtk_receiver_id" },
    { 127, "GPS_RTK", "wn", "wn" },
    { 127, "GPS_RTK", "tow", "tow (ms)" },
    { 127, "GPS_RTK", "rtk_health", "rtk_health" },
    { 127, "GPS_RTK", "rtk_rate", "rtk_rate (Hz)" },
    { 127, "GPS_RTK", "nsats", "nsats" },
    { 127, "GPS_RTK", "baseline_coords_type", "baseline_coords_type" },
    { 127, "GPS_RTK", "baseline_a_mm", "baseline_a_mm (mm)" },
    { 127, "GPS_RTK", "baseline_b_mm", "baseline_b_mm (mm)" },
    { 127, "GPS_RTK", "baseline_c_mm", "baseline_c_mm (mm)" },
    { 127, "GPS_RTK", "accuracy", "accuracy" },
    { 127, "GPS_RTK", "iar_num_hypotheses", "iar_num_hypotheses" },
    { 128, "GPS2_RTK", "time_last_baseline_ms", "time_last_baseline_ms (ms)" },
    { 128, "GPS2_RTK", "rtk_receiver_id", "rtk_receiver_id" },
    { 128, "GPS2_RTK", "wn", "wn" },
    { 128, "GPS2_RTK", "tow", "tow (ms)" },
    { 128, "GPS2_RTK", "rtk_health", "rtk_health" },
    { 128, "GPS2_RTK", "rtk_rate", "rtk_rate (Hz)" },
    { 128, "GPS2_RTK", "nsats", "nsats" },
    { 128, "GPS2_RTK", "baseline_coords_type", "baseline_coords_type" },
    { 128, "GPS2_RTK", "baseline_a_mm", "baseline_a_mm (mm)" },
    { 128, "GPS2_RTK", "baseline_b_mm", "baseline_b_mm (mm)" },
    { 128, "GPS2_RTK", "baseline_c_mm", "baseline_c_mm (mm)" },
    { 128, "GPS2_RTK", "accuracy", "accuracy" },
    { 128, "GPS2_RTK", "iar_num_hypotheses", "iar_num_hypotheses" },
    { 129, "SCALED_IMU3", "time_boot_ms", "time_boot_ms (ms)" },
    { 129, "SCALED_IMU3", "xacc", "xacc (mG)" },
    { 129, "SCALED_IMU3", "yacc", "yacc (mG)" },
    { 129, "SCALED_IMU3", "zacc", "zacc (mG)" },
    { 129, "SCALED_IMU3", "xgyro", "xgyro (mrad/s)" },
    { 129, "SCALED_IMU3", "ygyro", "ygyro (mrad/s)" },
    { 129, "SCALED_IMU3", "zgyro", "zgyro (mrad/s)" },
    { 129, "SCALED_IMU3", "xmag", "xmag (mgauss)" },
    { 129, "SCALED_IMU3", "ymag", "ymag (mgauss)" },
    { 129, "SCALED_IMU3", "zmag", "zmag (mgauss)" },
    { 129, "SCALED_IMU3", "temperature", "temperature (cdegC)" },
    { 130, "DATA_TRANSMISSION_HANDSHAKE", "type", "type" },
    { 130, "DATA_TRANSMISSION_HANDSHAKE", "size", "size (bytes)" },
    { 130, "DATA_TRANSMISSION_HANDSHAKE", "width", "width" },
    { 130, "DATA_TRANSMISSION_HANDSHAKE", "height", "height" },
    { 130, "DATA_TRANSMISSION_HANDSHAKE", "packets", "packets" },
    { 130, "DATA_TRANSMISSION_HANDSHAKE", "payload", "payload (bytes)" },
    { 130, "DATA_TRANSMISSION_HANDSHAKE", "jpg_quality", "jpg_quality (%)" },
    { 131, "ENCAPSULATED_DATA", "seqnr", "seqnr" },
    { 132, "DISTANCE_SENSOR", "time_boot_ms", "time_boot_ms (ms)" },
    { 132, "DISTANCE_SENSOR", "min_distance", "min_distance (cm)" },
    { 132, "DISTANCE_SENSOR", "max_distance", "max_distance (cm)" },
    { 132, "DISTANCE_SENSOR", "current_distance", "current_distance (cm)" },
    { 132, "DISTANCE_SENSOR", "type", "type" },
    { 132, "DISTANCE_SENSOR", "id", "id" },
    { 132, "DISTANCE_SENSOR", "orientation", "orientation" },
    { 132, "DISTANCE_SENSOR", "covariance", "covariance (cm^2)" },
    { 132, "DISTANCE_SENSOR", "horizontal_fov", "horizontal_fov (rad)" },
    { 132, "DISTANCE_SENSOR", "vertical_fov", "vertical_fov (rad)" },
    { 132, "DISTANCE_SENSOR", "quaternion[0]", "quaternion[0]" },
    { 132, "DISTANCE_SENSOR", "quaternion[1]", "quaternion[1]" },
    { 132, "DISTANCE_SENSOR", "quaternion[2]", "quaternion[2]" },
    { 132, "DISTANCE_SENSOR", "quaternion[3]", "quaternion[3]" },
    { 132, "DISTANCE_SENSOR", "signal_quality", "signal_quality (%)" },
    { 133, "TERRAIN_REQUEST", "lat", "lat (degE7)" },
    { 133, "TERRAIN_REQUEST", "lon", "lon (degE7)" },
    { 133, "TERRAIN_REQUEST", "grid_spacing", "grid_spacing (m)" },
    { 133, "TERRAIN_REQUEST", "mask", "mask" },
    { 134, "TERRAIN_DATA", "lat", "lat (degE7)" },
    { 134, "TERRAIN_DATA", "lon", "lon (degE7)" },
    { 134, "TERRAIN_DATA", "grid_spacing", "grid_spacing (m)" },
    { 134, "TERRAIN_DATA", "gridbit", "gridbit" },
    { 135, "TERRAIN_CHECK", "lat", "lat (degE7)" },
    { 135, "TERRAIN_CHECK", "lon", "lon (degE7)" },
    { 136, "TERRAIN_REPORT", "lat", "lat (degE7)" },
    { 136, "TERRAIN_REPORT", "lon", "lon (degE7)" },
    { 136, "TERRAIN_REPORT", "spacing", "spacing" },
    { 136, "TERRAIN_REPORT", "terrain_height", "terrain_height (m)" },
    { 136, "TERRAIN_REPORT", "current_height", "current_height (m)" },
    { 136, "TERRAIN_REPORT", "pending", "pending" },
    { 136, "TERRAIN_REPORT", "loaded", "loaded" },
    { 137, "SCALED_PRESSURE2", "time_boot_ms", "time_boot_ms (ms)" },
    { 137, "SCALED_PRESSURE2", "press_abs", "press_abs (hPa)" },
    { 137, "SCALED_PRESSURE2", "press_diff", "press_diff (hPa)" },
    { 137, "SCALED_PRESSURE2", "temperature", "temperature (cdegC)" },
    { 137, "SCALED_PRESSURE2", "temperature_press_diff", "temperature_press_diff (cdegC)" },
    { 138, "ATT_POS_MOCAP", "time_usec", "time_usec (us)" },
    { 138, "ATT_POS_MOCAP", "q[0]", "q[0]" },
    { 138, "ATT_POS_MOCAP", "q[1]", "q[1]" },
    { 138, "ATT_POS_MOCAP", "q[2]", "q[2]" },
    { 138, "ATT_POS_MOCAP", "q[3]", "q[3]" },
    { 138, "ATT_POS_MOCAP", "x", "x (m)" },
    { 138, "ATT_POS_MOCAP", "y", "y (m)" },
    { 138, "ATT_POS_MOCAP", "z", "z (m)" },
    { 140, "ACTUATOR_CONTROL_TARGET", "time_usec", "time_usec (us)" },
    { 140, "ACTUATOR_CONTROL_TARGET", "group_mlx", "group_mlx" },
    { 141, "ALTITUDE", "time_usec", "time_usec (us)" },
    { 141, "ALTITUDE", "altitude_monotonic", "altitude_monotonic (m)" },
    { 141, "ALTITUDE", "altitude_amsl", "altitude_amsl (m)" },
    { 141, "ALTITUDE", "altitude_local", "altitude_local (m)" },
    { 141, "ALTITUDE", "altitude_relative", "altitude_relative (m)" },
    { 141, "ALTITUDE", "altitude_terrain", "altitude_terrain (m)" },
    { 141, "ALTITUDE", "bottom_clearance", "bottom_clearance (m)" },
    { 143, "SCALED_PRESSURE3", "time_boot_ms", "time_boot_ms (ms)" },
    { 143, "SCALED_PRESSURE3", "press_abs", "press_abs (hPa)" },
    { 143, "SCALED_PRESSURE3", "press_diff", "press_diff (hPa)" },
    { 143, "SCALED_PRESSURE3", "temperature", "temperature (cdegC)" },
    { 143, "SCALED_PRESSURE3", "temperature_press_diff", "temperature_press_diff (cdegC)" },
    { 144, "FOLLOW_TARGET", "timestamp", "timestamp (ms)" },
    { 144, "FOLLOW_TARGET", "est_capabilities", "est_capabilities" },
    { 144, "FOLLOW_TARGET", "lat", "lat (degE7)" },
    { 144, "FOLLOW_TARGET", "lon", "lon (degE7)" },
    { 144, "FOLLOW_TARGET", "alt", "alt (m)" },
    { 144, "FOLLOW_TARGET", "vel[0]", "vel[0] (m/s)" },
    { 144, "FOLLOW_TARGET", "vel[1]", "vel[1] (m/s)" },
    { 144, "FOLLOW_TARGET", "vel[2]", "vel[2] (m/s)" },
    { 144, "FOLLOW_TARGET", "acc[0]", "acc[0] (m/s/s)" },
    { 144, "FOLLOW_TARGET", "acc[1]", "acc[1] (m/s/s)" },
    { 144, "FOLLOW_TARGET", "acc[2]", "acc[2] (m/s/s)" },
    { 144, "FOLLOW_TARGET", "attitude_q[0]", "attitude_q[0]" },
    { 144, "FOLLOW_TARGET", "attitude_q[1]", "attitude_q[1]" },
    { 144, "FOLLOW_TARGET", "attitude_q[2]", "attitude_q[2]" },
    { 144, "FOLLOW_TARGET", "attitude_q[3]", "attitude_q[3]" },
    { 144, "FOLLOW_TARGET", "rates[0]", "rates[0]" },
    { 144, "FOLLOW_TARGET", "rates[1]", "rates[1]" },
    { 144, "FOLLOW_TARGET", "rates[2]", "rates[2]" },
    { 144, "FOLLOW_TARGET", "position_cov[0]", "position_cov[0]" },
    { 144, "FOLLOW_TARGET", "position_cov[1]", "position_cov[1]" },
    { 144, "FOLLOW_TARGET", "position_cov[2]", "position_cov[2]" },
    { 144, "FOLLOW_TARGET", "custom_state", "custom_state" },
    { 146, "CONTROL_SYSTEM_STATE", "time_usec", "time_usec (us)" },
    { 146, "CONTROL_SYSTEM_STATE", "x_acc", "x_acc (m/s/s)" },
    { 146, "CONTROL_SYSTEM_STATE", "y_acc", "y_acc (m/s/s)" },
    { 146, "CONTROL_SYSTEM_STATE", "z_acc", "z_acc (m/s/s)" },
    { 146, "CONTROL_SYSTEM_STATE", "x_vel", "x_vel (m/s)" },
    { 146, "CONTROL_SYSTEM_STATE", "y_vel", "y_vel (m/s)" },
    { 146, "CONTROL_SYSTEM_STATE", "z_vel", "z_vel (m/s)" },
    { 146, "CONTROL_SYSTEM_STATE", "x_pos", "x_pos (m)" },
    { 146, "CONTROL_SYSTEM_STATE", "y_pos", "y_pos (m)" },
    { 146, "CONTROL_SYSTEM_STATE", "z_pos", "z_pos (m)" },
    { 146, "CONTROL_SYSTEM_STATE", "airspeed", "airspeed (m/s)" },
    { 146, "CONTROL_SYSTEM_STATE", "vel_variance[0]", "vel_variance[0]" },
    { 146, "CONTROL_SYSTEM_STATE", "vel_variance[1]", "vel_variance[1]" },
    { 146, "CONTROL_SYSTEM_STATE", "vel_variance[2]", "vel_variance[2]" },
    { 146, "CONTROL_SYSTEM_STATE", "pos_variance[0]", "pos_variance[0]" },
    { 146, "CONTROL_SYSTEM_STATE", "pos_variance[1]", "pos_variance[1]" },
    { 146, "CONTROL_SYSTEM_STATE", "pos_variance[2]", "pos_variance[2]" },
    { 146, "CONTROL_SYSTEM_STATE", "q[0]", "q[0]" },
    { 146, "CONTROL_SYSTEM_STATE", "q[1]", "q[1]" },
    { 146, "CONTROL_SYSTEM_STATE", "q[2]", "q[2]" },
    { 146, "CONTROL_SYSTEM_STATE", "q[3]", "q[3]" },
    { 146, "CONTROL_SYSTEM_STATE", "roll_rate", "roll_rate (rad/s)" },
    { 146, "CONTROL_SYSTEM_STATE", "pitch_rate", "pitch_rate (rad/s)" },
    { 146, "CONTROL_SYSTEM_STATE", "yaw_rate", "yaw_rate (rad/s)" },
    { 147, "BATTERY_STATUS", "id", "id" },
    { 147, "BATTERY_STATUS", "battery_function", "battery_function" },
    { 147, "BATTERY_STATUS", "type", "type" },
    { 147, "BATTERY_STATUS", "temperature", "temperature (cdegC)" },
    { 147, "BATTERY_STATUS", "current_battery", "current_battery (cA)" },
    { 147, "BATTERY_STATUS", "current_consumed", "current_consumed (mAh)" },
    { 147, "BATTERY_STATUS", "energy_consumed", "energy_consumed (hJ)" },
    { 147, "BATTERY_STATUS", "battery_remaining", "battery_remaining (%)" },
    { 147, "BATTERY_STATUS", "time_remaining", "time_remaining (s)" },
    { 147, "BATTERY_STATUS", "charge_state", "charge_state" },
    { 147, "BATTERY_STATUS", "voltages_ext[0]", "voltages_ext[0] (mV)" },
    { 147, "BATTERY_STATUS", "voltages_ext[1]", "voltages_ext[1] (mV)" },
    { 147, "BATTERY_STATUS", "voltages_ext[2]", "voltages_ext[2] (mV)" },
    { 147, "BATTERY_STATUS", "voltages_ext[3]", "voltages_ext[3] (mV)" },
    { 147, "BATTERY_STATUS", "mode", "mode" },
    { 147, "BATTERY_STATUS", "fault_bitmask", "fault_bitmask" },
    { 148, "AUTOPILOT_VERSION", "capabilities", "capabilities" },
    { 148, "AUTOPILOT_VERSION", "flight_sw_version", "flight_sw_version" },
    { 148, "AUTOPILOT_VERSION", "middleware_sw_version", "middleware_sw_version" },
    { 148, "AUTOPILOT_VERSION", "os_sw_version", "os_sw_version" },
    { 148, "AUTOPILOT_VERSION", "board_version", "board_version" },
    { 148, "AUTOPILOT_VERSION", "vendor_id", "vendor_id" },
    { 148, "AUTOPILOT_VERSION", "product_id", "product_id" },
    { 148, "AUTOPILOT_VERSION", "uid", "uid" },
    { 149, "LANDING_TARGET", "time_usec", "time_usec (us)" },
    { 149, "LANDING_TARGET", "target_num", "target_num" },
    { 149, "LANDING_TARGET", "frame", "frame" },
    { 149, "LANDING_TARGET", "angle_x", "angle_x (rad)" },
    { 149, "LANDING_TARGET", "angle_y", "angle_y (rad)" },
    { 149, "LANDING_TARGET", "distance", "distance (m)" },
    { 149, "LANDING_TARGET", "size_x", "size_x (rad)" },
    { 149, "LANDING_TARGET", "size_y", "size_y (rad)" },
    { 149, "LANDING_TARGET", "x", "x (m)" },
    { 149, "LANDING_TARGET", "y", "y (m)" },
    { 149, "LANDING_TARGET", "z", "z (m)" },
    { 149, "LANDING_TARGET", "q[0]", "q[0]" },
    { 149, "LANDING_TARGET", "q[1]", "q[1]" },
    { 149, "LANDING_TARGET", "q[2]", "q[2]" },
    { 149, "LANDING_TARGET", "q[3]", "q[3]" },
    { 149, "LANDING_TARGET", "type", "type" },
    { 149, "LANDING_TARGET", "position_valid", "position_valid" },
    { 150, "SENSOR_OFFSETS", "mag_ofs_x", "mag_ofs_x" },
    { 150, "SENSOR_OFFSETS", "mag_ofs_y", "mag_ofs_y" },
    { 150, "SENSOR_OFFSETS", "mag_ofs_z", "mag_ofs_z" },
    { 150, "SENSOR_OFFSETS", "mag_declination", "mag_declination (rad)" },
    { 150, "SENSOR_OFFSETS", "raw_press", "raw_press" },
    { 150, "SENSOR_OFFSETS", "raw_temp", "raw_temp" },
    { 150, "SENSOR_OFFSETS", "gyro_cal_x", "gyro_cal_x" },
    { 150, "SENSOR_OFFSETS", "gyro_cal_y", "gyro_cal_y" },
    { 150, "SENSOR_OFFSETS", "gyro_cal_z", "gyro_cal_z" },
    { 150, "SENSOR_OFFSETS", "accel_cal_x", "accel_cal_x" },
    { 150, "SENSOR_OFFSETS", "accel_cal_y", "accel_cal_y" },
    { 150, "SENSOR_OFFSETS", "accel_cal_z", "accel_cal_z" },
    { 152, "MEMINFO", "brkval", "brkval" },
    { 152, "MEMINFO", "freemem", "freemem (bytes)" },
    { 152, "MEMINFO", "freemem32", "freemem32 (bytes)" },
    { 153, "AP_ADC", "adc1", "adc1" },
    { 153, "AP_ADC", "adc2", "adc2" },
    { 153, "AP_ADC", "adc3", "adc3" },
    { 153, "AP_ADC", "adc4", "adc4" },
    { 153, "AP_ADC", "adc5", "adc5" },
    { 153, "AP_ADC", "adc6", "adc6" },
    { 158, "MOUNT_STATUS", "target_system", "target_system" },
    { 158, "MOUNT_STATUS", "target_component", "target_component" },
    { 158, "MOUNT_STATUS", "pointing_a", "pointing_a (cdeg)" },
    { 158, "MOUNT_STATUS", "pointing_b", "pointing_b (cdeg)" },
    { 158, "MOUNT_STATUS", "pointing_c", "pointing_c (cdeg)" },
    { 158, "MOUNT_STATUS", "mount_mode", "mount_mode" },
    { 162, "FENCE_STATUS", "breach_status", "breach_status" },
    { 162, "FENCE_STATUS", "breach_count", "breach_count" },
    { 162, "FENCE_STATUS", "breach_type", "breach_type" },
    { 162, "FENCE_STATUS", "breach_time", "breach_time (ms)" },
    { 162, "FENCE_STATUS", "breach_mitigation", "breach_mitigation" },
    { 163, "AHRS", "omegaIx", "omegaIx (rad/s)" },
    { 163, "AHRS", "omegaIy", "omegaIy (rad/s)" },
    { 163, "AHRS", "omegaIz", "omegaIz (rad/s)" },
    { 163, "AHRS", "accel_weight", "accel_weight" },
    { 163, "AHRS", "renorm_val", "renorm_val" },
    { 163, "AHRS", "error_rp", "error_rp" },
    { 163, "AHRS", "error_yaw", "error_yaw" },
    { 164, "SIMSTATE", "roll", "roll (rad)" },
    { 164, "SIMSTATE", "pitch", "pitch (rad)" },
    { 164, "SIMSTATE", "yaw", "yaw (rad)" },
    { 164, "SIMSTATE", "xacc", "xacc (m/s/s)" },
    { 164, "SIMSTATE", "yacc", "yacc (m/s/s)" },
    { 164, "SIMSTATE", "zacc", "zacc (m/s/s)" },
    { 164, "SIMSTATE", "xgyro", "xgyro (rad/s)" },
    { 164, "SIMSTATE", "ygyro", "ygyro (rad/s)" },
    { 164, "SIMSTATE", "zgyro", "zgyro (rad/s)" },
    { 164, "SIMSTATE", "lat", "lat (degE7)" },
    { 164, "SIMSTATE", "lng", "lng (degE7)" },
    { 165, "HWSTATUS", "Vcc", "Vcc (mV)" },
    { 165, "HWSTATUS", "I2Cerr", "I2Cerr" },
    { 166, "RADIO", "rssi", "rssi" },
    { 166, "RADIO", "remrssi", "remrssi" },
    { 166, "RADIO", "txbuf", "txbuf (%)" },
    { 166, "RADIO", "noise", "noise" },
    { 166, "RADIO", "remnoise", "remnoise" },
    { 166, "RADIO", "rxerrors", "rxerrors" },
    { 166, "RADIO", "fixed", "fixed" },
    { 167, "LIMITS_STATUS", "limits_state", "limits_state" },
    { 167, "LIMITS_STATUS", "last_trigger", "last_trigger (ms)" },
    { 167, "LIMITS_STATUS", "last_action", "last_action (ms)" },
    { 167, "LIMITS_STATUS", "last_recovery", "last_recovery (ms)" },
    { 167, "LIMITS_STATUS", "last_clear", "last_clear (ms)" },
    { 167, "LIMITS_STATUS", "breach_count", "breach_count" },
    { 167, "LIMITS_STATUS", "mods_enabled", "mods_enabled" },
    { 167, "LIMITS_STATUS", "mods_required", "mods_required" },
    { 167, "LIMITS_STATUS", "mods_triggered", "mods_triggered" },
    { 168, "WIND", "direction", "direction (deg)" },
    { 168, "WIND", "speed", "speed (m/s)" },
    { 168, "WIND", "speed_z", "speed_z (m/s)" },
    { 169, "DATA16", "type", "type" },
    { 169, "DATA16", "len", "len (bytes)" },
    { 170, "DATA32", "type", "type" },
    { 170, "DATA32", "len", "len (bytes)" },
    { 171, "DATA64", "type", "type" },
    { 171, "DATA64", "len", "len (bytes)" },
    { 172, "DATA96", "type", "type" },
    { 172, "DATA96", "len", "len (bytes)" },
    { 173, "RANGEFINDER", "distance", "distance (m)" },
    { 173, "RANGEFINDER", "voltage", "voltage (V)" },
    { 174, "AIRSPEED_AUTOCAL", "vx", "vx (m/s)" },
    { 174, "AIRSPEED_AUTOCAL", "vy", "vy (m/s)" },
    { 174, "AIRSPEED_AUTOCAL", "vz", "vz (m/s)" },
    { 174, "AIRSPEED_AUTOCAL", "diff_pressure", "diff_pressure (Pa)" },
    { 174, "AIRSPEED_AUTOCAL", "EAS2TAS", "EAS2TAS" },
    { 174, "AIRSPEED_AUTOCAL", "ratio", "ratio" },
    { 174, "AIRSPEED_AUTOCAL", "state_x", "state_x" },
    { 174, "AIRSPEED_AUTOCAL", "state_y", "state_y" },
    { 174, "AIRSPEED_AUTOCAL", "state_z", "state_z" },
    { 174, "AIRSPEED_AUTOCAL", "Pax", "Pax" },
    { 174, "AIRSPEED_AUTOCAL", "Pby", "Pby" },
    { 174, "AIRSPEED_AUTOCAL", "Pcz", "Pcz" },
    { 175, "RALLY_POINT", "target_system", "target_system" },
    { 175, "RALLY_POINT", "target_component", "target_component" },
    { 175, "RALLY_POINT", "idx", "idx" },
    { 175, "RALLY_POINT", "count", "count" },
    { 175, "RALLY_POINT", "lat", "lat (degE7)" },
    { 175, "RALLY_POINT", "lng", "lng (degE7)" },
    { 175, "RALLY_POINT", "alt", "alt (m)" },
    { 175, "RALLY_POINT", "break_alt", "break_alt (m)" },
    { 175, "RALLY_POINT", "land_dir", "land_dir (cdeg)" },
    { 175, "RALLY_POINT", "flags", "flags" },
    { 176, "RALLY_FETCH_POINT", "target_system", "target_system" },
    { 176, "RALLY_FETCH_POINT", "target_component", "target_component" },
    { 176, "RALLY_FETCH_POINT", "idx", "idx" },
    { 177, "COMPASSMOT_STATUS", "throttle", "throttle (d%)" },
    { 177, "COMPASSMOT_STATUS", "current", "current (A)" },
    { 177, "COMPASSMOT_STATUS", "interference", "interference (%)" },
    { 177, "COMPASSMOT_STATUS", "CompensationX", "CompensationX" },
    { 177, "COMPASSMOT_STATUS", "CompensationY", "CompensationY" },
    { 177, "COMPASSMOT_STATUS", "CompensationZ", "CompensationZ" },
    { 178, "AHRS2", "roll", "roll (rad)" },
    { 178, "AHRS2", "pitch", "pitch (rad)" },
    { 178, "AHRS2", "yaw", "yaw (rad)" },
    { 178, "AHRS2", "altitude", "altitude (m)" },
    { 178, "AHRS2", "lat", "lat (degE7)" },
    { 178, "AHRS2", "lng", "lng (degE7)" },
    { 179, "CAMERA_STATUS", "time_usec", "time_usec (us)" },
    { 179, "CAMERA_STATUS", "target_system", "target_system" },
    { 179, "CAMERA_STATUS", "cam_idx", "cam_idx" },
    { 179, "CAMERA_STATUS", "img_idx", "img_idx" },
    { 179, "CAMERA_STATUS", "event_id", "event_id" },
    { 179, "CAMERA_STATUS", "p1", "p1" },
    { 179, "CAMERA_STATUS", "p2", "p2" },
    { 179, "CAMERA_STATUS", "p3", "p3" },
    { 179, "CAMERA_STATUS", "p4", "p4" },
    { 180, "CAMERA_FEEDBACK", "time_usec", "time_usec (us)" },
    { 180, "CAMERA_FEEDBACK", "target_system", "target_system" },
    { 180, "CAMERA_FEEDBACK", "cam_idx", "cam_idx" },
    { 180, "CAMERA_FEEDBACK", "img_idx", "img_idx" },
    { 180, "CAMERA_FEEDBACK", "lat", "lat (degE7)" },
    { 180, "CAMERA_FEEDBACK", "lng", "lng (degE7)" },
    { 180, "CAMERA_FEEDBACK", "alt_msl", "alt_msl (m)" },
    { 180, "CAMERA_FEEDBACK", "alt_rel", "alt_rel (m)" },
    { 180, "CAMERA_FEEDBACK", "roll", "roll (deg)" },
    { 180, "CAMERA_FEEDBACK", "pitch", "pitch (deg)" },
    { 180, "CAMERA_FEEDBACK", "yaw", "yaw (deg)" },
    { 180, "CAMERA_FEEDBACK", "foc_len", "foc_len (mm)" },
    { 180, "CAMERA_FEEDBACK", "flags", "flags" },
    { 180, "CAMERA_FEEDBACK", "completed_captures", "completed_captures" },
    { 181, "BATTERY2", "voltage", "voltage (mV)" },
    { 181, "BATTERY2", "current_battery", "current_battery (cA)" },
    { 182, "AHRS3", "roll", "roll (rad)" },
    { 182, "AHRS3", "pitch", "pitch (rad)" },
    { 182, "AHRS3", "yaw", "yaw (rad)" },
    { 182, "AHRS3", "altitude", "altitude (m)" },
    { 182, "AHRS3", "lat", "lat (degE7)" },
    { 182, "AHRS3", "lng", "lng (degE7)" },
    { 182, "AHRS3", "v1", "v1" },
    { 182, "AHRS3", "v2", "v2" },
    { 182, "AHRS3", "v3", "v3" },
    { 182, "AHRS3", "v4", "v4" },
    { 185, "REMOTE_LOG_BLOCK_STATUS", "target_system", "target_system" },
    { 185, "REMOTE_LOG_BLOCK_STATUS", "target_component", "target_component" },
    { 185, "REMOTE_LOG_BLOCK_STATUS", "seqno", "seqno" },
    { 185, "REMOTE_LOG_BLOCK_STATUS", "status", "status" },
    { 191, "MAG_CAL_PROGRESS", "compass_id", "compass_id" },
    { 191, "MAG_CAL_PROGRESS", "cal_mask", "cal_mask" },
    { 191, "MAG_CAL_PROGRESS", "cal_status", "cal_status" },
    { 191, "MAG_CAL_PROGRESS", "attempt", "attempt" },
    { 191, "MAG_CAL_PROGRESS", "completion_pct", "completion_pct (%)" },
    { 191, "MAG_CAL_PROGRESS", "direction_x", "direction_x" },
    { 191, "MAG_CAL_PROGRESS", "direction_y", "direction_y" },
    { 191, "MAG_CAL_PROGRESS", "direction_z", "direction_z" },
    { 192, "MAG_CAL_REPORT", "compass_id", "compass_id" },
    { 192, "MAG_CAL_REPORT", "cal_mask", "cal_mask" },
    { 192, "MAG_CAL_REPORT", "cal_status", "cal_status" },
    { 192, "MAG_CAL_REPORT", "autosaved", "autosaved" },
    { 192, "MAG_CAL_REPORT", "fitness", "fitness (mgauss)" },
    { 192, "MAG_CAL_REPORT", "ofs_x", "ofs_x" },
    { 192, "MAG_CAL_REPORT", "ofs_y", "ofs_y" },
    { 192, "MAG_CAL_REPORT", "ofs_z", "ofs_z" },
    { 192, "MAG_CAL_REPORT", "diag_x", "diag_x" },
    { 192, "MAG_CAL_REPORT", "diag_y", "diag_y" },
    { 192, "MAG_CAL_REPORT", "diag_z", "diag_z" },
    { 192, "MAG_CAL_REPORT", "offdiag_x", "offdiag_x" },
    { 192, "MAG_CAL_REPORT", "offdiag_y", "offdiag_y" },
    { 192, "MAG_CAL_REPORT", "offdiag_z", "offdiag_z" },
    { 192, "MAG_CAL_REPORT", "orientation_confidence", "orientation_confidence" },
    { 192, "MAG_CAL_REPORT", "old_orientation", "old_orientation" },
    { 192, "MAG_CAL_REPORT", "new_orientation", "new_orientation" },
    { 192, "MAG_CAL_REPORT", "scale_factor", "scale_factor" },
    { 193, "EKF_STATUS_REPORT", "flags", "flags" },
    { 193, "EKF_STATUS_REPORT", "velocity_variance", "velocity_variance" },
    { 193, "EKF_STATUS_REPORT", "pos_horiz_variance", "pos_horiz_variance" },
    { 193, "EKF_STATUS_REPORT", "pos_vert_variance", "pos_vert_variance" },
    { 193, "EKF_STATUS_REPORT", "compass_variance", "compass_variance" },
    { 193, "EKF_STATUS_REPORT", "terrain_alt_variance", "terrain_alt_variance" },
    { 193, "EKF_STATUS_REPORT", "airspeed_variance", "airspeed_variance" },
    { 194, "PID_TUNING", "axis", "axis" },
    { 194, "PID_TUNING", "desired", "desired" },
    { 194, "PID_TUNING", "achieved", "achieved" },
    { 194, "PID_TUNING", "FF", "FF" },
    { 194, "PID_TUNING", "P", "P" },
    { 194, "PID_TUNING", "I", "I" },
    { 194, "PID_TUNING", "D", "D" },
    { 194, "PID_TUNING", "SRate", "SRate" },
    { 194, "PID_TUNING", "PDmod", "PDmod" },
    { 195, "DEEPSTALL", "landing_lat", "landing_lat (degE7)" },
    { 195, "DEEPSTALL", "landing_lon", "landing_lon (degE7)" },
    { 195, "DEEPSTALL", "path_lat", "path_lat (degE7)" },
    { 195, "DEEPSTALL", "path_lon", "path_lon (degE7)" },
    { 195, "DEEPSTALL", "arc_entry_lat", "arc_entry_lat (degE7)" },
    { 195, "DEEPSTALL", "arc_entry_lon", "arc_entry_lon (degE7)" },
    { 195, "DEEPSTALL", "altitude", "altitude (m)" },
    { 195, "DEEPSTALL", "expected_travel_distance", "expected_travel_distance (m)" },
    { 195, "DEEPSTALL", "cross_track_error", "cross_track_error (m)" },
    { 195, "DEEPSTALL", "stage", "stage" },
    { 200, "GIMBAL_REPORT", "target_system", "target_system" },
    { 200, "GIMBAL_REPORT", "target_component", "target_component" },
    { 200, "GIMBAL_REPORT", "delta_time", "delta_time (s)" },
    { 200, "GIMBAL_REPORT", "delta_angle_x", "delta_angle_x (rad)" },
    { 200, "GIMBAL_REPORT", "delta_angle_y", "delta_angle_y (rad)" },
    { 200, "GIMBAL_REPORT", "delta_angle_z", "delta_angle_z (rad)" },
    { 200, "GIMBAL_REPORT", "delta_velocity_x", "delta_velocity_x (m/s)" },
    { 200, "GIMBAL_REPORT", "delta_velocity_y", "delta_velocity_y (m/s)" },
    { 200, "GIMBAL_REPORT", "delta_velocity_z", "delta_velocity_z (m/s)" },
    { 200, "GIMBAL_REPORT", "joint_roll", "joint_roll (rad)" },
    { 200, "GIMBAL_REPORT", "joint_el", "joint_el (rad)" },
    { 200, "GIMBAL_REPORT", "joint_az", "joint_az (rad)" },
    { 201, "GIMBAL_CONTROL", "target_system", "target_system" },
    { 201, "GIMBAL_CONTROL", "target_component", "target_component" },
    { 201, "GIMBAL_CONTROL", "demanded_rate_x", "demanded_rate_x (rad/s)" },
    { 201, "GIMBAL_CONTROL", "demanded_rate_y", "demanded_rate_y (rad/s)" },
    { 201, "GIMBAL_CONTROL", "demanded_rate_z", "demanded_rate_z (rad/s)" },
    { 214, "GIMBAL_TORQUE_CMD_REPORT", "target_system", "target_system" },
    { 214, "GIMBAL_TORQUE_CMD_REPORT", "target_component", "target_component" },
    { 214, "GIMBAL_TORQUE_CMD_REPORT", "rl_torque_cmd", "rl_torque_cmd" },
    { 214, "GIMBAL_TORQUE_CMD_REPORT", "el_torque_cmd", "el_torque_cmd" },
    { 214, "GIMBAL_TORQUE_CMD_REPORT", "az_torque_cmd", "az_torque_cmd" },
    { 215, "GOPRO_HEARTBEAT", "status", "status" },
    { 215, "GOPRO_HEARTBEAT", "capture_mode", "capture_mode" },
    { 215, "GOPRO_HEARTBEAT", "flags", "flags" },
    { 216, "GOPRO_GET_REQUEST", "target_system", "target_system" },
    { 216, "GOPRO_GET_REQUEST", "target_component", "target_component" },
    { 216, "GOPRO_GET_REQUEST", "cmd_id", "cmd_id" },
    { 217, "GOPRO_GET_RESPONSE", "cmd_id", "cmd_id" },
    { 217, "GOPRO_GET_RESPONSE", "status", "status" },
    { 217, "GOPRO_GET_RESPONSE", "value[0]", "value[0]" },
    { 217, "GOPRO_GET_RESPONSE", "value[1]", "value[1]" },
    { 217, "GOPRO_GET_RESPONSE", "value[2]", "value[2]" },
    { 217, "GOPRO_GET_RESPONSE", "value[3]", "value[3]" },
    { 219, "GOPRO_SET_RESPONSE", "cmd_id", "cmd_id" },
    { 219, "GOPRO_SET_RESPONSE", "status", "status" },
    { 225, "EFI_STATUS", "health", "health" },
    { 225, "EFI_STATUS", "ecu_index", "ecu_index" },
    { 225, "EFI_STATUS", "rpm", "rpm" },
    { 225, "EFI_STATUS", "fuel_consumed", "fuel_consumed (cm^3)" },
    { 225, "EFI_STATUS", "fuel_flow", "fuel_flow (cm^3/min)" },
    { 225, "EFI_STATUS", "engine_load", "engine_load (%)" },
    { 225, "EFI_STATUS", "throttle_position", "throttle_position (%)" },
    { 225, "EFI_STATUS", "spark_dwell_time", "spark_dwell_time (ms)" },
    { 225, "EFI_STATUS", "barometric_pressure", "barometric_pressure (kPa)" },
    { 225, "EFI_STATUS", "intake_manifold_pressure", "intake_manifold_pressure (kPa)" },
    { 225, "EFI_STATUS", "intake_manifold_temperature", "intake_manifold_temperature (degC)" },
    { 225, "EFI_STATUS", "cylinder_head_temperature", "cylinder_head_temperature (degC)" },
    { 225, "EFI_STATUS", "ignition_timing", "ignition_timing (deg)" },
    { 225, "EFI_STATUS", "injection_time", "injection_time (ms)" },
    { 225, "EFI_STATUS", "exhaust_gas_temperature", "exhaust_gas_temperature (degC)" },
    { 225, "EFI_STATUS", "throttle_out", "throttle_out (%)" },
    { 225, "EFI_STATUS", "pt_compensation", "pt_compensation" },
    { 225, "EFI_STATUS", "ignition_voltage", "ignition_voltage (V)" },
    { 225, "EFI_STATUS", "fuel_pressure", "fuel_pressure (kPa)" },
    { 226, "RPM", "rpm1", "rpm1" },
    { 226, "RPM", "rpm2", "rpm2" },
    { 230, "ESTIMATOR_STATUS", "time_usec", "time_usec (us)" },
    { 230, "ESTIMATOR_STATUS", "flags", "flags" },
    { 230, "ESTIMATOR_STATUS", "vel_ratio", "vel_ratio" },
    { 230, "ESTIMATOR_STATUS", "pos_horiz_ratio", "pos_horiz_ratio" },
    { 230, "ESTIMATOR_STATUS", "pos_vert_ratio", "pos_vert_ratio" },
    { 230, "ESTIMATOR_STATUS", "mag_ratio", "mag_ratio" },
    { 230, "ESTIMATOR_STATUS", "hagl_ratio", "hagl_ratio" },
    { 230, "ESTIMATOR_STATUS", "tas_ratio", "tas_ratio" },
    { 230, "ESTIMATOR_STATUS", "pos_horiz_accuracy", "pos_horiz_accuracy (m)" },
    { 230, "ESTIMATOR_STATUS", "pos_vert_accuracy", "pos_vert_accuracy (m)" },
    { 231, "WIND_COV", "time_usec", "time_usec (us)" },
    { 231, "WIND_COV", "wind_x", "wind_x (m/s)" },
    { 231, "WIND_COV", "wind_y", "wind_y (m/s)" },
    { 231, "WIND_COV", "wind_z", "wind_z (m/s)" },
    { 231, "WIND_COV", "var_horiz", "var_horiz (m/s)" },
    { 231, "WIND_COV", "var_vert", "var_vert (m/s)" },
    { 231, "WIND_COV", "wind_alt", "wind_alt (m)" },
    { 231, "WIND_COV", "horiz_accuracy", "horiz_accuracy (m/s)" },
    { 231, "WIND_COV", "vert_accuracy", "vert_accuracy (m/s)" },
    { 232, "GPS_INPUT", "time_usec", "time_usec (us)" },
    { 232, "GPS_INPUT", "gps_id", "gps_id" },
    { 232, "GPS_INPUT", "ignore_flags", "ignore_flags" },
    { 232, "GPS_INPUT", "time_week_ms", "time_week_ms (ms)" },
    { 232, "GPS_INPUT", "time_week", "time_week" },
    { 232, "GPS_INPUT", "fix_type", "fix_type" },
    { 232, "GPS_INPUT", "lat", "lat (degE7)" },
    { 232, "GPS_INPUT", "lon", "lon (degE7)" },
    { 232, "GPS_INPUT", "alt", "alt (m)" },
    { 232, "GPS_INPUT", "hdop", "hdop" },
    { 232, "GPS_INPUT", "vdop", "vdop" },
    { 232, "GPS_INPUT", "vn", "vn (m/s)" },
    { 232, "GPS_INPUT", "ve", "ve (m/s)" },
    { 232, "GPS_INPUT", "vd", "vd (m/s)" },
    { 232, "GPS_INPUT", "speed_accuracy", "speed_accuracy (m/s)" },
    { 232, "GPS_INPUT", "horiz_accuracy", "horiz_accuracy (m)" },
    { 232, "GPS_INPUT", "vert_accuracy", "vert_accuracy (m)" },
    { 232, "GPS_INPUT", "satellites_visible", "satellites_visible" },
    { 232, "GPS_INPUT", "yaw", "yaw (cdeg)" },
    { 233, "GPS_RTCM_DATA", "flags", "flags" },
    { 233, "GPS_RTCM_DATA", "len", "len (bytes)" },
    { 234, "HIGH_LATENCY", "base_mode", "base_mode" },
    { 234, "HIGH_LATENCY", "custom_mode", "custom_mode" },
    { 234, "HIGH_LATENCY", "landed_state", "landed_state" },
    { 234, "HIGH_LATENCY", "roll", "roll (cdeg)" },
    { 234, "HIGH_LATENCY", "pitch", "pitch (cdeg)" },
    { 234, "HIGH_LATENCY", "heading", "heading (cdeg)" },
    { 234, "HIGH_LATENCY", "throttle", "throttle (%)" },
    { 234, "HIGH_LATENCY", "heading_sp", "heading_sp (cdeg)" },
    { 234, "HIGH_LATENCY", "latitude", "latitude (degE7)" },
    { 234, "HIGH_LATENCY", "longitude", "longitude (degE7)" },
    { 234, "HIGH_LATENCY", "altitude_amsl", "altitude_amsl (m)" },
    { 234, "HIGH_LATENCY", "altitude_sp", "altitude_sp (m)" },
    { 234, "HIGH_LATENCY", "airspeed", "airspeed (m/s)" },
    { 234, "HIGH_LATENCY", "airspeed_sp", "airspeed_sp (m/s)" },
    { 234, "HIGH_LATENCY", "groundspeed", "groundspeed (m/s)" },
    { 234, "HIGH_LATENCY", "climb_rate", "climb_rate (m/s)" },
    { 234, "HIGH_LATENCY", "gps_nsat", "gps_nsat" },
    { 234, "HIGH_LATENCY", "gps_fix_type", "gps_fix_type" },
    { 234, "HIGH_LATENCY", "battery_remaining", "battery_remaining (%)" },
    { 234, "HIGH_LATENCY", "temperature", "temperature (degC)" },
    { 234, "HIGH_LATENCY", "temperature_air", "temperature_air (degC)" },
    { 234, "HIGH_LATENCY", "failsafe", "failsafe" },
    { 234, "HIGH_LATENCY", "wp_num", "wp_num" },
    { 234, "HIGH_LATENCY", "wp_distance", "wp_distance (m)" },
    { 235, "HIGH_LATENCY2", "timestamp", "timestamp (ms)" },
    { 235, "HIGH_LATENCY2", "type", "type" },
    { 235, "HIGH_LATENCY2", "autopilot", "autopilot" },
    { 235, "HIGH_LATENCY2", "custom_mode", "custom_mode" },
    { 235, "HIGH_LATENCY2", "latitude", "latitude (degE7)" },
    { 235, "HIGH_LATENCY2", "longitude", "longitude (degE7)" },
    { 235, "HIGH_LATENCY2", "altitude", "altitude (m)" },
    { 235, "HIGH_LATENCY2", "target_altitude", "target_altitude (m)" },
    { 235, "HIGH_LATENCY2", "heading", "heading (deg/2)" },
    { 235, "HIGH_LATENCY2", "target_heading", "target_heading (deg/2)" },
    { 235, "HIGH_LATENCY2", "target_distance", "target_distance (dam)" },
    { 235, "HIGH_LATENCY2", "throttle", "throttle (%)" },
    { 235, "HIGH_LATENCY2", "airspeed", "airspeed (m/s*5)" },
    { 235, "HIGH_LATENCY2", "airspeed_sp", "airspeed_sp (m/s*5)" },
    { 235, "HIGH_LATENCY2", "groundspeed", "groundspeed (m/s*5)" },
    { 235, "HIGH_LATENCY2", "windspeed", "windspeed (m/s*5)" },
    { 235, "HIGH_LATENCY2", "wind_heading", "wind_heading (deg/2)" },
    { 235, "HIGH_LATENCY2", "eph", "eph (dm)" },
    { 235, "HIGH_LATENCY2", "epv", "epv (dm)" },
    { 235, "HIGH_LATENCY2", "temperature_air", "temperature_air (degC)" },
    { 235, "HIGH_LATENCY2", "climb_rate", "climb_rate (dm/s)" },
    { 235, "HIGH_LATENCY2", "battery", "battery (%)" },
    { 235, "HIGH_LATENCY2", "wp_num", "wp_num" },
    { 235, "HIGH_LATENCY2", "failure_flags", "failure_flags" },
    { 235, "HIGH_LATENCY2", "custom0", "custom0" },
    { 235, "HIGH_LATENCY2", "custom1", "custom1" },
    { 235, "HIGH_LATENCY2", "custom2", "custom2" },
    { 241, "VIBRATION", "time_usec", "time_usec (us)" },
    { 241, "VIBRATION", "vibration_x", "vibration_x" },
    { 241, "VIBRATION", "vibration_y", "vibration_y" },
    { 241, "VIBRATION", "vibration_z", "vibration_z" },
    { 241, "VIBRATION", "clipping_0", "clipping_0" },
    { 241, "VIBRATION", "clipping_1", "clipping_1" },
    { 241, "VIBRATION", "clipping_2", "clipping_2" },
    { 242, "HOME_POSITION", "latitude", "latitude (degE7)" },
    { 242, "HOME_POSITION", "longitude", "longitude (degE7)" },
    { 242, "HOME_POSITION", "altitude", "altitude (mm)" },
    { 242, "HOME_POSITION", "x", "x (m)" },
    { 242, "HOME_POSITION", "y", "y (m)" },
    { 242, "HOME_POSITION", "z", "z (m)" },
    { 242, "HOME_POSITION", "q[0]", "q[0]" },
    { 242, "HOME_POSITION", "q[1]", "q[1]" },
    { 242, "HOME_POSITION", "q[2]", "q[2]" },
    { 242, "HOME_POSITION", "q[3]", "q[3]" },
    { 242, "HOME_POSITION", "approach_x", "approach_x (m)" },
    { 242, "HOME_POSITION", "approach_y", "approach_y (m)" },
    { 242, "HOME_POSITION", "approach_z", "approach_z (m)" },
    { 242, "HOME_POSITION", "time_usec", "time_usec (us)" },
    { 245, "EXTENDED_SYS_STATE", "vtol_state", "vtol_state" },
    { 245, "EXTENDED_SYS_STATE", "landed_state", "landed_state" },
    { 246, "ADSB_VEHICLE", "ICAO_address", "ICAO_address" },
    { 246, "ADSB_VEHICLE", "lat", "lat (degE7)" },
    { 246, "ADSB_VEHICLE", "lon", "lon (degE7)" },
    { 246, "ADSB_VEHICLE", "altitude_type", "altitude_type" },
    { 246, "ADSB_VEHICLE", "altitude", "altitude (mm)" },
    { 246, "ADSB_VEHICLE", "heading", "heading (cdeg)" },
    { 246, "ADSB_VEHICLE", "hor_velocity", "hor_velocity (cm/s)" },
    { 246, "ADSB_VEHICLE", "ver_velocity", "ver_velocity (cm/s)" },
    { 246, "ADSB_VEHICLE", "emitter_type", "emitter_type" },
    { 246, "ADSB_VEHICLE", "tslc", "tslc (s)" },
    { 246, "ADSB_VEHICLE", "flags", "flags" },
    { 246, "ADSB_VEHICLE", "squawk", "squawk" },
    { 247, "COLLISION", "src", "src" },
    { 247, "COLLISION", "id", "id" },
    { 247, "COLLISION", "action", "action" },
    { 247, "COLLISION", "threat_level", "threat_level" },
    { 247, "COLLISION", "time_to_minimum_delta", "time_to_minimum_delta (s)" },
    { 247, "COLLISION", "altitude_minimum_delta", "altitude_minimum_delta (m)" },
    { 247, "COLLISION", "horizontal_minimum_delta", "horizontal_minimum_delta (m)" },
    { 249, "MEMORY_VECT", "address", "address" },
    { 249, "MEMORY_VECT", "ver", "ver" },
    { 249, "MEMORY_VECT", "type", "type" },
    { 250, "DEBUG_VECT", "time_usec", "time_usec (us)" },
    { 250, "DEBUG_VECT", "x", "x" },
    { 250, "DEBUG_VECT", "y", "y" },
    { 250, "DEBUG_VECT", "z", "z" },
    { 251, "NAMED_VALUE_FLOAT", "time_boot_ms", "time_boot_ms (ms)" },
    { 251, "NAMED_VALUE_FLOAT", "value", "value" },
    { 252, "NAMED_VALUE_INT", "time_boot_ms", "time_boot_ms (ms)" },
    { 252, "NAMED_VALUE_INT", "value", "value" },
    { 253, "STATUSTEXT", "severity", "severity" },
    { 253, "STATUSTEXT", "id", "id" },
    { 253, "STATUSTEXT", "chunk_seq", "chunk_seq" },
    { 254, "DEBUG", "time_boot_ms", "time_boot_ms (ms)" },
    { 254, "DEBUG", "ind", "ind" },
    { 254, "DEBUG", "value", "value" },
    { 257, "BUTTON_CHANGE", "time_boot_ms", "time_boot_ms (ms)" },
    { 257, "BUTTON_CHANGE", "last_change_ms", "last_change_ms (ms)" },
    { 257, "BUTTON_CHANGE", "state", "state" },
    { 259, "CAMERA_INFORMATION", "time_boot_ms", "time_boot_ms (ms)" },
    { 259, "CAMERA_INFORMATION", "firmware_version", "firmware_version" },
    { 259, "CAMERA_INFORMATION", "focal_length", "focal_length (mm)" },
    { 259, "CAMERA_INFORMATION", "sensor_size_h", "sensor_size_h (mm)" },
    { 259, "CAMERA_INFORMATION", "sensor_size_v", "sensor_size_v (mm)" },
    { 259, "CAMERA_INFORMATION", "resolution_h", "resolution_h (pix)" },
    { 259, "CAMERA_INFORMATION", "resolution_v", "resolution_v (pix)" },
    { 259, "CAMERA_INFORMATION", "lens_id", "lens_id" },
    { 259, "CAMERA_INFORMATION", "flags", "flags" },
    { 259, "CAMERA_INFORMATION", "cam_definition_version", "cam_definition_version" },
    { 259, "CAMERA_INFORMATION", "gimbal_device_id", "gimbal_device_id" },
    { 259, "CAMERA_INFORMATION", "camera_device_id", "camera_device_id" },
    { 260, "CAMERA_SETTINGS", "time_boot_ms", "time_boot_ms (ms)" },
    { 260, "CAMERA_SETTINGS", "mode_id", "mode_id" },
    { 260, "CAMERA_SETTINGS", "zoomLevel", "zoomLevel" },
    { 260, "CAMERA_SETTINGS", "focusLevel", "focusLevel" },
    { 260, "CAMERA_SETTINGS", "camera_device_id", "camera_device_id" },
    { 261, "STORAGE_INFORMATION", "time_boot_ms", "time_boot_ms (ms)" },
    { 261, "STORAGE_INFORMATION", "storage_id", "storage_id" },
    { 261, "STORAGE_INFORMATION", "storage_count", "storage_count" },
    { 261, "STORAGE_INFORMATION", "status", "status" },
    { 261, "STORAGE_INFORMATION", "total_capacity", "total_capacity (MiB)" },
    { 261, "STORAGE_INFORMATION", "used_capacity", "used_capacity (MiB)" },
    { 261, "STORAGE_INFORMATION", "available_capacity", "available_capacity (MiB)" },
    { 261, "STORAGE_INFORMATION", "read_speed", "read_speed (MiB/s)" },
    { 261, "STORAGE_INFORMATION", "write_speed", "write_speed (MiB/s)" },
    { 261, "STORAGE_INFORMATION", "type", "type" },
    { 261, "STORAGE_INFORMATION", "storage_usage", "storage_usage" },
    { 262, "CAMERA_CAPTURE_STATUS", "time_boot_ms", "time_boot_ms (ms)" },
    { 262, "CAMERA_CAPTURE_STATUS", "image_status", "image_status" },
    { 262, "CAMERA_CAPTURE_STATUS", "video_status", "video_status" },
    { 262, "CAMERA_CAPTURE_STATUS", "image_interval", "image_interval (s)" },
    { 262, "CAMERA_CAPTURE_STATUS", "recording_time_ms", "recording_time_ms (ms)" },
    { 262, "CAMERA_CAPTURE_STATUS", "available_capacity", "available_capacity (MiB)" },
    { 262, "CAMERA_CAPTURE_STATUS", "image_count", "image_count" },
    { 262, "CAMERA_CAPTURE_STATUS", "camera_device_id", "camera_device_id" },
    { 263, "CAMERA_IMAGE_CAPTURED", "time_boot_ms", "time_boot_ms (ms)" },
    { 263, "CAMERA_IMAGE_CAPTURED", "time_utc", "time_utc (us)" },
    { 263, "CAMERA_IMAGE_CAPTURED", "camera_id", "camera_id" },
    { 263, "CAMERA_IMAGE_CAPTURED", "lat", "lat (degE7)" },
    { 263, "CAMERA_IMAGE_CAPTURED", "lon", "lon (degE7)" },
    { 263, "CAMERA_IMAGE_CAPTURED", "alt", "alt (mm)" },
    { 263, "CAMERA_IMAGE_CAPTURED", "relative_alt", "relative_alt (mm)" },
    { 263, "CAMERA_IMAGE_CAPTURED", "q[0]", "q[0]" },
    { 263, "CAMERA_IMAGE_CAPTURED", "q[1]", "q[1]" },
    { 263, "CAMERA_IMAGE_CAPTURED", "q[2]", "q[2]" },
    { 263, "CAMERA_IMAGE_CAPTURED", "q[3]", "q[3]" },
    { 263, "CAMERA_IMAGE_CAPTURED", "image_index", "image_index" },
    { 263, "CAMERA_IMAGE_CAPTURED", "capture_result", "capture_result" },
    { 264, "FLIGHT_INFORMATION", "time_boot_ms", "time_boot_ms (ms)" },
    { 264, "FLIGHT_INFORMATION", "arming_time_utc", "arming_time_utc (us)" },
    { 264, "FLIGHT_INFORMATION", "takeoff_time_utc", "takeoff_time_utc (us)" },
    { 264, "FLIGHT_INFORMATION", "flight_uuid", "flight_uuid" },
    { 264, "FLIGHT_INFORMATION", "landing_time", "landing_time (ms)" },
    { 265, "MOUNT_ORIENTATION", "time_boot_ms", "time_boot_ms (ms)" },
    { 265, "MOUNT_ORIENTATION", "roll", "roll (deg)" },
    { 265, "MOUNT_ORIENTATION", "pitch", "pitch (deg)" },
    { 265, "MOUNT_ORIENTATION", "yaw", "yaw (deg)" },
    { 265, "MOUNT_ORIENTATION", "yaw_absolute", "yaw_absolute (deg)" },
    { 266, "LOGGING_DATA", "target_system", "target_system" },
    { 266, "LOGGING_DATA", "target_component", "target_component" },
    { 266, "LOGGING_DATA", "sequence", "sequence" },
    { 266, "LOGGING_DATA", "length", "length (bytes)" },
    { 266, "LOGGING_DATA", "first_message_offset", "first_message_offset (bytes)" },
    { 267, "LOGGING_DATA_ACKED", "target_system", "target_system" },
    { 267, "LOGGING_DATA_ACKED", "target_component", "target_component" },
    { 267, "LOGGING_DATA_ACKED", "sequence", "sequence" },
    { 267, "LOGGING_DATA_ACKED", "length", "length (bytes)" },
    { 267, "LOGGING_DATA_ACKED", "first_message_offset", "first_message_offset (bytes)" },
    { 268, "LOGGING_ACK", "target_system", "target_system" },
    { 268, "LOGGING_ACK", "target_component", "target_component" },
    { 268, "LOGGING_ACK", "sequence", "sequence" },
    { 269, "VIDEO_STREAM_INFORMATION", "stream_id", "stream_id" },
    { 269, "VIDEO_STREAM_INFORMATION", "count", "count" },
    { 269, "VIDEO_STREAM_INFORMATION", "type", "type" },
    { 269, "VIDEO_STREAM_INFORMATION", "flags", "flags" },
    { 269, "VIDEO_STREAM_INFORMATION", "framerate", "framerate (Hz)" },
    { 269, "VIDEO_STREAM_INFORMATION", "resolution_h", "resolution_h (pix)" },
    { 269, "VIDEO_STREAM_INFORMATION", "resolution_v", "resolution_v (pix)" },
    { 269, "VIDEO_STREAM_INFORMATION", "bitrate", "bitrate (bits/s)" },
    { 269, "VIDEO_STREAM_INFORMATION", "rotation", "rotation (deg)" },
    { 269, "VIDEO_STREAM_INFORMATION", "hfov", "hfov (deg)" },
    { 269, "VIDEO_STREAM_INFORMATION", "encoding", "encoding" },
    { 269, "VIDEO_STREAM_INFORMATION", "camera_device_id", "camera_device_id" },
    { 270, "VIDEO_STREAM_STATUS", "stream_id", "stream_id" },
    { 270, "VIDEO_STREAM_STATUS", "flags", "flags" },
    { 270, "VIDEO_STREAM_STATUS", "framerate", "framerate (Hz)" },
    { 270, "VIDEO_STREAM_STATUS", "resolution_h", "resolution_h (pix)" },
    { 270, "VIDEO_STREAM_STATUS", "resolution_v", "resolution_v (pix)" },
    { 270, "VIDEO_STREAM_STATUS", "bitrate", "bitrate (bits/s)" },
    { 270, "VIDEO_STREAM_STATUS", "rotation", "rotation (deg)" },
    { 270, "VIDEO_STREAM_STATUS", "hfov", "hfov (deg)" },
    { 270, "VIDEO_STREAM_STATUS", "camera_device_id", "camera_device_id" },
    { 271, "CAMERA_FOV_STATUS", "time_boot_ms", "time_boot_ms (ms)" },
    { 271, "CAMERA_FOV_STATUS", "lat_camera", "lat_camera (degE7)" },
    { 271, "CAMERA_FOV_STATUS", "lon_camera", "lon_camera (degE7)" },
    { 271, "CAMERA_FOV_STATUS", "alt_camera", "alt_camera (mm)" },
    { 271, "CAMERA_FOV_STATUS", "lat_image", "lat_image (degE7)" },
    { 271, "CAMERA_FOV_STATUS", "lon_image", "lon_image (degE7)" },
    { 271, "CAMERA_FOV_STATUS", "alt_image", "alt_image (mm)" },
    { 271, "CAMERA_FOV_STATUS", "q[0]", "q[0]" },
    { 271, "CAMERA_FOV_STATUS", "q[1]", "q[1]" },
    { 271, "CAMERA_FOV_STATUS", "q[2]", "q[2]" },
    { 271, "CAMERA_FOV_STATUS", "q[3]", "q[3]" },
    { 271, "CAMERA_FOV_STATUS", "hfov", "hfov (deg)" },
    { 271, "CAMERA_FOV_STATUS", "vfov", "vfov (deg)" },
    { 271, "CAMERA_FOV_STATUS", "camera_device_id", "camera_device_id" },
    { 275, "CAMERA_TRACKING_IMAGE_STATUS", "tracking_status", "tracking_status" },
    { 275, "CAMERA_TRACKING_IMAGE_STATUS", "tracking_mode", "tracking_mode" },
    { 275, "CAMERA_TRACKING_IMAGE_STATUS", "target_data", "target_data" },
    { 275, "CAMERA_TRACKING_IMAGE_STATUS", "point_x", "point_x" },
    { 275, "CAMERA_TRACKING_IMAGE_STATUS", "point_y", "point_y" },
    { 275, "CAMERA_TRACKING_IMAGE_STATUS", "radius", "radius" },
    { 275, "CAMERA_TRACKING_IMAGE_STATUS", "rec_top_x", "rec_top_x" },
    { 275, "CAMERA_TRACKING_IMAGE_STATUS", "rec_top_y", "rec_top_y" },
    { 275, "CAMERA_TRACKING_IMAGE_STATUS", "rec_bottom_x", "rec_bottom_x" },
    { 275, "CAMERA_TRACKING_IMAGE_STATUS", "rec_bottom_y", "rec_bottom_y" },
    { 275, "CAMERA_TRACKING_IMAGE_STATUS", "camera_device_id", "camera_device_id" },
    { 276, "CAMERA_TRACKING_GEO_STATUS", "tracking_status", "tracking_status" },
    { 276, "CAMERA_TRACKING_GEO_STATUS", "lat", "lat (degE7)" },
    { 276, "CAMERA_TRACKING_GEO_STATUS", "lon", "lon (degE7)" },
    { 276, "CAMERA_TRACKING_GEO_STATUS", "alt", "alt (m)" },
    { 276, "CAMERA_TRACKING_GEO_STATUS", "h_acc", "h_acc (m)" },
    { 276, "CAMERA_TRACKING_GEO_STATUS", "v_acc", "v_acc (m)" },
    { 276, "CAMERA_TRACKING_GEO_STATUS", "vel_n", "vel_n (m/s)" },
    { 276, "CAMERA_TRACKING_GEO_STATUS", "vel_e", "vel_e (m/s)" },
    { 276, "CAMERA_TRACKING_GEO_STATUS", "vel_d", "vel_d (m/s)" },
    { 276, "CAMERA_TRACKING_GEO_STATUS", "vel_acc", "vel_acc (m/s)" },
    { 276, "CAMERA_TRACKING_GEO_STATUS", "dist", "dist (m)" },
    { 276, "CAMERA_TRACKING_GEO_STATUS", "hdg", "hdg (rad)" },
    { 276, "CAMERA_TRACKING_GEO_STATUS", "hdg_acc", "hdg_acc (rad)" },
    { 276, "CAMERA_TRACKING_GEO_STATUS", "camera_device_id", "camera_device_id" },
    { 277, "CAMERA_THERMAL_RANGE", "time_boot_ms", "time_boot_ms (ms)" },
    { 277, "CAMERA_THERMAL_RANGE", "stream_id", "stream_id" },
    { 277, "CAMERA_THERMAL_RANGE", "camera_device_id", "camera_device_id" },
    { 277, "CAMERA_THERMAL_RANGE", "max", "max (degC)" },
    { 277, "CAMERA_THERMAL_RANGE", "max_point_x", "max_point_x" },
    { 277, "CAMERA_THERMAL_RANGE", "max_point_y", "max_point_y" },
    { 277, "CAMERA_THERMAL_RANGE", "min", "min (degC)" },
    { 277, "CAMERA_THERMAL_RANGE", "min_point_x", "min_point_x" },
    { 277, "CAMERA_THERMAL_RANGE", "min_point_y", "min_point_y" },
    { 280, "GIMBAL_MANAGER_INFORMATION", "time_boot_ms", "time_boot_ms (ms)" },
    { 280, "GIMBAL_MANAGER_INFORMATION", "cap_flags", "cap_flags" },
    { 280, "GIMBAL_MANAGER_INFORMATION", "gimbal_device_id", "gimbal_device_id" },
    { 280, "GIMBAL_MANAGER_INFORMATION", "roll_min", "roll_min (rad)" },
    { 280, "GIMBAL_MANAGER_INFORMATION", "roll_max", "roll_max (rad)" },
    { 280, "GIMBAL_MANAGER_INFORMATION", "pitch_min", "pitch_min (rad)" },
    { 280, "GIMBAL_MANAGER_INFORMATION", "pitch_max", "pitch_max (rad)" },
    { 280, "GIMBAL_MANAGER_INFORMATION", "yaw_min", "yaw_min (rad)" },
    { 280, "GIMBAL_MANAGER_INFORMATION", "yaw_max", "yaw_max (rad)" },
    { 281, "GIMBAL_MANAGER_STATUS", "time_boot_ms", "time_boot_ms (ms)" },
    { 281, "GIMBAL_MANAGER_STATUS", "flags", "flags" },
    { 281, "GIMBAL_MANAGER_STATUS", "gimbal_device_id", "gimbal_device_id" },
    { 281, "GIMBAL_MANAGER_STATUS", "primary_control_sysid", "primary_control_sysid" },
    { 281, "GIMBAL_MANAGER_STATUS", "primary_control_compid", "primary_control_compid" },
    { 281, "GIMBAL_MANAGER_STATUS", "secondary_control_sysid", "secondary_control_sysid" },
    { 281, "GIMBAL_MANAGER_STATUS", "secondary_control_compid", "secondary_control_compid" },
    { 283, "GIMBAL_DEVICE_INFORMATION", "time_boot_ms", "time_boot_ms (ms)" },
    { 283, "GIMBAL_DEVICE_INFORMATION", "firmware_version", "firmware_version" },
    { 283, "GIMBAL_DEVICE_INFORMATION", "hardware_version", "hardware_version" },
    { 283, "GIMBAL_DEVICE_INFORMATION", "uid", "uid" },
    { 283, "GIMBAL_DEVICE_INFORMATION", "cap_flags", "cap_flags" },
    { 283, "GIMBAL_DEVICE_INFORMATION", "custom_cap_flags", "custom_cap_flags" },
    { 283, "GIMBAL_DEVICE_INFORMATION", "roll_min", "roll_min (rad)" },
    { 283, "GIMBAL_DEVICE_INFORMATION", "roll_max", "roll_max (rad)" },
    { 283, "GIMBAL_DEVICE_INFORMATION", "pitch_min", "pitch_min (rad)" },
    { 283, "GIMBAL_DEVICE_INFORMATION", "pitch_max", "pitch_max (rad)" },
    { 283, "GIMBAL_DEVICE_INFORMATION", "yaw_min", "yaw_min (rad)" },
    { 283, "GIMBAL_DEVICE_INFORMATION", "yaw_max", "yaw_max (rad)" },
    { 283, "GIMBAL_DEVICE_INFORMATION", "gimbal_device_id", "gimbal_device_id" },
    { 283, "GIMBAL_DEVICE_INFORMATION", "cap_flags2", "cap_flags2" },
    { 285, "GIMBAL_DEVICE_ATTITUDE_STATUS", "target_system", "target_system" },
    { 285, "GIMBAL_DEVICE_ATTITUDE_STATUS", "target_component", "target_component" },
    { 285, "GIMBAL_DEVICE_ATTITUDE_STATUS", "time_boot_ms", "time_boot_ms (ms)" },
    { 285, "GIMBAL_DEVICE_ATTITUDE_STATUS", "flags", "flags" },
    { 285, "GIMBAL_DEVICE_ATTITUDE_STATUS", "q[0]", "q[0]" },
    { 285, "GIMBAL_DEVICE_ATTITUDE_STATUS", "q[1]", "q[1]" },
    { 285, "GIMBAL_DEVICE_ATTITUDE_STATUS", "q[2]", "q[2]" },
    { 285, "GIMBAL_DEVICE_ATTITUDE_STATUS", "q[3]", "q[3]" },
    { 285, "GIMBAL_DEVICE_ATTITUDE_STATUS", "angular_velocity_x", "angular_velocity_x (rad/s)" },
    { 285, "GIMBAL_DEVICE_ATTITUDE_STATUS", "angular_velocity_y", "angular_velocity_y (rad/s)" },
    { 285, "GIMBAL_DEVICE_ATTITUDE_STATUS", "angular_velocity_z", "angular_velocity_z (rad/s)" },
    { 285, "GIMBAL_DEVICE_ATTITUDE_STATUS", "failure_flags", "failure_flags" },
    { 285, "GIMBAL_DEVICE_ATTITUDE_STATUS", "delta_yaw", "delta_yaw (rad)" },
    { 285, "GIMBAL_DEVICE_ATTITUDE_STATUS", "delta_yaw_velocity", "delta_yaw_velocity (rad/s)" },
    { 285, "GIMBAL_DEVICE_ATTITUDE_STATUS", "gimbal_device_id", "gimbal_device_id" },
    { 286, "AUTOPILOT_STATE_FOR_GIMBAL_DEVICE", "target_system", "target_system" },
    { 286, "AUTOPILOT_STATE_FOR_GIMBAL_DEVICE", "target_component", "target_component" },
    { 286, "AUTOPILOT_STATE_FOR_GIMBAL_DEVICE", "time_boot_us", "time_boot_us (us)" },
    { 286, "AUTOPILOT_STATE_FOR_GIMBAL_DEVICE", "q[0]", "q[0]" },
    { 286, "AUTOPILOT_STATE_FOR_GIMBAL_DEVICE", "q[1]", "q[1]" },
    { 286, "AUTOPILOT_STATE_FOR_GIMBAL_DEVICE", "q[2]", "q[2]" },
    { 286, "AUTOPILOT_STATE_FOR_GIMBAL_DEVICE", "q[3]", "q[3]" },
    { 286, "AUTOPILOT_STATE_FOR_GIMBAL_DEVICE", "q_estimated_delay_us", "q_estimated_delay_us (us)" },
    { 286, "AUTOPILOT_STATE_FOR_GIMBAL_DEVICE", "vx", "vx (m/s)" },
    { 286, "AUTOPILOT_STATE_FOR_GIMBAL_DEVICE", "vy", "vy (m/s)" },
    { 286, "AUTOPILOT_STATE_FOR_GIMBAL_DEVICE", "vz", "vz (m/s)" },
    { 286, "AUTOPILOT_STATE_FOR_GIMBAL_DEVICE", "v_estimated_delay_us", "v_estimated_delay_us (us)" },
    { 286, "AUTOPILOT_STATE_FOR_GIMBAL_DEVICE", "feed_forward_angular_velocity_z", "feed_forward_angular_velocity_z (rad/s)" },
    { 286, "AUTOPILOT_STATE_FOR_GIMBAL_DEVICE", "estimator_status", "estimator_status" },
    { 286, "AUTOPILOT_STATE_FOR_GIMBAL_DEVICE", "landed_state", "landed_state" },
    { 286, "AUTOPILOT_STATE_FOR_GIMBAL_DEVICE", "angular_velocity_z", "angular_velocity_z (rad/s)" },
    { 290, "ESC_INFO", "index", "index" },
    { 290, "ESC_INFO", "time_usec", "time_usec (us)" },
    { 290, "ESC_INFO", "counter", "counter" },
    { 290, "ESC_INFO", "count", "count" },
    { 290, "ESC_INFO", "connection_type", "connection_type" },
    { 290, "ESC_INFO", "info", "info" },
    { 290, "ESC_INFO", "failure_flags[0]", "failure_flags[0]" },
    { 290, "ESC_INFO", "failure_flags[1]", "failure_flags[1]" },
    { 290, "ESC_INFO", "failure_flags[2]", "failure_flags[2]" },
    { 290, "ESC_INFO", "failure_flags[3]", "failure_flags[3]" },
    { 290, "ESC_INFO", "error_count[0]", "error_count[0]" },
    { 290, "ESC_INFO", "error_count[1]", "error_count[1]" },
    { 290, "ESC_INFO", "error_count[2]", "error_count[2]" },
    { 290, "ESC_INFO", "error_count[3]", "error_count[3]" },
    { 290, "ESC_INFO", "temperature[0]", "temperature[0] (cdegC)" },
    { 290, "ESC_INFO", "temperature[1]", "temperature[1] (cdegC)" },
    { 290, "ESC_INFO", "temperature[2]", "temperature[2] (cdegC)" },
    { 290, "ESC_INFO", "temperature[3]", "temperature[3] (cdegC)" },
    { 291, "ESC_STATUS", "index", "index" },
    { 291, "ESC_STATUS", "time_usec", "time_usec (us)" },
    { 291, "ESC_STATUS", "rpm[0]", "rpm[0] (rpm)" },
    { 291, "ESC_STATUS", "rpm[1]", "rpm[1] (rpm)" },
    { 291, "ESC_STATUS", "rpm[2]", "rpm[2] (rpm)" },
    { 291, "ESC_STATUS", "rpm[3]", "rpm[3] (rpm)" },
    { 291, "ESC_STATUS", "voltage[0]", "voltage[0] (V)" },
    { 291, "ESC_STATUS", "voltage[1]", "voltage[1] (V)" },
    { 291, "ESC_STATUS", "voltage[2]", "voltage[2] (V)" },
    { 291, "ESC_STATUS", "voltage[3]", "voltage[3] (V)" },
    { 291, "ESC_STATUS", "current[0]", "current[0] (A)" },
    { 291, "ESC_STATUS", "current[1]", "current[1] (A)" },
    { 291, "ESC_STATUS", "current[2]", "current[2] (A)" },
    { 291, "ESC_STATUS", "current[3]", "current[3] (A)" },
    { 295, "AIRSPEED", "id", "id" },
    { 295, "AIRSPEED", "airspeed", "airspeed (m/s)" },
    { 295, "AIRSPEED", "temperature", "temperature (cdegC)" },
    { 295, "AIRSPEED", "raw_press", "raw_press (hPa)" },
    { 295, "AIRSPEED", "flags", "flags" },
    { 296, "GLOBAL_POSITION_SENSOR", "target_system", "target_system" },
    { 296, "GLOBAL_POSITION_SENSOR", "target_component", "target_component" },
    { 296, "GLOBAL_POSITION_SENSOR", "id", "id" },
    { 296, "GLOBAL_POSITION_SENSOR", "time_usec", "time_usec (us)" },
    { 296, "GLOBAL_POSITION_SENSOR", "processing_time", "processing_time (us)" },
    { 296, "GLOBAL_POSITION_SENSOR", "source", "source" },
    { 296, "GLOBAL_POSITION_SENSOR", "flags", "flags" },
    { 296, "GLOBAL_POSITION_SENSOR", "lat", "lat (degE7)" },
    { 296, "GLOBAL_POSITION_SENSOR", "lon", "lon (degE7)" },
    { 296, "GLOBAL_POSITION_SENSOR", "alt_ellipsoid", "alt_ellipsoid (m)" },
    { 296, "GLOBAL_POSITION_SENSOR", "alt", "alt (m)" },
    { 296, "GLOBAL_POSITION_SENSOR", "eph", "eph (m)" },
    { 296, "GLOBAL_POSITION_SENSOR", "epv", "epv (m)" },
    { 301, "AIS_VESSEL", "MMSI", "MMSI" },
    { 301, "AIS_VESSEL", "lat", "lat (degE7)" },
    { 301, "AIS_VESSEL", "lon", "lon (degE7)" },
    { 301, "AIS_VESSEL", "COG", "COG (cdeg)" },
    { 301, "AIS_VESSEL", "heading", "heading (cdeg)" },
    { 301, "AIS_VESSEL", "velocity", "velocity (cm/s)" },
    { 301, "AIS_VESSEL", "turn_rate", "turn_rate (ddeg/s)" },
    { 301, "AIS_VESSEL", "navigational_status", "navigational_status" },
    { 301, "AIS_VESSEL", "type", "type" },
    { 301, "AIS_VESSEL", "dimension_bow", "dimension_bow (m)" },
    { 301, "AIS_VESSEL", "dimension_stern", "dimension_stern (m)" },
    { 301, "AIS_VESSEL", "dimension_port", "dimension_port (m)" },
    { 301, "AIS_VESSEL", "dimension_starboard", "dimension_starboard (m)" },
    { 301, "AIS_VESSEL", "tslc", "tslc (s)" },
    { 301, "AIS_VESSEL", "flags", "flags" },
    { 310, "UAVCAN_NODE_STATUS", "time_usec", "time_usec (us)" },
    { 310, "UAVCAN_NODE_STATUS", "uptime_sec", "uptime_sec (s)" },
    { 310, "UAVCAN_NODE_STATUS", "health", "health" },
    { 310, "UAVCAN_NODE_STATUS", "mode", "mode" },
    { 310, "UAVCAN_NODE_STATUS", "sub_mode", "sub_mode" },
    { 310, "UAVCAN_NODE_STATUS", "vendor_specific_status_code", "vendor_specific_status_code" },
    { 311, "UAVCAN_NODE_INFO", "time_usec", "time_usec (us)" },
    { 311, "UAVCAN_NODE_INFO", "uptime_sec", "uptime_sec (s)" },
    { 311, "UAVCAN_NODE_INFO", "hw_version_major", "hw_version_major" },
    { 311, "UAVCAN_NODE_INFO", "hw_version_minor", "hw_version_minor" },
    { 311, "UAVCAN_NODE_INFO", "sw_version_major", "sw_version_major" },
    { 311, "UAVCAN_NODE_INFO", "sw_version_minor", "sw_version_minor" },
    { 311, "UAVCAN_NODE_INFO", "sw_vcs_commit", "sw_vcs_commit" },
    { 322, "PARAM_EXT_VALUE", "param_type", "param_type" },
    { 322, "PARAM_EXT_VALUE", "param_count", "param_count" },
    { 322, "PARAM_EXT_VALUE", "param_index", "param_index" },
    { 324, "PARAM_EXT_ACK", "param_type", "param_type" },
    { 324, "PARAM_EXT_ACK", "param_result", "param_result" },
    { 330, "OBSTACLE_DISTANCE", "time_usec", "time_usec (us)" },
    { 330, "OBSTACLE_DISTANCE", "sensor_type", "sensor_type" },
    { 330, "OBSTACLE_DISTANCE", "increment", "increment (deg)" },
    { 330, "OBSTACLE_DISTANCE", "min_distance", "min_distance (cm)" },
    { 330, "OBSTACLE_DISTANCE", "max_distance", "max_distance (cm)" },
    { 330, "OBSTACLE_DISTANCE", "increment_f", "increment_f (deg)" },
    { 330, "OBSTACLE_DISTANCE", "angle_offset", "angle_offset (deg)" },
    { 330, "OBSTACLE_DISTANCE", "frame", "frame" },
    { 331, "ODOMETRY", "time_usec", "time_usec (us)" },
    { 331, "ODOMETRY", "frame_id", "frame_id" },
    { 331, "ODOMETRY", "child_frame_id", "child_frame_id" },
    { 331, "ODOMETRY", "x", "x (m)" },
    { 331, "ODOMETRY", "y", "y (m)" },
    { 331, "ODOMETRY", "z", "z (m)" },
    { 331, "ODOMETRY", "q[0]", "q[0]" },
    { 331, "ODOMETRY", "q[1]", "q[1]" },
    { 331, "ODOMETRY", "q[2]", "q[2]" },
    { 331, "ODOMETRY", "q[3]", "q[3]" },
    { 331, "ODOMETRY", "vx", "vx (m/s)" },
    { 331, "ODOMETRY", "vy", "vy (m/s)" },
    { 331, "ODOMETRY", "vz", "vz (m/s)" },
    { 331, "ODOMETRY", "rollspeed", "rollspeed (rad/s)" },
    { 331, "ODOMETRY", "pitchspeed", "pitchspeed (rad/s)" },
    { 331, "ODOMETRY", "yawspeed", "yawspeed (rad/s)" },
    { 331, "ODOMETRY", "reset_counter", "reset_counter" },
    { 331, "ODOMETRY", "estimator_type", "estimator_type" },
    { 331, "ODOMETRY", "quality", "quality (%)" },
    { 332, "TRAJECTORY_REPRESENTATION_WAYPOINTS", "time_usec", "time_usec (us)" },
    { 332, "TRAJECTORY_REPRESENTATION_WAYPOINTS", "valid_points", "valid_points" },
    { 333, "TRAJECTORY_REPRESENTATION_BEZIER", "time_usec", "time_usec (us)" },
    { 333, "TRAJECTORY_REPRESENTATION_BEZIER", "valid_points", "valid_points" },
    { 334, "CELLULAR_STATUS", "status", "status" },
    { 334, "CELLULAR_STATUS", "failure_reason", "failure_reason" },
    { 334, "CELLULAR_STATUS", "type", "type" },
    { 334, "CELLULAR_STATUS", "quality", "quality" },
    { 334, "CELLULAR_STATUS", "mcc", "mcc" },
    { 334, "CELLULAR_STATUS", "mnc", "mnc" },
    { 334, "CELLULAR_STATUS", "lac", "lac" },
    { 335, "ISBD_LINK_STATUS", "timestamp", "timestamp (us)" },
    { 335, "ISBD_LINK_STATUS", "last_heartbeat", "last_heartbeat (us)" },
    { 335, "ISBD_LINK_STATUS", "failed_sessions", "failed_sessions" },
    { 335, "ISBD_LINK_STATUS", "successful_sessions", "successful_sessions" },
    { 335, "ISBD_LINK_STATUS", "signal_quality", "signal_quality" },
    { 335, "ISBD_LINK_STATUS", "ring_pending", "ring_pending" },
    { 335, "ISBD_LINK_STATUS", "tx_session_pending", "tx_session_pending" },
    { 335, "ISBD_LINK_STATUS", "rx_session_pending", "rx_session_pending" },
    { 339, "RAW_RPM", "index", "index" },
    { 339, "RAW_RPM", "frequency", "frequency (rpm)" },
    { 340, "UTM_GLOBAL_POSITION", "time", "time (us)" },
    { 340, "UTM_GLOBAL_POSITION", "lat", "lat (degE7)" },
    { 340, "UTM_GLOBAL_POSITION", "lon", "lon (degE7)" },
    { 340, "UTM_GLOBAL_POSITION", "alt", "alt (mm)" },
    { 340, "UTM_GLOBAL_POSITION", "relative_alt", "relative_alt (mm)" },
    { 340, "UTM_GLOBAL_POSITION", "vx", "vx (cm/s)" },
    { 340, "UTM_GLOBAL_POSITION", "vy", "vy (cm/s)" },
    { 340, "UTM_GLOBAL_POSITION", "vz", "vz (cm/s)" },
    { 340, "UTM_GLOBAL_POSITION", "h_acc", "h_acc (mm)" },
    { 340, "UTM_GLOBAL_POSITION", "v_acc", "v_acc (mm)" },
    { 340, "UTM_GLOBAL_POSITION", "vel_acc", "vel_acc (cm/s)" },
    { 340, "UTM_GLOBAL_POSITION", "next_lat", "next_lat (degE7)" },
    { 340, "UTM_GLOBAL_POSITION", "next_lon", "next_lon (degE7)" },
    { 340, "UTM_GLOBAL_POSITION", "next_alt", "next_alt (mm)" },
    { 340, "UTM_GLOBAL_POSITION", "update_rate", "update_rate (cs)" },
    { 340, "UTM_GLOBAL_POSITION", "flight_state", "flight_state" },
    { 340, "UTM_GLOBAL_POSITION", "flags", "flags" },
    { 345, "PARAM_ERROR", "target_system", "target_system" },
    { 345, "PARAM_ERROR", "target_component", "target_component" },
    { 345, "PARAM_ERROR", "param_index", "param_index" },
    { 345, "PARAM_ERROR", "error", "error" },
    { 350, "DEBUG_FLOAT_ARRAY", "time_usec", "time_usec (us)" },
    { 350, "DEBUG_FLOAT_ARRAY", "array_id", "array_id" },
    { 360, "ORBIT_EXECUTION_STATUS", "time_usec", "time_usec (us)" },
    { 360, "ORBIT_EXECUTION_STATUS", "radius", "radius (m)" },
    { 360, "ORBIT_EXECUTION_STATUS", "frame", "frame" },
    { 360, "ORBIT_EXECUTION_STATUS", "x", "x" },
    { 360, "ORBIT_EXECUTION_STATUS", "y", "y" },
    { 360, "ORBIT_EXECUTION_STATUS", "z", "z (m)" },
    { 361, "FIGURE_EIGHT_EXECUTION_STATUS", "time_usec", "time_usec (us)" },
    { 361, "FIGURE_EIGHT_EXECUTION_STATUS", "major_radius", "major_radius (m)" },
    { 361, "FIGURE_EIGHT_EXECUTION_STATUS", "minor_radius", "minor_radius (m)" },
    { 361, "FIGURE_EIGHT_EXECUTION_STATUS", "orientation", "orientation (rad)" },
    { 361, "FIGURE_EIGHT_EXECUTION_STATUS", "frame", "frame" },
    { 361, "FIGURE_EIGHT_EXECUTION_STATUS", "x", "x" },
    { 361, "FIGURE_EIGHT_EXECUTION_STATUS", "y", "y" },
    { 361, "FIGURE_EIGHT_EXECUTION_STATUS", "z", "z (m)" },
    { 370, "SMART_BATTERY_INFO", "id", "id" },
    { 370, "SMART_BATTERY_INFO", "battery_function", "battery_function" },
    { 370, "SMART_BATTERY_INFO", "type", "type" },
    { 370, "SMART_BATTERY_INFO", "capacity_full_specification", "capacity_full_specification (mAh)" },
    { 370, "SMART_BATTERY_INFO", "capacity_full", "capacity_full (mAh)" },
    { 370, "SMART_BATTERY_INFO", "cycle_count", "cycle_count" },
    { 370, "SMART_BATTERY_INFO", "weight", "weight (g)" },
    { 370, "SMART_BATTERY_INFO", "discharge_minimum_voltage", "discharge_minimum_voltage (mV)" },
    { 370, "SMART_BATTERY_INFO", "charging_minimum_voltage", "charging_minimum_voltage (mV)" },
    { 370, "SMART_BATTERY_INFO", "resting_minimum_voltage", "resting_minimum_voltage (mV)" },
    { 370, "SMART_BATTERY_INFO", "charging_maximum_voltage", "charging_maximum_voltage (mV)" },
    { 370, "SMART_BATTERY_INFO", "cells_in_series", "cells_in_series" },
    { 370, "SMART_BATTERY_INFO", "discharge_maximum_current", "discharge_maximum_current (mA)" },
    { 370, "SMART_BATTERY_INFO", "discharge_maximum_burst_current", "discharge_maximum_burst_current (mA)" },
    { 371, "FUEL_STATUS", "id", "id" },
    { 371, "FUEL_STATUS", "maximum_fuel", "maximum_fuel" },
    { 371, "FUEL_STATUS", "consumed_fuel", "consumed_fuel" },
    { 371, "FUEL_STATUS", "remaining_fuel", "remaining_fuel" },
    { 371, "FUEL_STATUS", "percent_remaining", "percent_remaining (%)" },
    { 371, "FUEL_STATUS", "flow_rate", "flow_rate" },
    { 371, "FUEL_STATUS", "temperature", "temperature (K)" },
    { 371, "FUEL_STATUS", "fuel_type", "fuel_type" },
    { 372, "BATTERY_INFO", "id", "id" },
    { 372, "BATTERY_INFO", "battery_function", "battery_function" },
    { 372, "BATTERY_INFO", "type", "type" },
    { 372, "BATTERY_INFO", "state_of_health", "state_of_health (%)" },
    { 372, "BATTERY_INFO", "cells_in_series", "cells_in_series" },
    { 372, "BATTERY_INFO", "cycle_count", "cycle_count" },
    { 372, "BATTERY_INFO", "weight", "weight (g)" },
    { 372, "BATTERY_INFO", "discharge_minimum_voltage", "discharge_minimum_voltage (V)" },
    { 372, "BATTERY_INFO", "charging_minimum_voltage", "charging_minimum_voltage (V)" },
    { 372, "BATTERY_INFO", "resting_minimum_voltage", "resting_minimum_voltage (V)" },
    { 372, "BATTERY_INFO", "charging_maximum_voltage", "charging_maximum_voltage (V)" },
    { 372, "BATTERY_INFO", "charging_maximum_current", "charging_maximum_current (A)" },
    { 372, "BATTERY_INFO", "nominal_voltage", "nominal_voltage (V)" },
    { 372, "BATTERY_INFO", "discharge_maximum_current", "discharge_maximum_current (A)" },
    { 372, "BATTERY_INFO", "discharge_maximum_burst_current", "discharge_maximum_burst_current (A)" },
    { 372, "BATTERY_INFO", "design_capacity", "design_capacity (Ah)" },
    { 372, "BATTERY_INFO", "full_charge_capacity", "full_charge_capacity (Ah)" },
    { 373, "GENERATOR_STATUS", "status", "status" },
    { 373, "GENERATOR_STATUS", "generator_speed", "generator_speed (rpm)" },
    { 373, "GENERATOR_STATUS", "battery_current", "battery_current (A)" },
    { 373, "GENERATOR_STATUS", "load_current", "load_current (A)" },
    { 373, "GENERATOR_STATUS", "power_generated", "power_generated (W)" },
    { 373, "GENERATOR_STATUS", "bus_voltage", "bus_voltage (V)" },
    { 373, "GENERATOR_STATUS", "rectifier_temperature", "rectifier_temperature (degC)" },
    { 373, "GENERATOR_STATUS", "bat_current_setpoint", "bat_current_setpoint (A)" },
    { 373, "GENERATOR_STATUS", "generator_temperature", "generator_temperature (degC)" },
    { 373, "GENERATOR_STATUS", "runtime", "runtime (s)" },
    { 373, "GENERATOR_STATUS", "time_until_maintenance", "time_until_maintenance (s)" },
    { 375, "ACTUATOR_OUTPUT_STATUS", "time_usec", "time_usec (us)" },
    { 375, "ACTUATOR_OUTPUT_STATUS", "active", "active" },
    { 376, "RELAY_STATUS", "time_boot_ms", "time_boot_ms (ms)" },
    { 376, "RELAY_STATUS", "on", "on" },
    { 376, "RELAY_STATUS", "present", "present" },
    { 380, "TIME_ESTIMATE_TO_TARGET", "safe_return", "safe_return (s)" },
    { 380, "TIME_ESTIMATE_TO_TARGET", "land", "land (s)" },
    { 380, "TIME_ESTIMATE_TO_TARGET", "mission_next_item", "mission_next_item (s)" },
    { 380, "TIME_ESTIMATE_TO_TARGET", "mission_end", "mission_end (s)" },
    { 380, "TIME_ESTIMATE_TO_TARGET", "commanded_action", "commanded_action (s)" },
    { 385, "TUNNEL", "target_system", "target_system" },
    { 385, "TUNNEL", "target_component", "target_component" },
    { 385, "TUNNEL", "payload_type", "payload_type" },
    { 385, "TUNNEL", "payload_length", "payload_length" },
    { 386, "CAN_FRAME", "target_system", "target_system" },
    { 386, "CAN_FRAME", "target_component", "target_component" },
    { 386, "CAN_FRAME", "bus", "bus" },
    { 386, "CAN_FRAME", "len", "len" },
    { 386, "CAN_FRAME", "id", "id" },
    { 387, "CANFD_FRAME", "target_system", "target_system" },
    { 387, "CANFD_FRAME", "target_component", "target_component" },
    { 387, "CANFD_FRAME", "bus", "bus" },
    { 387, "CANFD_FRAME", "len", "len" },
    { 387, "CANFD_FRAME", "id", "id" },
    { 390, "ONBOARD_COMPUTER_STATUS", "time_usec", "time_usec (us)" },
    { 390, "ONBOARD_COMPUTER_STATUS", "uptime", "uptime (ms)" },
    { 390, "ONBOARD_COMPUTER_STATUS", "type", "type" },
    { 390, "ONBOARD_COMPUTER_STATUS", "gpu_cores[0]", "gpu_cores[0]" },
    { 390, "ONBOARD_COMPUTER_STATUS", "gpu_cores[1]", "gpu_cores[1]" },
    { 390, "ONBOARD_COMPUTER_STATUS", "gpu_cores[2]", "gpu_cores[2]" },
    { 390, "ONBOARD_COMPUTER_STATUS", "gpu_cores[3]", "gpu_cores[3]" },
    { 390, "ONBOARD_COMPUTER_STATUS", "temperature_board", "temperature_board (degC)" },
    { 390, "ONBOARD_COMPUTER_STATUS", "fan_speed[0]", "fan_speed[0] (rpm)" },
    { 390, "ONBOARD_COMPUTER_STATUS", "fan_speed[1]", "fan_speed[1] (rpm)" },
    { 390, "ONBOARD_COMPUTER_STATUS", "fan_speed[2]", "fan_speed[2] (rpm)" },
    { 390, "ONBOARD_COMPUTER_STATUS", "fan_speed[3]", "fan_speed[3] (rpm)" },
    { 390, "ONBOARD_COMPUTER_STATUS", "ram_usage", "ram_usage (MiB)" },
    { 390, "ONBOARD_COMPUTER_STATUS", "ram_total", "ram_total (MiB)" },
    { 390, "ONBOARD_COMPUTER_STATUS", "storage_type[0]", "storage_type[0]" },
    { 390, "ONBOARD_COMPUTER_STATUS", "storage_type[1]", "storage_type[1]" },
    { 390, "ONBOARD_COMPUTER_STATUS", "storage_type[2]", "storage_type[2]" },
    { 390, "ONBOARD_COMPUTER_STATUS", "storage_type[3]", "storage_type[3]" },
    { 390, "ONBOARD_COMPUTER_STATUS", "storage_usage[0]", "storage_usage[0] (MiB)" },
    { 390, "ONBOARD_COMPUTER_STATUS", "storage_usage[1]", "storage_usage[1] (MiB)" },
    { 390, "ONBOARD_COMPUTER_STATUS", "storage_usage[2]", "storage_usage[2] (MiB)" },
    { 390, "ONBOARD_COMPUTER_STATUS", "storage_usage[3]", "storage_usage[3] (MiB)" },
    { 390, "ONBOARD_COMPUTER_STATUS", "storage_total[0]", "storage_total[0] (MiB)" },
    { 390, "ONBOARD_COMPUTER_STATUS", "storage_total[1]", "storage_total[1] (MiB)" },
    { 390, "ONBOARD_COMPUTER_STATUS", "storage_total[2]", "storage_total[2] (MiB)" },
    { 390, "ONBOARD_COMPUTER_STATUS", "storage_total[3]", "storage_total[3] (MiB)" },
    { 390, "ONBOARD_COMPUTER_STATUS", "status_flags", "status_flags" },
    { 395, "COMPONENT_INFORMATION", "time_boot_ms", "time_boot_ms (ms)" },
    { 395, "COMPONENT_INFORMATION", "general_metadata_file_crc", "general_metadata_file_crc" },
    { 395, "COMPONENT_INFORMATION", "peripherals_metadata_file_crc", "peripherals_metadata_file_crc" },
    { 396, "COMPONENT_INFORMATION_BASIC", "time_boot_ms", "time_boot_ms (ms)" },
    { 396, "COMPONENT_INFORMATION_BASIC", "capabilities", "capabilities" },
    { 396, "COMPONENT_INFORMATION_BASIC", "time_manufacture_s", "time_manufacture_s (s)" },
    { 397, "COMPONENT_METADATA", "time_boot_ms", "time_boot_ms (ms)" },
    { 397, "COMPONENT_METADATA", "file_crc", "file_crc" },
    { 401, "SUPPORTED_TUNES", "target_system", "target_system" },
    { 401, "SUPPORTED_TUNES", "target_component", "target_component" },
    { 401, "SUPPORTED_TUNES", "format", "format" },
    { 410, "EVENT", "destination_component", "destination_component" },
    { 410, "EVENT", "destination_system", "destination_system" },
    { 410, "EVENT", "id", "id" },
    { 410, "EVENT", "event_time_boot_ms", "event_time_boot_ms (ms)" },
    { 410, "EVENT", "sequence", "sequence" },
    { 410, "EVENT", "log_levels", "log_levels" },
    { 411, "CURRENT_EVENT_SEQUENCE", "sequence", "sequence" },
    { 411, "CURRENT_EVENT_SEQUENCE", "flags", "flags" },
    { 413, "RESPONSE_EVENT_ERROR", "target_system", "target_system" },
    { 413, "RESPONSE_EVENT_ERROR", "target_component", "target_component" },
    { 413, "RESPONSE_EVENT_ERROR", "sequence", "sequence" },
    { 413, "RESPONSE_EVENT_ERROR", "sequence_oldest_available", "sequence_oldest_available" },
    { 413, "RESPONSE_EVENT_ERROR", "reason", "reason" },
    { 435, "AVAILABLE_MODES", "number_modes", "number_modes" },
    { 435, "AVAILABLE_MODES", "mode_index", "mode_index" },
    { 435, "AVAILABLE_MODES", "standard_mode", "standard_mode" },
    { 435, "AVAILABLE_MODES", "custom_mode", "custom_mode" },
    { 435, "AVAILABLE_MODES", "properties", "properties" },
    { 436, "CURRENT_MODE", "standard_mode", "standard_mode" },
    { 436, "CURRENT_MODE", "custom_mode", "custom_mode" },
    { 436, "CURRENT_MODE", "intended_custom_mode", "intended_custom_mode" },
    { 437, "AVAILABLE_MODES_MONITOR", "seq", "seq" },
    { 440, "ILLUMINATOR_STATUS", "uptime_ms", "uptime_ms (ms)" },
    { 440, "ILLUMINATOR_STATUS", "enable", "enable" },
    { 440, "ILLUMINATOR_STATUS", "mode_bitmask", "mode_bitmask" },
    { 440, "ILLUMINATOR_STATUS", "error_status", "error_status" },
    { 440, "ILLUMINATOR_STATUS", "mode", "mode" },
    { 440, "ILLUMINATOR_STATUS", "brightness", "brightness (%)" },
    { 440, "ILLUMINATOR_STATUS", "strobe_period", "strobe_period (s)" },
    { 440, "ILLUMINATOR_STATUS", "strobe_duty_cycle", "strobe_duty_cycle (%)" },
    { 440, "ILLUMINATOR_STATUS", "temp_c", "temp_c" },
    { 440, "ILLUMINATOR_STATUS", "min_strobe_period", "min_strobe_period (s)" },
    { 440, "ILLUMINATOR_STATUS", "max_strobe_period", "max_strobe_period (s)" },
    { 9000, "WHEEL_DISTANCE", "time_usec", "time_usec (us)" },
    { 9000, "WHEEL_DISTANCE", "count", "count" },
    { 9005, "WINCH_STATUS", "time_usec", "time_usec (us)" },
    { 9005, "WINCH_STATUS", "line_length", "line_length (m)" },
    { 9005, "WINCH_STATUS", "speed", "speed (m/s)" },
    { 9005, "WINCH_STATUS", "tension", "tension (kg)" },
    { 9005, "WINCH_STATUS", "voltage", "voltage (V)" },
    { 9005, "WINCH_STATUS", "current", "current (A)" },
    { 9005, "WINCH_STATUS", "temperature", "temperature (degC)" },
    { 9005, "WINCH_STATUS", "status", "status" },
    { 10001, "UAVIONIX_ADSB_OUT_CFG", "ICAO", "ICAO" },
    { 10001, "UAVIONIX_ADSB_OUT_CFG", "emitterType", "emitterType" },
    { 10001, "UAVIONIX_ADSB_OUT_CFG", "aircraftSize", "aircraftSize" },
    { 10001, "UAVIONIX_ADSB_OUT_CFG", "gpsOffsetLat", "gpsOffsetLat" },
    { 10001, "UAVIONIX_ADSB_OUT_CFG", "gpsOffsetLon", "gpsOffsetLon" },
    { 10001, "UAVIONIX_ADSB_OUT_CFG", "stallSpeed", "stallSpeed (cm/s)" },
    { 10001, "UAVIONIX_ADSB_OUT_CFG", "rfSelect", "rfSelect" },
    { 10002, "UAVIONIX_ADSB_OUT_DYNAMIC", "utcTime", "utcTime (s)" },
    { 10002, "UAVIONIX_ADSB_OUT_DYNAMIC", "gpsLat", "gpsLat (degE7)" },
    { 10002, "UAVIONIX_ADSB_OUT_DYNAMIC", "gpsLon", "gpsLon (degE7)" },
    { 10002, "UAVIONIX_ADSB_OUT_DYNAMIC", "gpsAlt", "gpsAlt (mm)" },
    { 10002, "UAVIONIX_ADSB_OUT_DYNAMIC", "gpsFix", "gpsFix" },
    { 10002, "UAVIONIX_ADSB_OUT_DYNAMIC", "numSats", "numSats" },
    { 10002, "UAVIONIX_ADSB_OUT_DYNAMIC", "baroAltMSL", "baroAltMSL (mbar)" },
    { 10002, "UAVIONIX_ADSB_OUT_DYNAMIC", "accuracyHor", "accuracyHor (mm)" },
    { 10002, "UAVIONIX_ADSB_OUT_DYNAMIC", "accuracyVert", "accuracyVert (cm)" },
    { 10002, "UAVIONIX_ADSB_OUT_DYNAMIC", "accuracyVel", "accuracyVel (mm/s)" },
    { 10002, "UAVIONIX_ADSB_OUT_DYNAMIC", "velVert", "velVert (cm/s)" },
    { 10002, "UAVIONIX_ADSB_OUT_DYNAMIC", "velNS", "velNS (cm/s)" },
    { 10002, "UAVIONIX_ADSB_OUT_DYNAMIC", "VelEW", "VelEW (cm/s)" },
    { 10002, "UAVIONIX_ADSB_OUT_DYNAMIC", "emergencyStatus", "emergencyStatus" },
    { 10002, "UAVIONIX_ADSB_OUT_DYNAMIC", "state", "state" },
    { 10002, "UAVIONIX_ADSB_OUT_DYNAMIC", "squawk", "squawk" },
    { 10003, "UAVIONIX_ADSB_TRANSCEIVER_HEALTH_REPORT", "rfHealth", "rfHealth" },
    { 10006, "UAVIONIX_ADSB_GET", "ReqMessageId", "ReqMessageId" },
    { 10007, "UAVIONIX_ADSB_OUT_CONTROL", "state", "state" },
    { 10007, "UAVIONIX_ADSB_OUT_CONTROL", "baroAltMSL", "baroAltMSL (mbar)" },
    { 10007, "UAVIONIX_ADSB_OUT_CONTROL", "squawk", "squawk" },
    { 10007, "UAVIONIX_ADSB_OUT_CONTROL", "emergencyStatus", "emergencyStatus" },
    { 10007, "UAVIONIX_ADSB_OUT_CONTROL", "x_bit", "x_bit" },
    { 10008, "UAVIONIX_ADSB_OUT_STATUS", "state", "state" },
    { 10008, "UAVIONIX_ADSB_OUT_STATUS", "squawk", "squawk" },
    { 10008, "UAVIONIX_ADSB_OUT_STATUS", "NIC_NACp", "NIC_NACp" },
    { 10008, "UAVIONIX_ADSB_OUT_STATUS", "boardTemp", "boardTemp" },
    { 10008, "UAVIONIX_ADSB_OUT_STATUS", "fault", "fault" },
    { 10151, "LOWEHEISER_GOV_EFI", "volt_batt", "volt_batt (V)" },
    { 10151, "LOWEHEISER_GOV_EFI", "curr_batt", "curr_batt (A)" },
    { 10151, "LOWEHEISER_GOV_EFI", "curr_gen", "curr_gen (A)" },
    { 10151, "LOWEHEISER_GOV_EFI", "curr_rot", "curr_rot (A)" },
    { 10151, "LOWEHEISER_GOV_EFI", "fuel_level", "fuel_level (l)" },
    { 10151, "LOWEHEISER_GOV_EFI", "throttle", "throttle (%)" },
    { 10151, "LOWEHEISER_GOV_EFI", "runtime", "runtime (s)" },
    { 10151, "LOWEHEISER_GOV_EFI", "until_maintenance", "until_maintenance (s)" },
    { 10151, "LOWEHEISER_GOV_EFI", "rectifier_temp", "rectifier_temp (degC)" },
    { 10151, "LOWEHEISER_GOV_EFI", "generator_temp", "generator_temp (degC)" },
    { 10151, "LOWEHEISER_GOV_EFI", "efi_batt", "efi_batt (V)" },
    { 10151, "LOWEHEISER_GOV_EFI", "efi_rpm", "efi_rpm (rpm)" },
    { 10151, "LOWEHEISER_GOV_EFI", "efi_pw", "efi_pw (ms)" },
    { 10151, "LOWEHEISER_GOV_EFI", "efi_fuel_flow", "efi_fuel_flow" },
    { 10151, "LOWEHEISER_GOV_EFI", "efi_fuel_consumed", "efi_fuel_consumed (l)" },
    { 10151, "LOWEHEISER_GOV_EFI", "efi_baro", "efi_baro (kPa)" },
    { 10151, "LOWEHEISER_GOV_EFI", "efi_mat", "efi_mat (degC)" },
    { 10151, "LOWEHEISER_GOV_EFI", "efi_clt", "efi_clt (degC)" },
    { 10151, "LOWEHEISER_GOV_EFI", "efi_tps", "efi_tps (%)" },
    { 10151, "LOWEHEISER_GOV_EFI", "efi_exhaust_gas_temperature", "efi_exhaust_gas_temperature (degC)" },
    { 10151, "LOWEHEISER_GOV_EFI", "efi_index", "efi_index" },
    { 10151, "LOWEHEISER_GOV_EFI", "generator_status", "generator_status" },
    { 10151, "LOWEHEISER_GOV_EFI", "efi_status", "efi_status" },
    { 11000, "DEVICE_OP_READ", "target_system", "target_system" },
    { 11000, "DEVICE_OP_READ", "target_component", "target_component" },
    { 11000, "DEVICE_OP_READ", "request_id", "request_id" },
    { 11000, "DEVICE_OP_READ", "bustype", "bustype" },
    { 11000, "DEVICE_OP_READ", "bus", "bus" },
    { 11000, "DEVICE_OP_READ", "address", "address" },
    { 11000, "DEVICE_OP_READ", "regstart", "regstart" },
    { 11000, "DEVICE_OP_READ", "count", "count" },
    { 11000, "DEVICE_OP_READ", "bank", "bank" },
    { 11001, "DEVICE_OP_READ_REPLY", "request_id", "request_id" },
    { 11001, "DEVICE_OP_READ_REPLY", "result", "result" },
    { 11001, "DEVICE_OP_READ_REPLY", "regstart", "regstart" },
    { 11001, "DEVICE_OP_READ_REPLY", "count", "count" },
    { 11001, "DEVICE_OP_READ_REPLY", "bank", "bank" },
    { 11003, "DEVICE_OP_WRITE_REPLY", "request_id", "request_id" },
    { 11003, "DEVICE_OP_WRITE_REPLY", "result", "result" },
    { 11005, "SECURE_COMMAND_REPLY", "sequence", "sequence" },
    { 11005, "SECURE_COMMAND_REPLY", "operation", "operation" },
    { 11005, "SECURE_COMMAND_REPLY", "result", "result" },
    { 11005, "SECURE_COMMAND_REPLY", "data_length", "data_length" },
    { 11010, "ADAP_TUNING", "axis", "axis" },
    { 11010, "ADAP_TUNING", "desired", "desired (deg/s)" },
    { 11010, "ADAP_TUNING", "achieved", "achieved (deg/s)" },
    { 11010, "ADAP_TUNING", "error", "error" },
    { 11010, "ADAP_TUNING", "theta", "theta" },
    { 11010, "ADAP_TUNING", "omega", "omega" },
    { 11010, "ADAP_TUNING", "sigma", "sigma" },
    { 11010, "ADAP_TUNING", "theta_dot", "theta_dot" },
    { 11010, "ADAP_TUNING", "omega_dot", "omega_dot" },
    { 11010, "ADAP_TUNING", "sigma_dot", "sigma_dot" },
    { 11010, "ADAP_TUNING", "f", "f" },
    { 11010, "ADAP_TUNING", "f_dot", "f_dot" },
    { 11010, "ADAP_TUNING", "u", "u" },
    { 11011, "VISION_POSITION_DELTA", "time_usec", "time_usec (us)" },
    { 11011, "VISION_POSITION_DELTA", "time_delta_usec", "time_delta_usec (us)" },
    { 11011, "VISION_POSITION_DELTA", "angle_delta[0]", "angle_delta[0] (rad)" },
    { 11011, "VISION_POSITION_DELTA", "angle_delta[1]", "angle_delta[1] (rad)" },
    { 11011, "VISION_POSITION_DELTA", "angle_delta[2]", "angle_delta[2] (rad)" },
    { 11011, "VISION_POSITION_DELTA", "position_delta[0]", "position_delta[0] (m)" },
    { 11011, "VISION_POSITION_DELTA", "position_delta[1]", "position_delta[1] (m)" },
    { 11011, "VISION_POSITION_DELTA", "position_delta[2]", "position_delta[2] (m)" },
    { 11011, "VISION_POSITION_DELTA", "confidence", "confidence (%)" },
    { 11020, "AOA_SSA", "time_usec", "time_usec (us)" },
    { 11020, "AOA_SSA", "AOA", "AOA (deg)" },
    { 11020, "AOA_SSA", "SSA", "SSA (deg)" },
    { 11030, "ESC_TELEMETRY_1_TO_4", "temperature[0]", "temperature[0] (degC)" },
    { 11030, "ESC_TELEMETRY_1_TO_4", "temperature[1]", "temperature[1] (degC)" },
    { 11030, "ESC_TELEMETRY_1_TO_4", "temperature[2]", "temperature[2] (degC)" },
    { 11030, "ESC_TELEMETRY_1_TO_4", "temperature[3]", "temperature[3] (degC)" },
    { 11030, "ESC_TELEMETRY_1_TO_4", "voltage[0]", "voltage[0] (cV)" },
    { 11030, "ESC_TELEMETRY_1_TO_4", "voltage[1]", "voltage[1] (cV)" },
    { 11030, "ESC_TELEMETRY_1_TO_4", "voltage[2]", "voltage[2] (cV)" },
    { 11030, "ESC_TELEMETRY_1_TO_4", "voltage[3]", "voltage[3] (cV)" },
    { 11030, "ESC_TELEMETRY_1_TO_4", "current[0]", "current[0] (cA)" },
    { 11030, "ESC_TELEMETRY_1_TO_4", "current[1]", "current[1] (cA)" },
    { 11030, "ESC_TELEMETRY_1_TO_4", "current[2]", "current[2] (cA)" },
    { 11030, "ESC_TELEMETRY_1_TO_4", "current[3]", "current[3] (cA)" },
    { 11030, "ESC_TELEMETRY_1_TO_4", "totalcurrent[0]", "totalcurrent[0] (mAh)" },
    { 11030, "ESC_TELEMETRY_1_TO_4", "totalcurrent[1]", "totalcurrent[1] (mAh)" },
    { 11030, "ESC_TELEMETRY_1_TO_4", "totalcurrent[2]", "totalcurrent[2] (mAh)" },
    { 11030, "ESC_TELEMETRY_1_TO_4", "totalcurrent[3]", "totalcurrent[3] (mAh)" },
    { 11030, "ESC_TELEMETRY_1_TO_4", "rpm[0]", "rpm[0] (rpm)" },
    { 11030, "ESC_TELEMETRY_1_TO_4", "rpm[1]", "rpm[1] (rpm)" },
    { 11030, "ESC_TELEMETRY_1_TO_4", "rpm[2]", "rpm[2] (rpm)" },
    { 11030, "ESC_TELEMETRY_1_TO_4", "rpm[3]", "rpm[3] (rpm)" },
    { 11030, "ESC_TELEMETRY_1_TO_4", "count[0]", "count[0]" },
    { 11030, "ESC_TELEMETRY_1_TO_4", "count[1]", "count[1]" },
    { 11030, "ESC_TELEMETRY_1_TO_4", "count[2]", "count[2]" },
    { 11030, "ESC_TELEMETRY_1_TO_4", "count[3]", "count[3]" },
    { 11031, "ESC_TELEMETRY_5_TO_8", "temperature[0]", "temperature[0] (degC)" },
    { 11031, "ESC_TELEMETRY_5_TO_8", "temperature[1]", "temperature[1] (degC)" },
    { 11031, "ESC_TELEMETRY_5_TO_8", "temperature[2]", "temperature[2] (degC)" },
    { 11031, "ESC_TELEMETRY_5_TO_8", "temperature[3]", "temperature[3] (degC)" },
    { 11031, "ESC_TELEMETRY_5_TO_8", "voltage[0]", "voltage[0] (cV)" },
    { 11031, "ESC_TELEMETRY_5_TO_8", "voltage[1]", "voltage[1] (cV)" },
    { 11031, "ESC_TELEMETRY_5_TO_8", "voltage[2]", "voltage[2] (cV)" },
    { 11031, "ESC_TELEMETRY_5_TO_8", "voltage[3]", "voltage[3] (cV)" },
    { 11031, "ESC_TELEMETRY_5_TO_8", "current[0]", "current[0] (cA)" },
    { 11031, "ESC_TELEMETRY_5_TO_8", "current[1]", "current[1] (cA)" },
    { 11031, "ESC_TELEMETRY_5_TO_8", "current[2]", "current[2] (cA)" },
    { 11031, "ESC_TELEMETRY_5_TO_8", "current[3]", "current[3] (cA)" },
    { 11031, "ESC_TELEMETRY_5_TO_8", "totalcurrent[0]", "totalcurrent[0] (mAh)" },
    { 11031, "ESC_TELEMETRY_5_TO_8", "totalcurrent[1]", "totalcurrent[1] (mAh)" },
    { 11031, "ESC_TELEMETRY_5_TO_8", "totalcurrent[2]", "totalcurrent[2] (mAh)" },
    { 11031, "ESC_TELEMETRY_5_TO_8", "totalcurrent[3]", "totalcurrent[3] (mAh)" },
    { 11031, "ESC_TELEMETRY_5_TO_8", "rpm[0]", "rpm[0] (rpm)" },
    { 11031, "ESC_TELEMETRY_5_TO_8", "rpm[1]", "rpm[1] (rpm)" },
    { 11031, "ESC_TELEMETRY_5_TO_8", "rpm[2]", "rpm[2] (rpm)" },
    { 11031, "ESC_TELEMETRY_5_TO_8", "rpm[3]", "rpm[3] (rpm)" },
    { 11031, "ESC_TELEMETRY_5_TO_8", "count[0]", "count[0]" },
    { 11031, "ESC_TELEMETRY_5_TO_8", "count[1]", "count[1]" },
    { 11031, "ESC_TELEMETRY_5_TO_8", "count[2]", "count[2]" },
    { 11031, "ESC_TELEMETRY_5_TO_8", "count[3]", "count[3]" },
    { 11032, "ESC_TELEMETRY_9_TO_12", "temperature[0]", "temperature[0] (degC)" },
    { 11032, "ESC_TELEMETRY_9_TO_12", "temperature[1]", "temperature[1] (degC)" },
    { 11032, "ESC_TELEMETRY_9_TO_12", "temperature[2]", "temperature[2] (degC)" },
    { 11032, "ESC_TELEMETRY_9_TO_12", "temperature[3]", "temperature[3] (degC)" },
    { 11032, "ESC_TELEMETRY_9_TO_12", "voltage[0]", "voltage[0] (cV)" },
    { 11032, "ESC_TELEMETRY_9_TO_12", "voltage[1]", "voltage[1] (cV)" },
    { 11032, "ESC_TELEMETRY_9_TO_12", "voltage[2]", "voltage[2] (cV)" },
    { 11032, "ESC_TELEMETRY_9_TO_12", "voltage[3]", "voltage[3] (cV)" },
    { 11032, "ESC_TELEMETRY_9_TO_12", "current[0]", "current[0] (cA)" },
    { 11032, "ESC_TELEMETRY_9_TO_12", "current[1]", "current[1] (cA)" },
    { 11032, "ESC_TELEMETRY_9_TO_12", "current[2]", "current[2] (cA)" },
    { 11032, "ESC_TELEMETRY_9_TO_12", "current[3]", "current[3] (cA)" },
    { 11032, "ESC_TELEMETRY_9_TO_12", "totalcurrent[0]", "totalcurrent[0] (mAh)" },
    { 11032, "ESC_TELEMETRY_9_TO_12", "totalcurrent[1]", "totalcurrent[1] (mAh)" },
    { 11032, "ESC_TELEMETRY_9_TO_12", "totalcurrent[2]", "totalcurrent[2] (mAh)" },
    { 11032, "ESC_TELEMETRY_9_TO_12", "totalcurrent[3]", "totalcurrent[3] (mAh)" },
    { 11032, "ESC_TELEMETRY_9_TO_12", "rpm[0]", "rpm[0] (rpm)" },
    { 11032, "ESC_TELEMETRY_9_TO_12", "rpm[1]", "rpm[1] (rpm)" },
    { 11032, "ESC_TELEMETRY_9_TO_12", "rpm[2]", "rpm[2] (rpm)" },
    { 11032, "ESC_TELEMETRY_9_TO_12", "rpm[3]", "rpm[3] (rpm)" },
    { 11032, "ESC_TELEMETRY_9_TO_12", "count[0]", "count[0]" },
    { 11032, "ESC_TELEMETRY_9_TO_12", "count[1]", "count[1]" },
    { 11032, "ESC_TELEMETRY_9_TO_12", "count[2]", "count[2]" },
    { 11032, "ESC_TELEMETRY_9_TO_12", "count[3]", "count[3]" },
    { 11034, "OSD_PARAM_CONFIG_REPLY", "request_id", "request_id" },
    { 11034, "OSD_PARAM_CONFIG_REPLY", "result", "result" },
    { 11036, "OSD_PARAM_SHOW_CONFIG_REPLY", "request_id", "request_id" },
    { 11036, "OSD_PARAM_SHOW_CONFIG_REPLY", "result", "result" },
    { 11036, "OSD_PARAM_SHOW_CONFIG_REPLY", "config_type", "config_type" },
    { 11036, "OSD_PARAM_SHOW_CONFIG_REPLY", "min_value", "min_value" },
    { 11036, "OSD_PARAM_SHOW_CONFIG_REPLY", "max_value", "max_value" },
    { 11036, "OSD_PARAM_SHOW_CONFIG_REPLY", "increment", "increment" },
    { 11037, "OBSTACLE_DISTANCE_3D", "time_boot_ms", "time_boot_ms (ms)" },
    { 11037, "OBSTACLE_DISTANCE_3D", "sensor_type", "sensor_type" },
    { 11037, "OBSTACLE_DISTANCE_3D", "frame", "frame" },
    { 11037, "OBSTACLE_DISTANCE_3D", "obstacle_id", "obstacle_id" },
    { 11037, "OBSTACLE_DISTANCE_3D", "x", "x (m)" },
    { 11037, "OBSTACLE_DISTANCE_3D", "y", "y (m)" },
    { 11037, "OBSTACLE_DISTANCE_3D", "z", "z (m)" },
    { 11037, "OBSTACLE_DISTANCE_3D", "min_distance", "min_distance (m)" },
    { 11037, "OBSTACLE_DISTANCE_3D", "max_distance", "max_distance (m)" },
    { 11038, "WATER_DEPTH", "time_boot_ms", "time_boot_ms (ms)" },
    { 11038, "WATER_DEPTH", "id", "id" },
    { 11038, "WATER_DEPTH", "healthy", "healthy" },
    { 11038, "WATER_DEPTH", "lat", "lat (degE7)" },
    { 11038, "WATER_DEPTH", "lng", "lng (degE7)" },
    { 11038, "WATER_DEPTH", "alt", "alt (m)" },
    { 11038, "WATER_DEPTH", "roll", "roll (rad)" },
    { 11038, "WATER_DEPTH", "pitch", "pitch (rad)" },
    { 11038, "WATER_DEPTH", "yaw", "yaw (rad)" },
    { 11038, "WATER_DEPTH", "distance", "distance (m)" },
    { 11038, "WATER_DEPTH", "temperature", "temperature (degC)" },
    { 11039, "MCU_STATUS", "id", "id" },
    { 11039, "MCU_STATUS", "MCU_temperature", "MCU_temperature (cdegC)" },
    { 11039, "MCU_STATUS", "MCU_voltage", "MCU_voltage (mV)" },
    { 11039, "MCU_STATUS", "MCU_voltage_min", "MCU_voltage_min (mV)" },
    { 11039, "MCU_STATUS", "MCU_voltage_max", "MCU_voltage_max (mV)" },
    { 11040, "ESC_TELEMETRY_13_TO_16", "temperature[0]", "temperature[0] (degC)" },
    { 11040, "ESC_TELEMETRY_13_TO_16", "temperature[1]", "temperature[1] (degC)" },
    { 11040, "ESC_TELEMETRY_13_TO_16", "temperature[2]", "temperature[2] (degC)" },
    { 11040, "ESC_TELEMETRY_13_TO_16", "temperature[3]", "temperature[3] (degC)" },
    { 11040, "ESC_TELEMETRY_13_TO_16", "voltage[0]", "voltage[0] (cV)" },
    { 11040, "ESC_TELEMETRY_13_TO_16", "voltage[1]", "voltage[1] (cV)" },
    { 11040, "ESC_TELEMETRY_13_TO_16", "voltage[2]", "voltage[2] (cV)" },
    { 11040, "ESC_TELEMETRY_13_TO_16", "voltage[3]", "voltage[3] (cV)" },
    { 11040, "ESC_TELEMETRY_13_TO_16", "current[0]", "current[0] (cA)" },
    { 11040, "ESC_TELEMETRY_13_TO_16", "current[1]", "current[1] (cA)" },
    { 11040, "ESC_TELEMETRY_13_TO_16", "current[2]", "current[2] (cA)" },
    { 11040, "ESC_TELEMETRY_13_TO_16", "current[3]", "current[3] (cA)" },
    { 11040, "ESC_TELEMETRY_13_TO_16", "totalcurrent[0]", "totalcurrent[0] (mAh)" },
    { 11040, "ESC_TELEMETRY_13_TO_16", "totalcurrent[1]", "totalcurrent[1] (mAh)" },
    { 11040, "ESC_TELEMETRY_13_TO_16", "totalcurrent[2]", "totalcurrent[2] (mAh)" },
    { 11040, "ESC_TELEMETRY_13_TO_16", "totalcurrent[3]", "totalcurrent[3] (mAh)" },
    { 11040, "ESC_TELEMETRY_13_TO_16", "rpm[0]", "rpm[0] (rpm)" },
    { 11040, "ESC_TELEMETRY_13_TO_16", "rpm[1]", "rpm[1] (rpm)" },
    { 11040, "ESC_TELEMETRY_13_TO_16", "rpm[2]", "rpm[2] (rpm)" },
    { 11040, "ESC_TELEMETRY_13_TO_16", "rpm[3]", "rpm[3] (rpm)" },
    { 11040, "ESC_TELEMETRY_13_TO_16", "count[0]", "count[0]" },
    { 11040, "ESC_TELEMETRY_13_TO_16", "count[1]", "count[1]" },
    { 11040, "ESC_TELEMETRY_13_TO_16", "count[2]", "count[2]" },
    { 11040, "ESC_TELEMETRY_13_TO_16", "count[3]", "count[3]" },
    { 11041, "ESC_TELEMETRY_17_TO_20", "temperature[0]", "temperature[0] (degC)" },
    { 11041, "ESC_TELEMETRY_17_TO_20", "temperature[1]", "temperature[1] (degC)" },
    { 11041, "ESC_TELEMETRY_17_TO_20", "temperature[2]", "temperature[2] (degC)" },
    { 11041, "ESC_TELEMETRY_17_TO_20", "temperature[3]", "temperature[3] (degC)" },
    { 11041, "ESC_TELEMETRY_17_TO_20", "voltage[0]", "voltage[0] (cV)" },
    { 11041, "ESC_TELEMETRY_17_TO_20", "voltage[1]", "voltage[1] (cV)" },
    { 11041, "ESC_TELEMETRY_17_TO_20", "voltage[2]", "voltage[2] (cV)" },
    { 11041, "ESC_TELEMETRY_17_TO_20", "voltage[3]", "voltage[3] (cV)" },
    { 11041, "ESC_TELEMETRY_17_TO_20", "current[0]", "current[0] (cA)" },
    { 11041, "ESC_TELEMETRY_17_TO_20", "current[1]", "current[1] (cA)" },
    { 11041, "ESC_TELEMETRY_17_TO_20", "current[2]", "current[2] (cA)" },
    { 11041, "ESC_TELEMETRY_17_TO_20", "current[3]", "current[3] (cA)" },
    { 11041, "ESC_TELEMETRY_17_TO_20", "totalcurrent[0]", "totalcurrent[0] (mAh)" },
    { 11041, "ESC_TELEMETRY_17_TO_20", "totalcurrent[1]", "totalcurrent[1] (mAh)" },
    { 11041, "ESC_TELEMETRY_17_TO_20", "totalcurrent[2]", "totalcurrent[2] (mAh)" },
    { 11041, "ESC_TELEMETRY_17_TO_20", "totalcurrent[3]", "totalcurrent[3] (mAh)" },
    { 11041, "ESC_TELEMETRY_17_TO_20", "rpm[0]", "rpm[0] (rpm)" },
    { 11041, "ESC_TELEMETRY_17_TO_20", "rpm[1]", "rpm[1] (rpm)" },
    { 11041, "ESC_TELEMETRY_17_TO_20", "rpm[2]", "rpm[2] (rpm)" },
    { 11041, "ESC_TELEMETRY_17_TO_20", "rpm[3]", "rpm[3] (rpm)" },
    { 11041, "ESC_TELEMETRY_17_TO_20", "count[0]", "count[0]" },
    { 11041, "ESC_TELEMETRY_17_TO_20", "count[1]", "count[1]" },
    { 11041, "ESC_TELEMETRY_17_TO_20", "count[2]", "count[2]" },
    { 11041, "ESC_TELEMETRY_17_TO_20", "count[3]", "count[3]" },
    { 11042, "ESC_TELEMETRY_21_TO_24", "temperature[0]", "temperature[0] (degC)" },
    { 11042, "ESC_TELEMETRY_21_TO_24", "temperature[1]", "temperature[1] (degC)" },
    { 11042, "ESC_TELEMETRY_21_TO_24", "temperature[2]", "temperature[2] (degC)" },
    { 11042, "ESC_TELEMETRY_21_TO_24", "temperature[3]", "temperature[3] (degC)" },
    { 11042, "ESC_TELEMETRY_21_TO_24", "voltage[0]", "voltage[0] (cV)" },
    { 11042, "ESC_TELEMETRY_21_TO_24", "voltage[1]", "voltage[1] (cV)" },
    { 11042, "ESC_TELEMETRY_21_TO_24", "voltage[2]", "voltage[2] (cV)" },
    { 11042, "ESC_TELEMETRY_21_TO_24", "voltage[3]", "voltage[3] (cV)" },
    { 11042, "ESC_TELEMETRY_21_TO_24", "current[0]", "current[0] (cA)" },
    { 11042, "ESC_TELEMETRY_21_TO_24", "current[1]", "current[1] (cA)" },
    { 11042, "ESC_TELEMETRY_21_TO_24", "current[2]", "current[2] (cA)" },
    { 11042, "ESC_TELEMETRY_21_TO_24", "current[3]", "current[3] (cA)" },
    { 11042, "ESC_TELEMETRY_21_TO_24", "totalcurrent[0]", "totalcurrent[0] (mAh)" },
    { 11042, "ESC_TELEMETRY_21_TO_24", "totalcurrent[1]", "totalcurrent[1] (mAh)" },
    { 11042, "ESC_TELEMETRY_21_TO_24", "totalcurrent[2]", "totalcurrent[2] (mAh)" },
    { 11042, "ESC_TELEMETRY_21_TO_24", "totalcurrent[3]", "totalcurrent[3] (mAh)" },
    { 11042, "ESC_TELEMETRY_21_TO_24", "rpm[0]", "rpm[0] (rpm)" },
    { 11042, "ESC_TELEMETRY_21_TO_24", "rpm[1]", "rpm[1] (rpm)" },
    { 11042, "ESC_TELEMETRY_21_TO_24", "rpm[2]", "rpm[2] (rpm)" },
    { 11042, "ESC_TELEMETRY_21_TO_24", "rpm[3]", "rpm[3] (rpm)" },
    { 11042, "ESC_TELEMETRY_21_TO_24", "count[0]", "count[0]" },
    { 11042, "ESC_TELEMETRY_21_TO_24", "count[1]", "count[1]" },
    { 11042, "ESC_TELEMETRY_21_TO_24", "count[2]", "count[2]" },
    { 11042, "ESC_TELEMETRY_21_TO_24", "count[3]", "count[3]" },
    { 11043, "ESC_TELEMETRY_25_TO_28", "temperature[0]", "temperature[0] (degC)" },
    { 11043, "ESC_TELEMETRY_25_TO_28", "temperature[1]", "temperature[1] (degC)" },
    { 11043, "ESC_TELEMETRY_25_TO_28", "temperature[2]", "temperature[2] (degC)" },
    { 11043, "ESC_TELEMETRY_25_TO_28", "temperature[3]", "temperature[3] (degC)" },
    { 11043, "ESC_TELEMETRY_25_TO_28", "voltage[0]", "voltage[0] (cV)" },
    { 11043, "ESC_TELEMETRY_25_TO_28", "voltage[1]", "voltage[1] (cV)" },
    { 11043, "ESC_TELEMETRY_25_TO_28", "voltage[2]", "voltage[2] (cV)" },
    { 11043, "ESC_TELEMETRY_25_TO_28", "voltage[3]", "voltage[3] (cV)" },
    { 11043, "ESC_TELEMETRY_25_TO_28", "current[0]", "current[0] (cA)" },
    { 11043, "ESC_TELEMETRY_25_TO_28", "current[1]", "current[1] (cA)" },
    { 11043, "ESC_TELEMETRY_25_TO_28", "current[2]", "current[2] (cA)" },
    { 11043, "ESC_TELEMETRY_25_TO_28", "current[3]", "current[3] (cA)" },
    { 11043, "ESC_TELEMETRY_25_TO_28", "totalcurrent[0]", "totalcurrent[0] (mAh)" },
    { 11043, "ESC_TELEMETRY_25_TO_28", "totalcurrent[1]", "totalcurrent[1] (mAh)" },
    { 11043, "ESC_TELEMETRY_25_TO_28", "totalcurrent[2]", "totalcurrent[2] (mAh)" },
    { 11043, "ESC_TELEMETRY_25_TO_28", "totalcurrent[3]", "totalcurrent[3] (mAh)" },
    { 11043, "ESC_TELEMETRY_25_TO_28", "rpm[0]", "rpm[0] (rpm)" },
    { 11043, "ESC_TELEMETRY_25_TO_28", "rpm[1]", "rpm[1] (rpm)" },
    { 11043, "ESC_TELEMETRY_25_TO_28", "rpm[2]", "rpm[2] (rpm)" },
    { 11043, "ESC_TELEMETRY_25_TO_28", "rpm[3]", "rpm[3] (rpm)" },
    { 11043, "ESC_TELEMETRY_25_TO_28", "count[0]", "count[0]" },
    { 11043, "ESC_TELEMETRY_25_TO_28", "count[1]", "count[1]" },
    { 11043, "ESC_TELEMETRY_25_TO_28", "count[2]", "count[2]" },
    { 11043, "ESC_TELEMETRY_25_TO_28", "count[3]", "count[3]" },
    { 11044, "ESC_TELEMETRY_29_TO_32", "temperature[0]", "temperature[0] (degC)" },
    { 11044, "ESC_TELEMETRY_29_TO_32", "temperature[1]", "temperature[1] (degC)" },
    { 11044, "ESC_TELEMETRY_29_TO_32", "temperature[2]", "temperature[2] (degC)" },
    { 11044, "ESC_TELEMETRY_29_TO_32", "temperature[3]", "temperature[3] (degC)" },
    { 11044, "ESC_TELEMETRY_29_TO_32", "voltage[0]", "voltage[0] (cV)" },
    { 11044, "ESC_TELEMETRY_29_TO_32", "voltage[1]", "voltage[1] (cV)" },
    { 11044, "ESC_TELEMETRY_29_TO_32", "voltage[2]", "voltage[2] (cV)" },
    { 11044, "ESC_TELEMETRY_29_TO_32", "voltage[3]", "voltage[3] (cV)" },
    { 11044, "ESC_TELEMETRY_29_TO_32", "current[0]", "current[0] (cA)" },
    { 11044, "ESC_TELEMETRY_29_TO_32", "current[1]", "current[1] (cA)" },
    { 11044, "ESC_TELEMETRY_29_TO_32", "current[2]", "current[2] (cA)" },
    { 11044, "ESC_TELEMETRY_29_TO_32", "current[3]", "current[3] (cA)" },
    { 11044, "ESC_TELEMETRY_29_TO_32", "totalcurrent[0]", "totalcurrent[0] (mAh)" },
    { 11044, "ESC_TELEMETRY_29_TO_32", "totalcurrent[1]", "totalcurrent[1] (mAh)" },
    { 11044, "ESC_TELEMETRY_29_TO_32", "totalcurrent[2]", "totalcurrent[2] (mAh)" },
    { 11044, "ESC_TELEMETRY_29_TO_32", "totalcurrent[3]", "totalcurrent[3] (mAh)" },
    { 11044, "ESC_TELEMETRY_29_TO_32", "rpm[0]", "rpm[0] (rpm)" },
    { 11044, "ESC_TELEMETRY_29_TO_32", "rpm[1]", "rpm[1] (rpm)" },
    { 11044, "ESC_TELEMETRY_29_TO_32", "rpm[2]", "rpm[2] (rpm)" },
    { 11044, "ESC_TELEMETRY_29_TO_32", "rpm[3]", "rpm[3] (rpm)" },
    { 11044, "ESC_TELEMETRY_29_TO_32", "count[0]", "count[0]" },
    { 11044, "ESC_TELEMETRY_29_TO_32", "count[1]", "count[1]" },
    { 11044, "ESC_TELEMETRY_29_TO_32", "count[2]", "count[2]" },
    { 11044, "ESC_TELEMETRY_29_TO_32", "count[3]", "count[3]" },
    { 11060, "NAMED_VALUE_STRING", "time_boot_ms", "time_boot_ms (ms)" },
    { 12900, "OPEN_DRONE_ID_BASIC_ID", "target_system", "target_system" },
    { 12900, "OPEN_DRONE_ID_BASIC_ID", "target_component", "target_component" },
    { 12900, "OPEN_DRONE_ID_BASIC_ID", "id_type", "id_type" },
    { 12900, "OPEN_DRONE_ID_BASIC_ID", "ua_type", "ua_type" },
    { 12901, "OPEN_DRONE_ID_LOCATION", "target_system", "target_system" },
    { 12901, "OPEN_DRONE_ID_LOCATION", "target_component", "target_component" },
    { 12901, "OPEN_DRONE_ID_LOCATION", "status", "status" },
    { 12901, "OPEN_DRONE_ID_LOCATION", "direction", "direction (cdeg)" },
    { 12901, "OPEN_DRONE_ID_LOCATION", "speed_horizontal", "speed_horizontal (cm/s)" },
    { 12901, "OPEN_DRONE_ID_LOCATION", "speed_vertical", "speed_vertical (cm/s)" },
    { 12901, "OPEN_DRONE_ID_LOCATION", "latitude", "latitude (degE7)" },
    { 12901, "OPEN_DRONE_ID_LOCATION", "longitude", "longitude (degE7)" },
    { 12901, "OPEN_DRONE_ID_LOCATION", "altitude_barometric", "altitude_barometric (m)" },
    { 12901, "OPEN_DRONE_ID_LOCATION", "altitude_geodetic", "altitude_geodetic (m)" },
    { 12901, "OPEN_DRONE_ID_LOCATION", "height_reference", "height_reference" },
    { 12901, "OPEN_DRONE_ID_LOCATION", "height", "height (m)" },
    { 12901, "OPEN_DRONE_ID_LOCATION", "horizontal_accuracy", "horizontal_accuracy" },
    { 12901, "OPEN_DRONE_ID_LOCATION", "vertical_accuracy", "vertical_accuracy" },
    { 12901, "OPEN_DRONE_ID_LOCATION", "barometer_accuracy", "barometer_accuracy" },
    { 12901, "OPEN_DRONE_ID_LOCATION", "speed_accuracy", "speed_accuracy" },
    { 12901, "OPEN_DRONE_ID_LOCATION", "timestamp", "timestamp (s)" },
    { 12901, "OPEN_DRONE_ID_LOCATION", "timestamp_accuracy", "timestamp_accuracy" },
    { 12902, "OPEN_DRONE_ID_AUTHENTICATION", "target_system", "target_system" },
    { 12902, "OPEN_DRONE_ID_AUTHENTICATION", "target_component", "target_component" },
    { 12902, "OPEN_DRONE_ID_AUTHENTICATION", "authentication_type", "authentication_type" },
    { 12902, "OPEN_DRONE_ID_AUTHENTICATION", "data_page", "data_page" },
    { 12902, "OPEN_DRONE_ID_AUTHENTICATION", "last_page_index", "last_page_index" },
    { 12902, "OPEN_DRONE_ID_AUTHENTICATION", "length", "length (bytes)" },
    { 12902, "OPEN_DRONE_ID_AUTHENTICATION", "timestamp", "timestamp (s)" },
    { 12903, "OPEN_DRONE_ID_SELF_ID", "target_system", "target_system" },
    { 12903, "OPEN_DRONE_ID_SELF_ID", "target_component", "target_component" },
    { 12903, "OPEN_DRONE_ID_SELF_ID", "description_type", "description_type" },
    { 12904, "OPEN_DRONE_ID_SYSTEM", "target_system", "target_system" },
    { 12904, "OPEN_DRONE_ID_SYSTEM", "target_component", "target_component" },
    { 12904, "OPEN_DRONE_ID_SYSTEM", "operator_location_type", "operator_location_type" },
    { 12904, "OPEN_DRONE_ID_SYSTEM", "classification_type", "classification_type" },
    { 12904, "OPEN_DRONE_ID_SYSTEM", "operator_latitude", "operator_latitude (degE7)" },
    { 12904, "OPEN_DRONE_ID_SYSTEM", "operator_longitude", "operator_longitude (degE7)" },
    { 12904, "OPEN_DRONE_ID_SYSTEM", "area_count", "area_count" },
    { 12904, "OPEN_DRONE_ID_SYSTEM", "area_radius", "area_radius (m)" },
    { 12904, "OPEN_DRONE_ID_SYSTEM", "area_ceiling", "area_ceiling (m)" },
    { 12904, "OPEN_DRONE_ID_SYSTEM", "area_floor", "area_floor (m)" },
    { 12904, "OPEN_DRONE_ID_SYSTEM", "category_eu", "category_eu" },
    { 12904, "OPEN_DRONE_ID_SYSTEM", "class_eu", "class_eu" },
    { 12904, "OPEN_DRONE_ID_SYSTEM", "operator_altitude_geo", "operator_altitude_geo (m)" },
    { 12904, "OPEN_DRONE_ID_SYSTEM", "timestamp", "timestamp (s)" },
    { 12905, "OPEN_DRONE_ID_OPERATOR_ID", "target_system", "target_system" },
    { 12905, "OPEN_DRONE_ID_OPERATOR_ID", "target_component", "target_component" },
    { 12905, "OPEN_DRONE_ID_OPERATOR_ID", "operator_id_type", "operator_id_type" },
    { 12915, "OPEN_DRONE_ID_MESSAGE_PACK", "target_system", "target_system" },
    { 12915, "OPEN_DRONE_ID_MESSAGE_PACK", "target_component", "target_component" },
    { 12915, "OPEN_DRONE_ID_MESSAGE_PACK", "single_message_size", "single_message_size (bytes)" },
    { 12915, "OPEN_DRONE_ID_MESSAGE_PACK", "msg_pack_size", "msg_pack_size" },
    { 12918, "OPEN_DRONE_ID_ARM_STATUS", "status", "status" },
    { 12919, "OPEN_DRONE_ID_SYSTEM_UPDATE", "target_system", "target_system" },
    { 12919, "OPEN_DRONE_ID_SYSTEM_UPDATE", "target_component", "target_component" },
    { 12919, "OPEN_DRONE_ID_SYSTEM_UPDATE", "operator_latitude", "operator_latitude (degE7)" },
    { 12919, "OPEN_DRONE_ID_SYSTEM_UPDATE", "operator_longitude", "operator_longitude (degE7)" },
    { 12919, "OPEN_DRONE_ID_SYSTEM_UPDATE", "operator_altitude_geo", "operator_altitude_geo (m)" },
    { 12919, "OPEN_DRONE_ID_SYSTEM_UPDATE", "timestamp", "timestamp (s)" },
    { 12920, "HYGROMETER_SENSOR", "id", "id" },
    { 12920, "HYGROMETER_SENSOR", "temperature", "temperature (cdegC)" },
    { 12920, "HYGROMETER_SENSOR", "humidity", "humidity (c%)" },
    { 42000, "ICAROUS_HEARTBEAT", "status", "status" },
    { 42001, "ICAROUS_KINEMATIC_BANDS", "numBands", "numBands" },
    { 42001, "ICAROUS_KINEMATIC_BANDS", "type1", "type1" },
    { 42001, "ICAROUS_KINEMATIC_BANDS", "min1", "min1 (deg)" },
    { 42001, "ICAROUS_KINEMATIC_BANDS", "max1", "max1 (deg)" },
    { 42001, "ICAROUS_KINEMATIC_BANDS", "type2", "type2" },
    { 42001, "ICAROUS_KINEMATIC_BANDS", "min2", "min2 (deg)" },
    { 42001, "ICAROUS_KINEMATIC_BANDS", "max2", "max2 (deg)" },
    { 42001, "ICAROUS_KINEMATIC_BANDS", "type3", "type3" },
    { 42001, "ICAROUS_KINEMATIC_BANDS", "min3", "min3 (deg)" },
    { 42001, "ICAROUS_KINEMATIC_BANDS", "max3", "max3 (deg)" },
    { 42001, "ICAROUS_KINEMATIC_BANDS", "type4", "type4" },
    { 42001, "ICAROUS_KINEMATIC_BANDS", "min4", "min4 (deg)" },
    { 42001, "ICAROUS_KINEMATIC_BANDS", "max4", "max4 (deg)" },
    { 42001, "ICAROUS_KINEMATIC_BANDS", "type5", "type5" },
    { 42001, "ICAROUS_KINEMATIC_BANDS", "min5", "min5 (deg)" },
    { 42001, "ICAROUS_KINEMATIC_BANDS", "max5", "max5 (deg)" },
    { 50002, "HERELINK_VIDEO_STREAM_INFORMATION", "camera_id", "camera_id" },
    { 50002, "HERELINK_VIDEO_STREAM_INFORMATION", "status", "status" },
    { 50002, "HERELINK_VIDEO_STREAM_INFORMATION", "framerate", "framerate (Hz)" },
    { 50002, "HERELINK_VIDEO_STREAM_INFORMATION", "resolution_h", "resolution_h (pix)" },
    { 50002, "HERELINK_VIDEO_STREAM_INFORMATION", "resolution_v", "resolution_v (pix)" },
    { 50002, "HERELINK_VIDEO_STREAM_INFORMATION", "bitrate", "bitrate (bits/s)" },
    { 50002, "HERELINK_VIDEO_STREAM_INFORMATION", "rotation", "rotation (deg)" },
    { 50003, "HERELINK_TELEM", "rssi", "rssi" },
    { 50003, "HERELINK_TELEM", "snr", "snr" },
    { 50003, "HERELINK_TELEM", "rf_freq", "rf_freq" },
    { 50003, "HERELINK_TELEM", "link_bw", "link_bw" },
    { 50003, "HERELINK_TELEM", "link_rate", "link_rate" },
    { 50003, "HERELINK_TELEM", "cpu_temp", "cpu_temp" },
    { 50003, "HERELINK_TELEM", "board_temp", "board_temp" },
    { 50005, "CUBEPILOT_FIRMWARE_UPDATE_RESP", "target_system", "target_system" },
    { 50005, "CUBEPILOT_FIRMWARE_UPDATE_RESP", "target_component", "target_component" },
    { 50005, "CUBEPILOT_FIRMWARE_UPDATE_RESP", "offset", "offset (bytes)" },
    { 52001, "AIRLINK_AUTH_RESPONSE", "resp_type", "resp_type" },
};

const size_t kMavlinkFieldCatalogCount = 1997;

bool mavlink_field_read(uint32_t msg_id, const char* field_name,
                        const mavlink_message_t& raw, double* out_value)
{
    switch (msg_id) {
    case 0: {
        mavlink_heartbeat_t d;
        mavlink_msg_heartbeat_decode(&raw, &d);
        if (strcmp(field_name, "type") == 0) { *out_value = (double)d.type; return true; }
        if (strcmp(field_name, "autopilot") == 0) { *out_value = (double)d.autopilot; return true; }
        if (strcmp(field_name, "base_mode") == 0) { *out_value = (double)d.base_mode; return true; }
        if (strcmp(field_name, "custom_mode") == 0) { *out_value = (double)d.custom_mode; return true; }
        if (strcmp(field_name, "system_status") == 0) { *out_value = (double)d.system_status; return true; }
        return false;
    }
    case 1: {
        mavlink_sys_status_t d;
        mavlink_msg_sys_status_decode(&raw, &d);
        if (strcmp(field_name, "onboard_control_sensors_present") == 0) { *out_value = (double)d.onboard_control_sensors_present; return true; }
        if (strcmp(field_name, "onboard_control_sensors_enabled") == 0) { *out_value = (double)d.onboard_control_sensors_enabled; return true; }
        if (strcmp(field_name, "onboard_control_sensors_health") == 0) { *out_value = (double)d.onboard_control_sensors_health; return true; }
        if (strcmp(field_name, "load") == 0) { *out_value = (double)d.load; return true; }
        if (strcmp(field_name, "voltage_battery") == 0) { *out_value = (double)d.voltage_battery; return true; }
        if (strcmp(field_name, "current_battery") == 0) { *out_value = (double)d.current_battery; return true; }
        if (strcmp(field_name, "battery_remaining") == 0) { *out_value = (double)d.battery_remaining; return true; }
        if (strcmp(field_name, "drop_rate_comm") == 0) { *out_value = (double)d.drop_rate_comm; return true; }
        if (strcmp(field_name, "errors_comm") == 0) { *out_value = (double)d.errors_comm; return true; }
        if (strcmp(field_name, "errors_count1") == 0) { *out_value = (double)d.errors_count1; return true; }
        if (strcmp(field_name, "errors_count2") == 0) { *out_value = (double)d.errors_count2; return true; }
        if (strcmp(field_name, "errors_count3") == 0) { *out_value = (double)d.errors_count3; return true; }
        if (strcmp(field_name, "errors_count4") == 0) { *out_value = (double)d.errors_count4; return true; }
        if (strcmp(field_name, "onboard_control_sensors_present_extended") == 0) { *out_value = (double)d.onboard_control_sensors_present_extended; return true; }
        if (strcmp(field_name, "onboard_control_sensors_enabled_extended") == 0) { *out_value = (double)d.onboard_control_sensors_enabled_extended; return true; }
        if (strcmp(field_name, "onboard_control_sensors_health_extended") == 0) { *out_value = (double)d.onboard_control_sensors_health_extended; return true; }
        return false;
    }
    case 2: {
        mavlink_system_time_t d;
        mavlink_msg_system_time_decode(&raw, &d);
        if (strcmp(field_name, "time_unix_usec") == 0) { *out_value = (double)d.time_unix_usec; return true; }
        if (strcmp(field_name, "time_boot_ms") == 0) { *out_value = (double)d.time_boot_ms; return true; }
        return false;
    }
    case 4: {
        mavlink_ping_t d;
        mavlink_msg_ping_decode(&raw, &d);
        if (strcmp(field_name, "time_usec") == 0) { *out_value = (double)d.time_usec; return true; }
        if (strcmp(field_name, "seq") == 0) { *out_value = (double)d.seq; return true; }
        if (strcmp(field_name, "target_system") == 0) { *out_value = (double)d.target_system; return true; }
        if (strcmp(field_name, "target_component") == 0) { *out_value = (double)d.target_component; return true; }
        return false;
    }
    case 8: {
        mavlink_link_node_status_t d;
        mavlink_msg_link_node_status_decode(&raw, &d);
        if (strcmp(field_name, "timestamp") == 0) { *out_value = (double)d.timestamp; return true; }
        if (strcmp(field_name, "tx_buf") == 0) { *out_value = (double)d.tx_buf; return true; }
        if (strcmp(field_name, "rx_buf") == 0) { *out_value = (double)d.rx_buf; return true; }
        if (strcmp(field_name, "tx_rate") == 0) { *out_value = (double)d.tx_rate; return true; }
        if (strcmp(field_name, "rx_rate") == 0) { *out_value = (double)d.rx_rate; return true; }
        if (strcmp(field_name, "rx_parse_err") == 0) { *out_value = (double)d.rx_parse_err; return true; }
        if (strcmp(field_name, "tx_overflows") == 0) { *out_value = (double)d.tx_overflows; return true; }
        if (strcmp(field_name, "rx_overflows") == 0) { *out_value = (double)d.rx_overflows; return true; }
        if (strcmp(field_name, "messages_sent") == 0) { *out_value = (double)d.messages_sent; return true; }
        if (strcmp(field_name, "messages_received") == 0) { *out_value = (double)d.messages_received; return true; }
        if (strcmp(field_name, "messages_lost") == 0) { *out_value = (double)d.messages_lost; return true; }
        return false;
    }
    case 22: {
        mavlink_param_value_t d;
        mavlink_msg_param_value_decode(&raw, &d);
        if (strcmp(field_name, "param_value") == 0) { *out_value = (double)d.param_value; return true; }
        if (strcmp(field_name, "param_type") == 0) { *out_value = (double)d.param_type; return true; }
        if (strcmp(field_name, "param_count") == 0) { *out_value = (double)d.param_count; return true; }
        if (strcmp(field_name, "param_index") == 0) { *out_value = (double)d.param_index; return true; }
        return false;
    }
    case 24: {
        mavlink_gps_raw_int_t d;
        mavlink_msg_gps_raw_int_decode(&raw, &d);
        if (strcmp(field_name, "time_usec") == 0) { *out_value = (double)d.time_usec; return true; }
        if (strcmp(field_name, "fix_type") == 0) { *out_value = (double)d.fix_type; return true; }
        if (strcmp(field_name, "lat") == 0) { *out_value = (double)d.lat; return true; }
        if (strcmp(field_name, "lon") == 0) { *out_value = (double)d.lon; return true; }
        if (strcmp(field_name, "alt") == 0) { *out_value = (double)d.alt; return true; }
        if (strcmp(field_name, "eph") == 0) { *out_value = (double)d.eph; return true; }
        if (strcmp(field_name, "epv") == 0) { *out_value = (double)d.epv; return true; }
        if (strcmp(field_name, "vel") == 0) { *out_value = (double)d.vel; return true; }
        if (strcmp(field_name, "cog") == 0) { *out_value = (double)d.cog; return true; }
        if (strcmp(field_name, "satellites_visible") == 0) { *out_value = (double)d.satellites_visible; return true; }
        if (strcmp(field_name, "alt_ellipsoid") == 0) { *out_value = (double)d.alt_ellipsoid; return true; }
        if (strcmp(field_name, "h_acc") == 0) { *out_value = (double)d.h_acc; return true; }
        if (strcmp(field_name, "v_acc") == 0) { *out_value = (double)d.v_acc; return true; }
        if (strcmp(field_name, "vel_acc") == 0) { *out_value = (double)d.vel_acc; return true; }
        if (strcmp(field_name, "hdg_acc") == 0) { *out_value = (double)d.hdg_acc; return true; }
        if (strcmp(field_name, "yaw") == 0) { *out_value = (double)d.yaw; return true; }
        return false;
    }
    case 25: {
        mavlink_gps_status_t d;
        mavlink_msg_gps_status_decode(&raw, &d);
        if (strcmp(field_name, "satellites_visible") == 0) { *out_value = (double)d.satellites_visible; return true; }
        return false;
    }
    case 26: {
        mavlink_scaled_imu_t d;
        mavlink_msg_scaled_imu_decode(&raw, &d);
        if (strcmp(field_name, "time_boot_ms") == 0) { *out_value = (double)d.time_boot_ms; return true; }
        if (strcmp(field_name, "xacc") == 0) { *out_value = (double)d.xacc; return true; }
        if (strcmp(field_name, "yacc") == 0) { *out_value = (double)d.yacc; return true; }
        if (strcmp(field_name, "zacc") == 0) { *out_value = (double)d.zacc; return true; }
        if (strcmp(field_name, "xgyro") == 0) { *out_value = (double)d.xgyro; return true; }
        if (strcmp(field_name, "ygyro") == 0) { *out_value = (double)d.ygyro; return true; }
        if (strcmp(field_name, "zgyro") == 0) { *out_value = (double)d.zgyro; return true; }
        if (strcmp(field_name, "xmag") == 0) { *out_value = (double)d.xmag; return true; }
        if (strcmp(field_name, "ymag") == 0) { *out_value = (double)d.ymag; return true; }
        if (strcmp(field_name, "zmag") == 0) { *out_value = (double)d.zmag; return true; }
        if (strcmp(field_name, "temperature") == 0) { *out_value = (double)d.temperature; return true; }
        return false;
    }
    case 27: {
        mavlink_raw_imu_t d;
        mavlink_msg_raw_imu_decode(&raw, &d);
        if (strcmp(field_name, "time_usec") == 0) { *out_value = (double)d.time_usec; return true; }
        if (strcmp(field_name, "xacc") == 0) { *out_value = (double)d.xacc; return true; }
        if (strcmp(field_name, "yacc") == 0) { *out_value = (double)d.yacc; return true; }
        if (strcmp(field_name, "zacc") == 0) { *out_value = (double)d.zacc; return true; }
        if (strcmp(field_name, "xgyro") == 0) { *out_value = (double)d.xgyro; return true; }
        if (strcmp(field_name, "ygyro") == 0) { *out_value = (double)d.ygyro; return true; }
        if (strcmp(field_name, "zgyro") == 0) { *out_value = (double)d.zgyro; return true; }
        if (strcmp(field_name, "xmag") == 0) { *out_value = (double)d.xmag; return true; }
        if (strcmp(field_name, "ymag") == 0) { *out_value = (double)d.ymag; return true; }
        if (strcmp(field_name, "zmag") == 0) { *out_value = (double)d.zmag; return true; }
        if (strcmp(field_name, "id") == 0) { *out_value = (double)d.id; return true; }
        if (strcmp(field_name, "temperature") == 0) { *out_value = (double)d.temperature; return true; }
        return false;
    }
    case 28: {
        mavlink_raw_pressure_t d;
        mavlink_msg_raw_pressure_decode(&raw, &d);
        if (strcmp(field_name, "time_usec") == 0) { *out_value = (double)d.time_usec; return true; }
        if (strcmp(field_name, "press_abs") == 0) { *out_value = (double)d.press_abs; return true; }
        if (strcmp(field_name, "press_diff1") == 0) { *out_value = (double)d.press_diff1; return true; }
        if (strcmp(field_name, "press_diff2") == 0) { *out_value = (double)d.press_diff2; return true; }
        if (strcmp(field_name, "temperature") == 0) { *out_value = (double)d.temperature; return true; }
        return false;
    }
    case 29: {
        mavlink_scaled_pressure_t d;
        mavlink_msg_scaled_pressure_decode(&raw, &d);
        if (strcmp(field_name, "time_boot_ms") == 0) { *out_value = (double)d.time_boot_ms; return true; }
        if (strcmp(field_name, "press_abs") == 0) { *out_value = (double)d.press_abs; return true; }
        if (strcmp(field_name, "press_diff") == 0) { *out_value = (double)d.press_diff; return true; }
        if (strcmp(field_name, "temperature") == 0) { *out_value = (double)d.temperature; return true; }
        if (strcmp(field_name, "temperature_press_diff") == 0) { *out_value = (double)d.temperature_press_diff; return true; }
        return false;
    }
    case 30: {
        mavlink_attitude_t d;
        mavlink_msg_attitude_decode(&raw, &d);
        if (strcmp(field_name, "time_boot_ms") == 0) { *out_value = (double)d.time_boot_ms; return true; }
        if (strcmp(field_name, "roll") == 0) { *out_value = (double)d.roll; return true; }
        if (strcmp(field_name, "pitch") == 0) { *out_value = (double)d.pitch; return true; }
        if (strcmp(field_name, "yaw") == 0) { *out_value = (double)d.yaw; return true; }
        if (strcmp(field_name, "rollspeed") == 0) { *out_value = (double)d.rollspeed; return true; }
        if (strcmp(field_name, "pitchspeed") == 0) { *out_value = (double)d.pitchspeed; return true; }
        if (strcmp(field_name, "yawspeed") == 0) { *out_value = (double)d.yawspeed; return true; }
        return false;
    }
    case 31: {
        mavlink_attitude_quaternion_t d;
        mavlink_msg_attitude_quaternion_decode(&raw, &d);
        if (strcmp(field_name, "time_boot_ms") == 0) { *out_value = (double)d.time_boot_ms; return true; }
        if (strcmp(field_name, "q1") == 0) { *out_value = (double)d.q1; return true; }
        if (strcmp(field_name, "q2") == 0) { *out_value = (double)d.q2; return true; }
        if (strcmp(field_name, "q3") == 0) { *out_value = (double)d.q3; return true; }
        if (strcmp(field_name, "q4") == 0) { *out_value = (double)d.q4; return true; }
        if (strcmp(field_name, "rollspeed") == 0) { *out_value = (double)d.rollspeed; return true; }
        if (strcmp(field_name, "pitchspeed") == 0) { *out_value = (double)d.pitchspeed; return true; }
        if (strcmp(field_name, "yawspeed") == 0) { *out_value = (double)d.yawspeed; return true; }
        if (strcmp(field_name, "repr_offset_q[0]") == 0) { *out_value = (double)d.repr_offset_q[0]; return true; }
        if (strcmp(field_name, "repr_offset_q[1]") == 0) { *out_value = (double)d.repr_offset_q[1]; return true; }
        if (strcmp(field_name, "repr_offset_q[2]") == 0) { *out_value = (double)d.repr_offset_q[2]; return true; }
        if (strcmp(field_name, "repr_offset_q[3]") == 0) { *out_value = (double)d.repr_offset_q[3]; return true; }
        return false;
    }
    case 32: {
        mavlink_local_position_ned_t d;
        mavlink_msg_local_position_ned_decode(&raw, &d);
        if (strcmp(field_name, "time_boot_ms") == 0) { *out_value = (double)d.time_boot_ms; return true; }
        if (strcmp(field_name, "x") == 0) { *out_value = (double)d.x; return true; }
        if (strcmp(field_name, "y") == 0) { *out_value = (double)d.y; return true; }
        if (strcmp(field_name, "z") == 0) { *out_value = (double)d.z; return true; }
        if (strcmp(field_name, "vx") == 0) { *out_value = (double)d.vx; return true; }
        if (strcmp(field_name, "vy") == 0) { *out_value = (double)d.vy; return true; }
        if (strcmp(field_name, "vz") == 0) { *out_value = (double)d.vz; return true; }
        return false;
    }
    case 33: {
        mavlink_global_position_int_t d;
        mavlink_msg_global_position_int_decode(&raw, &d);
        if (strcmp(field_name, "time_boot_ms") == 0) { *out_value = (double)d.time_boot_ms; return true; }
        if (strcmp(field_name, "lat") == 0) { *out_value = (double)d.lat; return true; }
        if (strcmp(field_name, "lon") == 0) { *out_value = (double)d.lon; return true; }
        if (strcmp(field_name, "alt") == 0) { *out_value = (double)d.alt; return true; }
        if (strcmp(field_name, "relative_alt") == 0) { *out_value = (double)d.relative_alt; return true; }
        if (strcmp(field_name, "vx") == 0) { *out_value = (double)d.vx; return true; }
        if (strcmp(field_name, "vy") == 0) { *out_value = (double)d.vy; return true; }
        if (strcmp(field_name, "vz") == 0) { *out_value = (double)d.vz; return true; }
        if (strcmp(field_name, "hdg") == 0) { *out_value = (double)d.hdg; return true; }
        return false;
    }
    case 34: {
        mavlink_rc_channels_scaled_t d;
        mavlink_msg_rc_channels_scaled_decode(&raw, &d);
        if (strcmp(field_name, "time_boot_ms") == 0) { *out_value = (double)d.time_boot_ms; return true; }
        if (strcmp(field_name, "port") == 0) { *out_value = (double)d.port; return true; }
        if (strcmp(field_name, "chan1_scaled") == 0) { *out_value = (double)d.chan1_scaled; return true; }
        if (strcmp(field_name, "chan2_scaled") == 0) { *out_value = (double)d.chan2_scaled; return true; }
        if (strcmp(field_name, "chan3_scaled") == 0) { *out_value = (double)d.chan3_scaled; return true; }
        if (strcmp(field_name, "chan4_scaled") == 0) { *out_value = (double)d.chan4_scaled; return true; }
        if (strcmp(field_name, "chan5_scaled") == 0) { *out_value = (double)d.chan5_scaled; return true; }
        if (strcmp(field_name, "chan6_scaled") == 0) { *out_value = (double)d.chan6_scaled; return true; }
        if (strcmp(field_name, "chan7_scaled") == 0) { *out_value = (double)d.chan7_scaled; return true; }
        if (strcmp(field_name, "chan8_scaled") == 0) { *out_value = (double)d.chan8_scaled; return true; }
        if (strcmp(field_name, "rssi") == 0) { *out_value = (double)d.rssi; return true; }
        return false;
    }
    case 35: {
        mavlink_rc_channels_raw_t d;
        mavlink_msg_rc_channels_raw_decode(&raw, &d);
        if (strcmp(field_name, "time_boot_ms") == 0) { *out_value = (double)d.time_boot_ms; return true; }
        if (strcmp(field_name, "port") == 0) { *out_value = (double)d.port; return true; }
        if (strcmp(field_name, "chan1_raw") == 0) { *out_value = (double)d.chan1_raw; return true; }
        if (strcmp(field_name, "chan2_raw") == 0) { *out_value = (double)d.chan2_raw; return true; }
        if (strcmp(field_name, "chan3_raw") == 0) { *out_value = (double)d.chan3_raw; return true; }
        if (strcmp(field_name, "chan4_raw") == 0) { *out_value = (double)d.chan4_raw; return true; }
        if (strcmp(field_name, "chan5_raw") == 0) { *out_value = (double)d.chan5_raw; return true; }
        if (strcmp(field_name, "chan6_raw") == 0) { *out_value = (double)d.chan6_raw; return true; }
        if (strcmp(field_name, "chan7_raw") == 0) { *out_value = (double)d.chan7_raw; return true; }
        if (strcmp(field_name, "chan8_raw") == 0) { *out_value = (double)d.chan8_raw; return true; }
        if (strcmp(field_name, "rssi") == 0) { *out_value = (double)d.rssi; return true; }
        return false;
    }
    case 36: {
        mavlink_servo_output_raw_t d;
        mavlink_msg_servo_output_raw_decode(&raw, &d);
        if (strcmp(field_name, "time_usec") == 0) { *out_value = (double)d.time_usec; return true; }
        if (strcmp(field_name, "port") == 0) { *out_value = (double)d.port; return true; }
        if (strcmp(field_name, "servo1_raw") == 0) { *out_value = (double)d.servo1_raw; return true; }
        if (strcmp(field_name, "servo2_raw") == 0) { *out_value = (double)d.servo2_raw; return true; }
        if (strcmp(field_name, "servo3_raw") == 0) { *out_value = (double)d.servo3_raw; return true; }
        if (strcmp(field_name, "servo4_raw") == 0) { *out_value = (double)d.servo4_raw; return true; }
        if (strcmp(field_name, "servo5_raw") == 0) { *out_value = (double)d.servo5_raw; return true; }
        if (strcmp(field_name, "servo6_raw") == 0) { *out_value = (double)d.servo6_raw; return true; }
        if (strcmp(field_name, "servo7_raw") == 0) { *out_value = (double)d.servo7_raw; return true; }
        if (strcmp(field_name, "servo8_raw") == 0) { *out_value = (double)d.servo8_raw; return true; }
        if (strcmp(field_name, "servo9_raw") == 0) { *out_value = (double)d.servo9_raw; return true; }
        if (strcmp(field_name, "servo10_raw") == 0) { *out_value = (double)d.servo10_raw; return true; }
        if (strcmp(field_name, "servo11_raw") == 0) { *out_value = (double)d.servo11_raw; return true; }
        if (strcmp(field_name, "servo12_raw") == 0) { *out_value = (double)d.servo12_raw; return true; }
        if (strcmp(field_name, "servo13_raw") == 0) { *out_value = (double)d.servo13_raw; return true; }
        if (strcmp(field_name, "servo14_raw") == 0) { *out_value = (double)d.servo14_raw; return true; }
        if (strcmp(field_name, "servo15_raw") == 0) { *out_value = (double)d.servo15_raw; return true; }
        if (strcmp(field_name, "servo16_raw") == 0) { *out_value = (double)d.servo16_raw; return true; }
        return false;
    }
    case 42: {
        mavlink_mission_current_t d;
        mavlink_msg_mission_current_decode(&raw, &d);
        if (strcmp(field_name, "seq") == 0) { *out_value = (double)d.seq; return true; }
        if (strcmp(field_name, "total") == 0) { *out_value = (double)d.total; return true; }
        if (strcmp(field_name, "mission_state") == 0) { *out_value = (double)d.mission_state; return true; }
        if (strcmp(field_name, "mission_mode") == 0) { *out_value = (double)d.mission_mode; return true; }
        if (strcmp(field_name, "mission_id") == 0) { *out_value = (double)d.mission_id; return true; }
        if (strcmp(field_name, "fence_id") == 0) { *out_value = (double)d.fence_id; return true; }
        if (strcmp(field_name, "rally_points_id") == 0) { *out_value = (double)d.rally_points_id; return true; }
        return false;
    }
    case 44: {
        mavlink_mission_count_t d;
        mavlink_msg_mission_count_decode(&raw, &d);
        if (strcmp(field_name, "target_system") == 0) { *out_value = (double)d.target_system; return true; }
        if (strcmp(field_name, "target_component") == 0) { *out_value = (double)d.target_component; return true; }
        if (strcmp(field_name, "count") == 0) { *out_value = (double)d.count; return true; }
        if (strcmp(field_name, "mission_type") == 0) { *out_value = (double)d.mission_type; return true; }
        if (strcmp(field_name, "opaque_id") == 0) { *out_value = (double)d.opaque_id; return true; }
        return false;
    }
    case 46: {
        mavlink_mission_item_reached_t d;
        mavlink_msg_mission_item_reached_decode(&raw, &d);
        if (strcmp(field_name, "seq") == 0) { *out_value = (double)d.seq; return true; }
        return false;
    }
    case 47: {
        mavlink_mission_ack_t d;
        mavlink_msg_mission_ack_decode(&raw, &d);
        if (strcmp(field_name, "target_system") == 0) { *out_value = (double)d.target_system; return true; }
        if (strcmp(field_name, "target_component") == 0) { *out_value = (double)d.target_component; return true; }
        if (strcmp(field_name, "type") == 0) { *out_value = (double)d.type; return true; }
        if (strcmp(field_name, "mission_type") == 0) { *out_value = (double)d.mission_type; return true; }
        if (strcmp(field_name, "opaque_id") == 0) { *out_value = (double)d.opaque_id; return true; }
        return false;
    }
    case 49: {
        mavlink_gps_global_origin_t d;
        mavlink_msg_gps_global_origin_decode(&raw, &d);
        if (strcmp(field_name, "latitude") == 0) { *out_value = (double)d.latitude; return true; }
        if (strcmp(field_name, "longitude") == 0) { *out_value = (double)d.longitude; return true; }
        if (strcmp(field_name, "altitude") == 0) { *out_value = (double)d.altitude; return true; }
        if (strcmp(field_name, "time_usec") == 0) { *out_value = (double)d.time_usec; return true; }
        return false;
    }
    case 55: {
        mavlink_safety_allowed_area_t d;
        mavlink_msg_safety_allowed_area_decode(&raw, &d);
        if (strcmp(field_name, "frame") == 0) { *out_value = (double)d.frame; return true; }
        if (strcmp(field_name, "p1x") == 0) { *out_value = (double)d.p1x; return true; }
        if (strcmp(field_name, "p1y") == 0) { *out_value = (double)d.p1y; return true; }
        if (strcmp(field_name, "p1z") == 0) { *out_value = (double)d.p1z; return true; }
        if (strcmp(field_name, "p2x") == 0) { *out_value = (double)d.p2x; return true; }
        if (strcmp(field_name, "p2y") == 0) { *out_value = (double)d.p2y; return true; }
        if (strcmp(field_name, "p2z") == 0) { *out_value = (double)d.p2z; return true; }
        return false;
    }
    case 61: {
        mavlink_attitude_quaternion_cov_t d;
        mavlink_msg_attitude_quaternion_cov_decode(&raw, &d);
        if (strcmp(field_name, "time_usec") == 0) { *out_value = (double)d.time_usec; return true; }
        if (strcmp(field_name, "q[0]") == 0) { *out_value = (double)d.q[0]; return true; }
        if (strcmp(field_name, "q[1]") == 0) { *out_value = (double)d.q[1]; return true; }
        if (strcmp(field_name, "q[2]") == 0) { *out_value = (double)d.q[2]; return true; }
        if (strcmp(field_name, "q[3]") == 0) { *out_value = (double)d.q[3]; return true; }
        if (strcmp(field_name, "rollspeed") == 0) { *out_value = (double)d.rollspeed; return true; }
        if (strcmp(field_name, "pitchspeed") == 0) { *out_value = (double)d.pitchspeed; return true; }
        if (strcmp(field_name, "yawspeed") == 0) { *out_value = (double)d.yawspeed; return true; }
        return false;
    }
    case 62: {
        mavlink_nav_controller_output_t d;
        mavlink_msg_nav_controller_output_decode(&raw, &d);
        if (strcmp(field_name, "nav_roll") == 0) { *out_value = (double)d.nav_roll; return true; }
        if (strcmp(field_name, "nav_pitch") == 0) { *out_value = (double)d.nav_pitch; return true; }
        if (strcmp(field_name, "nav_bearing") == 0) { *out_value = (double)d.nav_bearing; return true; }
        if (strcmp(field_name, "target_bearing") == 0) { *out_value = (double)d.target_bearing; return true; }
        if (strcmp(field_name, "wp_dist") == 0) { *out_value = (double)d.wp_dist; return true; }
        if (strcmp(field_name, "alt_error") == 0) { *out_value = (double)d.alt_error; return true; }
        if (strcmp(field_name, "aspd_error") == 0) { *out_value = (double)d.aspd_error; return true; }
        if (strcmp(field_name, "xtrack_error") == 0) { *out_value = (double)d.xtrack_error; return true; }
        return false;
    }
    case 63: {
        mavlink_global_position_int_cov_t d;
        mavlink_msg_global_position_int_cov_decode(&raw, &d);
        if (strcmp(field_name, "time_usec") == 0) { *out_value = (double)d.time_usec; return true; }
        if (strcmp(field_name, "estimator_type") == 0) { *out_value = (double)d.estimator_type; return true; }
        if (strcmp(field_name, "lat") == 0) { *out_value = (double)d.lat; return true; }
        if (strcmp(field_name, "lon") == 0) { *out_value = (double)d.lon; return true; }
        if (strcmp(field_name, "alt") == 0) { *out_value = (double)d.alt; return true; }
        if (strcmp(field_name, "relative_alt") == 0) { *out_value = (double)d.relative_alt; return true; }
        if (strcmp(field_name, "vx") == 0) { *out_value = (double)d.vx; return true; }
        if (strcmp(field_name, "vy") == 0) { *out_value = (double)d.vy; return true; }
        if (strcmp(field_name, "vz") == 0) { *out_value = (double)d.vz; return true; }
        return false;
    }
    case 64: {
        mavlink_local_position_ned_cov_t d;
        mavlink_msg_local_position_ned_cov_decode(&raw, &d);
        if (strcmp(field_name, "time_usec") == 0) { *out_value = (double)d.time_usec; return true; }
        if (strcmp(field_name, "estimator_type") == 0) { *out_value = (double)d.estimator_type; return true; }
        if (strcmp(field_name, "x") == 0) { *out_value = (double)d.x; return true; }
        if (strcmp(field_name, "y") == 0) { *out_value = (double)d.y; return true; }
        if (strcmp(field_name, "z") == 0) { *out_value = (double)d.z; return true; }
        if (strcmp(field_name, "vx") == 0) { *out_value = (double)d.vx; return true; }
        if (strcmp(field_name, "vy") == 0) { *out_value = (double)d.vy; return true; }
        if (strcmp(field_name, "vz") == 0) { *out_value = (double)d.vz; return true; }
        if (strcmp(field_name, "ax") == 0) { *out_value = (double)d.ax; return true; }
        if (strcmp(field_name, "ay") == 0) { *out_value = (double)d.ay; return true; }
        if (strcmp(field_name, "az") == 0) { *out_value = (double)d.az; return true; }
        return false;
    }
    case 65: {
        mavlink_rc_channels_t d;
        mavlink_msg_rc_channels_decode(&raw, &d);
        if (strcmp(field_name, "time_boot_ms") == 0) { *out_value = (double)d.time_boot_ms; return true; }
        if (strcmp(field_name, "chancount") == 0) { *out_value = (double)d.chancount; return true; }
        if (strcmp(field_name, "chan1_raw") == 0) { *out_value = (double)d.chan1_raw; return true; }
        if (strcmp(field_name, "chan2_raw") == 0) { *out_value = (double)d.chan2_raw; return true; }
        if (strcmp(field_name, "chan3_raw") == 0) { *out_value = (double)d.chan3_raw; return true; }
        if (strcmp(field_name, "chan4_raw") == 0) { *out_value = (double)d.chan4_raw; return true; }
        if (strcmp(field_name, "chan5_raw") == 0) { *out_value = (double)d.chan5_raw; return true; }
        if (strcmp(field_name, "chan6_raw") == 0) { *out_value = (double)d.chan6_raw; return true; }
        if (strcmp(field_name, "chan7_raw") == 0) { *out_value = (double)d.chan7_raw; return true; }
        if (strcmp(field_name, "chan8_raw") == 0) { *out_value = (double)d.chan8_raw; return true; }
        if (strcmp(field_name, "chan9_raw") == 0) { *out_value = (double)d.chan9_raw; return true; }
        if (strcmp(field_name, "chan10_raw") == 0) { *out_value = (double)d.chan10_raw; return true; }
        if (strcmp(field_name, "chan11_raw") == 0) { *out_value = (double)d.chan11_raw; return true; }
        if (strcmp(field_name, "chan12_raw") == 0) { *out_value = (double)d.chan12_raw; return true; }
        if (strcmp(field_name, "chan13_raw") == 0) { *out_value = (double)d.chan13_raw; return true; }
        if (strcmp(field_name, "chan14_raw") == 0) { *out_value = (double)d.chan14_raw; return true; }
        if (strcmp(field_name, "chan15_raw") == 0) { *out_value = (double)d.chan15_raw; return true; }
        if (strcmp(field_name, "chan16_raw") == 0) { *out_value = (double)d.chan16_raw; return true; }
        if (strcmp(field_name, "chan17_raw") == 0) { *out_value = (double)d.chan17_raw; return true; }
        if (strcmp(field_name, "chan18_raw") == 0) { *out_value = (double)d.chan18_raw; return true; }
        if (strcmp(field_name, "rssi") == 0) { *out_value = (double)d.rssi; return true; }
        return false;
    }
    case 67: {
        mavlink_data_stream_t d;
        mavlink_msg_data_stream_decode(&raw, &d);
        if (strcmp(field_name, "stream_id") == 0) { *out_value = (double)d.stream_id; return true; }
        if (strcmp(field_name, "message_rate") == 0) { *out_value = (double)d.message_rate; return true; }
        if (strcmp(field_name, "on_off") == 0) { *out_value = (double)d.on_off; return true; }
        return false;
    }
    case 74: {
        mavlink_vfr_hud_t d;
        mavlink_msg_vfr_hud_decode(&raw, &d);
        if (strcmp(field_name, "airspeed") == 0) { *out_value = (double)d.airspeed; return true; }
        if (strcmp(field_name, "groundspeed") == 0) { *out_value = (double)d.groundspeed; return true; }
        if (strcmp(field_name, "heading") == 0) { *out_value = (double)d.heading; return true; }
        if (strcmp(field_name, "throttle") == 0) { *out_value = (double)d.throttle; return true; }
        if (strcmp(field_name, "alt") == 0) { *out_value = (double)d.alt; return true; }
        if (strcmp(field_name, "climb") == 0) { *out_value = (double)d.climb; return true; }
        return false;
    }
    case 77: {
        mavlink_command_ack_t d;
        mavlink_msg_command_ack_decode(&raw, &d);
        if (strcmp(field_name, "command") == 0) { *out_value = (double)d.command; return true; }
        if (strcmp(field_name, "result") == 0) { *out_value = (double)d.result; return true; }
        if (strcmp(field_name, "progress") == 0) { *out_value = (double)d.progress; return true; }
        if (strcmp(field_name, "result_param2") == 0) { *out_value = (double)d.result_param2; return true; }
        if (strcmp(field_name, "target_system") == 0) { *out_value = (double)d.target_system; return true; }
        if (strcmp(field_name, "target_component") == 0) { *out_value = (double)d.target_component; return true; }
        return false;
    }
    case 83: {
        mavlink_attitude_target_t d;
        mavlink_msg_attitude_target_decode(&raw, &d);
        if (strcmp(field_name, "time_boot_ms") == 0) { *out_value = (double)d.time_boot_ms; return true; }
        if (strcmp(field_name, "type_mask") == 0) { *out_value = (double)d.type_mask; return true; }
        if (strcmp(field_name, "q[0]") == 0) { *out_value = (double)d.q[0]; return true; }
        if (strcmp(field_name, "q[1]") == 0) { *out_value = (double)d.q[1]; return true; }
        if (strcmp(field_name, "q[2]") == 0) { *out_value = (double)d.q[2]; return true; }
        if (strcmp(field_name, "q[3]") == 0) { *out_value = (double)d.q[3]; return true; }
        if (strcmp(field_name, "body_roll_rate") == 0) { *out_value = (double)d.body_roll_rate; return true; }
        if (strcmp(field_name, "body_pitch_rate") == 0) { *out_value = (double)d.body_pitch_rate; return true; }
        if (strcmp(field_name, "body_yaw_rate") == 0) { *out_value = (double)d.body_yaw_rate; return true; }
        if (strcmp(field_name, "thrust") == 0) { *out_value = (double)d.thrust; return true; }
        return false;
    }
    case 85: {
        mavlink_position_target_local_ned_t d;
        mavlink_msg_position_target_local_ned_decode(&raw, &d);
        if (strcmp(field_name, "time_boot_ms") == 0) { *out_value = (double)d.time_boot_ms; return true; }
        if (strcmp(field_name, "coordinate_frame") == 0) { *out_value = (double)d.coordinate_frame; return true; }
        if (strcmp(field_name, "type_mask") == 0) { *out_value = (double)d.type_mask; return true; }
        if (strcmp(field_name, "x") == 0) { *out_value = (double)d.x; return true; }
        if (strcmp(field_name, "y") == 0) { *out_value = (double)d.y; return true; }
        if (strcmp(field_name, "z") == 0) { *out_value = (double)d.z; return true; }
        if (strcmp(field_name, "vx") == 0) { *out_value = (double)d.vx; return true; }
        if (strcmp(field_name, "vy") == 0) { *out_value = (double)d.vy; return true; }
        if (strcmp(field_name, "vz") == 0) { *out_value = (double)d.vz; return true; }
        if (strcmp(field_name, "afx") == 0) { *out_value = (double)d.afx; return true; }
        if (strcmp(field_name, "afy") == 0) { *out_value = (double)d.afy; return true; }
        if (strcmp(field_name, "afz") == 0) { *out_value = (double)d.afz; return true; }
        if (strcmp(field_name, "yaw") == 0) { *out_value = (double)d.yaw; return true; }
        if (strcmp(field_name, "yaw_rate") == 0) { *out_value = (double)d.yaw_rate; return true; }
        return false;
    }
    case 87: {
        mavlink_position_target_global_int_t d;
        mavlink_msg_position_target_global_int_decode(&raw, &d);
        if (strcmp(field_name, "time_boot_ms") == 0) { *out_value = (double)d.time_boot_ms; return true; }
        if (strcmp(field_name, "coordinate_frame") == 0) { *out_value = (double)d.coordinate_frame; return true; }
        if (strcmp(field_name, "type_mask") == 0) { *out_value = (double)d.type_mask; return true; }
        if (strcmp(field_name, "lat_int") == 0) { *out_value = (double)d.lat_int; return true; }
        if (strcmp(field_name, "lon_int") == 0) { *out_value = (double)d.lon_int; return true; }
        if (strcmp(field_name, "alt") == 0) { *out_value = (double)d.alt; return true; }
        if (strcmp(field_name, "vx") == 0) { *out_value = (double)d.vx; return true; }
        if (strcmp(field_name, "vy") == 0) { *out_value = (double)d.vy; return true; }
        if (strcmp(field_name, "vz") == 0) { *out_value = (double)d.vz; return true; }
        if (strcmp(field_name, "afx") == 0) { *out_value = (double)d.afx; return true; }
        if (strcmp(field_name, "afy") == 0) { *out_value = (double)d.afy; return true; }
        if (strcmp(field_name, "afz") == 0) { *out_value = (double)d.afz; return true; }
        if (strcmp(field_name, "yaw") == 0) { *out_value = (double)d.yaw; return true; }
        if (strcmp(field_name, "yaw_rate") == 0) { *out_value = (double)d.yaw_rate; return true; }
        return false;
    }
    case 89: {
        mavlink_local_position_ned_system_global_offset_t d;
        mavlink_msg_local_position_ned_system_global_offset_decode(&raw, &d);
        if (strcmp(field_name, "time_boot_ms") == 0) { *out_value = (double)d.time_boot_ms; return true; }
        if (strcmp(field_name, "x") == 0) { *out_value = (double)d.x; return true; }
        if (strcmp(field_name, "y") == 0) { *out_value = (double)d.y; return true; }
        if (strcmp(field_name, "z") == 0) { *out_value = (double)d.z; return true; }
        if (strcmp(field_name, "roll") == 0) { *out_value = (double)d.roll; return true; }
        if (strcmp(field_name, "pitch") == 0) { *out_value = (double)d.pitch; return true; }
        if (strcmp(field_name, "yaw") == 0) { *out_value = (double)d.yaw; return true; }
        return false;
    }
    case 100: {
        mavlink_optical_flow_t d;
        mavlink_msg_optical_flow_decode(&raw, &d);
        if (strcmp(field_name, "time_usec") == 0) { *out_value = (double)d.time_usec; return true; }
        if (strcmp(field_name, "sensor_id") == 0) { *out_value = (double)d.sensor_id; return true; }
        if (strcmp(field_name, "flow_x") == 0) { *out_value = (double)d.flow_x; return true; }
        if (strcmp(field_name, "flow_y") == 0) { *out_value = (double)d.flow_y; return true; }
        if (strcmp(field_name, "flow_comp_m_x") == 0) { *out_value = (double)d.flow_comp_m_x; return true; }
        if (strcmp(field_name, "flow_comp_m_y") == 0) { *out_value = (double)d.flow_comp_m_y; return true; }
        if (strcmp(field_name, "quality") == 0) { *out_value = (double)d.quality; return true; }
        if (strcmp(field_name, "ground_distance") == 0) { *out_value = (double)d.ground_distance; return true; }
        if (strcmp(field_name, "flow_rate_x") == 0) { *out_value = (double)d.flow_rate_x; return true; }
        if (strcmp(field_name, "flow_rate_y") == 0) { *out_value = (double)d.flow_rate_y; return true; }
        return false;
    }
    case 101: {
        mavlink_global_vision_position_estimate_t d;
        mavlink_msg_global_vision_position_estimate_decode(&raw, &d);
        if (strcmp(field_name, "usec") == 0) { *out_value = (double)d.usec; return true; }
        if (strcmp(field_name, "x") == 0) { *out_value = (double)d.x; return true; }
        if (strcmp(field_name, "y") == 0) { *out_value = (double)d.y; return true; }
        if (strcmp(field_name, "z") == 0) { *out_value = (double)d.z; return true; }
        if (strcmp(field_name, "roll") == 0) { *out_value = (double)d.roll; return true; }
        if (strcmp(field_name, "pitch") == 0) { *out_value = (double)d.pitch; return true; }
        if (strcmp(field_name, "yaw") == 0) { *out_value = (double)d.yaw; return true; }
        if (strcmp(field_name, "reset_counter") == 0) { *out_value = (double)d.reset_counter; return true; }
        return false;
    }
    case 102: {
        mavlink_vision_position_estimate_t d;
        mavlink_msg_vision_position_estimate_decode(&raw, &d);
        if (strcmp(field_name, "usec") == 0) { *out_value = (double)d.usec; return true; }
        if (strcmp(field_name, "x") == 0) { *out_value = (double)d.x; return true; }
        if (strcmp(field_name, "y") == 0) { *out_value = (double)d.y; return true; }
        if (strcmp(field_name, "z") == 0) { *out_value = (double)d.z; return true; }
        if (strcmp(field_name, "roll") == 0) { *out_value = (double)d.roll; return true; }
        if (strcmp(field_name, "pitch") == 0) { *out_value = (double)d.pitch; return true; }
        if (strcmp(field_name, "yaw") == 0) { *out_value = (double)d.yaw; return true; }
        if (strcmp(field_name, "reset_counter") == 0) { *out_value = (double)d.reset_counter; return true; }
        return false;
    }
    case 103: {
        mavlink_vision_speed_estimate_t d;
        mavlink_msg_vision_speed_estimate_decode(&raw, &d);
        if (strcmp(field_name, "usec") == 0) { *out_value = (double)d.usec; return true; }
        if (strcmp(field_name, "x") == 0) { *out_value = (double)d.x; return true; }
        if (strcmp(field_name, "y") == 0) { *out_value = (double)d.y; return true; }
        if (strcmp(field_name, "z") == 0) { *out_value = (double)d.z; return true; }
        if (strcmp(field_name, "reset_counter") == 0) { *out_value = (double)d.reset_counter; return true; }
        return false;
    }
    case 104: {
        mavlink_vicon_position_estimate_t d;
        mavlink_msg_vicon_position_estimate_decode(&raw, &d);
        if (strcmp(field_name, "usec") == 0) { *out_value = (double)d.usec; return true; }
        if (strcmp(field_name, "x") == 0) { *out_value = (double)d.x; return true; }
        if (strcmp(field_name, "y") == 0) { *out_value = (double)d.y; return true; }
        if (strcmp(field_name, "z") == 0) { *out_value = (double)d.z; return true; }
        if (strcmp(field_name, "roll") == 0) { *out_value = (double)d.roll; return true; }
        if (strcmp(field_name, "pitch") == 0) { *out_value = (double)d.pitch; return true; }
        if (strcmp(field_name, "yaw") == 0) { *out_value = (double)d.yaw; return true; }
        return false;
    }
    case 105: {
        mavlink_highres_imu_t d;
        mavlink_msg_highres_imu_decode(&raw, &d);
        if (strcmp(field_name, "time_usec") == 0) { *out_value = (double)d.time_usec; return true; }
        if (strcmp(field_name, "xacc") == 0) { *out_value = (double)d.xacc; return true; }
        if (strcmp(field_name, "yacc") == 0) { *out_value = (double)d.yacc; return true; }
        if (strcmp(field_name, "zacc") == 0) { *out_value = (double)d.zacc; return true; }
        if (strcmp(field_name, "xgyro") == 0) { *out_value = (double)d.xgyro; return true; }
        if (strcmp(field_name, "ygyro") == 0) { *out_value = (double)d.ygyro; return true; }
        if (strcmp(field_name, "zgyro") == 0) { *out_value = (double)d.zgyro; return true; }
        if (strcmp(field_name, "xmag") == 0) { *out_value = (double)d.xmag; return true; }
        if (strcmp(field_name, "ymag") == 0) { *out_value = (double)d.ymag; return true; }
        if (strcmp(field_name, "zmag") == 0) { *out_value = (double)d.zmag; return true; }
        if (strcmp(field_name, "abs_pressure") == 0) { *out_value = (double)d.abs_pressure; return true; }
        if (strcmp(field_name, "diff_pressure") == 0) { *out_value = (double)d.diff_pressure; return true; }
        if (strcmp(field_name, "pressure_alt") == 0) { *out_value = (double)d.pressure_alt; return true; }
        if (strcmp(field_name, "temperature") == 0) { *out_value = (double)d.temperature; return true; }
        if (strcmp(field_name, "fields_updated") == 0) { *out_value = (double)d.fields_updated; return true; }
        if (strcmp(field_name, "id") == 0) { *out_value = (double)d.id; return true; }
        return false;
    }
    case 106: {
        mavlink_optical_flow_rad_t d;
        mavlink_msg_optical_flow_rad_decode(&raw, &d);
        if (strcmp(field_name, "time_usec") == 0) { *out_value = (double)d.time_usec; return true; }
        if (strcmp(field_name, "sensor_id") == 0) { *out_value = (double)d.sensor_id; return true; }
        if (strcmp(field_name, "integration_time_us") == 0) { *out_value = (double)d.integration_time_us; return true; }
        if (strcmp(field_name, "integrated_x") == 0) { *out_value = (double)d.integrated_x; return true; }
        if (strcmp(field_name, "integrated_y") == 0) { *out_value = (double)d.integrated_y; return true; }
        if (strcmp(field_name, "integrated_xgyro") == 0) { *out_value = (double)d.integrated_xgyro; return true; }
        if (strcmp(field_name, "integrated_ygyro") == 0) { *out_value = (double)d.integrated_ygyro; return true; }
        if (strcmp(field_name, "integrated_zgyro") == 0) { *out_value = (double)d.integrated_zgyro; return true; }
        if (strcmp(field_name, "temperature") == 0) { *out_value = (double)d.temperature; return true; }
        if (strcmp(field_name, "quality") == 0) { *out_value = (double)d.quality; return true; }
        if (strcmp(field_name, "time_delta_distance_us") == 0) { *out_value = (double)d.time_delta_distance_us; return true; }
        if (strcmp(field_name, "distance") == 0) { *out_value = (double)d.distance; return true; }
        return false;
    }
    case 109: {
        mavlink_radio_status_t d;
        mavlink_msg_radio_status_decode(&raw, &d);
        if (strcmp(field_name, "rssi") == 0) { *out_value = (double)d.rssi; return true; }
        if (strcmp(field_name, "remrssi") == 0) { *out_value = (double)d.remrssi; return true; }
        if (strcmp(field_name, "txbuf") == 0) { *out_value = (double)d.txbuf; return true; }
        if (strcmp(field_name, "noise") == 0) { *out_value = (double)d.noise; return true; }
        if (strcmp(field_name, "remnoise") == 0) { *out_value = (double)d.remnoise; return true; }
        if (strcmp(field_name, "rxerrors") == 0) { *out_value = (double)d.rxerrors; return true; }
        if (strcmp(field_name, "fixed") == 0) { *out_value = (double)d.fixed; return true; }
        return false;
    }
    case 110: {
        mavlink_file_transfer_protocol_t d;
        mavlink_msg_file_transfer_protocol_decode(&raw, &d);
        if (strcmp(field_name, "target_network") == 0) { *out_value = (double)d.target_network; return true; }
        if (strcmp(field_name, "target_system") == 0) { *out_value = (double)d.target_system; return true; }
        if (strcmp(field_name, "target_component") == 0) { *out_value = (double)d.target_component; return true; }
        return false;
    }
    case 111: {
        mavlink_timesync_t d;
        mavlink_msg_timesync_decode(&raw, &d);
        if (strcmp(field_name, "tc1") == 0) { *out_value = (double)d.tc1; return true; }
        if (strcmp(field_name, "ts1") == 0) { *out_value = (double)d.ts1; return true; }
        if (strcmp(field_name, "target_system") == 0) { *out_value = (double)d.target_system; return true; }
        if (strcmp(field_name, "target_component") == 0) { *out_value = (double)d.target_component; return true; }
        return false;
    }
    case 112: {
        mavlink_camera_trigger_t d;
        mavlink_msg_camera_trigger_decode(&raw, &d);
        if (strcmp(field_name, "time_usec") == 0) { *out_value = (double)d.time_usec; return true; }
        if (strcmp(field_name, "seq") == 0) { *out_value = (double)d.seq; return true; }
        return false;
    }
    case 116: {
        mavlink_scaled_imu2_t d;
        mavlink_msg_scaled_imu2_decode(&raw, &d);
        if (strcmp(field_name, "time_boot_ms") == 0) { *out_value = (double)d.time_boot_ms; return true; }
        if (strcmp(field_name, "xacc") == 0) { *out_value = (double)d.xacc; return true; }
        if (strcmp(field_name, "yacc") == 0) { *out_value = (double)d.yacc; return true; }
        if (strcmp(field_name, "zacc") == 0) { *out_value = (double)d.zacc; return true; }
        if (strcmp(field_name, "xgyro") == 0) { *out_value = (double)d.xgyro; return true; }
        if (strcmp(field_name, "ygyro") == 0) { *out_value = (double)d.ygyro; return true; }
        if (strcmp(field_name, "zgyro") == 0) { *out_value = (double)d.zgyro; return true; }
        if (strcmp(field_name, "xmag") == 0) { *out_value = (double)d.xmag; return true; }
        if (strcmp(field_name, "ymag") == 0) { *out_value = (double)d.ymag; return true; }
        if (strcmp(field_name, "zmag") == 0) { *out_value = (double)d.zmag; return true; }
        if (strcmp(field_name, "temperature") == 0) { *out_value = (double)d.temperature; return true; }
        return false;
    }
    case 118: {
        mavlink_log_entry_t d;
        mavlink_msg_log_entry_decode(&raw, &d);
        if (strcmp(field_name, "id") == 0) { *out_value = (double)d.id; return true; }
        if (strcmp(field_name, "num_logs") == 0) { *out_value = (double)d.num_logs; return true; }
        if (strcmp(field_name, "last_log_num") == 0) { *out_value = (double)d.last_log_num; return true; }
        if (strcmp(field_name, "time_utc") == 0) { *out_value = (double)d.time_utc; return true; }
        if (strcmp(field_name, "size") == 0) { *out_value = (double)d.size; return true; }
        return false;
    }
    case 120: {
        mavlink_log_data_t d;
        mavlink_msg_log_data_decode(&raw, &d);
        if (strcmp(field_name, "id") == 0) { *out_value = (double)d.id; return true; }
        if (strcmp(field_name, "ofs") == 0) { *out_value = (double)d.ofs; return true; }
        if (strcmp(field_name, "count") == 0) { *out_value = (double)d.count; return true; }
        return false;
    }
    case 124: {
        mavlink_gps2_raw_t d;
        mavlink_msg_gps2_raw_decode(&raw, &d);
        if (strcmp(field_name, "time_usec") == 0) { *out_value = (double)d.time_usec; return true; }
        if (strcmp(field_name, "fix_type") == 0) { *out_value = (double)d.fix_type; return true; }
        if (strcmp(field_name, "lat") == 0) { *out_value = (double)d.lat; return true; }
        if (strcmp(field_name, "lon") == 0) { *out_value = (double)d.lon; return true; }
        if (strcmp(field_name, "alt") == 0) { *out_value = (double)d.alt; return true; }
        if (strcmp(field_name, "eph") == 0) { *out_value = (double)d.eph; return true; }
        if (strcmp(field_name, "epv") == 0) { *out_value = (double)d.epv; return true; }
        if (strcmp(field_name, "vel") == 0) { *out_value = (double)d.vel; return true; }
        if (strcmp(field_name, "cog") == 0) { *out_value = (double)d.cog; return true; }
        if (strcmp(field_name, "satellites_visible") == 0) { *out_value = (double)d.satellites_visible; return true; }
        if (strcmp(field_name, "dgps_numch") == 0) { *out_value = (double)d.dgps_numch; return true; }
        if (strcmp(field_name, "dgps_age") == 0) { *out_value = (double)d.dgps_age; return true; }
        if (strcmp(field_name, "yaw") == 0) { *out_value = (double)d.yaw; return true; }
        if (strcmp(field_name, "alt_ellipsoid") == 0) { *out_value = (double)d.alt_ellipsoid; return true; }
        if (strcmp(field_name, "h_acc") == 0) { *out_value = (double)d.h_acc; return true; }
        if (strcmp(field_name, "v_acc") == 0) { *out_value = (double)d.v_acc; return true; }
        if (strcmp(field_name, "vel_acc") == 0) { *out_value = (double)d.vel_acc; return true; }
        if (strcmp(field_name, "hdg_acc") == 0) { *out_value = (double)d.hdg_acc; return true; }
        return false;
    }
    case 125: {
        mavlink_power_status_t d;
        mavlink_msg_power_status_decode(&raw, &d);
        if (strcmp(field_name, "Vcc") == 0) { *out_value = (double)d.Vcc; return true; }
        if (strcmp(field_name, "Vservo") == 0) { *out_value = (double)d.Vservo; return true; }
        if (strcmp(field_name, "flags") == 0) { *out_value = (double)d.flags; return true; }
        return false;
    }
    case 127: {
        mavlink_gps_rtk_t d;
        mavlink_msg_gps_rtk_decode(&raw, &d);
        if (strcmp(field_name, "time_last_baseline_ms") == 0) { *out_value = (double)d.time_last_baseline_ms; return true; }
        if (strcmp(field_name, "rtk_receiver_id") == 0) { *out_value = (double)d.rtk_receiver_id; return true; }
        if (strcmp(field_name, "wn") == 0) { *out_value = (double)d.wn; return true; }
        if (strcmp(field_name, "tow") == 0) { *out_value = (double)d.tow; return true; }
        if (strcmp(field_name, "rtk_health") == 0) { *out_value = (double)d.rtk_health; return true; }
        if (strcmp(field_name, "rtk_rate") == 0) { *out_value = (double)d.rtk_rate; return true; }
        if (strcmp(field_name, "nsats") == 0) { *out_value = (double)d.nsats; return true; }
        if (strcmp(field_name, "baseline_coords_type") == 0) { *out_value = (double)d.baseline_coords_type; return true; }
        if (strcmp(field_name, "baseline_a_mm") == 0) { *out_value = (double)d.baseline_a_mm; return true; }
        if (strcmp(field_name, "baseline_b_mm") == 0) { *out_value = (double)d.baseline_b_mm; return true; }
        if (strcmp(field_name, "baseline_c_mm") == 0) { *out_value = (double)d.baseline_c_mm; return true; }
        if (strcmp(field_name, "accuracy") == 0) { *out_value = (double)d.accuracy; return true; }
        if (strcmp(field_name, "iar_num_hypotheses") == 0) { *out_value = (double)d.iar_num_hypotheses; return true; }
        return false;
    }
    case 128: {
        mavlink_gps2_rtk_t d;
        mavlink_msg_gps2_rtk_decode(&raw, &d);
        if (strcmp(field_name, "time_last_baseline_ms") == 0) { *out_value = (double)d.time_last_baseline_ms; return true; }
        if (strcmp(field_name, "rtk_receiver_id") == 0) { *out_value = (double)d.rtk_receiver_id; return true; }
        if (strcmp(field_name, "wn") == 0) { *out_value = (double)d.wn; return true; }
        if (strcmp(field_name, "tow") == 0) { *out_value = (double)d.tow; return true; }
        if (strcmp(field_name, "rtk_health") == 0) { *out_value = (double)d.rtk_health; return true; }
        if (strcmp(field_name, "rtk_rate") == 0) { *out_value = (double)d.rtk_rate; return true; }
        if (strcmp(field_name, "nsats") == 0) { *out_value = (double)d.nsats; return true; }
        if (strcmp(field_name, "baseline_coords_type") == 0) { *out_value = (double)d.baseline_coords_type; return true; }
        if (strcmp(field_name, "baseline_a_mm") == 0) { *out_value = (double)d.baseline_a_mm; return true; }
        if (strcmp(field_name, "baseline_b_mm") == 0) { *out_value = (double)d.baseline_b_mm; return true; }
        if (strcmp(field_name, "baseline_c_mm") == 0) { *out_value = (double)d.baseline_c_mm; return true; }
        if (strcmp(field_name, "accuracy") == 0) { *out_value = (double)d.accuracy; return true; }
        if (strcmp(field_name, "iar_num_hypotheses") == 0) { *out_value = (double)d.iar_num_hypotheses; return true; }
        return false;
    }
    case 129: {
        mavlink_scaled_imu3_t d;
        mavlink_msg_scaled_imu3_decode(&raw, &d);
        if (strcmp(field_name, "time_boot_ms") == 0) { *out_value = (double)d.time_boot_ms; return true; }
        if (strcmp(field_name, "xacc") == 0) { *out_value = (double)d.xacc; return true; }
        if (strcmp(field_name, "yacc") == 0) { *out_value = (double)d.yacc; return true; }
        if (strcmp(field_name, "zacc") == 0) { *out_value = (double)d.zacc; return true; }
        if (strcmp(field_name, "xgyro") == 0) { *out_value = (double)d.xgyro; return true; }
        if (strcmp(field_name, "ygyro") == 0) { *out_value = (double)d.ygyro; return true; }
        if (strcmp(field_name, "zgyro") == 0) { *out_value = (double)d.zgyro; return true; }
        if (strcmp(field_name, "xmag") == 0) { *out_value = (double)d.xmag; return true; }
        if (strcmp(field_name, "ymag") == 0) { *out_value = (double)d.ymag; return true; }
        if (strcmp(field_name, "zmag") == 0) { *out_value = (double)d.zmag; return true; }
        if (strcmp(field_name, "temperature") == 0) { *out_value = (double)d.temperature; return true; }
        return false;
    }
    case 130: {
        mavlink_data_transmission_handshake_t d;
        mavlink_msg_data_transmission_handshake_decode(&raw, &d);
        if (strcmp(field_name, "type") == 0) { *out_value = (double)d.type; return true; }
        if (strcmp(field_name, "size") == 0) { *out_value = (double)d.size; return true; }
        if (strcmp(field_name, "width") == 0) { *out_value = (double)d.width; return true; }
        if (strcmp(field_name, "height") == 0) { *out_value = (double)d.height; return true; }
        if (strcmp(field_name, "packets") == 0) { *out_value = (double)d.packets; return true; }
        if (strcmp(field_name, "payload") == 0) { *out_value = (double)d.payload; return true; }
        if (strcmp(field_name, "jpg_quality") == 0) { *out_value = (double)d.jpg_quality; return true; }
        return false;
    }
    case 131: {
        mavlink_encapsulated_data_t d;
        mavlink_msg_encapsulated_data_decode(&raw, &d);
        if (strcmp(field_name, "seqnr") == 0) { *out_value = (double)d.seqnr; return true; }
        return false;
    }
    case 132: {
        mavlink_distance_sensor_t d;
        mavlink_msg_distance_sensor_decode(&raw, &d);
        if (strcmp(field_name, "time_boot_ms") == 0) { *out_value = (double)d.time_boot_ms; return true; }
        if (strcmp(field_name, "min_distance") == 0) { *out_value = (double)d.min_distance; return true; }
        if (strcmp(field_name, "max_distance") == 0) { *out_value = (double)d.max_distance; return true; }
        if (strcmp(field_name, "current_distance") == 0) { *out_value = (double)d.current_distance; return true; }
        if (strcmp(field_name, "type") == 0) { *out_value = (double)d.type; return true; }
        if (strcmp(field_name, "id") == 0) { *out_value = (double)d.id; return true; }
        if (strcmp(field_name, "orientation") == 0) { *out_value = (double)d.orientation; return true; }
        if (strcmp(field_name, "covariance") == 0) { *out_value = (double)d.covariance; return true; }
        if (strcmp(field_name, "horizontal_fov") == 0) { *out_value = (double)d.horizontal_fov; return true; }
        if (strcmp(field_name, "vertical_fov") == 0) { *out_value = (double)d.vertical_fov; return true; }
        if (strcmp(field_name, "quaternion[0]") == 0) { *out_value = (double)d.quaternion[0]; return true; }
        if (strcmp(field_name, "quaternion[1]") == 0) { *out_value = (double)d.quaternion[1]; return true; }
        if (strcmp(field_name, "quaternion[2]") == 0) { *out_value = (double)d.quaternion[2]; return true; }
        if (strcmp(field_name, "quaternion[3]") == 0) { *out_value = (double)d.quaternion[3]; return true; }
        if (strcmp(field_name, "signal_quality") == 0) { *out_value = (double)d.signal_quality; return true; }
        return false;
    }
    case 133: {
        mavlink_terrain_request_t d;
        mavlink_msg_terrain_request_decode(&raw, &d);
        if (strcmp(field_name, "lat") == 0) { *out_value = (double)d.lat; return true; }
        if (strcmp(field_name, "lon") == 0) { *out_value = (double)d.lon; return true; }
        if (strcmp(field_name, "grid_spacing") == 0) { *out_value = (double)d.grid_spacing; return true; }
        if (strcmp(field_name, "mask") == 0) { *out_value = (double)d.mask; return true; }
        return false;
    }
    case 134: {
        mavlink_terrain_data_t d;
        mavlink_msg_terrain_data_decode(&raw, &d);
        if (strcmp(field_name, "lat") == 0) { *out_value = (double)d.lat; return true; }
        if (strcmp(field_name, "lon") == 0) { *out_value = (double)d.lon; return true; }
        if (strcmp(field_name, "grid_spacing") == 0) { *out_value = (double)d.grid_spacing; return true; }
        if (strcmp(field_name, "gridbit") == 0) { *out_value = (double)d.gridbit; return true; }
        return false;
    }
    case 135: {
        mavlink_terrain_check_t d;
        mavlink_msg_terrain_check_decode(&raw, &d);
        if (strcmp(field_name, "lat") == 0) { *out_value = (double)d.lat; return true; }
        if (strcmp(field_name, "lon") == 0) { *out_value = (double)d.lon; return true; }
        return false;
    }
    case 136: {
        mavlink_terrain_report_t d;
        mavlink_msg_terrain_report_decode(&raw, &d);
        if (strcmp(field_name, "lat") == 0) { *out_value = (double)d.lat; return true; }
        if (strcmp(field_name, "lon") == 0) { *out_value = (double)d.lon; return true; }
        if (strcmp(field_name, "spacing") == 0) { *out_value = (double)d.spacing; return true; }
        if (strcmp(field_name, "terrain_height") == 0) { *out_value = (double)d.terrain_height; return true; }
        if (strcmp(field_name, "current_height") == 0) { *out_value = (double)d.current_height; return true; }
        if (strcmp(field_name, "pending") == 0) { *out_value = (double)d.pending; return true; }
        if (strcmp(field_name, "loaded") == 0) { *out_value = (double)d.loaded; return true; }
        return false;
    }
    case 137: {
        mavlink_scaled_pressure2_t d;
        mavlink_msg_scaled_pressure2_decode(&raw, &d);
        if (strcmp(field_name, "time_boot_ms") == 0) { *out_value = (double)d.time_boot_ms; return true; }
        if (strcmp(field_name, "press_abs") == 0) { *out_value = (double)d.press_abs; return true; }
        if (strcmp(field_name, "press_diff") == 0) { *out_value = (double)d.press_diff; return true; }
        if (strcmp(field_name, "temperature") == 0) { *out_value = (double)d.temperature; return true; }
        if (strcmp(field_name, "temperature_press_diff") == 0) { *out_value = (double)d.temperature_press_diff; return true; }
        return false;
    }
    case 138: {
        mavlink_att_pos_mocap_t d;
        mavlink_msg_att_pos_mocap_decode(&raw, &d);
        if (strcmp(field_name, "time_usec") == 0) { *out_value = (double)d.time_usec; return true; }
        if (strcmp(field_name, "q[0]") == 0) { *out_value = (double)d.q[0]; return true; }
        if (strcmp(field_name, "q[1]") == 0) { *out_value = (double)d.q[1]; return true; }
        if (strcmp(field_name, "q[2]") == 0) { *out_value = (double)d.q[2]; return true; }
        if (strcmp(field_name, "q[3]") == 0) { *out_value = (double)d.q[3]; return true; }
        if (strcmp(field_name, "x") == 0) { *out_value = (double)d.x; return true; }
        if (strcmp(field_name, "y") == 0) { *out_value = (double)d.y; return true; }
        if (strcmp(field_name, "z") == 0) { *out_value = (double)d.z; return true; }
        return false;
    }
    case 140: {
        mavlink_actuator_control_target_t d;
        mavlink_msg_actuator_control_target_decode(&raw, &d);
        if (strcmp(field_name, "time_usec") == 0) { *out_value = (double)d.time_usec; return true; }
        if (strcmp(field_name, "group_mlx") == 0) { *out_value = (double)d.group_mlx; return true; }
        return false;
    }
    case 141: {
        mavlink_altitude_t d;
        mavlink_msg_altitude_decode(&raw, &d);
        if (strcmp(field_name, "time_usec") == 0) { *out_value = (double)d.time_usec; return true; }
        if (strcmp(field_name, "altitude_monotonic") == 0) { *out_value = (double)d.altitude_monotonic; return true; }
        if (strcmp(field_name, "altitude_amsl") == 0) { *out_value = (double)d.altitude_amsl; return true; }
        if (strcmp(field_name, "altitude_local") == 0) { *out_value = (double)d.altitude_local; return true; }
        if (strcmp(field_name, "altitude_relative") == 0) { *out_value = (double)d.altitude_relative; return true; }
        if (strcmp(field_name, "altitude_terrain") == 0) { *out_value = (double)d.altitude_terrain; return true; }
        if (strcmp(field_name, "bottom_clearance") == 0) { *out_value = (double)d.bottom_clearance; return true; }
        return false;
    }
    case 143: {
        mavlink_scaled_pressure3_t d;
        mavlink_msg_scaled_pressure3_decode(&raw, &d);
        if (strcmp(field_name, "time_boot_ms") == 0) { *out_value = (double)d.time_boot_ms; return true; }
        if (strcmp(field_name, "press_abs") == 0) { *out_value = (double)d.press_abs; return true; }
        if (strcmp(field_name, "press_diff") == 0) { *out_value = (double)d.press_diff; return true; }
        if (strcmp(field_name, "temperature") == 0) { *out_value = (double)d.temperature; return true; }
        if (strcmp(field_name, "temperature_press_diff") == 0) { *out_value = (double)d.temperature_press_diff; return true; }
        return false;
    }
    case 144: {
        mavlink_follow_target_t d;
        mavlink_msg_follow_target_decode(&raw, &d);
        if (strcmp(field_name, "timestamp") == 0) { *out_value = (double)d.timestamp; return true; }
        if (strcmp(field_name, "est_capabilities") == 0) { *out_value = (double)d.est_capabilities; return true; }
        if (strcmp(field_name, "lat") == 0) { *out_value = (double)d.lat; return true; }
        if (strcmp(field_name, "lon") == 0) { *out_value = (double)d.lon; return true; }
        if (strcmp(field_name, "alt") == 0) { *out_value = (double)d.alt; return true; }
        if (strcmp(field_name, "vel[0]") == 0) { *out_value = (double)d.vel[0]; return true; }
        if (strcmp(field_name, "vel[1]") == 0) { *out_value = (double)d.vel[1]; return true; }
        if (strcmp(field_name, "vel[2]") == 0) { *out_value = (double)d.vel[2]; return true; }
        if (strcmp(field_name, "acc[0]") == 0) { *out_value = (double)d.acc[0]; return true; }
        if (strcmp(field_name, "acc[1]") == 0) { *out_value = (double)d.acc[1]; return true; }
        if (strcmp(field_name, "acc[2]") == 0) { *out_value = (double)d.acc[2]; return true; }
        if (strcmp(field_name, "attitude_q[0]") == 0) { *out_value = (double)d.attitude_q[0]; return true; }
        if (strcmp(field_name, "attitude_q[1]") == 0) { *out_value = (double)d.attitude_q[1]; return true; }
        if (strcmp(field_name, "attitude_q[2]") == 0) { *out_value = (double)d.attitude_q[2]; return true; }
        if (strcmp(field_name, "attitude_q[3]") == 0) { *out_value = (double)d.attitude_q[3]; return true; }
        if (strcmp(field_name, "rates[0]") == 0) { *out_value = (double)d.rates[0]; return true; }
        if (strcmp(field_name, "rates[1]") == 0) { *out_value = (double)d.rates[1]; return true; }
        if (strcmp(field_name, "rates[2]") == 0) { *out_value = (double)d.rates[2]; return true; }
        if (strcmp(field_name, "position_cov[0]") == 0) { *out_value = (double)d.position_cov[0]; return true; }
        if (strcmp(field_name, "position_cov[1]") == 0) { *out_value = (double)d.position_cov[1]; return true; }
        if (strcmp(field_name, "position_cov[2]") == 0) { *out_value = (double)d.position_cov[2]; return true; }
        if (strcmp(field_name, "custom_state") == 0) { *out_value = (double)d.custom_state; return true; }
        return false;
    }
    case 146: {
        mavlink_control_system_state_t d;
        mavlink_msg_control_system_state_decode(&raw, &d);
        if (strcmp(field_name, "time_usec") == 0) { *out_value = (double)d.time_usec; return true; }
        if (strcmp(field_name, "x_acc") == 0) { *out_value = (double)d.x_acc; return true; }
        if (strcmp(field_name, "y_acc") == 0) { *out_value = (double)d.y_acc; return true; }
        if (strcmp(field_name, "z_acc") == 0) { *out_value = (double)d.z_acc; return true; }
        if (strcmp(field_name, "x_vel") == 0) { *out_value = (double)d.x_vel; return true; }
        if (strcmp(field_name, "y_vel") == 0) { *out_value = (double)d.y_vel; return true; }
        if (strcmp(field_name, "z_vel") == 0) { *out_value = (double)d.z_vel; return true; }
        if (strcmp(field_name, "x_pos") == 0) { *out_value = (double)d.x_pos; return true; }
        if (strcmp(field_name, "y_pos") == 0) { *out_value = (double)d.y_pos; return true; }
        if (strcmp(field_name, "z_pos") == 0) { *out_value = (double)d.z_pos; return true; }
        if (strcmp(field_name, "airspeed") == 0) { *out_value = (double)d.airspeed; return true; }
        if (strcmp(field_name, "vel_variance[0]") == 0) { *out_value = (double)d.vel_variance[0]; return true; }
        if (strcmp(field_name, "vel_variance[1]") == 0) { *out_value = (double)d.vel_variance[1]; return true; }
        if (strcmp(field_name, "vel_variance[2]") == 0) { *out_value = (double)d.vel_variance[2]; return true; }
        if (strcmp(field_name, "pos_variance[0]") == 0) { *out_value = (double)d.pos_variance[0]; return true; }
        if (strcmp(field_name, "pos_variance[1]") == 0) { *out_value = (double)d.pos_variance[1]; return true; }
        if (strcmp(field_name, "pos_variance[2]") == 0) { *out_value = (double)d.pos_variance[2]; return true; }
        if (strcmp(field_name, "q[0]") == 0) { *out_value = (double)d.q[0]; return true; }
        if (strcmp(field_name, "q[1]") == 0) { *out_value = (double)d.q[1]; return true; }
        if (strcmp(field_name, "q[2]") == 0) { *out_value = (double)d.q[2]; return true; }
        if (strcmp(field_name, "q[3]") == 0) { *out_value = (double)d.q[3]; return true; }
        if (strcmp(field_name, "roll_rate") == 0) { *out_value = (double)d.roll_rate; return true; }
        if (strcmp(field_name, "pitch_rate") == 0) { *out_value = (double)d.pitch_rate; return true; }
        if (strcmp(field_name, "yaw_rate") == 0) { *out_value = (double)d.yaw_rate; return true; }
        return false;
    }
    case 147: {
        mavlink_battery_status_t d;
        mavlink_msg_battery_status_decode(&raw, &d);
        if (strcmp(field_name, "id") == 0) { *out_value = (double)d.id; return true; }
        if (strcmp(field_name, "battery_function") == 0) { *out_value = (double)d.battery_function; return true; }
        if (strcmp(field_name, "type") == 0) { *out_value = (double)d.type; return true; }
        if (strcmp(field_name, "temperature") == 0) { *out_value = (double)d.temperature; return true; }
        if (strcmp(field_name, "current_battery") == 0) { *out_value = (double)d.current_battery; return true; }
        if (strcmp(field_name, "current_consumed") == 0) { *out_value = (double)d.current_consumed; return true; }
        if (strcmp(field_name, "energy_consumed") == 0) { *out_value = (double)d.energy_consumed; return true; }
        if (strcmp(field_name, "battery_remaining") == 0) { *out_value = (double)d.battery_remaining; return true; }
        if (strcmp(field_name, "time_remaining") == 0) { *out_value = (double)d.time_remaining; return true; }
        if (strcmp(field_name, "charge_state") == 0) { *out_value = (double)d.charge_state; return true; }
        if (strcmp(field_name, "voltages_ext[0]") == 0) { *out_value = (double)d.voltages_ext[0]; return true; }
        if (strcmp(field_name, "voltages_ext[1]") == 0) { *out_value = (double)d.voltages_ext[1]; return true; }
        if (strcmp(field_name, "voltages_ext[2]") == 0) { *out_value = (double)d.voltages_ext[2]; return true; }
        if (strcmp(field_name, "voltages_ext[3]") == 0) { *out_value = (double)d.voltages_ext[3]; return true; }
        if (strcmp(field_name, "mode") == 0) { *out_value = (double)d.mode; return true; }
        if (strcmp(field_name, "fault_bitmask") == 0) { *out_value = (double)d.fault_bitmask; return true; }
        return false;
    }
    case 148: {
        mavlink_autopilot_version_t d;
        mavlink_msg_autopilot_version_decode(&raw, &d);
        if (strcmp(field_name, "capabilities") == 0) { *out_value = (double)d.capabilities; return true; }
        if (strcmp(field_name, "flight_sw_version") == 0) { *out_value = (double)d.flight_sw_version; return true; }
        if (strcmp(field_name, "middleware_sw_version") == 0) { *out_value = (double)d.middleware_sw_version; return true; }
        if (strcmp(field_name, "os_sw_version") == 0) { *out_value = (double)d.os_sw_version; return true; }
        if (strcmp(field_name, "board_version") == 0) { *out_value = (double)d.board_version; return true; }
        if (strcmp(field_name, "vendor_id") == 0) { *out_value = (double)d.vendor_id; return true; }
        if (strcmp(field_name, "product_id") == 0) { *out_value = (double)d.product_id; return true; }
        if (strcmp(field_name, "uid") == 0) { *out_value = (double)d.uid; return true; }
        return false;
    }
    case 149: {
        mavlink_landing_target_t d;
        mavlink_msg_landing_target_decode(&raw, &d);
        if (strcmp(field_name, "time_usec") == 0) { *out_value = (double)d.time_usec; return true; }
        if (strcmp(field_name, "target_num") == 0) { *out_value = (double)d.target_num; return true; }
        if (strcmp(field_name, "frame") == 0) { *out_value = (double)d.frame; return true; }
        if (strcmp(field_name, "angle_x") == 0) { *out_value = (double)d.angle_x; return true; }
        if (strcmp(field_name, "angle_y") == 0) { *out_value = (double)d.angle_y; return true; }
        if (strcmp(field_name, "distance") == 0) { *out_value = (double)d.distance; return true; }
        if (strcmp(field_name, "size_x") == 0) { *out_value = (double)d.size_x; return true; }
        if (strcmp(field_name, "size_y") == 0) { *out_value = (double)d.size_y; return true; }
        if (strcmp(field_name, "x") == 0) { *out_value = (double)d.x; return true; }
        if (strcmp(field_name, "y") == 0) { *out_value = (double)d.y; return true; }
        if (strcmp(field_name, "z") == 0) { *out_value = (double)d.z; return true; }
        if (strcmp(field_name, "q[0]") == 0) { *out_value = (double)d.q[0]; return true; }
        if (strcmp(field_name, "q[1]") == 0) { *out_value = (double)d.q[1]; return true; }
        if (strcmp(field_name, "q[2]") == 0) { *out_value = (double)d.q[2]; return true; }
        if (strcmp(field_name, "q[3]") == 0) { *out_value = (double)d.q[3]; return true; }
        if (strcmp(field_name, "type") == 0) { *out_value = (double)d.type; return true; }
        if (strcmp(field_name, "position_valid") == 0) { *out_value = (double)d.position_valid; return true; }
        return false;
    }
    case 150: {
        mavlink_sensor_offsets_t d;
        mavlink_msg_sensor_offsets_decode(&raw, &d);
        if (strcmp(field_name, "mag_ofs_x") == 0) { *out_value = (double)d.mag_ofs_x; return true; }
        if (strcmp(field_name, "mag_ofs_y") == 0) { *out_value = (double)d.mag_ofs_y; return true; }
        if (strcmp(field_name, "mag_ofs_z") == 0) { *out_value = (double)d.mag_ofs_z; return true; }
        if (strcmp(field_name, "mag_declination") == 0) { *out_value = (double)d.mag_declination; return true; }
        if (strcmp(field_name, "raw_press") == 0) { *out_value = (double)d.raw_press; return true; }
        if (strcmp(field_name, "raw_temp") == 0) { *out_value = (double)d.raw_temp; return true; }
        if (strcmp(field_name, "gyro_cal_x") == 0) { *out_value = (double)d.gyro_cal_x; return true; }
        if (strcmp(field_name, "gyro_cal_y") == 0) { *out_value = (double)d.gyro_cal_y; return true; }
        if (strcmp(field_name, "gyro_cal_z") == 0) { *out_value = (double)d.gyro_cal_z; return true; }
        if (strcmp(field_name, "accel_cal_x") == 0) { *out_value = (double)d.accel_cal_x; return true; }
        if (strcmp(field_name, "accel_cal_y") == 0) { *out_value = (double)d.accel_cal_y; return true; }
        if (strcmp(field_name, "accel_cal_z") == 0) { *out_value = (double)d.accel_cal_z; return true; }
        return false;
    }
    case 152: {
        mavlink_meminfo_t d;
        mavlink_msg_meminfo_decode(&raw, &d);
        if (strcmp(field_name, "brkval") == 0) { *out_value = (double)d.brkval; return true; }
        if (strcmp(field_name, "freemem") == 0) { *out_value = (double)d.freemem; return true; }
        if (strcmp(field_name, "freemem32") == 0) { *out_value = (double)d.freemem32; return true; }
        return false;
    }
    case 153: {
        mavlink_ap_adc_t d;
        mavlink_msg_ap_adc_decode(&raw, &d);
        if (strcmp(field_name, "adc1") == 0) { *out_value = (double)d.adc1; return true; }
        if (strcmp(field_name, "adc2") == 0) { *out_value = (double)d.adc2; return true; }
        if (strcmp(field_name, "adc3") == 0) { *out_value = (double)d.adc3; return true; }
        if (strcmp(field_name, "adc4") == 0) { *out_value = (double)d.adc4; return true; }
        if (strcmp(field_name, "adc5") == 0) { *out_value = (double)d.adc5; return true; }
        if (strcmp(field_name, "adc6") == 0) { *out_value = (double)d.adc6; return true; }
        return false;
    }
    case 158: {
        mavlink_mount_status_t d;
        mavlink_msg_mount_status_decode(&raw, &d);
        if (strcmp(field_name, "target_system") == 0) { *out_value = (double)d.target_system; return true; }
        if (strcmp(field_name, "target_component") == 0) { *out_value = (double)d.target_component; return true; }
        if (strcmp(field_name, "pointing_a") == 0) { *out_value = (double)d.pointing_a; return true; }
        if (strcmp(field_name, "pointing_b") == 0) { *out_value = (double)d.pointing_b; return true; }
        if (strcmp(field_name, "pointing_c") == 0) { *out_value = (double)d.pointing_c; return true; }
        if (strcmp(field_name, "mount_mode") == 0) { *out_value = (double)d.mount_mode; return true; }
        return false;
    }
    case 162: {
        mavlink_fence_status_t d;
        mavlink_msg_fence_status_decode(&raw, &d);
        if (strcmp(field_name, "breach_status") == 0) { *out_value = (double)d.breach_status; return true; }
        if (strcmp(field_name, "breach_count") == 0) { *out_value = (double)d.breach_count; return true; }
        if (strcmp(field_name, "breach_type") == 0) { *out_value = (double)d.breach_type; return true; }
        if (strcmp(field_name, "breach_time") == 0) { *out_value = (double)d.breach_time; return true; }
        if (strcmp(field_name, "breach_mitigation") == 0) { *out_value = (double)d.breach_mitigation; return true; }
        return false;
    }
    case 163: {
        mavlink_ahrs_t d;
        mavlink_msg_ahrs_decode(&raw, &d);
        if (strcmp(field_name, "omegaIx") == 0) { *out_value = (double)d.omegaIx; return true; }
        if (strcmp(field_name, "omegaIy") == 0) { *out_value = (double)d.omegaIy; return true; }
        if (strcmp(field_name, "omegaIz") == 0) { *out_value = (double)d.omegaIz; return true; }
        if (strcmp(field_name, "accel_weight") == 0) { *out_value = (double)d.accel_weight; return true; }
        if (strcmp(field_name, "renorm_val") == 0) { *out_value = (double)d.renorm_val; return true; }
        if (strcmp(field_name, "error_rp") == 0) { *out_value = (double)d.error_rp; return true; }
        if (strcmp(field_name, "error_yaw") == 0) { *out_value = (double)d.error_yaw; return true; }
        return false;
    }
    case 164: {
        mavlink_simstate_t d;
        mavlink_msg_simstate_decode(&raw, &d);
        if (strcmp(field_name, "roll") == 0) { *out_value = (double)d.roll; return true; }
        if (strcmp(field_name, "pitch") == 0) { *out_value = (double)d.pitch; return true; }
        if (strcmp(field_name, "yaw") == 0) { *out_value = (double)d.yaw; return true; }
        if (strcmp(field_name, "xacc") == 0) { *out_value = (double)d.xacc; return true; }
        if (strcmp(field_name, "yacc") == 0) { *out_value = (double)d.yacc; return true; }
        if (strcmp(field_name, "zacc") == 0) { *out_value = (double)d.zacc; return true; }
        if (strcmp(field_name, "xgyro") == 0) { *out_value = (double)d.xgyro; return true; }
        if (strcmp(field_name, "ygyro") == 0) { *out_value = (double)d.ygyro; return true; }
        if (strcmp(field_name, "zgyro") == 0) { *out_value = (double)d.zgyro; return true; }
        if (strcmp(field_name, "lat") == 0) { *out_value = (double)d.lat; return true; }
        if (strcmp(field_name, "lng") == 0) { *out_value = (double)d.lng; return true; }
        return false;
    }
    case 165: {
        mavlink_hwstatus_t d;
        mavlink_msg_hwstatus_decode(&raw, &d);
        if (strcmp(field_name, "Vcc") == 0) { *out_value = (double)d.Vcc; return true; }
        if (strcmp(field_name, "I2Cerr") == 0) { *out_value = (double)d.I2Cerr; return true; }
        return false;
    }
    case 166: {
        mavlink_radio_t d;
        mavlink_msg_radio_decode(&raw, &d);
        if (strcmp(field_name, "rssi") == 0) { *out_value = (double)d.rssi; return true; }
        if (strcmp(field_name, "remrssi") == 0) { *out_value = (double)d.remrssi; return true; }
        if (strcmp(field_name, "txbuf") == 0) { *out_value = (double)d.txbuf; return true; }
        if (strcmp(field_name, "noise") == 0) { *out_value = (double)d.noise; return true; }
        if (strcmp(field_name, "remnoise") == 0) { *out_value = (double)d.remnoise; return true; }
        if (strcmp(field_name, "rxerrors") == 0) { *out_value = (double)d.rxerrors; return true; }
        if (strcmp(field_name, "fixed") == 0) { *out_value = (double)d.fixed; return true; }
        return false;
    }
    case 167: {
        mavlink_limits_status_t d;
        mavlink_msg_limits_status_decode(&raw, &d);
        if (strcmp(field_name, "limits_state") == 0) { *out_value = (double)d.limits_state; return true; }
        if (strcmp(field_name, "last_trigger") == 0) { *out_value = (double)d.last_trigger; return true; }
        if (strcmp(field_name, "last_action") == 0) { *out_value = (double)d.last_action; return true; }
        if (strcmp(field_name, "last_recovery") == 0) { *out_value = (double)d.last_recovery; return true; }
        if (strcmp(field_name, "last_clear") == 0) { *out_value = (double)d.last_clear; return true; }
        if (strcmp(field_name, "breach_count") == 0) { *out_value = (double)d.breach_count; return true; }
        if (strcmp(field_name, "mods_enabled") == 0) { *out_value = (double)d.mods_enabled; return true; }
        if (strcmp(field_name, "mods_required") == 0) { *out_value = (double)d.mods_required; return true; }
        if (strcmp(field_name, "mods_triggered") == 0) { *out_value = (double)d.mods_triggered; return true; }
        return false;
    }
    case 168: {
        mavlink_wind_t d;
        mavlink_msg_wind_decode(&raw, &d);
        if (strcmp(field_name, "direction") == 0) { *out_value = (double)d.direction; return true; }
        if (strcmp(field_name, "speed") == 0) { *out_value = (double)d.speed; return true; }
        if (strcmp(field_name, "speed_z") == 0) { *out_value = (double)d.speed_z; return true; }
        return false;
    }
    case 169: {
        mavlink_data16_t d;
        mavlink_msg_data16_decode(&raw, &d);
        if (strcmp(field_name, "type") == 0) { *out_value = (double)d.type; return true; }
        if (strcmp(field_name, "len") == 0) { *out_value = (double)d.len; return true; }
        return false;
    }
    case 170: {
        mavlink_data32_t d;
        mavlink_msg_data32_decode(&raw, &d);
        if (strcmp(field_name, "type") == 0) { *out_value = (double)d.type; return true; }
        if (strcmp(field_name, "len") == 0) { *out_value = (double)d.len; return true; }
        return false;
    }
    case 171: {
        mavlink_data64_t d;
        mavlink_msg_data64_decode(&raw, &d);
        if (strcmp(field_name, "type") == 0) { *out_value = (double)d.type; return true; }
        if (strcmp(field_name, "len") == 0) { *out_value = (double)d.len; return true; }
        return false;
    }
    case 172: {
        mavlink_data96_t d;
        mavlink_msg_data96_decode(&raw, &d);
        if (strcmp(field_name, "type") == 0) { *out_value = (double)d.type; return true; }
        if (strcmp(field_name, "len") == 0) { *out_value = (double)d.len; return true; }
        return false;
    }
    case 173: {
        mavlink_rangefinder_t d;
        mavlink_msg_rangefinder_decode(&raw, &d);
        if (strcmp(field_name, "distance") == 0) { *out_value = (double)d.distance; return true; }
        if (strcmp(field_name, "voltage") == 0) { *out_value = (double)d.voltage; return true; }
        return false;
    }
    case 174: {
        mavlink_airspeed_autocal_t d;
        mavlink_msg_airspeed_autocal_decode(&raw, &d);
        if (strcmp(field_name, "vx") == 0) { *out_value = (double)d.vx; return true; }
        if (strcmp(field_name, "vy") == 0) { *out_value = (double)d.vy; return true; }
        if (strcmp(field_name, "vz") == 0) { *out_value = (double)d.vz; return true; }
        if (strcmp(field_name, "diff_pressure") == 0) { *out_value = (double)d.diff_pressure; return true; }
        if (strcmp(field_name, "EAS2TAS") == 0) { *out_value = (double)d.EAS2TAS; return true; }
        if (strcmp(field_name, "ratio") == 0) { *out_value = (double)d.ratio; return true; }
        if (strcmp(field_name, "state_x") == 0) { *out_value = (double)d.state_x; return true; }
        if (strcmp(field_name, "state_y") == 0) { *out_value = (double)d.state_y; return true; }
        if (strcmp(field_name, "state_z") == 0) { *out_value = (double)d.state_z; return true; }
        if (strcmp(field_name, "Pax") == 0) { *out_value = (double)d.Pax; return true; }
        if (strcmp(field_name, "Pby") == 0) { *out_value = (double)d.Pby; return true; }
        if (strcmp(field_name, "Pcz") == 0) { *out_value = (double)d.Pcz; return true; }
        return false;
    }
    case 175: {
        mavlink_rally_point_t d;
        mavlink_msg_rally_point_decode(&raw, &d);
        if (strcmp(field_name, "target_system") == 0) { *out_value = (double)d.target_system; return true; }
        if (strcmp(field_name, "target_component") == 0) { *out_value = (double)d.target_component; return true; }
        if (strcmp(field_name, "idx") == 0) { *out_value = (double)d.idx; return true; }
        if (strcmp(field_name, "count") == 0) { *out_value = (double)d.count; return true; }
        if (strcmp(field_name, "lat") == 0) { *out_value = (double)d.lat; return true; }
        if (strcmp(field_name, "lng") == 0) { *out_value = (double)d.lng; return true; }
        if (strcmp(field_name, "alt") == 0) { *out_value = (double)d.alt; return true; }
        if (strcmp(field_name, "break_alt") == 0) { *out_value = (double)d.break_alt; return true; }
        if (strcmp(field_name, "land_dir") == 0) { *out_value = (double)d.land_dir; return true; }
        if (strcmp(field_name, "flags") == 0) { *out_value = (double)d.flags; return true; }
        return false;
    }
    case 176: {
        mavlink_rally_fetch_point_t d;
        mavlink_msg_rally_fetch_point_decode(&raw, &d);
        if (strcmp(field_name, "target_system") == 0) { *out_value = (double)d.target_system; return true; }
        if (strcmp(field_name, "target_component") == 0) { *out_value = (double)d.target_component; return true; }
        if (strcmp(field_name, "idx") == 0) { *out_value = (double)d.idx; return true; }
        return false;
    }
    case 177: {
        mavlink_compassmot_status_t d;
        mavlink_msg_compassmot_status_decode(&raw, &d);
        if (strcmp(field_name, "throttle") == 0) { *out_value = (double)d.throttle; return true; }
        if (strcmp(field_name, "current") == 0) { *out_value = (double)d.current; return true; }
        if (strcmp(field_name, "interference") == 0) { *out_value = (double)d.interference; return true; }
        if (strcmp(field_name, "CompensationX") == 0) { *out_value = (double)d.CompensationX; return true; }
        if (strcmp(field_name, "CompensationY") == 0) { *out_value = (double)d.CompensationY; return true; }
        if (strcmp(field_name, "CompensationZ") == 0) { *out_value = (double)d.CompensationZ; return true; }
        return false;
    }
    case 178: {
        mavlink_ahrs2_t d;
        mavlink_msg_ahrs2_decode(&raw, &d);
        if (strcmp(field_name, "roll") == 0) { *out_value = (double)d.roll; return true; }
        if (strcmp(field_name, "pitch") == 0) { *out_value = (double)d.pitch; return true; }
        if (strcmp(field_name, "yaw") == 0) { *out_value = (double)d.yaw; return true; }
        if (strcmp(field_name, "altitude") == 0) { *out_value = (double)d.altitude; return true; }
        if (strcmp(field_name, "lat") == 0) { *out_value = (double)d.lat; return true; }
        if (strcmp(field_name, "lng") == 0) { *out_value = (double)d.lng; return true; }
        return false;
    }
    case 179: {
        mavlink_camera_status_t d;
        mavlink_msg_camera_status_decode(&raw, &d);
        if (strcmp(field_name, "time_usec") == 0) { *out_value = (double)d.time_usec; return true; }
        if (strcmp(field_name, "target_system") == 0) { *out_value = (double)d.target_system; return true; }
        if (strcmp(field_name, "cam_idx") == 0) { *out_value = (double)d.cam_idx; return true; }
        if (strcmp(field_name, "img_idx") == 0) { *out_value = (double)d.img_idx; return true; }
        if (strcmp(field_name, "event_id") == 0) { *out_value = (double)d.event_id; return true; }
        if (strcmp(field_name, "p1") == 0) { *out_value = (double)d.p1; return true; }
        if (strcmp(field_name, "p2") == 0) { *out_value = (double)d.p2; return true; }
        if (strcmp(field_name, "p3") == 0) { *out_value = (double)d.p3; return true; }
        if (strcmp(field_name, "p4") == 0) { *out_value = (double)d.p4; return true; }
        return false;
    }
    case 180: {
        mavlink_camera_feedback_t d;
        mavlink_msg_camera_feedback_decode(&raw, &d);
        if (strcmp(field_name, "time_usec") == 0) { *out_value = (double)d.time_usec; return true; }
        if (strcmp(field_name, "target_system") == 0) { *out_value = (double)d.target_system; return true; }
        if (strcmp(field_name, "cam_idx") == 0) { *out_value = (double)d.cam_idx; return true; }
        if (strcmp(field_name, "img_idx") == 0) { *out_value = (double)d.img_idx; return true; }
        if (strcmp(field_name, "lat") == 0) { *out_value = (double)d.lat; return true; }
        if (strcmp(field_name, "lng") == 0) { *out_value = (double)d.lng; return true; }
        if (strcmp(field_name, "alt_msl") == 0) { *out_value = (double)d.alt_msl; return true; }
        if (strcmp(field_name, "alt_rel") == 0) { *out_value = (double)d.alt_rel; return true; }
        if (strcmp(field_name, "roll") == 0) { *out_value = (double)d.roll; return true; }
        if (strcmp(field_name, "pitch") == 0) { *out_value = (double)d.pitch; return true; }
        if (strcmp(field_name, "yaw") == 0) { *out_value = (double)d.yaw; return true; }
        if (strcmp(field_name, "foc_len") == 0) { *out_value = (double)d.foc_len; return true; }
        if (strcmp(field_name, "flags") == 0) { *out_value = (double)d.flags; return true; }
        if (strcmp(field_name, "completed_captures") == 0) { *out_value = (double)d.completed_captures; return true; }
        return false;
    }
    case 181: {
        mavlink_battery2_t d;
        mavlink_msg_battery2_decode(&raw, &d);
        if (strcmp(field_name, "voltage") == 0) { *out_value = (double)d.voltage; return true; }
        if (strcmp(field_name, "current_battery") == 0) { *out_value = (double)d.current_battery; return true; }
        return false;
    }
    case 182: {
        mavlink_ahrs3_t d;
        mavlink_msg_ahrs3_decode(&raw, &d);
        if (strcmp(field_name, "roll") == 0) { *out_value = (double)d.roll; return true; }
        if (strcmp(field_name, "pitch") == 0) { *out_value = (double)d.pitch; return true; }
        if (strcmp(field_name, "yaw") == 0) { *out_value = (double)d.yaw; return true; }
        if (strcmp(field_name, "altitude") == 0) { *out_value = (double)d.altitude; return true; }
        if (strcmp(field_name, "lat") == 0) { *out_value = (double)d.lat; return true; }
        if (strcmp(field_name, "lng") == 0) { *out_value = (double)d.lng; return true; }
        if (strcmp(field_name, "v1") == 0) { *out_value = (double)d.v1; return true; }
        if (strcmp(field_name, "v2") == 0) { *out_value = (double)d.v2; return true; }
        if (strcmp(field_name, "v3") == 0) { *out_value = (double)d.v3; return true; }
        if (strcmp(field_name, "v4") == 0) { *out_value = (double)d.v4; return true; }
        return false;
    }
    case 185: {
        mavlink_remote_log_block_status_t d;
        mavlink_msg_remote_log_block_status_decode(&raw, &d);
        if (strcmp(field_name, "target_system") == 0) { *out_value = (double)d.target_system; return true; }
        if (strcmp(field_name, "target_component") == 0) { *out_value = (double)d.target_component; return true; }
        if (strcmp(field_name, "seqno") == 0) { *out_value = (double)d.seqno; return true; }
        if (strcmp(field_name, "status") == 0) { *out_value = (double)d.status; return true; }
        return false;
    }
    case 191: {
        mavlink_mag_cal_progress_t d;
        mavlink_msg_mag_cal_progress_decode(&raw, &d);
        if (strcmp(field_name, "compass_id") == 0) { *out_value = (double)d.compass_id; return true; }
        if (strcmp(field_name, "cal_mask") == 0) { *out_value = (double)d.cal_mask; return true; }
        if (strcmp(field_name, "cal_status") == 0) { *out_value = (double)d.cal_status; return true; }
        if (strcmp(field_name, "attempt") == 0) { *out_value = (double)d.attempt; return true; }
        if (strcmp(field_name, "completion_pct") == 0) { *out_value = (double)d.completion_pct; return true; }
        if (strcmp(field_name, "direction_x") == 0) { *out_value = (double)d.direction_x; return true; }
        if (strcmp(field_name, "direction_y") == 0) { *out_value = (double)d.direction_y; return true; }
        if (strcmp(field_name, "direction_z") == 0) { *out_value = (double)d.direction_z; return true; }
        return false;
    }
    case 192: {
        mavlink_mag_cal_report_t d;
        mavlink_msg_mag_cal_report_decode(&raw, &d);
        if (strcmp(field_name, "compass_id") == 0) { *out_value = (double)d.compass_id; return true; }
        if (strcmp(field_name, "cal_mask") == 0) { *out_value = (double)d.cal_mask; return true; }
        if (strcmp(field_name, "cal_status") == 0) { *out_value = (double)d.cal_status; return true; }
        if (strcmp(field_name, "autosaved") == 0) { *out_value = (double)d.autosaved; return true; }
        if (strcmp(field_name, "fitness") == 0) { *out_value = (double)d.fitness; return true; }
        if (strcmp(field_name, "ofs_x") == 0) { *out_value = (double)d.ofs_x; return true; }
        if (strcmp(field_name, "ofs_y") == 0) { *out_value = (double)d.ofs_y; return true; }
        if (strcmp(field_name, "ofs_z") == 0) { *out_value = (double)d.ofs_z; return true; }
        if (strcmp(field_name, "diag_x") == 0) { *out_value = (double)d.diag_x; return true; }
        if (strcmp(field_name, "diag_y") == 0) { *out_value = (double)d.diag_y; return true; }
        if (strcmp(field_name, "diag_z") == 0) { *out_value = (double)d.diag_z; return true; }
        if (strcmp(field_name, "offdiag_x") == 0) { *out_value = (double)d.offdiag_x; return true; }
        if (strcmp(field_name, "offdiag_y") == 0) { *out_value = (double)d.offdiag_y; return true; }
        if (strcmp(field_name, "offdiag_z") == 0) { *out_value = (double)d.offdiag_z; return true; }
        if (strcmp(field_name, "orientation_confidence") == 0) { *out_value = (double)d.orientation_confidence; return true; }
        if (strcmp(field_name, "old_orientation") == 0) { *out_value = (double)d.old_orientation; return true; }
        if (strcmp(field_name, "new_orientation") == 0) { *out_value = (double)d.new_orientation; return true; }
        if (strcmp(field_name, "scale_factor") == 0) { *out_value = (double)d.scale_factor; return true; }
        return false;
    }
    case 193: {
        mavlink_ekf_status_report_t d;
        mavlink_msg_ekf_status_report_decode(&raw, &d);
        if (strcmp(field_name, "flags") == 0) { *out_value = (double)d.flags; return true; }
        if (strcmp(field_name, "velocity_variance") == 0) { *out_value = (double)d.velocity_variance; return true; }
        if (strcmp(field_name, "pos_horiz_variance") == 0) { *out_value = (double)d.pos_horiz_variance; return true; }
        if (strcmp(field_name, "pos_vert_variance") == 0) { *out_value = (double)d.pos_vert_variance; return true; }
        if (strcmp(field_name, "compass_variance") == 0) { *out_value = (double)d.compass_variance; return true; }
        if (strcmp(field_name, "terrain_alt_variance") == 0) { *out_value = (double)d.terrain_alt_variance; return true; }
        if (strcmp(field_name, "airspeed_variance") == 0) { *out_value = (double)d.airspeed_variance; return true; }
        return false;
    }
    case 194: {
        mavlink_pid_tuning_t d;
        mavlink_msg_pid_tuning_decode(&raw, &d);
        if (strcmp(field_name, "axis") == 0) { *out_value = (double)d.axis; return true; }
        if (strcmp(field_name, "desired") == 0) { *out_value = (double)d.desired; return true; }
        if (strcmp(field_name, "achieved") == 0) { *out_value = (double)d.achieved; return true; }
        if (strcmp(field_name, "FF") == 0) { *out_value = (double)d.FF; return true; }
        if (strcmp(field_name, "P") == 0) { *out_value = (double)d.P; return true; }
        if (strcmp(field_name, "I") == 0) { *out_value = (double)d.I; return true; }
        if (strcmp(field_name, "D") == 0) { *out_value = (double)d.D; return true; }
        if (strcmp(field_name, "SRate") == 0) { *out_value = (double)d.SRate; return true; }
        if (strcmp(field_name, "PDmod") == 0) { *out_value = (double)d.PDmod; return true; }
        return false;
    }
    case 195: {
        mavlink_deepstall_t d;
        mavlink_msg_deepstall_decode(&raw, &d);
        if (strcmp(field_name, "landing_lat") == 0) { *out_value = (double)d.landing_lat; return true; }
        if (strcmp(field_name, "landing_lon") == 0) { *out_value = (double)d.landing_lon; return true; }
        if (strcmp(field_name, "path_lat") == 0) { *out_value = (double)d.path_lat; return true; }
        if (strcmp(field_name, "path_lon") == 0) { *out_value = (double)d.path_lon; return true; }
        if (strcmp(field_name, "arc_entry_lat") == 0) { *out_value = (double)d.arc_entry_lat; return true; }
        if (strcmp(field_name, "arc_entry_lon") == 0) { *out_value = (double)d.arc_entry_lon; return true; }
        if (strcmp(field_name, "altitude") == 0) { *out_value = (double)d.altitude; return true; }
        if (strcmp(field_name, "expected_travel_distance") == 0) { *out_value = (double)d.expected_travel_distance; return true; }
        if (strcmp(field_name, "cross_track_error") == 0) { *out_value = (double)d.cross_track_error; return true; }
        if (strcmp(field_name, "stage") == 0) { *out_value = (double)d.stage; return true; }
        return false;
    }
    case 200: {
        mavlink_gimbal_report_t d;
        mavlink_msg_gimbal_report_decode(&raw, &d);
        if (strcmp(field_name, "target_system") == 0) { *out_value = (double)d.target_system; return true; }
        if (strcmp(field_name, "target_component") == 0) { *out_value = (double)d.target_component; return true; }
        if (strcmp(field_name, "delta_time") == 0) { *out_value = (double)d.delta_time; return true; }
        if (strcmp(field_name, "delta_angle_x") == 0) { *out_value = (double)d.delta_angle_x; return true; }
        if (strcmp(field_name, "delta_angle_y") == 0) { *out_value = (double)d.delta_angle_y; return true; }
        if (strcmp(field_name, "delta_angle_z") == 0) { *out_value = (double)d.delta_angle_z; return true; }
        if (strcmp(field_name, "delta_velocity_x") == 0) { *out_value = (double)d.delta_velocity_x; return true; }
        if (strcmp(field_name, "delta_velocity_y") == 0) { *out_value = (double)d.delta_velocity_y; return true; }
        if (strcmp(field_name, "delta_velocity_z") == 0) { *out_value = (double)d.delta_velocity_z; return true; }
        if (strcmp(field_name, "joint_roll") == 0) { *out_value = (double)d.joint_roll; return true; }
        if (strcmp(field_name, "joint_el") == 0) { *out_value = (double)d.joint_el; return true; }
        if (strcmp(field_name, "joint_az") == 0) { *out_value = (double)d.joint_az; return true; }
        return false;
    }
    case 201: {
        mavlink_gimbal_control_t d;
        mavlink_msg_gimbal_control_decode(&raw, &d);
        if (strcmp(field_name, "target_system") == 0) { *out_value = (double)d.target_system; return true; }
        if (strcmp(field_name, "target_component") == 0) { *out_value = (double)d.target_component; return true; }
        if (strcmp(field_name, "demanded_rate_x") == 0) { *out_value = (double)d.demanded_rate_x; return true; }
        if (strcmp(field_name, "demanded_rate_y") == 0) { *out_value = (double)d.demanded_rate_y; return true; }
        if (strcmp(field_name, "demanded_rate_z") == 0) { *out_value = (double)d.demanded_rate_z; return true; }
        return false;
    }
    case 214: {
        mavlink_gimbal_torque_cmd_report_t d;
        mavlink_msg_gimbal_torque_cmd_report_decode(&raw, &d);
        if (strcmp(field_name, "target_system") == 0) { *out_value = (double)d.target_system; return true; }
        if (strcmp(field_name, "target_component") == 0) { *out_value = (double)d.target_component; return true; }
        if (strcmp(field_name, "rl_torque_cmd") == 0) { *out_value = (double)d.rl_torque_cmd; return true; }
        if (strcmp(field_name, "el_torque_cmd") == 0) { *out_value = (double)d.el_torque_cmd; return true; }
        if (strcmp(field_name, "az_torque_cmd") == 0) { *out_value = (double)d.az_torque_cmd; return true; }
        return false;
    }
    case 215: {
        mavlink_gopro_heartbeat_t d;
        mavlink_msg_gopro_heartbeat_decode(&raw, &d);
        if (strcmp(field_name, "status") == 0) { *out_value = (double)d.status; return true; }
        if (strcmp(field_name, "capture_mode") == 0) { *out_value = (double)d.capture_mode; return true; }
        if (strcmp(field_name, "flags") == 0) { *out_value = (double)d.flags; return true; }
        return false;
    }
    case 216: {
        mavlink_gopro_get_request_t d;
        mavlink_msg_gopro_get_request_decode(&raw, &d);
        if (strcmp(field_name, "target_system") == 0) { *out_value = (double)d.target_system; return true; }
        if (strcmp(field_name, "target_component") == 0) { *out_value = (double)d.target_component; return true; }
        if (strcmp(field_name, "cmd_id") == 0) { *out_value = (double)d.cmd_id; return true; }
        return false;
    }
    case 217: {
        mavlink_gopro_get_response_t d;
        mavlink_msg_gopro_get_response_decode(&raw, &d);
        if (strcmp(field_name, "cmd_id") == 0) { *out_value = (double)d.cmd_id; return true; }
        if (strcmp(field_name, "status") == 0) { *out_value = (double)d.status; return true; }
        if (strcmp(field_name, "value[0]") == 0) { *out_value = (double)d.value[0]; return true; }
        if (strcmp(field_name, "value[1]") == 0) { *out_value = (double)d.value[1]; return true; }
        if (strcmp(field_name, "value[2]") == 0) { *out_value = (double)d.value[2]; return true; }
        if (strcmp(field_name, "value[3]") == 0) { *out_value = (double)d.value[3]; return true; }
        return false;
    }
    case 219: {
        mavlink_gopro_set_response_t d;
        mavlink_msg_gopro_set_response_decode(&raw, &d);
        if (strcmp(field_name, "cmd_id") == 0) { *out_value = (double)d.cmd_id; return true; }
        if (strcmp(field_name, "status") == 0) { *out_value = (double)d.status; return true; }
        return false;
    }
    case 225: {
        mavlink_efi_status_t d;
        mavlink_msg_efi_status_decode(&raw, &d);
        if (strcmp(field_name, "health") == 0) { *out_value = (double)d.health; return true; }
        if (strcmp(field_name, "ecu_index") == 0) { *out_value = (double)d.ecu_index; return true; }
        if (strcmp(field_name, "rpm") == 0) { *out_value = (double)d.rpm; return true; }
        if (strcmp(field_name, "fuel_consumed") == 0) { *out_value = (double)d.fuel_consumed; return true; }
        if (strcmp(field_name, "fuel_flow") == 0) { *out_value = (double)d.fuel_flow; return true; }
        if (strcmp(field_name, "engine_load") == 0) { *out_value = (double)d.engine_load; return true; }
        if (strcmp(field_name, "throttle_position") == 0) { *out_value = (double)d.throttle_position; return true; }
        if (strcmp(field_name, "spark_dwell_time") == 0) { *out_value = (double)d.spark_dwell_time; return true; }
        if (strcmp(field_name, "barometric_pressure") == 0) { *out_value = (double)d.barometric_pressure; return true; }
        if (strcmp(field_name, "intake_manifold_pressure") == 0) { *out_value = (double)d.intake_manifold_pressure; return true; }
        if (strcmp(field_name, "intake_manifold_temperature") == 0) { *out_value = (double)d.intake_manifold_temperature; return true; }
        if (strcmp(field_name, "cylinder_head_temperature") == 0) { *out_value = (double)d.cylinder_head_temperature; return true; }
        if (strcmp(field_name, "ignition_timing") == 0) { *out_value = (double)d.ignition_timing; return true; }
        if (strcmp(field_name, "injection_time") == 0) { *out_value = (double)d.injection_time; return true; }
        if (strcmp(field_name, "exhaust_gas_temperature") == 0) { *out_value = (double)d.exhaust_gas_temperature; return true; }
        if (strcmp(field_name, "throttle_out") == 0) { *out_value = (double)d.throttle_out; return true; }
        if (strcmp(field_name, "pt_compensation") == 0) { *out_value = (double)d.pt_compensation; return true; }
        if (strcmp(field_name, "ignition_voltage") == 0) { *out_value = (double)d.ignition_voltage; return true; }
        if (strcmp(field_name, "fuel_pressure") == 0) { *out_value = (double)d.fuel_pressure; return true; }
        return false;
    }
    case 226: {
        mavlink_rpm_t d;
        mavlink_msg_rpm_decode(&raw, &d);
        if (strcmp(field_name, "rpm1") == 0) { *out_value = (double)d.rpm1; return true; }
        if (strcmp(field_name, "rpm2") == 0) { *out_value = (double)d.rpm2; return true; }
        return false;
    }
    case 230: {
        mavlink_estimator_status_t d;
        mavlink_msg_estimator_status_decode(&raw, &d);
        if (strcmp(field_name, "time_usec") == 0) { *out_value = (double)d.time_usec; return true; }
        if (strcmp(field_name, "flags") == 0) { *out_value = (double)d.flags; return true; }
        if (strcmp(field_name, "vel_ratio") == 0) { *out_value = (double)d.vel_ratio; return true; }
        if (strcmp(field_name, "pos_horiz_ratio") == 0) { *out_value = (double)d.pos_horiz_ratio; return true; }
        if (strcmp(field_name, "pos_vert_ratio") == 0) { *out_value = (double)d.pos_vert_ratio; return true; }
        if (strcmp(field_name, "mag_ratio") == 0) { *out_value = (double)d.mag_ratio; return true; }
        if (strcmp(field_name, "hagl_ratio") == 0) { *out_value = (double)d.hagl_ratio; return true; }
        if (strcmp(field_name, "tas_ratio") == 0) { *out_value = (double)d.tas_ratio; return true; }
        if (strcmp(field_name, "pos_horiz_accuracy") == 0) { *out_value = (double)d.pos_horiz_accuracy; return true; }
        if (strcmp(field_name, "pos_vert_accuracy") == 0) { *out_value = (double)d.pos_vert_accuracy; return true; }
        return false;
    }
    case 231: {
        mavlink_wind_cov_t d;
        mavlink_msg_wind_cov_decode(&raw, &d);
        if (strcmp(field_name, "time_usec") == 0) { *out_value = (double)d.time_usec; return true; }
        if (strcmp(field_name, "wind_x") == 0) { *out_value = (double)d.wind_x; return true; }
        if (strcmp(field_name, "wind_y") == 0) { *out_value = (double)d.wind_y; return true; }
        if (strcmp(field_name, "wind_z") == 0) { *out_value = (double)d.wind_z; return true; }
        if (strcmp(field_name, "var_horiz") == 0) { *out_value = (double)d.var_horiz; return true; }
        if (strcmp(field_name, "var_vert") == 0) { *out_value = (double)d.var_vert; return true; }
        if (strcmp(field_name, "wind_alt") == 0) { *out_value = (double)d.wind_alt; return true; }
        if (strcmp(field_name, "horiz_accuracy") == 0) { *out_value = (double)d.horiz_accuracy; return true; }
        if (strcmp(field_name, "vert_accuracy") == 0) { *out_value = (double)d.vert_accuracy; return true; }
        return false;
    }
    case 232: {
        mavlink_gps_input_t d;
        mavlink_msg_gps_input_decode(&raw, &d);
        if (strcmp(field_name, "time_usec") == 0) { *out_value = (double)d.time_usec; return true; }
        if (strcmp(field_name, "gps_id") == 0) { *out_value = (double)d.gps_id; return true; }
        if (strcmp(field_name, "ignore_flags") == 0) { *out_value = (double)d.ignore_flags; return true; }
        if (strcmp(field_name, "time_week_ms") == 0) { *out_value = (double)d.time_week_ms; return true; }
        if (strcmp(field_name, "time_week") == 0) { *out_value = (double)d.time_week; return true; }
        if (strcmp(field_name, "fix_type") == 0) { *out_value = (double)d.fix_type; return true; }
        if (strcmp(field_name, "lat") == 0) { *out_value = (double)d.lat; return true; }
        if (strcmp(field_name, "lon") == 0) { *out_value = (double)d.lon; return true; }
        if (strcmp(field_name, "alt") == 0) { *out_value = (double)d.alt; return true; }
        if (strcmp(field_name, "hdop") == 0) { *out_value = (double)d.hdop; return true; }
        if (strcmp(field_name, "vdop") == 0) { *out_value = (double)d.vdop; return true; }
        if (strcmp(field_name, "vn") == 0) { *out_value = (double)d.vn; return true; }
        if (strcmp(field_name, "ve") == 0) { *out_value = (double)d.ve; return true; }
        if (strcmp(field_name, "vd") == 0) { *out_value = (double)d.vd; return true; }
        if (strcmp(field_name, "speed_accuracy") == 0) { *out_value = (double)d.speed_accuracy; return true; }
        if (strcmp(field_name, "horiz_accuracy") == 0) { *out_value = (double)d.horiz_accuracy; return true; }
        if (strcmp(field_name, "vert_accuracy") == 0) { *out_value = (double)d.vert_accuracy; return true; }
        if (strcmp(field_name, "satellites_visible") == 0) { *out_value = (double)d.satellites_visible; return true; }
        if (strcmp(field_name, "yaw") == 0) { *out_value = (double)d.yaw; return true; }
        return false;
    }
    case 233: {
        mavlink_gps_rtcm_data_t d;
        mavlink_msg_gps_rtcm_data_decode(&raw, &d);
        if (strcmp(field_name, "flags") == 0) { *out_value = (double)d.flags; return true; }
        if (strcmp(field_name, "len") == 0) { *out_value = (double)d.len; return true; }
        return false;
    }
    case 234: {
        mavlink_high_latency_t d;
        mavlink_msg_high_latency_decode(&raw, &d);
        if (strcmp(field_name, "base_mode") == 0) { *out_value = (double)d.base_mode; return true; }
        if (strcmp(field_name, "custom_mode") == 0) { *out_value = (double)d.custom_mode; return true; }
        if (strcmp(field_name, "landed_state") == 0) { *out_value = (double)d.landed_state; return true; }
        if (strcmp(field_name, "roll") == 0) { *out_value = (double)d.roll; return true; }
        if (strcmp(field_name, "pitch") == 0) { *out_value = (double)d.pitch; return true; }
        if (strcmp(field_name, "heading") == 0) { *out_value = (double)d.heading; return true; }
        if (strcmp(field_name, "throttle") == 0) { *out_value = (double)d.throttle; return true; }
        if (strcmp(field_name, "heading_sp") == 0) { *out_value = (double)d.heading_sp; return true; }
        if (strcmp(field_name, "latitude") == 0) { *out_value = (double)d.latitude; return true; }
        if (strcmp(field_name, "longitude") == 0) { *out_value = (double)d.longitude; return true; }
        if (strcmp(field_name, "altitude_amsl") == 0) { *out_value = (double)d.altitude_amsl; return true; }
        if (strcmp(field_name, "altitude_sp") == 0) { *out_value = (double)d.altitude_sp; return true; }
        if (strcmp(field_name, "airspeed") == 0) { *out_value = (double)d.airspeed; return true; }
        if (strcmp(field_name, "airspeed_sp") == 0) { *out_value = (double)d.airspeed_sp; return true; }
        if (strcmp(field_name, "groundspeed") == 0) { *out_value = (double)d.groundspeed; return true; }
        if (strcmp(field_name, "climb_rate") == 0) { *out_value = (double)d.climb_rate; return true; }
        if (strcmp(field_name, "gps_nsat") == 0) { *out_value = (double)d.gps_nsat; return true; }
        if (strcmp(field_name, "gps_fix_type") == 0) { *out_value = (double)d.gps_fix_type; return true; }
        if (strcmp(field_name, "battery_remaining") == 0) { *out_value = (double)d.battery_remaining; return true; }
        if (strcmp(field_name, "temperature") == 0) { *out_value = (double)d.temperature; return true; }
        if (strcmp(field_name, "temperature_air") == 0) { *out_value = (double)d.temperature_air; return true; }
        if (strcmp(field_name, "failsafe") == 0) { *out_value = (double)d.failsafe; return true; }
        if (strcmp(field_name, "wp_num") == 0) { *out_value = (double)d.wp_num; return true; }
        if (strcmp(field_name, "wp_distance") == 0) { *out_value = (double)d.wp_distance; return true; }
        return false;
    }
    case 235: {
        mavlink_high_latency2_t d;
        mavlink_msg_high_latency2_decode(&raw, &d);
        if (strcmp(field_name, "timestamp") == 0) { *out_value = (double)d.timestamp; return true; }
        if (strcmp(field_name, "type") == 0) { *out_value = (double)d.type; return true; }
        if (strcmp(field_name, "autopilot") == 0) { *out_value = (double)d.autopilot; return true; }
        if (strcmp(field_name, "custom_mode") == 0) { *out_value = (double)d.custom_mode; return true; }
        if (strcmp(field_name, "latitude") == 0) { *out_value = (double)d.latitude; return true; }
        if (strcmp(field_name, "longitude") == 0) { *out_value = (double)d.longitude; return true; }
        if (strcmp(field_name, "altitude") == 0) { *out_value = (double)d.altitude; return true; }
        if (strcmp(field_name, "target_altitude") == 0) { *out_value = (double)d.target_altitude; return true; }
        if (strcmp(field_name, "heading") == 0) { *out_value = (double)d.heading; return true; }
        if (strcmp(field_name, "target_heading") == 0) { *out_value = (double)d.target_heading; return true; }
        if (strcmp(field_name, "target_distance") == 0) { *out_value = (double)d.target_distance; return true; }
        if (strcmp(field_name, "throttle") == 0) { *out_value = (double)d.throttle; return true; }
        if (strcmp(field_name, "airspeed") == 0) { *out_value = (double)d.airspeed; return true; }
        if (strcmp(field_name, "airspeed_sp") == 0) { *out_value = (double)d.airspeed_sp; return true; }
        if (strcmp(field_name, "groundspeed") == 0) { *out_value = (double)d.groundspeed; return true; }
        if (strcmp(field_name, "windspeed") == 0) { *out_value = (double)d.windspeed; return true; }
        if (strcmp(field_name, "wind_heading") == 0) { *out_value = (double)d.wind_heading; return true; }
        if (strcmp(field_name, "eph") == 0) { *out_value = (double)d.eph; return true; }
        if (strcmp(field_name, "epv") == 0) { *out_value = (double)d.epv; return true; }
        if (strcmp(field_name, "temperature_air") == 0) { *out_value = (double)d.temperature_air; return true; }
        if (strcmp(field_name, "climb_rate") == 0) { *out_value = (double)d.climb_rate; return true; }
        if (strcmp(field_name, "battery") == 0) { *out_value = (double)d.battery; return true; }
        if (strcmp(field_name, "wp_num") == 0) { *out_value = (double)d.wp_num; return true; }
        if (strcmp(field_name, "failure_flags") == 0) { *out_value = (double)d.failure_flags; return true; }
        if (strcmp(field_name, "custom0") == 0) { *out_value = (double)d.custom0; return true; }
        if (strcmp(field_name, "custom1") == 0) { *out_value = (double)d.custom1; return true; }
        if (strcmp(field_name, "custom2") == 0) { *out_value = (double)d.custom2; return true; }
        return false;
    }
    case 241: {
        mavlink_vibration_t d;
        mavlink_msg_vibration_decode(&raw, &d);
        if (strcmp(field_name, "time_usec") == 0) { *out_value = (double)d.time_usec; return true; }
        if (strcmp(field_name, "vibration_x") == 0) { *out_value = (double)d.vibration_x; return true; }
        if (strcmp(field_name, "vibration_y") == 0) { *out_value = (double)d.vibration_y; return true; }
        if (strcmp(field_name, "vibration_z") == 0) { *out_value = (double)d.vibration_z; return true; }
        if (strcmp(field_name, "clipping_0") == 0) { *out_value = (double)d.clipping_0; return true; }
        if (strcmp(field_name, "clipping_1") == 0) { *out_value = (double)d.clipping_1; return true; }
        if (strcmp(field_name, "clipping_2") == 0) { *out_value = (double)d.clipping_2; return true; }
        return false;
    }
    case 242: {
        mavlink_home_position_t d;
        mavlink_msg_home_position_decode(&raw, &d);
        if (strcmp(field_name, "latitude") == 0) { *out_value = (double)d.latitude; return true; }
        if (strcmp(field_name, "longitude") == 0) { *out_value = (double)d.longitude; return true; }
        if (strcmp(field_name, "altitude") == 0) { *out_value = (double)d.altitude; return true; }
        if (strcmp(field_name, "x") == 0) { *out_value = (double)d.x; return true; }
        if (strcmp(field_name, "y") == 0) { *out_value = (double)d.y; return true; }
        if (strcmp(field_name, "z") == 0) { *out_value = (double)d.z; return true; }
        if (strcmp(field_name, "q[0]") == 0) { *out_value = (double)d.q[0]; return true; }
        if (strcmp(field_name, "q[1]") == 0) { *out_value = (double)d.q[1]; return true; }
        if (strcmp(field_name, "q[2]") == 0) { *out_value = (double)d.q[2]; return true; }
        if (strcmp(field_name, "q[3]") == 0) { *out_value = (double)d.q[3]; return true; }
        if (strcmp(field_name, "approach_x") == 0) { *out_value = (double)d.approach_x; return true; }
        if (strcmp(field_name, "approach_y") == 0) { *out_value = (double)d.approach_y; return true; }
        if (strcmp(field_name, "approach_z") == 0) { *out_value = (double)d.approach_z; return true; }
        if (strcmp(field_name, "time_usec") == 0) { *out_value = (double)d.time_usec; return true; }
        return false;
    }
    case 245: {
        mavlink_extended_sys_state_t d;
        mavlink_msg_extended_sys_state_decode(&raw, &d);
        if (strcmp(field_name, "vtol_state") == 0) { *out_value = (double)d.vtol_state; return true; }
        if (strcmp(field_name, "landed_state") == 0) { *out_value = (double)d.landed_state; return true; }
        return false;
    }
    case 246: {
        mavlink_adsb_vehicle_t d;
        mavlink_msg_adsb_vehicle_decode(&raw, &d);
        if (strcmp(field_name, "ICAO_address") == 0) { *out_value = (double)d.ICAO_address; return true; }
        if (strcmp(field_name, "lat") == 0) { *out_value = (double)d.lat; return true; }
        if (strcmp(field_name, "lon") == 0) { *out_value = (double)d.lon; return true; }
        if (strcmp(field_name, "altitude_type") == 0) { *out_value = (double)d.altitude_type; return true; }
        if (strcmp(field_name, "altitude") == 0) { *out_value = (double)d.altitude; return true; }
        if (strcmp(field_name, "heading") == 0) { *out_value = (double)d.heading; return true; }
        if (strcmp(field_name, "hor_velocity") == 0) { *out_value = (double)d.hor_velocity; return true; }
        if (strcmp(field_name, "ver_velocity") == 0) { *out_value = (double)d.ver_velocity; return true; }
        if (strcmp(field_name, "emitter_type") == 0) { *out_value = (double)d.emitter_type; return true; }
        if (strcmp(field_name, "tslc") == 0) { *out_value = (double)d.tslc; return true; }
        if (strcmp(field_name, "flags") == 0) { *out_value = (double)d.flags; return true; }
        if (strcmp(field_name, "squawk") == 0) { *out_value = (double)d.squawk; return true; }
        return false;
    }
    case 247: {
        mavlink_collision_t d;
        mavlink_msg_collision_decode(&raw, &d);
        if (strcmp(field_name, "src") == 0) { *out_value = (double)d.src; return true; }
        if (strcmp(field_name, "id") == 0) { *out_value = (double)d.id; return true; }
        if (strcmp(field_name, "action") == 0) { *out_value = (double)d.action; return true; }
        if (strcmp(field_name, "threat_level") == 0) { *out_value = (double)d.threat_level; return true; }
        if (strcmp(field_name, "time_to_minimum_delta") == 0) { *out_value = (double)d.time_to_minimum_delta; return true; }
        if (strcmp(field_name, "altitude_minimum_delta") == 0) { *out_value = (double)d.altitude_minimum_delta; return true; }
        if (strcmp(field_name, "horizontal_minimum_delta") == 0) { *out_value = (double)d.horizontal_minimum_delta; return true; }
        return false;
    }
    case 249: {
        mavlink_memory_vect_t d;
        mavlink_msg_memory_vect_decode(&raw, &d);
        if (strcmp(field_name, "address") == 0) { *out_value = (double)d.address; return true; }
        if (strcmp(field_name, "ver") == 0) { *out_value = (double)d.ver; return true; }
        if (strcmp(field_name, "type") == 0) { *out_value = (double)d.type; return true; }
        return false;
    }
    case 250: {
        mavlink_debug_vect_t d;
        mavlink_msg_debug_vect_decode(&raw, &d);
        if (strcmp(field_name, "time_usec") == 0) { *out_value = (double)d.time_usec; return true; }
        if (strcmp(field_name, "x") == 0) { *out_value = (double)d.x; return true; }
        if (strcmp(field_name, "y") == 0) { *out_value = (double)d.y; return true; }
        if (strcmp(field_name, "z") == 0) { *out_value = (double)d.z; return true; }
        return false;
    }
    case 251: {
        mavlink_named_value_float_t d;
        mavlink_msg_named_value_float_decode(&raw, &d);
        if (strcmp(field_name, "time_boot_ms") == 0) { *out_value = (double)d.time_boot_ms; return true; }
        if (strcmp(field_name, "value") == 0) { *out_value = (double)d.value; return true; }
        return false;
    }
    case 252: {
        mavlink_named_value_int_t d;
        mavlink_msg_named_value_int_decode(&raw, &d);
        if (strcmp(field_name, "time_boot_ms") == 0) { *out_value = (double)d.time_boot_ms; return true; }
        if (strcmp(field_name, "value") == 0) { *out_value = (double)d.value; return true; }
        return false;
    }
    case 253: {
        mavlink_statustext_t d;
        mavlink_msg_statustext_decode(&raw, &d);
        if (strcmp(field_name, "severity") == 0) { *out_value = (double)d.severity; return true; }
        if (strcmp(field_name, "id") == 0) { *out_value = (double)d.id; return true; }
        if (strcmp(field_name, "chunk_seq") == 0) { *out_value = (double)d.chunk_seq; return true; }
        return false;
    }
    case 254: {
        mavlink_debug_t d;
        mavlink_msg_debug_decode(&raw, &d);
        if (strcmp(field_name, "time_boot_ms") == 0) { *out_value = (double)d.time_boot_ms; return true; }
        if (strcmp(field_name, "ind") == 0) { *out_value = (double)d.ind; return true; }
        if (strcmp(field_name, "value") == 0) { *out_value = (double)d.value; return true; }
        return false;
    }
    case 257: {
        mavlink_button_change_t d;
        mavlink_msg_button_change_decode(&raw, &d);
        if (strcmp(field_name, "time_boot_ms") == 0) { *out_value = (double)d.time_boot_ms; return true; }
        if (strcmp(field_name, "last_change_ms") == 0) { *out_value = (double)d.last_change_ms; return true; }
        if (strcmp(field_name, "state") == 0) { *out_value = (double)d.state; return true; }
        return false;
    }
    case 259: {
        mavlink_camera_information_t d;
        mavlink_msg_camera_information_decode(&raw, &d);
        if (strcmp(field_name, "time_boot_ms") == 0) { *out_value = (double)d.time_boot_ms; return true; }
        if (strcmp(field_name, "firmware_version") == 0) { *out_value = (double)d.firmware_version; return true; }
        if (strcmp(field_name, "focal_length") == 0) { *out_value = (double)d.focal_length; return true; }
        if (strcmp(field_name, "sensor_size_h") == 0) { *out_value = (double)d.sensor_size_h; return true; }
        if (strcmp(field_name, "sensor_size_v") == 0) { *out_value = (double)d.sensor_size_v; return true; }
        if (strcmp(field_name, "resolution_h") == 0) { *out_value = (double)d.resolution_h; return true; }
        if (strcmp(field_name, "resolution_v") == 0) { *out_value = (double)d.resolution_v; return true; }
        if (strcmp(field_name, "lens_id") == 0) { *out_value = (double)d.lens_id; return true; }
        if (strcmp(field_name, "flags") == 0) { *out_value = (double)d.flags; return true; }
        if (strcmp(field_name, "cam_definition_version") == 0) { *out_value = (double)d.cam_definition_version; return true; }
        if (strcmp(field_name, "gimbal_device_id") == 0) { *out_value = (double)d.gimbal_device_id; return true; }
        if (strcmp(field_name, "camera_device_id") == 0) { *out_value = (double)d.camera_device_id; return true; }
        return false;
    }
    case 260: {
        mavlink_camera_settings_t d;
        mavlink_msg_camera_settings_decode(&raw, &d);
        if (strcmp(field_name, "time_boot_ms") == 0) { *out_value = (double)d.time_boot_ms; return true; }
        if (strcmp(field_name, "mode_id") == 0) { *out_value = (double)d.mode_id; return true; }
        if (strcmp(field_name, "zoomLevel") == 0) { *out_value = (double)d.zoomLevel; return true; }
        if (strcmp(field_name, "focusLevel") == 0) { *out_value = (double)d.focusLevel; return true; }
        if (strcmp(field_name, "camera_device_id") == 0) { *out_value = (double)d.camera_device_id; return true; }
        return false;
    }
    case 261: {
        mavlink_storage_information_t d;
        mavlink_msg_storage_information_decode(&raw, &d);
        if (strcmp(field_name, "time_boot_ms") == 0) { *out_value = (double)d.time_boot_ms; return true; }
        if (strcmp(field_name, "storage_id") == 0) { *out_value = (double)d.storage_id; return true; }
        if (strcmp(field_name, "storage_count") == 0) { *out_value = (double)d.storage_count; return true; }
        if (strcmp(field_name, "status") == 0) { *out_value = (double)d.status; return true; }
        if (strcmp(field_name, "total_capacity") == 0) { *out_value = (double)d.total_capacity; return true; }
        if (strcmp(field_name, "used_capacity") == 0) { *out_value = (double)d.used_capacity; return true; }
        if (strcmp(field_name, "available_capacity") == 0) { *out_value = (double)d.available_capacity; return true; }
        if (strcmp(field_name, "read_speed") == 0) { *out_value = (double)d.read_speed; return true; }
        if (strcmp(field_name, "write_speed") == 0) { *out_value = (double)d.write_speed; return true; }
        if (strcmp(field_name, "type") == 0) { *out_value = (double)d.type; return true; }
        if (strcmp(field_name, "storage_usage") == 0) { *out_value = (double)d.storage_usage; return true; }
        return false;
    }
    case 262: {
        mavlink_camera_capture_status_t d;
        mavlink_msg_camera_capture_status_decode(&raw, &d);
        if (strcmp(field_name, "time_boot_ms") == 0) { *out_value = (double)d.time_boot_ms; return true; }
        if (strcmp(field_name, "image_status") == 0) { *out_value = (double)d.image_status; return true; }
        if (strcmp(field_name, "video_status") == 0) { *out_value = (double)d.video_status; return true; }
        if (strcmp(field_name, "image_interval") == 0) { *out_value = (double)d.image_interval; return true; }
        if (strcmp(field_name, "recording_time_ms") == 0) { *out_value = (double)d.recording_time_ms; return true; }
        if (strcmp(field_name, "available_capacity") == 0) { *out_value = (double)d.available_capacity; return true; }
        if (strcmp(field_name, "image_count") == 0) { *out_value = (double)d.image_count; return true; }
        if (strcmp(field_name, "camera_device_id") == 0) { *out_value = (double)d.camera_device_id; return true; }
        return false;
    }
    case 263: {
        mavlink_camera_image_captured_t d;
        mavlink_msg_camera_image_captured_decode(&raw, &d);
        if (strcmp(field_name, "time_boot_ms") == 0) { *out_value = (double)d.time_boot_ms; return true; }
        if (strcmp(field_name, "time_utc") == 0) { *out_value = (double)d.time_utc; return true; }
        if (strcmp(field_name, "camera_id") == 0) { *out_value = (double)d.camera_id; return true; }
        if (strcmp(field_name, "lat") == 0) { *out_value = (double)d.lat; return true; }
        if (strcmp(field_name, "lon") == 0) { *out_value = (double)d.lon; return true; }
        if (strcmp(field_name, "alt") == 0) { *out_value = (double)d.alt; return true; }
        if (strcmp(field_name, "relative_alt") == 0) { *out_value = (double)d.relative_alt; return true; }
        if (strcmp(field_name, "q[0]") == 0) { *out_value = (double)d.q[0]; return true; }
        if (strcmp(field_name, "q[1]") == 0) { *out_value = (double)d.q[1]; return true; }
        if (strcmp(field_name, "q[2]") == 0) { *out_value = (double)d.q[2]; return true; }
        if (strcmp(field_name, "q[3]") == 0) { *out_value = (double)d.q[3]; return true; }
        if (strcmp(field_name, "image_index") == 0) { *out_value = (double)d.image_index; return true; }
        if (strcmp(field_name, "capture_result") == 0) { *out_value = (double)d.capture_result; return true; }
        return false;
    }
    case 264: {
        mavlink_flight_information_t d;
        mavlink_msg_flight_information_decode(&raw, &d);
        if (strcmp(field_name, "time_boot_ms") == 0) { *out_value = (double)d.time_boot_ms; return true; }
        if (strcmp(field_name, "arming_time_utc") == 0) { *out_value = (double)d.arming_time_utc; return true; }
        if (strcmp(field_name, "takeoff_time_utc") == 0) { *out_value = (double)d.takeoff_time_utc; return true; }
        if (strcmp(field_name, "flight_uuid") == 0) { *out_value = (double)d.flight_uuid; return true; }
        if (strcmp(field_name, "landing_time") == 0) { *out_value = (double)d.landing_time; return true; }
        return false;
    }
    case 265: {
        mavlink_mount_orientation_t d;
        mavlink_msg_mount_orientation_decode(&raw, &d);
        if (strcmp(field_name, "time_boot_ms") == 0) { *out_value = (double)d.time_boot_ms; return true; }
        if (strcmp(field_name, "roll") == 0) { *out_value = (double)d.roll; return true; }
        if (strcmp(field_name, "pitch") == 0) { *out_value = (double)d.pitch; return true; }
        if (strcmp(field_name, "yaw") == 0) { *out_value = (double)d.yaw; return true; }
        if (strcmp(field_name, "yaw_absolute") == 0) { *out_value = (double)d.yaw_absolute; return true; }
        return false;
    }
    case 266: {
        mavlink_logging_data_t d;
        mavlink_msg_logging_data_decode(&raw, &d);
        if (strcmp(field_name, "target_system") == 0) { *out_value = (double)d.target_system; return true; }
        if (strcmp(field_name, "target_component") == 0) { *out_value = (double)d.target_component; return true; }
        if (strcmp(field_name, "sequence") == 0) { *out_value = (double)d.sequence; return true; }
        if (strcmp(field_name, "length") == 0) { *out_value = (double)d.length; return true; }
        if (strcmp(field_name, "first_message_offset") == 0) { *out_value = (double)d.first_message_offset; return true; }
        return false;
    }
    case 267: {
        mavlink_logging_data_acked_t d;
        mavlink_msg_logging_data_acked_decode(&raw, &d);
        if (strcmp(field_name, "target_system") == 0) { *out_value = (double)d.target_system; return true; }
        if (strcmp(field_name, "target_component") == 0) { *out_value = (double)d.target_component; return true; }
        if (strcmp(field_name, "sequence") == 0) { *out_value = (double)d.sequence; return true; }
        if (strcmp(field_name, "length") == 0) { *out_value = (double)d.length; return true; }
        if (strcmp(field_name, "first_message_offset") == 0) { *out_value = (double)d.first_message_offset; return true; }
        return false;
    }
    case 268: {
        mavlink_logging_ack_t d;
        mavlink_msg_logging_ack_decode(&raw, &d);
        if (strcmp(field_name, "target_system") == 0) { *out_value = (double)d.target_system; return true; }
        if (strcmp(field_name, "target_component") == 0) { *out_value = (double)d.target_component; return true; }
        if (strcmp(field_name, "sequence") == 0) { *out_value = (double)d.sequence; return true; }
        return false;
    }
    case 269: {
        mavlink_video_stream_information_t d;
        mavlink_msg_video_stream_information_decode(&raw, &d);
        if (strcmp(field_name, "stream_id") == 0) { *out_value = (double)d.stream_id; return true; }
        if (strcmp(field_name, "count") == 0) { *out_value = (double)d.count; return true; }
        if (strcmp(field_name, "type") == 0) { *out_value = (double)d.type; return true; }
        if (strcmp(field_name, "flags") == 0) { *out_value = (double)d.flags; return true; }
        if (strcmp(field_name, "framerate") == 0) { *out_value = (double)d.framerate; return true; }
        if (strcmp(field_name, "resolution_h") == 0) { *out_value = (double)d.resolution_h; return true; }
        if (strcmp(field_name, "resolution_v") == 0) { *out_value = (double)d.resolution_v; return true; }
        if (strcmp(field_name, "bitrate") == 0) { *out_value = (double)d.bitrate; return true; }
        if (strcmp(field_name, "rotation") == 0) { *out_value = (double)d.rotation; return true; }
        if (strcmp(field_name, "hfov") == 0) { *out_value = (double)d.hfov; return true; }
        if (strcmp(field_name, "encoding") == 0) { *out_value = (double)d.encoding; return true; }
        if (strcmp(field_name, "camera_device_id") == 0) { *out_value = (double)d.camera_device_id; return true; }
        return false;
    }
    case 270: {
        mavlink_video_stream_status_t d;
        mavlink_msg_video_stream_status_decode(&raw, &d);
        if (strcmp(field_name, "stream_id") == 0) { *out_value = (double)d.stream_id; return true; }
        if (strcmp(field_name, "flags") == 0) { *out_value = (double)d.flags; return true; }
        if (strcmp(field_name, "framerate") == 0) { *out_value = (double)d.framerate; return true; }
        if (strcmp(field_name, "resolution_h") == 0) { *out_value = (double)d.resolution_h; return true; }
        if (strcmp(field_name, "resolution_v") == 0) { *out_value = (double)d.resolution_v; return true; }
        if (strcmp(field_name, "bitrate") == 0) { *out_value = (double)d.bitrate; return true; }
        if (strcmp(field_name, "rotation") == 0) { *out_value = (double)d.rotation; return true; }
        if (strcmp(field_name, "hfov") == 0) { *out_value = (double)d.hfov; return true; }
        if (strcmp(field_name, "camera_device_id") == 0) { *out_value = (double)d.camera_device_id; return true; }
        return false;
    }
    case 271: {
        mavlink_camera_fov_status_t d;
        mavlink_msg_camera_fov_status_decode(&raw, &d);
        if (strcmp(field_name, "time_boot_ms") == 0) { *out_value = (double)d.time_boot_ms; return true; }
        if (strcmp(field_name, "lat_camera") == 0) { *out_value = (double)d.lat_camera; return true; }
        if (strcmp(field_name, "lon_camera") == 0) { *out_value = (double)d.lon_camera; return true; }
        if (strcmp(field_name, "alt_camera") == 0) { *out_value = (double)d.alt_camera; return true; }
        if (strcmp(field_name, "lat_image") == 0) { *out_value = (double)d.lat_image; return true; }
        if (strcmp(field_name, "lon_image") == 0) { *out_value = (double)d.lon_image; return true; }
        if (strcmp(field_name, "alt_image") == 0) { *out_value = (double)d.alt_image; return true; }
        if (strcmp(field_name, "q[0]") == 0) { *out_value = (double)d.q[0]; return true; }
        if (strcmp(field_name, "q[1]") == 0) { *out_value = (double)d.q[1]; return true; }
        if (strcmp(field_name, "q[2]") == 0) { *out_value = (double)d.q[2]; return true; }
        if (strcmp(field_name, "q[3]") == 0) { *out_value = (double)d.q[3]; return true; }
        if (strcmp(field_name, "hfov") == 0) { *out_value = (double)d.hfov; return true; }
        if (strcmp(field_name, "vfov") == 0) { *out_value = (double)d.vfov; return true; }
        if (strcmp(field_name, "camera_device_id") == 0) { *out_value = (double)d.camera_device_id; return true; }
        return false;
    }
    case 275: {
        mavlink_camera_tracking_image_status_t d;
        mavlink_msg_camera_tracking_image_status_decode(&raw, &d);
        if (strcmp(field_name, "tracking_status") == 0) { *out_value = (double)d.tracking_status; return true; }
        if (strcmp(field_name, "tracking_mode") == 0) { *out_value = (double)d.tracking_mode; return true; }
        if (strcmp(field_name, "target_data") == 0) { *out_value = (double)d.target_data; return true; }
        if (strcmp(field_name, "point_x") == 0) { *out_value = (double)d.point_x; return true; }
        if (strcmp(field_name, "point_y") == 0) { *out_value = (double)d.point_y; return true; }
        if (strcmp(field_name, "radius") == 0) { *out_value = (double)d.radius; return true; }
        if (strcmp(field_name, "rec_top_x") == 0) { *out_value = (double)d.rec_top_x; return true; }
        if (strcmp(field_name, "rec_top_y") == 0) { *out_value = (double)d.rec_top_y; return true; }
        if (strcmp(field_name, "rec_bottom_x") == 0) { *out_value = (double)d.rec_bottom_x; return true; }
        if (strcmp(field_name, "rec_bottom_y") == 0) { *out_value = (double)d.rec_bottom_y; return true; }
        if (strcmp(field_name, "camera_device_id") == 0) { *out_value = (double)d.camera_device_id; return true; }
        return false;
    }
    case 276: {
        mavlink_camera_tracking_geo_status_t d;
        mavlink_msg_camera_tracking_geo_status_decode(&raw, &d);
        if (strcmp(field_name, "tracking_status") == 0) { *out_value = (double)d.tracking_status; return true; }
        if (strcmp(field_name, "lat") == 0) { *out_value = (double)d.lat; return true; }
        if (strcmp(field_name, "lon") == 0) { *out_value = (double)d.lon; return true; }
        if (strcmp(field_name, "alt") == 0) { *out_value = (double)d.alt; return true; }
        if (strcmp(field_name, "h_acc") == 0) { *out_value = (double)d.h_acc; return true; }
        if (strcmp(field_name, "v_acc") == 0) { *out_value = (double)d.v_acc; return true; }
        if (strcmp(field_name, "vel_n") == 0) { *out_value = (double)d.vel_n; return true; }
        if (strcmp(field_name, "vel_e") == 0) { *out_value = (double)d.vel_e; return true; }
        if (strcmp(field_name, "vel_d") == 0) { *out_value = (double)d.vel_d; return true; }
        if (strcmp(field_name, "vel_acc") == 0) { *out_value = (double)d.vel_acc; return true; }
        if (strcmp(field_name, "dist") == 0) { *out_value = (double)d.dist; return true; }
        if (strcmp(field_name, "hdg") == 0) { *out_value = (double)d.hdg; return true; }
        if (strcmp(field_name, "hdg_acc") == 0) { *out_value = (double)d.hdg_acc; return true; }
        if (strcmp(field_name, "camera_device_id") == 0) { *out_value = (double)d.camera_device_id; return true; }
        return false;
    }
    case 277: {
        mavlink_camera_thermal_range_t d;
        mavlink_msg_camera_thermal_range_decode(&raw, &d);
        if (strcmp(field_name, "time_boot_ms") == 0) { *out_value = (double)d.time_boot_ms; return true; }
        if (strcmp(field_name, "stream_id") == 0) { *out_value = (double)d.stream_id; return true; }
        if (strcmp(field_name, "camera_device_id") == 0) { *out_value = (double)d.camera_device_id; return true; }
        if (strcmp(field_name, "max") == 0) { *out_value = (double)d.max; return true; }
        if (strcmp(field_name, "max_point_x") == 0) { *out_value = (double)d.max_point_x; return true; }
        if (strcmp(field_name, "max_point_y") == 0) { *out_value = (double)d.max_point_y; return true; }
        if (strcmp(field_name, "min") == 0) { *out_value = (double)d.min; return true; }
        if (strcmp(field_name, "min_point_x") == 0) { *out_value = (double)d.min_point_x; return true; }
        if (strcmp(field_name, "min_point_y") == 0) { *out_value = (double)d.min_point_y; return true; }
        return false;
    }
    case 280: {
        mavlink_gimbal_manager_information_t d;
        mavlink_msg_gimbal_manager_information_decode(&raw, &d);
        if (strcmp(field_name, "time_boot_ms") == 0) { *out_value = (double)d.time_boot_ms; return true; }
        if (strcmp(field_name, "cap_flags") == 0) { *out_value = (double)d.cap_flags; return true; }
        if (strcmp(field_name, "gimbal_device_id") == 0) { *out_value = (double)d.gimbal_device_id; return true; }
        if (strcmp(field_name, "roll_min") == 0) { *out_value = (double)d.roll_min; return true; }
        if (strcmp(field_name, "roll_max") == 0) { *out_value = (double)d.roll_max; return true; }
        if (strcmp(field_name, "pitch_min") == 0) { *out_value = (double)d.pitch_min; return true; }
        if (strcmp(field_name, "pitch_max") == 0) { *out_value = (double)d.pitch_max; return true; }
        if (strcmp(field_name, "yaw_min") == 0) { *out_value = (double)d.yaw_min; return true; }
        if (strcmp(field_name, "yaw_max") == 0) { *out_value = (double)d.yaw_max; return true; }
        return false;
    }
    case 281: {
        mavlink_gimbal_manager_status_t d;
        mavlink_msg_gimbal_manager_status_decode(&raw, &d);
        if (strcmp(field_name, "time_boot_ms") == 0) { *out_value = (double)d.time_boot_ms; return true; }
        if (strcmp(field_name, "flags") == 0) { *out_value = (double)d.flags; return true; }
        if (strcmp(field_name, "gimbal_device_id") == 0) { *out_value = (double)d.gimbal_device_id; return true; }
        if (strcmp(field_name, "primary_control_sysid") == 0) { *out_value = (double)d.primary_control_sysid; return true; }
        if (strcmp(field_name, "primary_control_compid") == 0) { *out_value = (double)d.primary_control_compid; return true; }
        if (strcmp(field_name, "secondary_control_sysid") == 0) { *out_value = (double)d.secondary_control_sysid; return true; }
        if (strcmp(field_name, "secondary_control_compid") == 0) { *out_value = (double)d.secondary_control_compid; return true; }
        return false;
    }
    case 283: {
        mavlink_gimbal_device_information_t d;
        mavlink_msg_gimbal_device_information_decode(&raw, &d);
        if (strcmp(field_name, "time_boot_ms") == 0) { *out_value = (double)d.time_boot_ms; return true; }
        if (strcmp(field_name, "firmware_version") == 0) { *out_value = (double)d.firmware_version; return true; }
        if (strcmp(field_name, "hardware_version") == 0) { *out_value = (double)d.hardware_version; return true; }
        if (strcmp(field_name, "uid") == 0) { *out_value = (double)d.uid; return true; }
        if (strcmp(field_name, "cap_flags") == 0) { *out_value = (double)d.cap_flags; return true; }
        if (strcmp(field_name, "custom_cap_flags") == 0) { *out_value = (double)d.custom_cap_flags; return true; }
        if (strcmp(field_name, "roll_min") == 0) { *out_value = (double)d.roll_min; return true; }
        if (strcmp(field_name, "roll_max") == 0) { *out_value = (double)d.roll_max; return true; }
        if (strcmp(field_name, "pitch_min") == 0) { *out_value = (double)d.pitch_min; return true; }
        if (strcmp(field_name, "pitch_max") == 0) { *out_value = (double)d.pitch_max; return true; }
        if (strcmp(field_name, "yaw_min") == 0) { *out_value = (double)d.yaw_min; return true; }
        if (strcmp(field_name, "yaw_max") == 0) { *out_value = (double)d.yaw_max; return true; }
        if (strcmp(field_name, "gimbal_device_id") == 0) { *out_value = (double)d.gimbal_device_id; return true; }
        if (strcmp(field_name, "cap_flags2") == 0) { *out_value = (double)d.cap_flags2; return true; }
        return false;
    }
    case 285: {
        mavlink_gimbal_device_attitude_status_t d;
        mavlink_msg_gimbal_device_attitude_status_decode(&raw, &d);
        if (strcmp(field_name, "target_system") == 0) { *out_value = (double)d.target_system; return true; }
        if (strcmp(field_name, "target_component") == 0) { *out_value = (double)d.target_component; return true; }
        if (strcmp(field_name, "time_boot_ms") == 0) { *out_value = (double)d.time_boot_ms; return true; }
        if (strcmp(field_name, "flags") == 0) { *out_value = (double)d.flags; return true; }
        if (strcmp(field_name, "q[0]") == 0) { *out_value = (double)d.q[0]; return true; }
        if (strcmp(field_name, "q[1]") == 0) { *out_value = (double)d.q[1]; return true; }
        if (strcmp(field_name, "q[2]") == 0) { *out_value = (double)d.q[2]; return true; }
        if (strcmp(field_name, "q[3]") == 0) { *out_value = (double)d.q[3]; return true; }
        if (strcmp(field_name, "angular_velocity_x") == 0) { *out_value = (double)d.angular_velocity_x; return true; }
        if (strcmp(field_name, "angular_velocity_y") == 0) { *out_value = (double)d.angular_velocity_y; return true; }
        if (strcmp(field_name, "angular_velocity_z") == 0) { *out_value = (double)d.angular_velocity_z; return true; }
        if (strcmp(field_name, "failure_flags") == 0) { *out_value = (double)d.failure_flags; return true; }
        if (strcmp(field_name, "delta_yaw") == 0) { *out_value = (double)d.delta_yaw; return true; }
        if (strcmp(field_name, "delta_yaw_velocity") == 0) { *out_value = (double)d.delta_yaw_velocity; return true; }
        if (strcmp(field_name, "gimbal_device_id") == 0) { *out_value = (double)d.gimbal_device_id; return true; }
        return false;
    }
    case 286: {
        mavlink_autopilot_state_for_gimbal_device_t d;
        mavlink_msg_autopilot_state_for_gimbal_device_decode(&raw, &d);
        if (strcmp(field_name, "target_system") == 0) { *out_value = (double)d.target_system; return true; }
        if (strcmp(field_name, "target_component") == 0) { *out_value = (double)d.target_component; return true; }
        if (strcmp(field_name, "time_boot_us") == 0) { *out_value = (double)d.time_boot_us; return true; }
        if (strcmp(field_name, "q[0]") == 0) { *out_value = (double)d.q[0]; return true; }
        if (strcmp(field_name, "q[1]") == 0) { *out_value = (double)d.q[1]; return true; }
        if (strcmp(field_name, "q[2]") == 0) { *out_value = (double)d.q[2]; return true; }
        if (strcmp(field_name, "q[3]") == 0) { *out_value = (double)d.q[3]; return true; }
        if (strcmp(field_name, "q_estimated_delay_us") == 0) { *out_value = (double)d.q_estimated_delay_us; return true; }
        if (strcmp(field_name, "vx") == 0) { *out_value = (double)d.vx; return true; }
        if (strcmp(field_name, "vy") == 0) { *out_value = (double)d.vy; return true; }
        if (strcmp(field_name, "vz") == 0) { *out_value = (double)d.vz; return true; }
        if (strcmp(field_name, "v_estimated_delay_us") == 0) { *out_value = (double)d.v_estimated_delay_us; return true; }
        if (strcmp(field_name, "feed_forward_angular_velocity_z") == 0) { *out_value = (double)d.feed_forward_angular_velocity_z; return true; }
        if (strcmp(field_name, "estimator_status") == 0) { *out_value = (double)d.estimator_status; return true; }
        if (strcmp(field_name, "landed_state") == 0) { *out_value = (double)d.landed_state; return true; }
        if (strcmp(field_name, "angular_velocity_z") == 0) { *out_value = (double)d.angular_velocity_z; return true; }
        return false;
    }
    case 290: {
        mavlink_esc_info_t d;
        mavlink_msg_esc_info_decode(&raw, &d);
        if (strcmp(field_name, "index") == 0) { *out_value = (double)d.index; return true; }
        if (strcmp(field_name, "time_usec") == 0) { *out_value = (double)d.time_usec; return true; }
        if (strcmp(field_name, "counter") == 0) { *out_value = (double)d.counter; return true; }
        if (strcmp(field_name, "count") == 0) { *out_value = (double)d.count; return true; }
        if (strcmp(field_name, "connection_type") == 0) { *out_value = (double)d.connection_type; return true; }
        if (strcmp(field_name, "info") == 0) { *out_value = (double)d.info; return true; }
        if (strcmp(field_name, "failure_flags[0]") == 0) { *out_value = (double)d.failure_flags[0]; return true; }
        if (strcmp(field_name, "failure_flags[1]") == 0) { *out_value = (double)d.failure_flags[1]; return true; }
        if (strcmp(field_name, "failure_flags[2]") == 0) { *out_value = (double)d.failure_flags[2]; return true; }
        if (strcmp(field_name, "failure_flags[3]") == 0) { *out_value = (double)d.failure_flags[3]; return true; }
        if (strcmp(field_name, "error_count[0]") == 0) { *out_value = (double)d.error_count[0]; return true; }
        if (strcmp(field_name, "error_count[1]") == 0) { *out_value = (double)d.error_count[1]; return true; }
        if (strcmp(field_name, "error_count[2]") == 0) { *out_value = (double)d.error_count[2]; return true; }
        if (strcmp(field_name, "error_count[3]") == 0) { *out_value = (double)d.error_count[3]; return true; }
        if (strcmp(field_name, "temperature[0]") == 0) { *out_value = (double)d.temperature[0]; return true; }
        if (strcmp(field_name, "temperature[1]") == 0) { *out_value = (double)d.temperature[1]; return true; }
        if (strcmp(field_name, "temperature[2]") == 0) { *out_value = (double)d.temperature[2]; return true; }
        if (strcmp(field_name, "temperature[3]") == 0) { *out_value = (double)d.temperature[3]; return true; }
        return false;
    }
    case 291: {
        mavlink_esc_status_t d;
        mavlink_msg_esc_status_decode(&raw, &d);
        if (strcmp(field_name, "index") == 0) { *out_value = (double)d.index; return true; }
        if (strcmp(field_name, "time_usec") == 0) { *out_value = (double)d.time_usec; return true; }
        if (strcmp(field_name, "rpm[0]") == 0) { *out_value = (double)d.rpm[0]; return true; }
        if (strcmp(field_name, "rpm[1]") == 0) { *out_value = (double)d.rpm[1]; return true; }
        if (strcmp(field_name, "rpm[2]") == 0) { *out_value = (double)d.rpm[2]; return true; }
        if (strcmp(field_name, "rpm[3]") == 0) { *out_value = (double)d.rpm[3]; return true; }
        if (strcmp(field_name, "voltage[0]") == 0) { *out_value = (double)d.voltage[0]; return true; }
        if (strcmp(field_name, "voltage[1]") == 0) { *out_value = (double)d.voltage[1]; return true; }
        if (strcmp(field_name, "voltage[2]") == 0) { *out_value = (double)d.voltage[2]; return true; }
        if (strcmp(field_name, "voltage[3]") == 0) { *out_value = (double)d.voltage[3]; return true; }
        if (strcmp(field_name, "current[0]") == 0) { *out_value = (double)d.current[0]; return true; }
        if (strcmp(field_name, "current[1]") == 0) { *out_value = (double)d.current[1]; return true; }
        if (strcmp(field_name, "current[2]") == 0) { *out_value = (double)d.current[2]; return true; }
        if (strcmp(field_name, "current[3]") == 0) { *out_value = (double)d.current[3]; return true; }
        return false;
    }
    case 295: {
        mavlink_airspeed_t d;
        mavlink_msg_airspeed_decode(&raw, &d);
        if (strcmp(field_name, "id") == 0) { *out_value = (double)d.id; return true; }
        if (strcmp(field_name, "airspeed") == 0) { *out_value = (double)d.airspeed; return true; }
        if (strcmp(field_name, "temperature") == 0) { *out_value = (double)d.temperature; return true; }
        if (strcmp(field_name, "raw_press") == 0) { *out_value = (double)d.raw_press; return true; }
        if (strcmp(field_name, "flags") == 0) { *out_value = (double)d.flags; return true; }
        return false;
    }
    case 296: {
        mavlink_global_position_sensor_t d;
        mavlink_msg_global_position_sensor_decode(&raw, &d);
        if (strcmp(field_name, "target_system") == 0) { *out_value = (double)d.target_system; return true; }
        if (strcmp(field_name, "target_component") == 0) { *out_value = (double)d.target_component; return true; }
        if (strcmp(field_name, "id") == 0) { *out_value = (double)d.id; return true; }
        if (strcmp(field_name, "time_usec") == 0) { *out_value = (double)d.time_usec; return true; }
        if (strcmp(field_name, "processing_time") == 0) { *out_value = (double)d.processing_time; return true; }
        if (strcmp(field_name, "source") == 0) { *out_value = (double)d.source; return true; }
        if (strcmp(field_name, "flags") == 0) { *out_value = (double)d.flags; return true; }
        if (strcmp(field_name, "lat") == 0) { *out_value = (double)d.lat; return true; }
        if (strcmp(field_name, "lon") == 0) { *out_value = (double)d.lon; return true; }
        if (strcmp(field_name, "alt_ellipsoid") == 0) { *out_value = (double)d.alt_ellipsoid; return true; }
        if (strcmp(field_name, "alt") == 0) { *out_value = (double)d.alt; return true; }
        if (strcmp(field_name, "eph") == 0) { *out_value = (double)d.eph; return true; }
        if (strcmp(field_name, "epv") == 0) { *out_value = (double)d.epv; return true; }
        return false;
    }
    case 301: {
        mavlink_ais_vessel_t d;
        mavlink_msg_ais_vessel_decode(&raw, &d);
        if (strcmp(field_name, "MMSI") == 0) { *out_value = (double)d.MMSI; return true; }
        if (strcmp(field_name, "lat") == 0) { *out_value = (double)d.lat; return true; }
        if (strcmp(field_name, "lon") == 0) { *out_value = (double)d.lon; return true; }
        if (strcmp(field_name, "COG") == 0) { *out_value = (double)d.COG; return true; }
        if (strcmp(field_name, "heading") == 0) { *out_value = (double)d.heading; return true; }
        if (strcmp(field_name, "velocity") == 0) { *out_value = (double)d.velocity; return true; }
        if (strcmp(field_name, "turn_rate") == 0) { *out_value = (double)d.turn_rate; return true; }
        if (strcmp(field_name, "navigational_status") == 0) { *out_value = (double)d.navigational_status; return true; }
        if (strcmp(field_name, "type") == 0) { *out_value = (double)d.type; return true; }
        if (strcmp(field_name, "dimension_bow") == 0) { *out_value = (double)d.dimension_bow; return true; }
        if (strcmp(field_name, "dimension_stern") == 0) { *out_value = (double)d.dimension_stern; return true; }
        if (strcmp(field_name, "dimension_port") == 0) { *out_value = (double)d.dimension_port; return true; }
        if (strcmp(field_name, "dimension_starboard") == 0) { *out_value = (double)d.dimension_starboard; return true; }
        if (strcmp(field_name, "tslc") == 0) { *out_value = (double)d.tslc; return true; }
        if (strcmp(field_name, "flags") == 0) { *out_value = (double)d.flags; return true; }
        return false;
    }
    case 310: {
        mavlink_uavcan_node_status_t d;
        mavlink_msg_uavcan_node_status_decode(&raw, &d);
        if (strcmp(field_name, "time_usec") == 0) { *out_value = (double)d.time_usec; return true; }
        if (strcmp(field_name, "uptime_sec") == 0) { *out_value = (double)d.uptime_sec; return true; }
        if (strcmp(field_name, "health") == 0) { *out_value = (double)d.health; return true; }
        if (strcmp(field_name, "mode") == 0) { *out_value = (double)d.mode; return true; }
        if (strcmp(field_name, "sub_mode") == 0) { *out_value = (double)d.sub_mode; return true; }
        if (strcmp(field_name, "vendor_specific_status_code") == 0) { *out_value = (double)d.vendor_specific_status_code; return true; }
        return false;
    }
    case 311: {
        mavlink_uavcan_node_info_t d;
        mavlink_msg_uavcan_node_info_decode(&raw, &d);
        if (strcmp(field_name, "time_usec") == 0) { *out_value = (double)d.time_usec; return true; }
        if (strcmp(field_name, "uptime_sec") == 0) { *out_value = (double)d.uptime_sec; return true; }
        if (strcmp(field_name, "hw_version_major") == 0) { *out_value = (double)d.hw_version_major; return true; }
        if (strcmp(field_name, "hw_version_minor") == 0) { *out_value = (double)d.hw_version_minor; return true; }
        if (strcmp(field_name, "sw_version_major") == 0) { *out_value = (double)d.sw_version_major; return true; }
        if (strcmp(field_name, "sw_version_minor") == 0) { *out_value = (double)d.sw_version_minor; return true; }
        if (strcmp(field_name, "sw_vcs_commit") == 0) { *out_value = (double)d.sw_vcs_commit; return true; }
        return false;
    }
    case 322: {
        mavlink_param_ext_value_t d;
        mavlink_msg_param_ext_value_decode(&raw, &d);
        if (strcmp(field_name, "param_type") == 0) { *out_value = (double)d.param_type; return true; }
        if (strcmp(field_name, "param_count") == 0) { *out_value = (double)d.param_count; return true; }
        if (strcmp(field_name, "param_index") == 0) { *out_value = (double)d.param_index; return true; }
        return false;
    }
    case 324: {
        mavlink_param_ext_ack_t d;
        mavlink_msg_param_ext_ack_decode(&raw, &d);
        if (strcmp(field_name, "param_type") == 0) { *out_value = (double)d.param_type; return true; }
        if (strcmp(field_name, "param_result") == 0) { *out_value = (double)d.param_result; return true; }
        return false;
    }
    case 330: {
        mavlink_obstacle_distance_t d;
        mavlink_msg_obstacle_distance_decode(&raw, &d);
        if (strcmp(field_name, "time_usec") == 0) { *out_value = (double)d.time_usec; return true; }
        if (strcmp(field_name, "sensor_type") == 0) { *out_value = (double)d.sensor_type; return true; }
        if (strcmp(field_name, "increment") == 0) { *out_value = (double)d.increment; return true; }
        if (strcmp(field_name, "min_distance") == 0) { *out_value = (double)d.min_distance; return true; }
        if (strcmp(field_name, "max_distance") == 0) { *out_value = (double)d.max_distance; return true; }
        if (strcmp(field_name, "increment_f") == 0) { *out_value = (double)d.increment_f; return true; }
        if (strcmp(field_name, "angle_offset") == 0) { *out_value = (double)d.angle_offset; return true; }
        if (strcmp(field_name, "frame") == 0) { *out_value = (double)d.frame; return true; }
        return false;
    }
    case 331: {
        mavlink_odometry_t d;
        mavlink_msg_odometry_decode(&raw, &d);
        if (strcmp(field_name, "time_usec") == 0) { *out_value = (double)d.time_usec; return true; }
        if (strcmp(field_name, "frame_id") == 0) { *out_value = (double)d.frame_id; return true; }
        if (strcmp(field_name, "child_frame_id") == 0) { *out_value = (double)d.child_frame_id; return true; }
        if (strcmp(field_name, "x") == 0) { *out_value = (double)d.x; return true; }
        if (strcmp(field_name, "y") == 0) { *out_value = (double)d.y; return true; }
        if (strcmp(field_name, "z") == 0) { *out_value = (double)d.z; return true; }
        if (strcmp(field_name, "q[0]") == 0) { *out_value = (double)d.q[0]; return true; }
        if (strcmp(field_name, "q[1]") == 0) { *out_value = (double)d.q[1]; return true; }
        if (strcmp(field_name, "q[2]") == 0) { *out_value = (double)d.q[2]; return true; }
        if (strcmp(field_name, "q[3]") == 0) { *out_value = (double)d.q[3]; return true; }
        if (strcmp(field_name, "vx") == 0) { *out_value = (double)d.vx; return true; }
        if (strcmp(field_name, "vy") == 0) { *out_value = (double)d.vy; return true; }
        if (strcmp(field_name, "vz") == 0) { *out_value = (double)d.vz; return true; }
        if (strcmp(field_name, "rollspeed") == 0) { *out_value = (double)d.rollspeed; return true; }
        if (strcmp(field_name, "pitchspeed") == 0) { *out_value = (double)d.pitchspeed; return true; }
        if (strcmp(field_name, "yawspeed") == 0) { *out_value = (double)d.yawspeed; return true; }
        if (strcmp(field_name, "reset_counter") == 0) { *out_value = (double)d.reset_counter; return true; }
        if (strcmp(field_name, "estimator_type") == 0) { *out_value = (double)d.estimator_type; return true; }
        if (strcmp(field_name, "quality") == 0) { *out_value = (double)d.quality; return true; }
        return false;
    }
    case 332: {
        mavlink_trajectory_representation_waypoints_t d;
        mavlink_msg_trajectory_representation_waypoints_decode(&raw, &d);
        if (strcmp(field_name, "time_usec") == 0) { *out_value = (double)d.time_usec; return true; }
        if (strcmp(field_name, "valid_points") == 0) { *out_value = (double)d.valid_points; return true; }
        return false;
    }
    case 333: {
        mavlink_trajectory_representation_bezier_t d;
        mavlink_msg_trajectory_representation_bezier_decode(&raw, &d);
        if (strcmp(field_name, "time_usec") == 0) { *out_value = (double)d.time_usec; return true; }
        if (strcmp(field_name, "valid_points") == 0) { *out_value = (double)d.valid_points; return true; }
        return false;
    }
    case 334: {
        mavlink_cellular_status_t d;
        mavlink_msg_cellular_status_decode(&raw, &d);
        if (strcmp(field_name, "status") == 0) { *out_value = (double)d.status; return true; }
        if (strcmp(field_name, "failure_reason") == 0) { *out_value = (double)d.failure_reason; return true; }
        if (strcmp(field_name, "type") == 0) { *out_value = (double)d.type; return true; }
        if (strcmp(field_name, "quality") == 0) { *out_value = (double)d.quality; return true; }
        if (strcmp(field_name, "mcc") == 0) { *out_value = (double)d.mcc; return true; }
        if (strcmp(field_name, "mnc") == 0) { *out_value = (double)d.mnc; return true; }
        if (strcmp(field_name, "lac") == 0) { *out_value = (double)d.lac; return true; }
        return false;
    }
    case 335: {
        mavlink_isbd_link_status_t d;
        mavlink_msg_isbd_link_status_decode(&raw, &d);
        if (strcmp(field_name, "timestamp") == 0) { *out_value = (double)d.timestamp; return true; }
        if (strcmp(field_name, "last_heartbeat") == 0) { *out_value = (double)d.last_heartbeat; return true; }
        if (strcmp(field_name, "failed_sessions") == 0) { *out_value = (double)d.failed_sessions; return true; }
        if (strcmp(field_name, "successful_sessions") == 0) { *out_value = (double)d.successful_sessions; return true; }
        if (strcmp(field_name, "signal_quality") == 0) { *out_value = (double)d.signal_quality; return true; }
        if (strcmp(field_name, "ring_pending") == 0) { *out_value = (double)d.ring_pending; return true; }
        if (strcmp(field_name, "tx_session_pending") == 0) { *out_value = (double)d.tx_session_pending; return true; }
        if (strcmp(field_name, "rx_session_pending") == 0) { *out_value = (double)d.rx_session_pending; return true; }
        return false;
    }
    case 339: {
        mavlink_raw_rpm_t d;
        mavlink_msg_raw_rpm_decode(&raw, &d);
        if (strcmp(field_name, "index") == 0) { *out_value = (double)d.index; return true; }
        if (strcmp(field_name, "frequency") == 0) { *out_value = (double)d.frequency; return true; }
        return false;
    }
    case 340: {
        mavlink_utm_global_position_t d;
        mavlink_msg_utm_global_position_decode(&raw, &d);
        if (strcmp(field_name, "time") == 0) { *out_value = (double)d.time; return true; }
        if (strcmp(field_name, "lat") == 0) { *out_value = (double)d.lat; return true; }
        if (strcmp(field_name, "lon") == 0) { *out_value = (double)d.lon; return true; }
        if (strcmp(field_name, "alt") == 0) { *out_value = (double)d.alt; return true; }
        if (strcmp(field_name, "relative_alt") == 0) { *out_value = (double)d.relative_alt; return true; }
        if (strcmp(field_name, "vx") == 0) { *out_value = (double)d.vx; return true; }
        if (strcmp(field_name, "vy") == 0) { *out_value = (double)d.vy; return true; }
        if (strcmp(field_name, "vz") == 0) { *out_value = (double)d.vz; return true; }
        if (strcmp(field_name, "h_acc") == 0) { *out_value = (double)d.h_acc; return true; }
        if (strcmp(field_name, "v_acc") == 0) { *out_value = (double)d.v_acc; return true; }
        if (strcmp(field_name, "vel_acc") == 0) { *out_value = (double)d.vel_acc; return true; }
        if (strcmp(field_name, "next_lat") == 0) { *out_value = (double)d.next_lat; return true; }
        if (strcmp(field_name, "next_lon") == 0) { *out_value = (double)d.next_lon; return true; }
        if (strcmp(field_name, "next_alt") == 0) { *out_value = (double)d.next_alt; return true; }
        if (strcmp(field_name, "update_rate") == 0) { *out_value = (double)d.update_rate; return true; }
        if (strcmp(field_name, "flight_state") == 0) { *out_value = (double)d.flight_state; return true; }
        if (strcmp(field_name, "flags") == 0) { *out_value = (double)d.flags; return true; }
        return false;
    }
    case 345: {
        mavlink_param_error_t d;
        mavlink_msg_param_error_decode(&raw, &d);
        if (strcmp(field_name, "target_system") == 0) { *out_value = (double)d.target_system; return true; }
        if (strcmp(field_name, "target_component") == 0) { *out_value = (double)d.target_component; return true; }
        if (strcmp(field_name, "param_index") == 0) { *out_value = (double)d.param_index; return true; }
        if (strcmp(field_name, "error") == 0) { *out_value = (double)d.error; return true; }
        return false;
    }
    case 350: {
        mavlink_debug_float_array_t d;
        mavlink_msg_debug_float_array_decode(&raw, &d);
        if (strcmp(field_name, "time_usec") == 0) { *out_value = (double)d.time_usec; return true; }
        if (strcmp(field_name, "array_id") == 0) { *out_value = (double)d.array_id; return true; }
        return false;
    }
    case 360: {
        mavlink_orbit_execution_status_t d;
        mavlink_msg_orbit_execution_status_decode(&raw, &d);
        if (strcmp(field_name, "time_usec") == 0) { *out_value = (double)d.time_usec; return true; }
        if (strcmp(field_name, "radius") == 0) { *out_value = (double)d.radius; return true; }
        if (strcmp(field_name, "frame") == 0) { *out_value = (double)d.frame; return true; }
        if (strcmp(field_name, "x") == 0) { *out_value = (double)d.x; return true; }
        if (strcmp(field_name, "y") == 0) { *out_value = (double)d.y; return true; }
        if (strcmp(field_name, "z") == 0) { *out_value = (double)d.z; return true; }
        return false;
    }
    case 361: {
        mavlink_figure_eight_execution_status_t d;
        mavlink_msg_figure_eight_execution_status_decode(&raw, &d);
        if (strcmp(field_name, "time_usec") == 0) { *out_value = (double)d.time_usec; return true; }
        if (strcmp(field_name, "major_radius") == 0) { *out_value = (double)d.major_radius; return true; }
        if (strcmp(field_name, "minor_radius") == 0) { *out_value = (double)d.minor_radius; return true; }
        if (strcmp(field_name, "orientation") == 0) { *out_value = (double)d.orientation; return true; }
        if (strcmp(field_name, "frame") == 0) { *out_value = (double)d.frame; return true; }
        if (strcmp(field_name, "x") == 0) { *out_value = (double)d.x; return true; }
        if (strcmp(field_name, "y") == 0) { *out_value = (double)d.y; return true; }
        if (strcmp(field_name, "z") == 0) { *out_value = (double)d.z; return true; }
        return false;
    }
    case 370: {
        mavlink_smart_battery_info_t d;
        mavlink_msg_smart_battery_info_decode(&raw, &d);
        if (strcmp(field_name, "id") == 0) { *out_value = (double)d.id; return true; }
        if (strcmp(field_name, "battery_function") == 0) { *out_value = (double)d.battery_function; return true; }
        if (strcmp(field_name, "type") == 0) { *out_value = (double)d.type; return true; }
        if (strcmp(field_name, "capacity_full_specification") == 0) { *out_value = (double)d.capacity_full_specification; return true; }
        if (strcmp(field_name, "capacity_full") == 0) { *out_value = (double)d.capacity_full; return true; }
        if (strcmp(field_name, "cycle_count") == 0) { *out_value = (double)d.cycle_count; return true; }
        if (strcmp(field_name, "weight") == 0) { *out_value = (double)d.weight; return true; }
        if (strcmp(field_name, "discharge_minimum_voltage") == 0) { *out_value = (double)d.discharge_minimum_voltage; return true; }
        if (strcmp(field_name, "charging_minimum_voltage") == 0) { *out_value = (double)d.charging_minimum_voltage; return true; }
        if (strcmp(field_name, "resting_minimum_voltage") == 0) { *out_value = (double)d.resting_minimum_voltage; return true; }
        if (strcmp(field_name, "charging_maximum_voltage") == 0) { *out_value = (double)d.charging_maximum_voltage; return true; }
        if (strcmp(field_name, "cells_in_series") == 0) { *out_value = (double)d.cells_in_series; return true; }
        if (strcmp(field_name, "discharge_maximum_current") == 0) { *out_value = (double)d.discharge_maximum_current; return true; }
        if (strcmp(field_name, "discharge_maximum_burst_current") == 0) { *out_value = (double)d.discharge_maximum_burst_current; return true; }
        return false;
    }
    case 371: {
        mavlink_fuel_status_t d;
        mavlink_msg_fuel_status_decode(&raw, &d);
        if (strcmp(field_name, "id") == 0) { *out_value = (double)d.id; return true; }
        if (strcmp(field_name, "maximum_fuel") == 0) { *out_value = (double)d.maximum_fuel; return true; }
        if (strcmp(field_name, "consumed_fuel") == 0) { *out_value = (double)d.consumed_fuel; return true; }
        if (strcmp(field_name, "remaining_fuel") == 0) { *out_value = (double)d.remaining_fuel; return true; }
        if (strcmp(field_name, "percent_remaining") == 0) { *out_value = (double)d.percent_remaining; return true; }
        if (strcmp(field_name, "flow_rate") == 0) { *out_value = (double)d.flow_rate; return true; }
        if (strcmp(field_name, "temperature") == 0) { *out_value = (double)d.temperature; return true; }
        if (strcmp(field_name, "fuel_type") == 0) { *out_value = (double)d.fuel_type; return true; }
        return false;
    }
    case 372: {
        mavlink_battery_info_t d;
        mavlink_msg_battery_info_decode(&raw, &d);
        if (strcmp(field_name, "id") == 0) { *out_value = (double)d.id; return true; }
        if (strcmp(field_name, "battery_function") == 0) { *out_value = (double)d.battery_function; return true; }
        if (strcmp(field_name, "type") == 0) { *out_value = (double)d.type; return true; }
        if (strcmp(field_name, "state_of_health") == 0) { *out_value = (double)d.state_of_health; return true; }
        if (strcmp(field_name, "cells_in_series") == 0) { *out_value = (double)d.cells_in_series; return true; }
        if (strcmp(field_name, "cycle_count") == 0) { *out_value = (double)d.cycle_count; return true; }
        if (strcmp(field_name, "weight") == 0) { *out_value = (double)d.weight; return true; }
        if (strcmp(field_name, "discharge_minimum_voltage") == 0) { *out_value = (double)d.discharge_minimum_voltage; return true; }
        if (strcmp(field_name, "charging_minimum_voltage") == 0) { *out_value = (double)d.charging_minimum_voltage; return true; }
        if (strcmp(field_name, "resting_minimum_voltage") == 0) { *out_value = (double)d.resting_minimum_voltage; return true; }
        if (strcmp(field_name, "charging_maximum_voltage") == 0) { *out_value = (double)d.charging_maximum_voltage; return true; }
        if (strcmp(field_name, "charging_maximum_current") == 0) { *out_value = (double)d.charging_maximum_current; return true; }
        if (strcmp(field_name, "nominal_voltage") == 0) { *out_value = (double)d.nominal_voltage; return true; }
        if (strcmp(field_name, "discharge_maximum_current") == 0) { *out_value = (double)d.discharge_maximum_current; return true; }
        if (strcmp(field_name, "discharge_maximum_burst_current") == 0) { *out_value = (double)d.discharge_maximum_burst_current; return true; }
        if (strcmp(field_name, "design_capacity") == 0) { *out_value = (double)d.design_capacity; return true; }
        if (strcmp(field_name, "full_charge_capacity") == 0) { *out_value = (double)d.full_charge_capacity; return true; }
        return false;
    }
    case 373: {
        mavlink_generator_status_t d;
        mavlink_msg_generator_status_decode(&raw, &d);
        if (strcmp(field_name, "status") == 0) { *out_value = (double)d.status; return true; }
        if (strcmp(field_name, "generator_speed") == 0) { *out_value = (double)d.generator_speed; return true; }
        if (strcmp(field_name, "battery_current") == 0) { *out_value = (double)d.battery_current; return true; }
        if (strcmp(field_name, "load_current") == 0) { *out_value = (double)d.load_current; return true; }
        if (strcmp(field_name, "power_generated") == 0) { *out_value = (double)d.power_generated; return true; }
        if (strcmp(field_name, "bus_voltage") == 0) { *out_value = (double)d.bus_voltage; return true; }
        if (strcmp(field_name, "rectifier_temperature") == 0) { *out_value = (double)d.rectifier_temperature; return true; }
        if (strcmp(field_name, "bat_current_setpoint") == 0) { *out_value = (double)d.bat_current_setpoint; return true; }
        if (strcmp(field_name, "generator_temperature") == 0) { *out_value = (double)d.generator_temperature; return true; }
        if (strcmp(field_name, "runtime") == 0) { *out_value = (double)d.runtime; return true; }
        if (strcmp(field_name, "time_until_maintenance") == 0) { *out_value = (double)d.time_until_maintenance; return true; }
        return false;
    }
    case 375: {
        mavlink_actuator_output_status_t d;
        mavlink_msg_actuator_output_status_decode(&raw, &d);
        if (strcmp(field_name, "time_usec") == 0) { *out_value = (double)d.time_usec; return true; }
        if (strcmp(field_name, "active") == 0) { *out_value = (double)d.active; return true; }
        return false;
    }
    case 376: {
        mavlink_relay_status_t d;
        mavlink_msg_relay_status_decode(&raw, &d);
        if (strcmp(field_name, "time_boot_ms") == 0) { *out_value = (double)d.time_boot_ms; return true; }
        if (strcmp(field_name, "on") == 0) { *out_value = (double)d.on; return true; }
        if (strcmp(field_name, "present") == 0) { *out_value = (double)d.present; return true; }
        return false;
    }
    case 380: {
        mavlink_time_estimate_to_target_t d;
        mavlink_msg_time_estimate_to_target_decode(&raw, &d);
        if (strcmp(field_name, "safe_return") == 0) { *out_value = (double)d.safe_return; return true; }
        if (strcmp(field_name, "land") == 0) { *out_value = (double)d.land; return true; }
        if (strcmp(field_name, "mission_next_item") == 0) { *out_value = (double)d.mission_next_item; return true; }
        if (strcmp(field_name, "mission_end") == 0) { *out_value = (double)d.mission_end; return true; }
        if (strcmp(field_name, "commanded_action") == 0) { *out_value = (double)d.commanded_action; return true; }
        return false;
    }
    case 385: {
        mavlink_tunnel_t d;
        mavlink_msg_tunnel_decode(&raw, &d);
        if (strcmp(field_name, "target_system") == 0) { *out_value = (double)d.target_system; return true; }
        if (strcmp(field_name, "target_component") == 0) { *out_value = (double)d.target_component; return true; }
        if (strcmp(field_name, "payload_type") == 0) { *out_value = (double)d.payload_type; return true; }
        if (strcmp(field_name, "payload_length") == 0) { *out_value = (double)d.payload_length; return true; }
        return false;
    }
    case 386: {
        mavlink_can_frame_t d;
        mavlink_msg_can_frame_decode(&raw, &d);
        if (strcmp(field_name, "target_system") == 0) { *out_value = (double)d.target_system; return true; }
        if (strcmp(field_name, "target_component") == 0) { *out_value = (double)d.target_component; return true; }
        if (strcmp(field_name, "bus") == 0) { *out_value = (double)d.bus; return true; }
        if (strcmp(field_name, "len") == 0) { *out_value = (double)d.len; return true; }
        if (strcmp(field_name, "id") == 0) { *out_value = (double)d.id; return true; }
        return false;
    }
    case 387: {
        mavlink_canfd_frame_t d;
        mavlink_msg_canfd_frame_decode(&raw, &d);
        if (strcmp(field_name, "target_system") == 0) { *out_value = (double)d.target_system; return true; }
        if (strcmp(field_name, "target_component") == 0) { *out_value = (double)d.target_component; return true; }
        if (strcmp(field_name, "bus") == 0) { *out_value = (double)d.bus; return true; }
        if (strcmp(field_name, "len") == 0) { *out_value = (double)d.len; return true; }
        if (strcmp(field_name, "id") == 0) { *out_value = (double)d.id; return true; }
        return false;
    }
    case 390: {
        mavlink_onboard_computer_status_t d;
        mavlink_msg_onboard_computer_status_decode(&raw, &d);
        if (strcmp(field_name, "time_usec") == 0) { *out_value = (double)d.time_usec; return true; }
        if (strcmp(field_name, "uptime") == 0) { *out_value = (double)d.uptime; return true; }
        if (strcmp(field_name, "type") == 0) { *out_value = (double)d.type; return true; }
        if (strcmp(field_name, "gpu_cores[0]") == 0) { *out_value = (double)d.gpu_cores[0]; return true; }
        if (strcmp(field_name, "gpu_cores[1]") == 0) { *out_value = (double)d.gpu_cores[1]; return true; }
        if (strcmp(field_name, "gpu_cores[2]") == 0) { *out_value = (double)d.gpu_cores[2]; return true; }
        if (strcmp(field_name, "gpu_cores[3]") == 0) { *out_value = (double)d.gpu_cores[3]; return true; }
        if (strcmp(field_name, "temperature_board") == 0) { *out_value = (double)d.temperature_board; return true; }
        if (strcmp(field_name, "fan_speed[0]") == 0) { *out_value = (double)d.fan_speed[0]; return true; }
        if (strcmp(field_name, "fan_speed[1]") == 0) { *out_value = (double)d.fan_speed[1]; return true; }
        if (strcmp(field_name, "fan_speed[2]") == 0) { *out_value = (double)d.fan_speed[2]; return true; }
        if (strcmp(field_name, "fan_speed[3]") == 0) { *out_value = (double)d.fan_speed[3]; return true; }
        if (strcmp(field_name, "ram_usage") == 0) { *out_value = (double)d.ram_usage; return true; }
        if (strcmp(field_name, "ram_total") == 0) { *out_value = (double)d.ram_total; return true; }
        if (strcmp(field_name, "storage_type[0]") == 0) { *out_value = (double)d.storage_type[0]; return true; }
        if (strcmp(field_name, "storage_type[1]") == 0) { *out_value = (double)d.storage_type[1]; return true; }
        if (strcmp(field_name, "storage_type[2]") == 0) { *out_value = (double)d.storage_type[2]; return true; }
        if (strcmp(field_name, "storage_type[3]") == 0) { *out_value = (double)d.storage_type[3]; return true; }
        if (strcmp(field_name, "storage_usage[0]") == 0) { *out_value = (double)d.storage_usage[0]; return true; }
        if (strcmp(field_name, "storage_usage[1]") == 0) { *out_value = (double)d.storage_usage[1]; return true; }
        if (strcmp(field_name, "storage_usage[2]") == 0) { *out_value = (double)d.storage_usage[2]; return true; }
        if (strcmp(field_name, "storage_usage[3]") == 0) { *out_value = (double)d.storage_usage[3]; return true; }
        if (strcmp(field_name, "storage_total[0]") == 0) { *out_value = (double)d.storage_total[0]; return true; }
        if (strcmp(field_name, "storage_total[1]") == 0) { *out_value = (double)d.storage_total[1]; return true; }
        if (strcmp(field_name, "storage_total[2]") == 0) { *out_value = (double)d.storage_total[2]; return true; }
        if (strcmp(field_name, "storage_total[3]") == 0) { *out_value = (double)d.storage_total[3]; return true; }
        if (strcmp(field_name, "status_flags") == 0) { *out_value = (double)d.status_flags; return true; }
        return false;
    }
    case 395: {
        mavlink_component_information_t d;
        mavlink_msg_component_information_decode(&raw, &d);
        if (strcmp(field_name, "time_boot_ms") == 0) { *out_value = (double)d.time_boot_ms; return true; }
        if (strcmp(field_name, "general_metadata_file_crc") == 0) { *out_value = (double)d.general_metadata_file_crc; return true; }
        if (strcmp(field_name, "peripherals_metadata_file_crc") == 0) { *out_value = (double)d.peripherals_metadata_file_crc; return true; }
        return false;
    }
    case 396: {
        mavlink_component_information_basic_t d;
        mavlink_msg_component_information_basic_decode(&raw, &d);
        if (strcmp(field_name, "time_boot_ms") == 0) { *out_value = (double)d.time_boot_ms; return true; }
        if (strcmp(field_name, "capabilities") == 0) { *out_value = (double)d.capabilities; return true; }
        if (strcmp(field_name, "time_manufacture_s") == 0) { *out_value = (double)d.time_manufacture_s; return true; }
        return false;
    }
    case 397: {
        mavlink_component_metadata_t d;
        mavlink_msg_component_metadata_decode(&raw, &d);
        if (strcmp(field_name, "time_boot_ms") == 0) { *out_value = (double)d.time_boot_ms; return true; }
        if (strcmp(field_name, "file_crc") == 0) { *out_value = (double)d.file_crc; return true; }
        return false;
    }
    case 401: {
        mavlink_supported_tunes_t d;
        mavlink_msg_supported_tunes_decode(&raw, &d);
        if (strcmp(field_name, "target_system") == 0) { *out_value = (double)d.target_system; return true; }
        if (strcmp(field_name, "target_component") == 0) { *out_value = (double)d.target_component; return true; }
        if (strcmp(field_name, "format") == 0) { *out_value = (double)d.format; return true; }
        return false;
    }
    case 410: {
        mavlink_event_t d;
        mavlink_msg_event_decode(&raw, &d);
        if (strcmp(field_name, "destination_component") == 0) { *out_value = (double)d.destination_component; return true; }
        if (strcmp(field_name, "destination_system") == 0) { *out_value = (double)d.destination_system; return true; }
        if (strcmp(field_name, "id") == 0) { *out_value = (double)d.id; return true; }
        if (strcmp(field_name, "event_time_boot_ms") == 0) { *out_value = (double)d.event_time_boot_ms; return true; }
        if (strcmp(field_name, "sequence") == 0) { *out_value = (double)d.sequence; return true; }
        if (strcmp(field_name, "log_levels") == 0) { *out_value = (double)d.log_levels; return true; }
        return false;
    }
    case 411: {
        mavlink_current_event_sequence_t d;
        mavlink_msg_current_event_sequence_decode(&raw, &d);
        if (strcmp(field_name, "sequence") == 0) { *out_value = (double)d.sequence; return true; }
        if (strcmp(field_name, "flags") == 0) { *out_value = (double)d.flags; return true; }
        return false;
    }
    case 413: {
        mavlink_response_event_error_t d;
        mavlink_msg_response_event_error_decode(&raw, &d);
        if (strcmp(field_name, "target_system") == 0) { *out_value = (double)d.target_system; return true; }
        if (strcmp(field_name, "target_component") == 0) { *out_value = (double)d.target_component; return true; }
        if (strcmp(field_name, "sequence") == 0) { *out_value = (double)d.sequence; return true; }
        if (strcmp(field_name, "sequence_oldest_available") == 0) { *out_value = (double)d.sequence_oldest_available; return true; }
        if (strcmp(field_name, "reason") == 0) { *out_value = (double)d.reason; return true; }
        return false;
    }
    case 435: {
        mavlink_available_modes_t d;
        mavlink_msg_available_modes_decode(&raw, &d);
        if (strcmp(field_name, "number_modes") == 0) { *out_value = (double)d.number_modes; return true; }
        if (strcmp(field_name, "mode_index") == 0) { *out_value = (double)d.mode_index; return true; }
        if (strcmp(field_name, "standard_mode") == 0) { *out_value = (double)d.standard_mode; return true; }
        if (strcmp(field_name, "custom_mode") == 0) { *out_value = (double)d.custom_mode; return true; }
        if (strcmp(field_name, "properties") == 0) { *out_value = (double)d.properties; return true; }
        return false;
    }
    case 436: {
        mavlink_current_mode_t d;
        mavlink_msg_current_mode_decode(&raw, &d);
        if (strcmp(field_name, "standard_mode") == 0) { *out_value = (double)d.standard_mode; return true; }
        if (strcmp(field_name, "custom_mode") == 0) { *out_value = (double)d.custom_mode; return true; }
        if (strcmp(field_name, "intended_custom_mode") == 0) { *out_value = (double)d.intended_custom_mode; return true; }
        return false;
    }
    case 437: {
        mavlink_available_modes_monitor_t d;
        mavlink_msg_available_modes_monitor_decode(&raw, &d);
        if (strcmp(field_name, "seq") == 0) { *out_value = (double)d.seq; return true; }
        return false;
    }
    case 440: {
        mavlink_illuminator_status_t d;
        mavlink_msg_illuminator_status_decode(&raw, &d);
        if (strcmp(field_name, "uptime_ms") == 0) { *out_value = (double)d.uptime_ms; return true; }
        if (strcmp(field_name, "enable") == 0) { *out_value = (double)d.enable; return true; }
        if (strcmp(field_name, "mode_bitmask") == 0) { *out_value = (double)d.mode_bitmask; return true; }
        if (strcmp(field_name, "error_status") == 0) { *out_value = (double)d.error_status; return true; }
        if (strcmp(field_name, "mode") == 0) { *out_value = (double)d.mode; return true; }
        if (strcmp(field_name, "brightness") == 0) { *out_value = (double)d.brightness; return true; }
        if (strcmp(field_name, "strobe_period") == 0) { *out_value = (double)d.strobe_period; return true; }
        if (strcmp(field_name, "strobe_duty_cycle") == 0) { *out_value = (double)d.strobe_duty_cycle; return true; }
        if (strcmp(field_name, "temp_c") == 0) { *out_value = (double)d.temp_c; return true; }
        if (strcmp(field_name, "min_strobe_period") == 0) { *out_value = (double)d.min_strobe_period; return true; }
        if (strcmp(field_name, "max_strobe_period") == 0) { *out_value = (double)d.max_strobe_period; return true; }
        return false;
    }
    case 9000: {
        mavlink_wheel_distance_t d;
        mavlink_msg_wheel_distance_decode(&raw, &d);
        if (strcmp(field_name, "time_usec") == 0) { *out_value = (double)d.time_usec; return true; }
        if (strcmp(field_name, "count") == 0) { *out_value = (double)d.count; return true; }
        return false;
    }
    case 9005: {
        mavlink_winch_status_t d;
        mavlink_msg_winch_status_decode(&raw, &d);
        if (strcmp(field_name, "time_usec") == 0) { *out_value = (double)d.time_usec; return true; }
        if (strcmp(field_name, "line_length") == 0) { *out_value = (double)d.line_length; return true; }
        if (strcmp(field_name, "speed") == 0) { *out_value = (double)d.speed; return true; }
        if (strcmp(field_name, "tension") == 0) { *out_value = (double)d.tension; return true; }
        if (strcmp(field_name, "voltage") == 0) { *out_value = (double)d.voltage; return true; }
        if (strcmp(field_name, "current") == 0) { *out_value = (double)d.current; return true; }
        if (strcmp(field_name, "temperature") == 0) { *out_value = (double)d.temperature; return true; }
        if (strcmp(field_name, "status") == 0) { *out_value = (double)d.status; return true; }
        return false;
    }
    case 10001: {
        mavlink_uavionix_adsb_out_cfg_t d;
        mavlink_msg_uavionix_adsb_out_cfg_decode(&raw, &d);
        if (strcmp(field_name, "ICAO") == 0) { *out_value = (double)d.ICAO; return true; }
        if (strcmp(field_name, "emitterType") == 0) { *out_value = (double)d.emitterType; return true; }
        if (strcmp(field_name, "aircraftSize") == 0) { *out_value = (double)d.aircraftSize; return true; }
        if (strcmp(field_name, "gpsOffsetLat") == 0) { *out_value = (double)d.gpsOffsetLat; return true; }
        if (strcmp(field_name, "gpsOffsetLon") == 0) { *out_value = (double)d.gpsOffsetLon; return true; }
        if (strcmp(field_name, "stallSpeed") == 0) { *out_value = (double)d.stallSpeed; return true; }
        if (strcmp(field_name, "rfSelect") == 0) { *out_value = (double)d.rfSelect; return true; }
        return false;
    }
    case 10002: {
        mavlink_uavionix_adsb_out_dynamic_t d;
        mavlink_msg_uavionix_adsb_out_dynamic_decode(&raw, &d);
        if (strcmp(field_name, "utcTime") == 0) { *out_value = (double)d.utcTime; return true; }
        if (strcmp(field_name, "gpsLat") == 0) { *out_value = (double)d.gpsLat; return true; }
        if (strcmp(field_name, "gpsLon") == 0) { *out_value = (double)d.gpsLon; return true; }
        if (strcmp(field_name, "gpsAlt") == 0) { *out_value = (double)d.gpsAlt; return true; }
        if (strcmp(field_name, "gpsFix") == 0) { *out_value = (double)d.gpsFix; return true; }
        if (strcmp(field_name, "numSats") == 0) { *out_value = (double)d.numSats; return true; }
        if (strcmp(field_name, "baroAltMSL") == 0) { *out_value = (double)d.baroAltMSL; return true; }
        if (strcmp(field_name, "accuracyHor") == 0) { *out_value = (double)d.accuracyHor; return true; }
        if (strcmp(field_name, "accuracyVert") == 0) { *out_value = (double)d.accuracyVert; return true; }
        if (strcmp(field_name, "accuracyVel") == 0) { *out_value = (double)d.accuracyVel; return true; }
        if (strcmp(field_name, "velVert") == 0) { *out_value = (double)d.velVert; return true; }
        if (strcmp(field_name, "velNS") == 0) { *out_value = (double)d.velNS; return true; }
        if (strcmp(field_name, "VelEW") == 0) { *out_value = (double)d.VelEW; return true; }
        if (strcmp(field_name, "emergencyStatus") == 0) { *out_value = (double)d.emergencyStatus; return true; }
        if (strcmp(field_name, "state") == 0) { *out_value = (double)d.state; return true; }
        if (strcmp(field_name, "squawk") == 0) { *out_value = (double)d.squawk; return true; }
        return false;
    }
    case 10003: {
        mavlink_uavionix_adsb_transceiver_health_report_t d;
        mavlink_msg_uavionix_adsb_transceiver_health_report_decode(&raw, &d);
        if (strcmp(field_name, "rfHealth") == 0) { *out_value = (double)d.rfHealth; return true; }
        return false;
    }
    case 10006: {
        mavlink_uavionix_adsb_get_t d;
        mavlink_msg_uavionix_adsb_get_decode(&raw, &d);
        if (strcmp(field_name, "ReqMessageId") == 0) { *out_value = (double)d.ReqMessageId; return true; }
        return false;
    }
    case 10007: {
        mavlink_uavionix_adsb_out_control_t d;
        mavlink_msg_uavionix_adsb_out_control_decode(&raw, &d);
        if (strcmp(field_name, "state") == 0) { *out_value = (double)d.state; return true; }
        if (strcmp(field_name, "baroAltMSL") == 0) { *out_value = (double)d.baroAltMSL; return true; }
        if (strcmp(field_name, "squawk") == 0) { *out_value = (double)d.squawk; return true; }
        if (strcmp(field_name, "emergencyStatus") == 0) { *out_value = (double)d.emergencyStatus; return true; }
        if (strcmp(field_name, "x_bit") == 0) { *out_value = (double)d.x_bit; return true; }
        return false;
    }
    case 10008: {
        mavlink_uavionix_adsb_out_status_t d;
        mavlink_msg_uavionix_adsb_out_status_decode(&raw, &d);
        if (strcmp(field_name, "state") == 0) { *out_value = (double)d.state; return true; }
        if (strcmp(field_name, "squawk") == 0) { *out_value = (double)d.squawk; return true; }
        if (strcmp(field_name, "NIC_NACp") == 0) { *out_value = (double)d.NIC_NACp; return true; }
        if (strcmp(field_name, "boardTemp") == 0) { *out_value = (double)d.boardTemp; return true; }
        if (strcmp(field_name, "fault") == 0) { *out_value = (double)d.fault; return true; }
        return false;
    }
    case 10151: {
        mavlink_loweheiser_gov_efi_t d;
        mavlink_msg_loweheiser_gov_efi_decode(&raw, &d);
        if (strcmp(field_name, "volt_batt") == 0) { *out_value = (double)d.volt_batt; return true; }
        if (strcmp(field_name, "curr_batt") == 0) { *out_value = (double)d.curr_batt; return true; }
        if (strcmp(field_name, "curr_gen") == 0) { *out_value = (double)d.curr_gen; return true; }
        if (strcmp(field_name, "curr_rot") == 0) { *out_value = (double)d.curr_rot; return true; }
        if (strcmp(field_name, "fuel_level") == 0) { *out_value = (double)d.fuel_level; return true; }
        if (strcmp(field_name, "throttle") == 0) { *out_value = (double)d.throttle; return true; }
        if (strcmp(field_name, "runtime") == 0) { *out_value = (double)d.runtime; return true; }
        if (strcmp(field_name, "until_maintenance") == 0) { *out_value = (double)d.until_maintenance; return true; }
        if (strcmp(field_name, "rectifier_temp") == 0) { *out_value = (double)d.rectifier_temp; return true; }
        if (strcmp(field_name, "generator_temp") == 0) { *out_value = (double)d.generator_temp; return true; }
        if (strcmp(field_name, "efi_batt") == 0) { *out_value = (double)d.efi_batt; return true; }
        if (strcmp(field_name, "efi_rpm") == 0) { *out_value = (double)d.efi_rpm; return true; }
        if (strcmp(field_name, "efi_pw") == 0) { *out_value = (double)d.efi_pw; return true; }
        if (strcmp(field_name, "efi_fuel_flow") == 0) { *out_value = (double)d.efi_fuel_flow; return true; }
        if (strcmp(field_name, "efi_fuel_consumed") == 0) { *out_value = (double)d.efi_fuel_consumed; return true; }
        if (strcmp(field_name, "efi_baro") == 0) { *out_value = (double)d.efi_baro; return true; }
        if (strcmp(field_name, "efi_mat") == 0) { *out_value = (double)d.efi_mat; return true; }
        if (strcmp(field_name, "efi_clt") == 0) { *out_value = (double)d.efi_clt; return true; }
        if (strcmp(field_name, "efi_tps") == 0) { *out_value = (double)d.efi_tps; return true; }
        if (strcmp(field_name, "efi_exhaust_gas_temperature") == 0) { *out_value = (double)d.efi_exhaust_gas_temperature; return true; }
        if (strcmp(field_name, "efi_index") == 0) { *out_value = (double)d.efi_index; return true; }
        if (strcmp(field_name, "generator_status") == 0) { *out_value = (double)d.generator_status; return true; }
        if (strcmp(field_name, "efi_status") == 0) { *out_value = (double)d.efi_status; return true; }
        return false;
    }
    case 11000: {
        mavlink_device_op_read_t d;
        mavlink_msg_device_op_read_decode(&raw, &d);
        if (strcmp(field_name, "target_system") == 0) { *out_value = (double)d.target_system; return true; }
        if (strcmp(field_name, "target_component") == 0) { *out_value = (double)d.target_component; return true; }
        if (strcmp(field_name, "request_id") == 0) { *out_value = (double)d.request_id; return true; }
        if (strcmp(field_name, "bustype") == 0) { *out_value = (double)d.bustype; return true; }
        if (strcmp(field_name, "bus") == 0) { *out_value = (double)d.bus; return true; }
        if (strcmp(field_name, "address") == 0) { *out_value = (double)d.address; return true; }
        if (strcmp(field_name, "regstart") == 0) { *out_value = (double)d.regstart; return true; }
        if (strcmp(field_name, "count") == 0) { *out_value = (double)d.count; return true; }
        if (strcmp(field_name, "bank") == 0) { *out_value = (double)d.bank; return true; }
        return false;
    }
    case 11001: {
        mavlink_device_op_read_reply_t d;
        mavlink_msg_device_op_read_reply_decode(&raw, &d);
        if (strcmp(field_name, "request_id") == 0) { *out_value = (double)d.request_id; return true; }
        if (strcmp(field_name, "result") == 0) { *out_value = (double)d.result; return true; }
        if (strcmp(field_name, "regstart") == 0) { *out_value = (double)d.regstart; return true; }
        if (strcmp(field_name, "count") == 0) { *out_value = (double)d.count; return true; }
        if (strcmp(field_name, "bank") == 0) { *out_value = (double)d.bank; return true; }
        return false;
    }
    case 11003: {
        mavlink_device_op_write_reply_t d;
        mavlink_msg_device_op_write_reply_decode(&raw, &d);
        if (strcmp(field_name, "request_id") == 0) { *out_value = (double)d.request_id; return true; }
        if (strcmp(field_name, "result") == 0) { *out_value = (double)d.result; return true; }
        return false;
    }
    case 11005: {
        mavlink_secure_command_reply_t d;
        mavlink_msg_secure_command_reply_decode(&raw, &d);
        if (strcmp(field_name, "sequence") == 0) { *out_value = (double)d.sequence; return true; }
        if (strcmp(field_name, "operation") == 0) { *out_value = (double)d.operation; return true; }
        if (strcmp(field_name, "result") == 0) { *out_value = (double)d.result; return true; }
        if (strcmp(field_name, "data_length") == 0) { *out_value = (double)d.data_length; return true; }
        return false;
    }
    case 11010: {
        mavlink_adap_tuning_t d;
        mavlink_msg_adap_tuning_decode(&raw, &d);
        if (strcmp(field_name, "axis") == 0) { *out_value = (double)d.axis; return true; }
        if (strcmp(field_name, "desired") == 0) { *out_value = (double)d.desired; return true; }
        if (strcmp(field_name, "achieved") == 0) { *out_value = (double)d.achieved; return true; }
        if (strcmp(field_name, "error") == 0) { *out_value = (double)d.error; return true; }
        if (strcmp(field_name, "theta") == 0) { *out_value = (double)d.theta; return true; }
        if (strcmp(field_name, "omega") == 0) { *out_value = (double)d.omega; return true; }
        if (strcmp(field_name, "sigma") == 0) { *out_value = (double)d.sigma; return true; }
        if (strcmp(field_name, "theta_dot") == 0) { *out_value = (double)d.theta_dot; return true; }
        if (strcmp(field_name, "omega_dot") == 0) { *out_value = (double)d.omega_dot; return true; }
        if (strcmp(field_name, "sigma_dot") == 0) { *out_value = (double)d.sigma_dot; return true; }
        if (strcmp(field_name, "f") == 0) { *out_value = (double)d.f; return true; }
        if (strcmp(field_name, "f_dot") == 0) { *out_value = (double)d.f_dot; return true; }
        if (strcmp(field_name, "u") == 0) { *out_value = (double)d.u; return true; }
        return false;
    }
    case 11011: {
        mavlink_vision_position_delta_t d;
        mavlink_msg_vision_position_delta_decode(&raw, &d);
        if (strcmp(field_name, "time_usec") == 0) { *out_value = (double)d.time_usec; return true; }
        if (strcmp(field_name, "time_delta_usec") == 0) { *out_value = (double)d.time_delta_usec; return true; }
        if (strcmp(field_name, "angle_delta[0]") == 0) { *out_value = (double)d.angle_delta[0]; return true; }
        if (strcmp(field_name, "angle_delta[1]") == 0) { *out_value = (double)d.angle_delta[1]; return true; }
        if (strcmp(field_name, "angle_delta[2]") == 0) { *out_value = (double)d.angle_delta[2]; return true; }
        if (strcmp(field_name, "position_delta[0]") == 0) { *out_value = (double)d.position_delta[0]; return true; }
        if (strcmp(field_name, "position_delta[1]") == 0) { *out_value = (double)d.position_delta[1]; return true; }
        if (strcmp(field_name, "position_delta[2]") == 0) { *out_value = (double)d.position_delta[2]; return true; }
        if (strcmp(field_name, "confidence") == 0) { *out_value = (double)d.confidence; return true; }
        return false;
    }
    case 11020: {
        mavlink_aoa_ssa_t d;
        mavlink_msg_aoa_ssa_decode(&raw, &d);
        if (strcmp(field_name, "time_usec") == 0) { *out_value = (double)d.time_usec; return true; }
        if (strcmp(field_name, "AOA") == 0) { *out_value = (double)d.AOA; return true; }
        if (strcmp(field_name, "SSA") == 0) { *out_value = (double)d.SSA; return true; }
        return false;
    }
    case 11030: {
        mavlink_esc_telemetry_1_to_4_t d;
        mavlink_msg_esc_telemetry_1_to_4_decode(&raw, &d);
        if (strcmp(field_name, "temperature[0]") == 0) { *out_value = (double)d.temperature[0]; return true; }
        if (strcmp(field_name, "temperature[1]") == 0) { *out_value = (double)d.temperature[1]; return true; }
        if (strcmp(field_name, "temperature[2]") == 0) { *out_value = (double)d.temperature[2]; return true; }
        if (strcmp(field_name, "temperature[3]") == 0) { *out_value = (double)d.temperature[3]; return true; }
        if (strcmp(field_name, "voltage[0]") == 0) { *out_value = (double)d.voltage[0]; return true; }
        if (strcmp(field_name, "voltage[1]") == 0) { *out_value = (double)d.voltage[1]; return true; }
        if (strcmp(field_name, "voltage[2]") == 0) { *out_value = (double)d.voltage[2]; return true; }
        if (strcmp(field_name, "voltage[3]") == 0) { *out_value = (double)d.voltage[3]; return true; }
        if (strcmp(field_name, "current[0]") == 0) { *out_value = (double)d.current[0]; return true; }
        if (strcmp(field_name, "current[1]") == 0) { *out_value = (double)d.current[1]; return true; }
        if (strcmp(field_name, "current[2]") == 0) { *out_value = (double)d.current[2]; return true; }
        if (strcmp(field_name, "current[3]") == 0) { *out_value = (double)d.current[3]; return true; }
        if (strcmp(field_name, "totalcurrent[0]") == 0) { *out_value = (double)d.totalcurrent[0]; return true; }
        if (strcmp(field_name, "totalcurrent[1]") == 0) { *out_value = (double)d.totalcurrent[1]; return true; }
        if (strcmp(field_name, "totalcurrent[2]") == 0) { *out_value = (double)d.totalcurrent[2]; return true; }
        if (strcmp(field_name, "totalcurrent[3]") == 0) { *out_value = (double)d.totalcurrent[3]; return true; }
        if (strcmp(field_name, "rpm[0]") == 0) { *out_value = (double)d.rpm[0]; return true; }
        if (strcmp(field_name, "rpm[1]") == 0) { *out_value = (double)d.rpm[1]; return true; }
        if (strcmp(field_name, "rpm[2]") == 0) { *out_value = (double)d.rpm[2]; return true; }
        if (strcmp(field_name, "rpm[3]") == 0) { *out_value = (double)d.rpm[3]; return true; }
        if (strcmp(field_name, "count[0]") == 0) { *out_value = (double)d.count[0]; return true; }
        if (strcmp(field_name, "count[1]") == 0) { *out_value = (double)d.count[1]; return true; }
        if (strcmp(field_name, "count[2]") == 0) { *out_value = (double)d.count[2]; return true; }
        if (strcmp(field_name, "count[3]") == 0) { *out_value = (double)d.count[3]; return true; }
        return false;
    }
    case 11031: {
        mavlink_esc_telemetry_5_to_8_t d;
        mavlink_msg_esc_telemetry_5_to_8_decode(&raw, &d);
        if (strcmp(field_name, "temperature[0]") == 0) { *out_value = (double)d.temperature[0]; return true; }
        if (strcmp(field_name, "temperature[1]") == 0) { *out_value = (double)d.temperature[1]; return true; }
        if (strcmp(field_name, "temperature[2]") == 0) { *out_value = (double)d.temperature[2]; return true; }
        if (strcmp(field_name, "temperature[3]") == 0) { *out_value = (double)d.temperature[3]; return true; }
        if (strcmp(field_name, "voltage[0]") == 0) { *out_value = (double)d.voltage[0]; return true; }
        if (strcmp(field_name, "voltage[1]") == 0) { *out_value = (double)d.voltage[1]; return true; }
        if (strcmp(field_name, "voltage[2]") == 0) { *out_value = (double)d.voltage[2]; return true; }
        if (strcmp(field_name, "voltage[3]") == 0) { *out_value = (double)d.voltage[3]; return true; }
        if (strcmp(field_name, "current[0]") == 0) { *out_value = (double)d.current[0]; return true; }
        if (strcmp(field_name, "current[1]") == 0) { *out_value = (double)d.current[1]; return true; }
        if (strcmp(field_name, "current[2]") == 0) { *out_value = (double)d.current[2]; return true; }
        if (strcmp(field_name, "current[3]") == 0) { *out_value = (double)d.current[3]; return true; }
        if (strcmp(field_name, "totalcurrent[0]") == 0) { *out_value = (double)d.totalcurrent[0]; return true; }
        if (strcmp(field_name, "totalcurrent[1]") == 0) { *out_value = (double)d.totalcurrent[1]; return true; }
        if (strcmp(field_name, "totalcurrent[2]") == 0) { *out_value = (double)d.totalcurrent[2]; return true; }
        if (strcmp(field_name, "totalcurrent[3]") == 0) { *out_value = (double)d.totalcurrent[3]; return true; }
        if (strcmp(field_name, "rpm[0]") == 0) { *out_value = (double)d.rpm[0]; return true; }
        if (strcmp(field_name, "rpm[1]") == 0) { *out_value = (double)d.rpm[1]; return true; }
        if (strcmp(field_name, "rpm[2]") == 0) { *out_value = (double)d.rpm[2]; return true; }
        if (strcmp(field_name, "rpm[3]") == 0) { *out_value = (double)d.rpm[3]; return true; }
        if (strcmp(field_name, "count[0]") == 0) { *out_value = (double)d.count[0]; return true; }
        if (strcmp(field_name, "count[1]") == 0) { *out_value = (double)d.count[1]; return true; }
        if (strcmp(field_name, "count[2]") == 0) { *out_value = (double)d.count[2]; return true; }
        if (strcmp(field_name, "count[3]") == 0) { *out_value = (double)d.count[3]; return true; }
        return false;
    }
    case 11032: {
        mavlink_esc_telemetry_9_to_12_t d;
        mavlink_msg_esc_telemetry_9_to_12_decode(&raw, &d);
        if (strcmp(field_name, "temperature[0]") == 0) { *out_value = (double)d.temperature[0]; return true; }
        if (strcmp(field_name, "temperature[1]") == 0) { *out_value = (double)d.temperature[1]; return true; }
        if (strcmp(field_name, "temperature[2]") == 0) { *out_value = (double)d.temperature[2]; return true; }
        if (strcmp(field_name, "temperature[3]") == 0) { *out_value = (double)d.temperature[3]; return true; }
        if (strcmp(field_name, "voltage[0]") == 0) { *out_value = (double)d.voltage[0]; return true; }
        if (strcmp(field_name, "voltage[1]") == 0) { *out_value = (double)d.voltage[1]; return true; }
        if (strcmp(field_name, "voltage[2]") == 0) { *out_value = (double)d.voltage[2]; return true; }
        if (strcmp(field_name, "voltage[3]") == 0) { *out_value = (double)d.voltage[3]; return true; }
        if (strcmp(field_name, "current[0]") == 0) { *out_value = (double)d.current[0]; return true; }
        if (strcmp(field_name, "current[1]") == 0) { *out_value = (double)d.current[1]; return true; }
        if (strcmp(field_name, "current[2]") == 0) { *out_value = (double)d.current[2]; return true; }
        if (strcmp(field_name, "current[3]") == 0) { *out_value = (double)d.current[3]; return true; }
        if (strcmp(field_name, "totalcurrent[0]") == 0) { *out_value = (double)d.totalcurrent[0]; return true; }
        if (strcmp(field_name, "totalcurrent[1]") == 0) { *out_value = (double)d.totalcurrent[1]; return true; }
        if (strcmp(field_name, "totalcurrent[2]") == 0) { *out_value = (double)d.totalcurrent[2]; return true; }
        if (strcmp(field_name, "totalcurrent[3]") == 0) { *out_value = (double)d.totalcurrent[3]; return true; }
        if (strcmp(field_name, "rpm[0]") == 0) { *out_value = (double)d.rpm[0]; return true; }
        if (strcmp(field_name, "rpm[1]") == 0) { *out_value = (double)d.rpm[1]; return true; }
        if (strcmp(field_name, "rpm[2]") == 0) { *out_value = (double)d.rpm[2]; return true; }
        if (strcmp(field_name, "rpm[3]") == 0) { *out_value = (double)d.rpm[3]; return true; }
        if (strcmp(field_name, "count[0]") == 0) { *out_value = (double)d.count[0]; return true; }
        if (strcmp(field_name, "count[1]") == 0) { *out_value = (double)d.count[1]; return true; }
        if (strcmp(field_name, "count[2]") == 0) { *out_value = (double)d.count[2]; return true; }
        if (strcmp(field_name, "count[3]") == 0) { *out_value = (double)d.count[3]; return true; }
        return false;
    }
    case 11034: {
        mavlink_osd_param_config_reply_t d;
        mavlink_msg_osd_param_config_reply_decode(&raw, &d);
        if (strcmp(field_name, "request_id") == 0) { *out_value = (double)d.request_id; return true; }
        if (strcmp(field_name, "result") == 0) { *out_value = (double)d.result; return true; }
        return false;
    }
    case 11036: {
        mavlink_osd_param_show_config_reply_t d;
        mavlink_msg_osd_param_show_config_reply_decode(&raw, &d);
        if (strcmp(field_name, "request_id") == 0) { *out_value = (double)d.request_id; return true; }
        if (strcmp(field_name, "result") == 0) { *out_value = (double)d.result; return true; }
        if (strcmp(field_name, "config_type") == 0) { *out_value = (double)d.config_type; return true; }
        if (strcmp(field_name, "min_value") == 0) { *out_value = (double)d.min_value; return true; }
        if (strcmp(field_name, "max_value") == 0) { *out_value = (double)d.max_value; return true; }
        if (strcmp(field_name, "increment") == 0) { *out_value = (double)d.increment; return true; }
        return false;
    }
    case 11037: {
        mavlink_obstacle_distance_3d_t d;
        mavlink_msg_obstacle_distance_3d_decode(&raw, &d);
        if (strcmp(field_name, "time_boot_ms") == 0) { *out_value = (double)d.time_boot_ms; return true; }
        if (strcmp(field_name, "sensor_type") == 0) { *out_value = (double)d.sensor_type; return true; }
        if (strcmp(field_name, "frame") == 0) { *out_value = (double)d.frame; return true; }
        if (strcmp(field_name, "obstacle_id") == 0) { *out_value = (double)d.obstacle_id; return true; }
        if (strcmp(field_name, "x") == 0) { *out_value = (double)d.x; return true; }
        if (strcmp(field_name, "y") == 0) { *out_value = (double)d.y; return true; }
        if (strcmp(field_name, "z") == 0) { *out_value = (double)d.z; return true; }
        if (strcmp(field_name, "min_distance") == 0) { *out_value = (double)d.min_distance; return true; }
        if (strcmp(field_name, "max_distance") == 0) { *out_value = (double)d.max_distance; return true; }
        return false;
    }
    case 11038: {
        mavlink_water_depth_t d;
        mavlink_msg_water_depth_decode(&raw, &d);
        if (strcmp(field_name, "time_boot_ms") == 0) { *out_value = (double)d.time_boot_ms; return true; }
        if (strcmp(field_name, "id") == 0) { *out_value = (double)d.id; return true; }
        if (strcmp(field_name, "healthy") == 0) { *out_value = (double)d.healthy; return true; }
        if (strcmp(field_name, "lat") == 0) { *out_value = (double)d.lat; return true; }
        if (strcmp(field_name, "lng") == 0) { *out_value = (double)d.lng; return true; }
        if (strcmp(field_name, "alt") == 0) { *out_value = (double)d.alt; return true; }
        if (strcmp(field_name, "roll") == 0) { *out_value = (double)d.roll; return true; }
        if (strcmp(field_name, "pitch") == 0) { *out_value = (double)d.pitch; return true; }
        if (strcmp(field_name, "yaw") == 0) { *out_value = (double)d.yaw; return true; }
        if (strcmp(field_name, "distance") == 0) { *out_value = (double)d.distance; return true; }
        if (strcmp(field_name, "temperature") == 0) { *out_value = (double)d.temperature; return true; }
        return false;
    }
    case 11039: {
        mavlink_mcu_status_t d;
        mavlink_msg_mcu_status_decode(&raw, &d);
        if (strcmp(field_name, "id") == 0) { *out_value = (double)d.id; return true; }
        if (strcmp(field_name, "MCU_temperature") == 0) { *out_value = (double)d.MCU_temperature; return true; }
        if (strcmp(field_name, "MCU_voltage") == 0) { *out_value = (double)d.MCU_voltage; return true; }
        if (strcmp(field_name, "MCU_voltage_min") == 0) { *out_value = (double)d.MCU_voltage_min; return true; }
        if (strcmp(field_name, "MCU_voltage_max") == 0) { *out_value = (double)d.MCU_voltage_max; return true; }
        return false;
    }
    case 11040: {
        mavlink_esc_telemetry_13_to_16_t d;
        mavlink_msg_esc_telemetry_13_to_16_decode(&raw, &d);
        if (strcmp(field_name, "temperature[0]") == 0) { *out_value = (double)d.temperature[0]; return true; }
        if (strcmp(field_name, "temperature[1]") == 0) { *out_value = (double)d.temperature[1]; return true; }
        if (strcmp(field_name, "temperature[2]") == 0) { *out_value = (double)d.temperature[2]; return true; }
        if (strcmp(field_name, "temperature[3]") == 0) { *out_value = (double)d.temperature[3]; return true; }
        if (strcmp(field_name, "voltage[0]") == 0) { *out_value = (double)d.voltage[0]; return true; }
        if (strcmp(field_name, "voltage[1]") == 0) { *out_value = (double)d.voltage[1]; return true; }
        if (strcmp(field_name, "voltage[2]") == 0) { *out_value = (double)d.voltage[2]; return true; }
        if (strcmp(field_name, "voltage[3]") == 0) { *out_value = (double)d.voltage[3]; return true; }
        if (strcmp(field_name, "current[0]") == 0) { *out_value = (double)d.current[0]; return true; }
        if (strcmp(field_name, "current[1]") == 0) { *out_value = (double)d.current[1]; return true; }
        if (strcmp(field_name, "current[2]") == 0) { *out_value = (double)d.current[2]; return true; }
        if (strcmp(field_name, "current[3]") == 0) { *out_value = (double)d.current[3]; return true; }
        if (strcmp(field_name, "totalcurrent[0]") == 0) { *out_value = (double)d.totalcurrent[0]; return true; }
        if (strcmp(field_name, "totalcurrent[1]") == 0) { *out_value = (double)d.totalcurrent[1]; return true; }
        if (strcmp(field_name, "totalcurrent[2]") == 0) { *out_value = (double)d.totalcurrent[2]; return true; }
        if (strcmp(field_name, "totalcurrent[3]") == 0) { *out_value = (double)d.totalcurrent[3]; return true; }
        if (strcmp(field_name, "rpm[0]") == 0) { *out_value = (double)d.rpm[0]; return true; }
        if (strcmp(field_name, "rpm[1]") == 0) { *out_value = (double)d.rpm[1]; return true; }
        if (strcmp(field_name, "rpm[2]") == 0) { *out_value = (double)d.rpm[2]; return true; }
        if (strcmp(field_name, "rpm[3]") == 0) { *out_value = (double)d.rpm[3]; return true; }
        if (strcmp(field_name, "count[0]") == 0) { *out_value = (double)d.count[0]; return true; }
        if (strcmp(field_name, "count[1]") == 0) { *out_value = (double)d.count[1]; return true; }
        if (strcmp(field_name, "count[2]") == 0) { *out_value = (double)d.count[2]; return true; }
        if (strcmp(field_name, "count[3]") == 0) { *out_value = (double)d.count[3]; return true; }
        return false;
    }
    case 11041: {
        mavlink_esc_telemetry_17_to_20_t d;
        mavlink_msg_esc_telemetry_17_to_20_decode(&raw, &d);
        if (strcmp(field_name, "temperature[0]") == 0) { *out_value = (double)d.temperature[0]; return true; }
        if (strcmp(field_name, "temperature[1]") == 0) { *out_value = (double)d.temperature[1]; return true; }
        if (strcmp(field_name, "temperature[2]") == 0) { *out_value = (double)d.temperature[2]; return true; }
        if (strcmp(field_name, "temperature[3]") == 0) { *out_value = (double)d.temperature[3]; return true; }
        if (strcmp(field_name, "voltage[0]") == 0) { *out_value = (double)d.voltage[0]; return true; }
        if (strcmp(field_name, "voltage[1]") == 0) { *out_value = (double)d.voltage[1]; return true; }
        if (strcmp(field_name, "voltage[2]") == 0) { *out_value = (double)d.voltage[2]; return true; }
        if (strcmp(field_name, "voltage[3]") == 0) { *out_value = (double)d.voltage[3]; return true; }
        if (strcmp(field_name, "current[0]") == 0) { *out_value = (double)d.current[0]; return true; }
        if (strcmp(field_name, "current[1]") == 0) { *out_value = (double)d.current[1]; return true; }
        if (strcmp(field_name, "current[2]") == 0) { *out_value = (double)d.current[2]; return true; }
        if (strcmp(field_name, "current[3]") == 0) { *out_value = (double)d.current[3]; return true; }
        if (strcmp(field_name, "totalcurrent[0]") == 0) { *out_value = (double)d.totalcurrent[0]; return true; }
        if (strcmp(field_name, "totalcurrent[1]") == 0) { *out_value = (double)d.totalcurrent[1]; return true; }
        if (strcmp(field_name, "totalcurrent[2]") == 0) { *out_value = (double)d.totalcurrent[2]; return true; }
        if (strcmp(field_name, "totalcurrent[3]") == 0) { *out_value = (double)d.totalcurrent[3]; return true; }
        if (strcmp(field_name, "rpm[0]") == 0) { *out_value = (double)d.rpm[0]; return true; }
        if (strcmp(field_name, "rpm[1]") == 0) { *out_value = (double)d.rpm[1]; return true; }
        if (strcmp(field_name, "rpm[2]") == 0) { *out_value = (double)d.rpm[2]; return true; }
        if (strcmp(field_name, "rpm[3]") == 0) { *out_value = (double)d.rpm[3]; return true; }
        if (strcmp(field_name, "count[0]") == 0) { *out_value = (double)d.count[0]; return true; }
        if (strcmp(field_name, "count[1]") == 0) { *out_value = (double)d.count[1]; return true; }
        if (strcmp(field_name, "count[2]") == 0) { *out_value = (double)d.count[2]; return true; }
        if (strcmp(field_name, "count[3]") == 0) { *out_value = (double)d.count[3]; return true; }
        return false;
    }
    case 11042: {
        mavlink_esc_telemetry_21_to_24_t d;
        mavlink_msg_esc_telemetry_21_to_24_decode(&raw, &d);
        if (strcmp(field_name, "temperature[0]") == 0) { *out_value = (double)d.temperature[0]; return true; }
        if (strcmp(field_name, "temperature[1]") == 0) { *out_value = (double)d.temperature[1]; return true; }
        if (strcmp(field_name, "temperature[2]") == 0) { *out_value = (double)d.temperature[2]; return true; }
        if (strcmp(field_name, "temperature[3]") == 0) { *out_value = (double)d.temperature[3]; return true; }
        if (strcmp(field_name, "voltage[0]") == 0) { *out_value = (double)d.voltage[0]; return true; }
        if (strcmp(field_name, "voltage[1]") == 0) { *out_value = (double)d.voltage[1]; return true; }
        if (strcmp(field_name, "voltage[2]") == 0) { *out_value = (double)d.voltage[2]; return true; }
        if (strcmp(field_name, "voltage[3]") == 0) { *out_value = (double)d.voltage[3]; return true; }
        if (strcmp(field_name, "current[0]") == 0) { *out_value = (double)d.current[0]; return true; }
        if (strcmp(field_name, "current[1]") == 0) { *out_value = (double)d.current[1]; return true; }
        if (strcmp(field_name, "current[2]") == 0) { *out_value = (double)d.current[2]; return true; }
        if (strcmp(field_name, "current[3]") == 0) { *out_value = (double)d.current[3]; return true; }
        if (strcmp(field_name, "totalcurrent[0]") == 0) { *out_value = (double)d.totalcurrent[0]; return true; }
        if (strcmp(field_name, "totalcurrent[1]") == 0) { *out_value = (double)d.totalcurrent[1]; return true; }
        if (strcmp(field_name, "totalcurrent[2]") == 0) { *out_value = (double)d.totalcurrent[2]; return true; }
        if (strcmp(field_name, "totalcurrent[3]") == 0) { *out_value = (double)d.totalcurrent[3]; return true; }
        if (strcmp(field_name, "rpm[0]") == 0) { *out_value = (double)d.rpm[0]; return true; }
        if (strcmp(field_name, "rpm[1]") == 0) { *out_value = (double)d.rpm[1]; return true; }
        if (strcmp(field_name, "rpm[2]") == 0) { *out_value = (double)d.rpm[2]; return true; }
        if (strcmp(field_name, "rpm[3]") == 0) { *out_value = (double)d.rpm[3]; return true; }
        if (strcmp(field_name, "count[0]") == 0) { *out_value = (double)d.count[0]; return true; }
        if (strcmp(field_name, "count[1]") == 0) { *out_value = (double)d.count[1]; return true; }
        if (strcmp(field_name, "count[2]") == 0) { *out_value = (double)d.count[2]; return true; }
        if (strcmp(field_name, "count[3]") == 0) { *out_value = (double)d.count[3]; return true; }
        return false;
    }
    case 11043: {
        mavlink_esc_telemetry_25_to_28_t d;
        mavlink_msg_esc_telemetry_25_to_28_decode(&raw, &d);
        if (strcmp(field_name, "temperature[0]") == 0) { *out_value = (double)d.temperature[0]; return true; }
        if (strcmp(field_name, "temperature[1]") == 0) { *out_value = (double)d.temperature[1]; return true; }
        if (strcmp(field_name, "temperature[2]") == 0) { *out_value = (double)d.temperature[2]; return true; }
        if (strcmp(field_name, "temperature[3]") == 0) { *out_value = (double)d.temperature[3]; return true; }
        if (strcmp(field_name, "voltage[0]") == 0) { *out_value = (double)d.voltage[0]; return true; }
        if (strcmp(field_name, "voltage[1]") == 0) { *out_value = (double)d.voltage[1]; return true; }
        if (strcmp(field_name, "voltage[2]") == 0) { *out_value = (double)d.voltage[2]; return true; }
        if (strcmp(field_name, "voltage[3]") == 0) { *out_value = (double)d.voltage[3]; return true; }
        if (strcmp(field_name, "current[0]") == 0) { *out_value = (double)d.current[0]; return true; }
        if (strcmp(field_name, "current[1]") == 0) { *out_value = (double)d.current[1]; return true; }
        if (strcmp(field_name, "current[2]") == 0) { *out_value = (double)d.current[2]; return true; }
        if (strcmp(field_name, "current[3]") == 0) { *out_value = (double)d.current[3]; return true; }
        if (strcmp(field_name, "totalcurrent[0]") == 0) { *out_value = (double)d.totalcurrent[0]; return true; }
        if (strcmp(field_name, "totalcurrent[1]") == 0) { *out_value = (double)d.totalcurrent[1]; return true; }
        if (strcmp(field_name, "totalcurrent[2]") == 0) { *out_value = (double)d.totalcurrent[2]; return true; }
        if (strcmp(field_name, "totalcurrent[3]") == 0) { *out_value = (double)d.totalcurrent[3]; return true; }
        if (strcmp(field_name, "rpm[0]") == 0) { *out_value = (double)d.rpm[0]; return true; }
        if (strcmp(field_name, "rpm[1]") == 0) { *out_value = (double)d.rpm[1]; return true; }
        if (strcmp(field_name, "rpm[2]") == 0) { *out_value = (double)d.rpm[2]; return true; }
        if (strcmp(field_name, "rpm[3]") == 0) { *out_value = (double)d.rpm[3]; return true; }
        if (strcmp(field_name, "count[0]") == 0) { *out_value = (double)d.count[0]; return true; }
        if (strcmp(field_name, "count[1]") == 0) { *out_value = (double)d.count[1]; return true; }
        if (strcmp(field_name, "count[2]") == 0) { *out_value = (double)d.count[2]; return true; }
        if (strcmp(field_name, "count[3]") == 0) { *out_value = (double)d.count[3]; return true; }
        return false;
    }
    case 11044: {
        mavlink_esc_telemetry_29_to_32_t d;
        mavlink_msg_esc_telemetry_29_to_32_decode(&raw, &d);
        if (strcmp(field_name, "temperature[0]") == 0) { *out_value = (double)d.temperature[0]; return true; }
        if (strcmp(field_name, "temperature[1]") == 0) { *out_value = (double)d.temperature[1]; return true; }
        if (strcmp(field_name, "temperature[2]") == 0) { *out_value = (double)d.temperature[2]; return true; }
        if (strcmp(field_name, "temperature[3]") == 0) { *out_value = (double)d.temperature[3]; return true; }
        if (strcmp(field_name, "voltage[0]") == 0) { *out_value = (double)d.voltage[0]; return true; }
        if (strcmp(field_name, "voltage[1]") == 0) { *out_value = (double)d.voltage[1]; return true; }
        if (strcmp(field_name, "voltage[2]") == 0) { *out_value = (double)d.voltage[2]; return true; }
        if (strcmp(field_name, "voltage[3]") == 0) { *out_value = (double)d.voltage[3]; return true; }
        if (strcmp(field_name, "current[0]") == 0) { *out_value = (double)d.current[0]; return true; }
        if (strcmp(field_name, "current[1]") == 0) { *out_value = (double)d.current[1]; return true; }
        if (strcmp(field_name, "current[2]") == 0) { *out_value = (double)d.current[2]; return true; }
        if (strcmp(field_name, "current[3]") == 0) { *out_value = (double)d.current[3]; return true; }
        if (strcmp(field_name, "totalcurrent[0]") == 0) { *out_value = (double)d.totalcurrent[0]; return true; }
        if (strcmp(field_name, "totalcurrent[1]") == 0) { *out_value = (double)d.totalcurrent[1]; return true; }
        if (strcmp(field_name, "totalcurrent[2]") == 0) { *out_value = (double)d.totalcurrent[2]; return true; }
        if (strcmp(field_name, "totalcurrent[3]") == 0) { *out_value = (double)d.totalcurrent[3]; return true; }
        if (strcmp(field_name, "rpm[0]") == 0) { *out_value = (double)d.rpm[0]; return true; }
        if (strcmp(field_name, "rpm[1]") == 0) { *out_value = (double)d.rpm[1]; return true; }
        if (strcmp(field_name, "rpm[2]") == 0) { *out_value = (double)d.rpm[2]; return true; }
        if (strcmp(field_name, "rpm[3]") == 0) { *out_value = (double)d.rpm[3]; return true; }
        if (strcmp(field_name, "count[0]") == 0) { *out_value = (double)d.count[0]; return true; }
        if (strcmp(field_name, "count[1]") == 0) { *out_value = (double)d.count[1]; return true; }
        if (strcmp(field_name, "count[2]") == 0) { *out_value = (double)d.count[2]; return true; }
        if (strcmp(field_name, "count[3]") == 0) { *out_value = (double)d.count[3]; return true; }
        return false;
    }
    case 11060: {
        mavlink_named_value_string_t d;
        mavlink_msg_named_value_string_decode(&raw, &d);
        if (strcmp(field_name, "time_boot_ms") == 0) { *out_value = (double)d.time_boot_ms; return true; }
        return false;
    }
    case 12900: {
        mavlink_open_drone_id_basic_id_t d;
        mavlink_msg_open_drone_id_basic_id_decode(&raw, &d);
        if (strcmp(field_name, "target_system") == 0) { *out_value = (double)d.target_system; return true; }
        if (strcmp(field_name, "target_component") == 0) { *out_value = (double)d.target_component; return true; }
        if (strcmp(field_name, "id_type") == 0) { *out_value = (double)d.id_type; return true; }
        if (strcmp(field_name, "ua_type") == 0) { *out_value = (double)d.ua_type; return true; }
        return false;
    }
    case 12901: {
        mavlink_open_drone_id_location_t d;
        mavlink_msg_open_drone_id_location_decode(&raw, &d);
        if (strcmp(field_name, "target_system") == 0) { *out_value = (double)d.target_system; return true; }
        if (strcmp(field_name, "target_component") == 0) { *out_value = (double)d.target_component; return true; }
        if (strcmp(field_name, "status") == 0) { *out_value = (double)d.status; return true; }
        if (strcmp(field_name, "direction") == 0) { *out_value = (double)d.direction; return true; }
        if (strcmp(field_name, "speed_horizontal") == 0) { *out_value = (double)d.speed_horizontal; return true; }
        if (strcmp(field_name, "speed_vertical") == 0) { *out_value = (double)d.speed_vertical; return true; }
        if (strcmp(field_name, "latitude") == 0) { *out_value = (double)d.latitude; return true; }
        if (strcmp(field_name, "longitude") == 0) { *out_value = (double)d.longitude; return true; }
        if (strcmp(field_name, "altitude_barometric") == 0) { *out_value = (double)d.altitude_barometric; return true; }
        if (strcmp(field_name, "altitude_geodetic") == 0) { *out_value = (double)d.altitude_geodetic; return true; }
        if (strcmp(field_name, "height_reference") == 0) { *out_value = (double)d.height_reference; return true; }
        if (strcmp(field_name, "height") == 0) { *out_value = (double)d.height; return true; }
        if (strcmp(field_name, "horizontal_accuracy") == 0) { *out_value = (double)d.horizontal_accuracy; return true; }
        if (strcmp(field_name, "vertical_accuracy") == 0) { *out_value = (double)d.vertical_accuracy; return true; }
        if (strcmp(field_name, "barometer_accuracy") == 0) { *out_value = (double)d.barometer_accuracy; return true; }
        if (strcmp(field_name, "speed_accuracy") == 0) { *out_value = (double)d.speed_accuracy; return true; }
        if (strcmp(field_name, "timestamp") == 0) { *out_value = (double)d.timestamp; return true; }
        if (strcmp(field_name, "timestamp_accuracy") == 0) { *out_value = (double)d.timestamp_accuracy; return true; }
        return false;
    }
    case 12902: {
        mavlink_open_drone_id_authentication_t d;
        mavlink_msg_open_drone_id_authentication_decode(&raw, &d);
        if (strcmp(field_name, "target_system") == 0) { *out_value = (double)d.target_system; return true; }
        if (strcmp(field_name, "target_component") == 0) { *out_value = (double)d.target_component; return true; }
        if (strcmp(field_name, "authentication_type") == 0) { *out_value = (double)d.authentication_type; return true; }
        if (strcmp(field_name, "data_page") == 0) { *out_value = (double)d.data_page; return true; }
        if (strcmp(field_name, "last_page_index") == 0) { *out_value = (double)d.last_page_index; return true; }
        if (strcmp(field_name, "length") == 0) { *out_value = (double)d.length; return true; }
        if (strcmp(field_name, "timestamp") == 0) { *out_value = (double)d.timestamp; return true; }
        return false;
    }
    case 12903: {
        mavlink_open_drone_id_self_id_t d;
        mavlink_msg_open_drone_id_self_id_decode(&raw, &d);
        if (strcmp(field_name, "target_system") == 0) { *out_value = (double)d.target_system; return true; }
        if (strcmp(field_name, "target_component") == 0) { *out_value = (double)d.target_component; return true; }
        if (strcmp(field_name, "description_type") == 0) { *out_value = (double)d.description_type; return true; }
        return false;
    }
    case 12904: {
        mavlink_open_drone_id_system_t d;
        mavlink_msg_open_drone_id_system_decode(&raw, &d);
        if (strcmp(field_name, "target_system") == 0) { *out_value = (double)d.target_system; return true; }
        if (strcmp(field_name, "target_component") == 0) { *out_value = (double)d.target_component; return true; }
        if (strcmp(field_name, "operator_location_type") == 0) { *out_value = (double)d.operator_location_type; return true; }
        if (strcmp(field_name, "classification_type") == 0) { *out_value = (double)d.classification_type; return true; }
        if (strcmp(field_name, "operator_latitude") == 0) { *out_value = (double)d.operator_latitude; return true; }
        if (strcmp(field_name, "operator_longitude") == 0) { *out_value = (double)d.operator_longitude; return true; }
        if (strcmp(field_name, "area_count") == 0) { *out_value = (double)d.area_count; return true; }
        if (strcmp(field_name, "area_radius") == 0) { *out_value = (double)d.area_radius; return true; }
        if (strcmp(field_name, "area_ceiling") == 0) { *out_value = (double)d.area_ceiling; return true; }
        if (strcmp(field_name, "area_floor") == 0) { *out_value = (double)d.area_floor; return true; }
        if (strcmp(field_name, "category_eu") == 0) { *out_value = (double)d.category_eu; return true; }
        if (strcmp(field_name, "class_eu") == 0) { *out_value = (double)d.class_eu; return true; }
        if (strcmp(field_name, "operator_altitude_geo") == 0) { *out_value = (double)d.operator_altitude_geo; return true; }
        if (strcmp(field_name, "timestamp") == 0) { *out_value = (double)d.timestamp; return true; }
        return false;
    }
    case 12905: {
        mavlink_open_drone_id_operator_id_t d;
        mavlink_msg_open_drone_id_operator_id_decode(&raw, &d);
        if (strcmp(field_name, "target_system") == 0) { *out_value = (double)d.target_system; return true; }
        if (strcmp(field_name, "target_component") == 0) { *out_value = (double)d.target_component; return true; }
        if (strcmp(field_name, "operator_id_type") == 0) { *out_value = (double)d.operator_id_type; return true; }
        return false;
    }
    case 12915: {
        mavlink_open_drone_id_message_pack_t d;
        mavlink_msg_open_drone_id_message_pack_decode(&raw, &d);
        if (strcmp(field_name, "target_system") == 0) { *out_value = (double)d.target_system; return true; }
        if (strcmp(field_name, "target_component") == 0) { *out_value = (double)d.target_component; return true; }
        if (strcmp(field_name, "single_message_size") == 0) { *out_value = (double)d.single_message_size; return true; }
        if (strcmp(field_name, "msg_pack_size") == 0) { *out_value = (double)d.msg_pack_size; return true; }
        return false;
    }
    case 12918: {
        mavlink_open_drone_id_arm_status_t d;
        mavlink_msg_open_drone_id_arm_status_decode(&raw, &d);
        if (strcmp(field_name, "status") == 0) { *out_value = (double)d.status; return true; }
        return false;
    }
    case 12919: {
        mavlink_open_drone_id_system_update_t d;
        mavlink_msg_open_drone_id_system_update_decode(&raw, &d);
        if (strcmp(field_name, "target_system") == 0) { *out_value = (double)d.target_system; return true; }
        if (strcmp(field_name, "target_component") == 0) { *out_value = (double)d.target_component; return true; }
        if (strcmp(field_name, "operator_latitude") == 0) { *out_value = (double)d.operator_latitude; return true; }
        if (strcmp(field_name, "operator_longitude") == 0) { *out_value = (double)d.operator_longitude; return true; }
        if (strcmp(field_name, "operator_altitude_geo") == 0) { *out_value = (double)d.operator_altitude_geo; return true; }
        if (strcmp(field_name, "timestamp") == 0) { *out_value = (double)d.timestamp; return true; }
        return false;
    }
    case 12920: {
        mavlink_hygrometer_sensor_t d;
        mavlink_msg_hygrometer_sensor_decode(&raw, &d);
        if (strcmp(field_name, "id") == 0) { *out_value = (double)d.id; return true; }
        if (strcmp(field_name, "temperature") == 0) { *out_value = (double)d.temperature; return true; }
        if (strcmp(field_name, "humidity") == 0) { *out_value = (double)d.humidity; return true; }
        return false;
    }
    case 42000: {
        mavlink_icarous_heartbeat_t d;
        mavlink_msg_icarous_heartbeat_decode(&raw, &d);
        if (strcmp(field_name, "status") == 0) { *out_value = (double)d.status; return true; }
        return false;
    }
    case 42001: {
        mavlink_icarous_kinematic_bands_t d;
        mavlink_msg_icarous_kinematic_bands_decode(&raw, &d);
        if (strcmp(field_name, "numBands") == 0) { *out_value = (double)d.numBands; return true; }
        if (strcmp(field_name, "type1") == 0) { *out_value = (double)d.type1; return true; }
        if (strcmp(field_name, "min1") == 0) { *out_value = (double)d.min1; return true; }
        if (strcmp(field_name, "max1") == 0) { *out_value = (double)d.max1; return true; }
        if (strcmp(field_name, "type2") == 0) { *out_value = (double)d.type2; return true; }
        if (strcmp(field_name, "min2") == 0) { *out_value = (double)d.min2; return true; }
        if (strcmp(field_name, "max2") == 0) { *out_value = (double)d.max2; return true; }
        if (strcmp(field_name, "type3") == 0) { *out_value = (double)d.type3; return true; }
        if (strcmp(field_name, "min3") == 0) { *out_value = (double)d.min3; return true; }
        if (strcmp(field_name, "max3") == 0) { *out_value = (double)d.max3; return true; }
        if (strcmp(field_name, "type4") == 0) { *out_value = (double)d.type4; return true; }
        if (strcmp(field_name, "min4") == 0) { *out_value = (double)d.min4; return true; }
        if (strcmp(field_name, "max4") == 0) { *out_value = (double)d.max4; return true; }
        if (strcmp(field_name, "type5") == 0) { *out_value = (double)d.type5; return true; }
        if (strcmp(field_name, "min5") == 0) { *out_value = (double)d.min5; return true; }
        if (strcmp(field_name, "max5") == 0) { *out_value = (double)d.max5; return true; }
        return false;
    }
    case 50001: {
        mavlink_cubepilot_raw_rc_t d;
        mavlink_msg_cubepilot_raw_rc_decode(&raw, &d);
        return false;
    }
    case 50002: {
        mavlink_herelink_video_stream_information_t d;
        mavlink_msg_herelink_video_stream_information_decode(&raw, &d);
        if (strcmp(field_name, "camera_id") == 0) { *out_value = (double)d.camera_id; return true; }
        if (strcmp(field_name, "status") == 0) { *out_value = (double)d.status; return true; }
        if (strcmp(field_name, "framerate") == 0) { *out_value = (double)d.framerate; return true; }
        if (strcmp(field_name, "resolution_h") == 0) { *out_value = (double)d.resolution_h; return true; }
        if (strcmp(field_name, "resolution_v") == 0) { *out_value = (double)d.resolution_v; return true; }
        if (strcmp(field_name, "bitrate") == 0) { *out_value = (double)d.bitrate; return true; }
        if (strcmp(field_name, "rotation") == 0) { *out_value = (double)d.rotation; return true; }
        return false;
    }
    case 50003: {
        mavlink_herelink_telem_t d;
        mavlink_msg_herelink_telem_decode(&raw, &d);
        if (strcmp(field_name, "rssi") == 0) { *out_value = (double)d.rssi; return true; }
        if (strcmp(field_name, "snr") == 0) { *out_value = (double)d.snr; return true; }
        if (strcmp(field_name, "rf_freq") == 0) { *out_value = (double)d.rf_freq; return true; }
        if (strcmp(field_name, "link_bw") == 0) { *out_value = (double)d.link_bw; return true; }
        if (strcmp(field_name, "link_rate") == 0) { *out_value = (double)d.link_rate; return true; }
        if (strcmp(field_name, "cpu_temp") == 0) { *out_value = (double)d.cpu_temp; return true; }
        if (strcmp(field_name, "board_temp") == 0) { *out_value = (double)d.board_temp; return true; }
        return false;
    }
    case 50005: {
        mavlink_cubepilot_firmware_update_resp_t d;
        mavlink_msg_cubepilot_firmware_update_resp_decode(&raw, &d);
        if (strcmp(field_name, "target_system") == 0) { *out_value = (double)d.target_system; return true; }
        if (strcmp(field_name, "target_component") == 0) { *out_value = (double)d.target_component; return true; }
        if (strcmp(field_name, "offset") == 0) { *out_value = (double)d.offset; return true; }
        return false;
    }
    case 52001: {
        mavlink_airlink_auth_response_t d;
        mavlink_msg_airlink_auth_response_decode(&raw, &d);
        if (strcmp(field_name, "resp_type") == 0) { *out_value = (double)d.resp_type; return true; }
        return false;
    }
    default: return false;
    }
}
