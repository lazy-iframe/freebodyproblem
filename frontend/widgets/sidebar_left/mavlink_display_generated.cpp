// AUTO-GENERATED — do not edit manually.
// Regenerate: re-run CMake or call gen_mavlink_display.py directly.
#include "mavlink_display_generated.hpp"
#include "mavlink_display_shim.hpp"
#include <cstdio>
#include <cstring>

const char* mavlink_display_name(uint32_t id)
{
    switch (id) {
    case 0: return "HEARTBEAT";
    case 1: return "SYS_STATUS";
    case 2: return "SYSTEM_TIME";
    case 4: return "PING";
    case 5: return "CHANGE_OPERATOR_CONTROL";
    case 6: return "CHANGE_OPERATOR_CONTROL_ACK";
    case 7: return "AUTH_KEY";
    case 8: return "LINK_NODE_STATUS";
    case 11: return "SET_MODE";
    case 20: return "PARAM_REQUEST_READ";
    case 21: return "PARAM_REQUEST_LIST";
    case 22: return "PARAM_VALUE";
    case 23: return "PARAM_SET";
    case 24: return "GPS_RAW_INT";
    case 25: return "GPS_STATUS";
    case 26: return "SCALED_IMU";
    case 27: return "RAW_IMU";
    case 28: return "RAW_PRESSURE";
    case 29: return "SCALED_PRESSURE";
    case 30: return "ATTITUDE";
    case 31: return "ATTITUDE_QUATERNION";
    case 32: return "LOCAL_POSITION_NED";
    case 33: return "GLOBAL_POSITION_INT";
    case 34: return "RC_CHANNELS_SCALED";
    case 35: return "RC_CHANNELS_RAW";
    case 36: return "SERVO_OUTPUT_RAW";
    case 37: return "MISSION_REQUEST_PARTIAL_LIST";
    case 38: return "MISSION_WRITE_PARTIAL_LIST";
    case 39: return "MISSION_ITEM";
    case 40: return "MISSION_REQUEST";
    case 41: return "MISSION_SET_CURRENT";
    case 42: return "MISSION_CURRENT";
    case 43: return "MISSION_REQUEST_LIST";
    case 44: return "MISSION_COUNT";
    case 45: return "MISSION_CLEAR_ALL";
    case 46: return "MISSION_ITEM_REACHED";
    case 47: return "MISSION_ACK";
    case 48: return "SET_GPS_GLOBAL_ORIGIN";
    case 49: return "GPS_GLOBAL_ORIGIN";
    case 50: return "PARAM_MAP_RC";
    case 51: return "MISSION_REQUEST_INT";
    case 54: return "SAFETY_SET_ALLOWED_AREA";
    case 55: return "SAFETY_ALLOWED_AREA";
    case 61: return "ATTITUDE_QUATERNION_COV";
    case 62: return "NAV_CONTROLLER_OUTPUT";
    case 63: return "GLOBAL_POSITION_INT_COV";
    case 64: return "LOCAL_POSITION_NED_COV";
    case 65: return "RC_CHANNELS";
    case 66: return "REQUEST_DATA_STREAM";
    case 67: return "DATA_STREAM";
    case 69: return "MANUAL_CONTROL";
    case 70: return "RC_CHANNELS_OVERRIDE";
    case 73: return "MISSION_ITEM_INT";
    case 74: return "VFR_HUD";
    case 75: return "COMMAND_INT";
    case 76: return "COMMAND_LONG";
    case 77: return "COMMAND_ACK";
    case 80: return "COMMAND_CANCEL";
    case 81: return "MANUAL_SETPOINT";
    case 82: return "SET_ATTITUDE_TARGET";
    case 83: return "ATTITUDE_TARGET";
    case 84: return "SET_POSITION_TARGET_LOCAL_NED";
    case 85: return "POSITION_TARGET_LOCAL_NED";
    case 86: return "SET_POSITION_TARGET_GLOBAL_INT";
    case 87: return "POSITION_TARGET_GLOBAL_INT";
    case 89: return "LOCAL_POSITION_NED_SYSTEM_GLOBAL_OFFSET";
    case 90: return "HIL_STATE";
    case 91: return "HIL_CONTROLS";
    case 92: return "HIL_RC_INPUTS_RAW";
    case 93: return "HIL_ACTUATOR_CONTROLS";
    case 100: return "OPTICAL_FLOW";
    case 101: return "GLOBAL_VISION_POSITION_ESTIMATE";
    case 102: return "VISION_POSITION_ESTIMATE";
    case 103: return "VISION_SPEED_ESTIMATE";
    case 104: return "VICON_POSITION_ESTIMATE";
    case 105: return "HIGHRES_IMU";
    case 106: return "OPTICAL_FLOW_RAD";
    case 107: return "HIL_SENSOR";
    case 108: return "SIM_STATE";
    case 109: return "RADIO_STATUS";
    case 110: return "FILE_TRANSFER_PROTOCOL";
    case 111: return "TIMESYNC";
    case 112: return "CAMERA_TRIGGER";
    case 113: return "HIL_GPS";
    case 114: return "HIL_OPTICAL_FLOW";
    case 115: return "HIL_STATE_QUATERNION";
    case 116: return "SCALED_IMU2";
    case 117: return "LOG_REQUEST_LIST";
    case 118: return "LOG_ENTRY";
    case 119: return "LOG_REQUEST_DATA";
    case 120: return "LOG_DATA";
    case 121: return "LOG_ERASE";
    case 122: return "LOG_REQUEST_END";
    case 123: return "GPS_INJECT_DATA";
    case 124: return "GPS2_RAW";
    case 125: return "POWER_STATUS";
    case 126: return "SERIAL_CONTROL";
    case 127: return "GPS_RTK";
    case 128: return "GPS2_RTK";
    case 129: return "SCALED_IMU3";
    case 130: return "DATA_TRANSMISSION_HANDSHAKE";
    case 131: return "ENCAPSULATED_DATA";
    case 132: return "DISTANCE_SENSOR";
    case 133: return "TERRAIN_REQUEST";
    case 134: return "TERRAIN_DATA";
    case 135: return "TERRAIN_CHECK";
    case 136: return "TERRAIN_REPORT";
    case 137: return "SCALED_PRESSURE2";
    case 138: return "ATT_POS_MOCAP";
    case 139: return "SET_ACTUATOR_CONTROL_TARGET";
    case 140: return "ACTUATOR_CONTROL_TARGET";
    case 141: return "ALTITUDE";
    case 142: return "RESOURCE_REQUEST";
    case 143: return "SCALED_PRESSURE3";
    case 144: return "FOLLOW_TARGET";
    case 146: return "CONTROL_SYSTEM_STATE";
    case 147: return "BATTERY_STATUS";
    case 148: return "AUTOPILOT_VERSION";
    case 149: return "LANDING_TARGET";
    case 150: return "SENSOR_OFFSETS";
    case 151: return "SET_MAG_OFFSETS";
    case 152: return "MEMINFO";
    case 153: return "AP_ADC";
    case 154: return "DIGICAM_CONFIGURE";
    case 155: return "DIGICAM_CONTROL";
    case 156: return "MOUNT_CONFIGURE";
    case 157: return "MOUNT_CONTROL";
    case 158: return "MOUNT_STATUS";
    case 160: return "FENCE_POINT";
    case 161: return "FENCE_FETCH_POINT";
    case 162: return "FENCE_STATUS";
    case 163: return "AHRS";
    case 164: return "SIMSTATE";
    case 165: return "HWSTATUS";
    case 166: return "RADIO";
    case 167: return "LIMITS_STATUS";
    case 168: return "WIND";
    case 169: return "DATA16";
    case 170: return "DATA32";
    case 171: return "DATA64";
    case 172: return "DATA96";
    case 173: return "RANGEFINDER";
    case 174: return "AIRSPEED_AUTOCAL";
    case 175: return "RALLY_POINT";
    case 176: return "RALLY_FETCH_POINT";
    case 177: return "COMPASSMOT_STATUS";
    case 178: return "AHRS2";
    case 179: return "CAMERA_STATUS";
    case 180: return "CAMERA_FEEDBACK";
    case 181: return "BATTERY2";
    case 182: return "AHRS3";
    case 183: return "AUTOPILOT_VERSION_REQUEST";
    case 184: return "REMOTE_LOG_DATA_BLOCK";
    case 185: return "REMOTE_LOG_BLOCK_STATUS";
    case 186: return "LED_CONTROL";
    case 191: return "MAG_CAL_PROGRESS";
    case 192: return "MAG_CAL_REPORT";
    case 193: return "EKF_STATUS_REPORT";
    case 194: return "PID_TUNING";
    case 195: return "DEEPSTALL";
    case 200: return "GIMBAL_REPORT";
    case 201: return "GIMBAL_CONTROL";
    case 214: return "GIMBAL_TORQUE_CMD_REPORT";
    case 215: return "GOPRO_HEARTBEAT";
    case 216: return "GOPRO_GET_REQUEST";
    case 217: return "GOPRO_GET_RESPONSE";
    case 218: return "GOPRO_SET_REQUEST";
    case 219: return "GOPRO_SET_RESPONSE";
    case 225: return "EFI_STATUS";
    case 226: return "RPM";
    case 230: return "ESTIMATOR_STATUS";
    case 231: return "WIND_COV";
    case 232: return "GPS_INPUT";
    case 233: return "GPS_RTCM_DATA";
    case 234: return "HIGH_LATENCY";
    case 235: return "HIGH_LATENCY2";
    case 241: return "VIBRATION";
    case 242: return "HOME_POSITION";
    case 243: return "SET_HOME_POSITION";
    case 244: return "MESSAGE_INTERVAL";
    case 245: return "EXTENDED_SYS_STATE";
    case 246: return "ADSB_VEHICLE";
    case 247: return "COLLISION";
    case 248: return "V2_EXTENSION";
    case 249: return "MEMORY_VECT";
    case 250: return "DEBUG_VECT";
    case 251: return "NAMED_VALUE_FLOAT";
    case 252: return "NAMED_VALUE_INT";
    case 253: return "STATUSTEXT";
    case 254: return "DEBUG";
    case 256: return "SETUP_SIGNING";
    case 257: return "BUTTON_CHANGE";
    case 258: return "PLAY_TUNE";
    case 259: return "CAMERA_INFORMATION";
    case 260: return "CAMERA_SETTINGS";
    case 261: return "STORAGE_INFORMATION";
    case 262: return "CAMERA_CAPTURE_STATUS";
    case 263: return "CAMERA_IMAGE_CAPTURED";
    case 264: return "FLIGHT_INFORMATION";
    case 265: return "MOUNT_ORIENTATION";
    case 266: return "LOGGING_DATA";
    case 267: return "LOGGING_DATA_ACKED";
    case 268: return "LOGGING_ACK";
    case 269: return "VIDEO_STREAM_INFORMATION";
    case 270: return "VIDEO_STREAM_STATUS";
    case 271: return "CAMERA_FOV_STATUS";
    case 275: return "CAMERA_TRACKING_IMAGE_STATUS";
    case 276: return "CAMERA_TRACKING_GEO_STATUS";
    case 277: return "CAMERA_THERMAL_RANGE";
    case 280: return "GIMBAL_MANAGER_INFORMATION";
    case 281: return "GIMBAL_MANAGER_STATUS";
    case 282: return "GIMBAL_MANAGER_SET_ATTITUDE";
    case 283: return "GIMBAL_DEVICE_INFORMATION";
    case 284: return "GIMBAL_DEVICE_SET_ATTITUDE";
    case 285: return "GIMBAL_DEVICE_ATTITUDE_STATUS";
    case 286: return "AUTOPILOT_STATE_FOR_GIMBAL_DEVICE";
    case 287: return "GIMBAL_MANAGER_SET_PITCHYAW";
    case 288: return "GIMBAL_MANAGER_SET_MANUAL_CONTROL";
    case 290: return "ESC_INFO";
    case 291: return "ESC_STATUS";
    case 295: return "AIRSPEED";
    case 296: return "GLOBAL_POSITION_SENSOR";
    case 299: return "WIFI_CONFIG_AP";
    case 300: return "PROTOCOL_VERSION";
    case 301: return "AIS_VESSEL";
    case 310: return "UAVCAN_NODE_STATUS";
    case 311: return "UAVCAN_NODE_INFO";
    case 320: return "PARAM_EXT_REQUEST_READ";
    case 321: return "PARAM_EXT_REQUEST_LIST";
    case 322: return "PARAM_EXT_VALUE";
    case 323: return "PARAM_EXT_SET";
    case 324: return "PARAM_EXT_ACK";
    case 330: return "OBSTACLE_DISTANCE";
    case 331: return "ODOMETRY";
    case 332: return "TRAJECTORY_REPRESENTATION_WAYPOINTS";
    case 333: return "TRAJECTORY_REPRESENTATION_BEZIER";
    case 334: return "CELLULAR_STATUS";
    case 335: return "ISBD_LINK_STATUS";
    case 336: return "CELLULAR_CONFIG";
    case 339: return "RAW_RPM";
    case 340: return "UTM_GLOBAL_POSITION";
    case 345: return "PARAM_ERROR";
    case 350: return "DEBUG_FLOAT_ARRAY";
    case 360: return "ORBIT_EXECUTION_STATUS";
    case 361: return "FIGURE_EIGHT_EXECUTION_STATUS";
    case 370: return "SMART_BATTERY_INFO";
    case 371: return "FUEL_STATUS";
    case 372: return "BATTERY_INFO";
    case 373: return "GENERATOR_STATUS";
    case 375: return "ACTUATOR_OUTPUT_STATUS";
    case 376: return "RELAY_STATUS";
    case 380: return "TIME_ESTIMATE_TO_TARGET";
    case 385: return "TUNNEL";
    case 386: return "CAN_FRAME";
    case 387: return "CANFD_FRAME";
    case 388: return "CAN_FILTER_MODIFY";
    case 390: return "ONBOARD_COMPUTER_STATUS";
    case 395: return "COMPONENT_INFORMATION";
    case 396: return "COMPONENT_INFORMATION_BASIC";
    case 397: return "COMPONENT_METADATA";
    case 400: return "PLAY_TUNE_V2";
    case 401: return "SUPPORTED_TUNES";
    case 410: return "EVENT";
    case 411: return "CURRENT_EVENT_SEQUENCE";
    case 412: return "REQUEST_EVENT";
    case 413: return "RESPONSE_EVENT_ERROR";
    case 435: return "AVAILABLE_MODES";
    case 436: return "CURRENT_MODE";
    case 437: return "AVAILABLE_MODES_MONITOR";
    case 440: return "ILLUMINATOR_STATUS";
    case 9000: return "WHEEL_DISTANCE";
    case 9005: return "WINCH_STATUS";
    case 10001: return "UAVIONIX_ADSB_OUT_CFG";
    case 10002: return "UAVIONIX_ADSB_OUT_DYNAMIC";
    case 10003: return "UAVIONIX_ADSB_TRANSCEIVER_HEALTH_REPORT";
    case 10004: return "UAVIONIX_ADSB_OUT_CFG_REGISTRATION";
    case 10005: return "UAVIONIX_ADSB_OUT_CFG_FLIGHTID";
    case 10006: return "UAVIONIX_ADSB_GET";
    case 10007: return "UAVIONIX_ADSB_OUT_CONTROL";
    case 10008: return "UAVIONIX_ADSB_OUT_STATUS";
    case 10151: return "LOWEHEISER_GOV_EFI";
    case 11000: return "DEVICE_OP_READ";
    case 11001: return "DEVICE_OP_READ_REPLY";
    case 11002: return "DEVICE_OP_WRITE";
    case 11003: return "DEVICE_OP_WRITE_REPLY";
    case 11004: return "SECURE_COMMAND";
    case 11005: return "SECURE_COMMAND_REPLY";
    case 11010: return "ADAP_TUNING";
    case 11011: return "VISION_POSITION_DELTA";
    case 11020: return "AOA_SSA";
    case 11030: return "ESC_TELEMETRY_1_TO_4";
    case 11031: return "ESC_TELEMETRY_5_TO_8";
    case 11032: return "ESC_TELEMETRY_9_TO_12";
    case 11033: return "OSD_PARAM_CONFIG";
    case 11034: return "OSD_PARAM_CONFIG_REPLY";
    case 11035: return "OSD_PARAM_SHOW_CONFIG";
    case 11036: return "OSD_PARAM_SHOW_CONFIG_REPLY";
    case 11037: return "OBSTACLE_DISTANCE_3D";
    case 11038: return "WATER_DEPTH";
    case 11039: return "MCU_STATUS";
    case 11040: return "ESC_TELEMETRY_13_TO_16";
    case 11041: return "ESC_TELEMETRY_17_TO_20";
    case 11042: return "ESC_TELEMETRY_21_TO_24";
    case 11043: return "ESC_TELEMETRY_25_TO_28";
    case 11044: return "ESC_TELEMETRY_29_TO_32";
    case 11060: return "NAMED_VALUE_STRING";
    case 12900: return "OPEN_DRONE_ID_BASIC_ID";
    case 12901: return "OPEN_DRONE_ID_LOCATION";
    case 12902: return "OPEN_DRONE_ID_AUTHENTICATION";
    case 12903: return "OPEN_DRONE_ID_SELF_ID";
    case 12904: return "OPEN_DRONE_ID_SYSTEM";
    case 12905: return "OPEN_DRONE_ID_OPERATOR_ID";
    case 12915: return "OPEN_DRONE_ID_MESSAGE_PACK";
    case 12918: return "OPEN_DRONE_ID_ARM_STATUS";
    case 12919: return "OPEN_DRONE_ID_SYSTEM_UPDATE";
    case 12920: return "HYGROMETER_SENSOR";
    case 42000: return "ICAROUS_HEARTBEAT";
    case 42001: return "ICAROUS_KINEMATIC_BANDS";
    case 50001: return "CUBEPILOT_RAW_RC";
    case 50002: return "HERELINK_VIDEO_STREAM_INFORMATION";
    case 50003: return "HERELINK_TELEM";
    case 50004: return "CUBEPILOT_FIRMWARE_UPDATE_START";
    case 50005: return "CUBEPILOT_FIRMWARE_UPDATE_RESP";
    case 52000: return "AIRLINK_AUTH";
    case 52001: return "AIRLINK_AUTH_RESPONSE";
    default: return nullptr;
    }
}

void mavlink_display_draw(uint32_t id, const mavlink_message_t& raw)
{
    switch (id) {
    case 0: { // HEARTBEAT
        mavlink_heartbeat_t d;
        mavlink_msg_heartbeat_decode(&raw, &d);
        display_kvf("type", "%u", (unsigned)d.type);
        display_kvf("autopilot", "%u", (unsigned)d.autopilot);
        display_kvf("base_mode", "%u", (unsigned)d.base_mode);
        display_kvf("custom_mode", "%u", (unsigned)d.custom_mode);
        display_kvf("system_status", "%u", (unsigned)d.system_status);
        display_kvf("mavlink_version", "%u", (unsigned)d.mavlink_version);
        break;
    }
    case 1: { // SYS_STATUS
        mavlink_sys_status_t d;
        mavlink_msg_sys_status_decode(&raw, &d);
        display_kvf("onboard_control_sensors_present", "0x%04x", (unsigned)d.onboard_control_sensors_present);
        display_kvf("onboard_control_sensors_enabled", "0x%04x", (unsigned)d.onboard_control_sensors_enabled);
        display_kvf("onboard_control_sensors_health", "0x%04x", (unsigned)d.onboard_control_sensors_health);
        display_kvf("load (d%)", "%u", (unsigned)d.load);
        display_kvf("voltage_battery (mV)", "%u", (unsigned)d.voltage_battery);
        display_kvf("current_battery (cA)", "%d", (int)d.current_battery);
        display_kvf("battery_remaining (%)", "%d", (int)d.battery_remaining);
        display_kvf("drop_rate_comm (c%)", "%u", (unsigned)d.drop_rate_comm);
        display_kvf("errors_comm", "%u", (unsigned)d.errors_comm);
        display_kvf("errors_count1", "%u", (unsigned)d.errors_count1);
        display_kvf("errors_count2", "%u", (unsigned)d.errors_count2);
        display_kvf("errors_count3", "%u", (unsigned)d.errors_count3);
        display_kvf("errors_count4", "%u", (unsigned)d.errors_count4);
        display_kvf("onboard_control_sensors_present_extended", "0x%04x", (unsigned)d.onboard_control_sensors_present_extended);
        display_kvf("onboard_control_sensors_enabled_extended", "0x%04x", (unsigned)d.onboard_control_sensors_enabled_extended);
        display_kvf("onboard_control_sensors_health_extended", "0x%04x", (unsigned)d.onboard_control_sensors_health_extended);
        break;
    }
    case 2: { // SYSTEM_TIME
        mavlink_system_time_t d;
        mavlink_msg_system_time_decode(&raw, &d);
        display_kvf("time_unix_usec (us)", "%llu", (unsigned long long)d.time_unix_usec);
        display_kvf("time_boot_ms (ms)", "%u", (unsigned)d.time_boot_ms);
        break;
    }
    case 4: { // PING
        mavlink_ping_t d;
        mavlink_msg_ping_decode(&raw, &d);
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        display_kvf("seq", "%u", (unsigned)d.seq);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        break;
    }
    case 5: { // CHANGE_OPERATOR_CONTROL
        mavlink_change_operator_control_t d;
        mavlink_msg_change_operator_control_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("control_request", "%u", (unsigned)d.control_request);
        display_kvf("version", "%u", (unsigned)d.version);
        {
            char _s[26];
            memcpy(_s, d.passkey, 25);
            _s[25] = '\0';
            display_kv("passkey", _s);
        }
        break;
    }
    case 6: { // CHANGE_OPERATOR_CONTROL_ACK
        mavlink_change_operator_control_ack_t d;
        mavlink_msg_change_operator_control_ack_decode(&raw, &d);
        display_kvf("gcs_system_id", "%u", (unsigned)d.gcs_system_id);
        display_kvf("control_request", "%u", (unsigned)d.control_request);
        display_kvf("ack", "%u", (unsigned)d.ack);
        break;
    }
    case 7: { // AUTH_KEY
        mavlink_auth_key_t d;
        mavlink_msg_auth_key_decode(&raw, &d);
        {
            char _s[33];
            memcpy(_s, d.key, 32);
            _s[32] = '\0';
            display_kv("key", _s);
        }
        break;
    }
    case 8: { // LINK_NODE_STATUS
        mavlink_link_node_status_t d;
        mavlink_msg_link_node_status_decode(&raw, &d);
        display_kvf("timestamp (ms)", "%llu", (unsigned long long)d.timestamp);
        display_kvf("tx_buf (%)", "%u", (unsigned)d.tx_buf);
        display_kvf("rx_buf (%)", "%u", (unsigned)d.rx_buf);
        display_kvf("tx_rate (bytes/s)", "%u", (unsigned)d.tx_rate);
        display_kvf("rx_rate (bytes/s)", "%u", (unsigned)d.rx_rate);
        display_kvf("rx_parse_err (bytes)", "%u", (unsigned)d.rx_parse_err);
        display_kvf("tx_overflows (bytes)", "%u", (unsigned)d.tx_overflows);
        display_kvf("rx_overflows (bytes)", "%u", (unsigned)d.rx_overflows);
        display_kvf("messages_sent", "%u", (unsigned)d.messages_sent);
        display_kvf("messages_received", "%u", (unsigned)d.messages_received);
        display_kvf("messages_lost", "%u", (unsigned)d.messages_lost);
        break;
    }
    case 11: { // SET_MODE
        mavlink_set_mode_t d;
        mavlink_msg_set_mode_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("base_mode", "%u", (unsigned)d.base_mode);
        display_kvf("custom_mode", "%u", (unsigned)d.custom_mode);
        break;
    }
    case 20: { // PARAM_REQUEST_READ
        mavlink_param_request_read_t d;
        mavlink_msg_param_request_read_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        {
            char _s[17];
            memcpy(_s, d.param_id, 16);
            _s[16] = '\0';
            display_kv("param_id", _s);
        }
        display_kvf("param_index", "%d", (int)d.param_index);
        break;
    }
    case 21: { // PARAM_REQUEST_LIST
        mavlink_param_request_list_t d;
        mavlink_msg_param_request_list_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        break;
    }
    case 22: { // PARAM_VALUE
        mavlink_param_value_t d;
        mavlink_msg_param_value_decode(&raw, &d);
        {
            char _s[17];
            memcpy(_s, d.param_id, 16);
            _s[16] = '\0';
            display_kv("param_id", _s);
        }
        display_kvf("param_value", "%.4g", (double)d.param_value);
        display_kvf("param_type", "%u", (unsigned)d.param_type);
        display_kvf("param_count", "%u", (unsigned)d.param_count);
        display_kvf("param_index", "%u", (unsigned)d.param_index);
        break;
    }
    case 23: { // PARAM_SET
        mavlink_param_set_t d;
        mavlink_msg_param_set_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        {
            char _s[17];
            memcpy(_s, d.param_id, 16);
            _s[16] = '\0';
            display_kv("param_id", _s);
        }
        display_kvf("param_value", "%.4g", (double)d.param_value);
        display_kvf("param_type", "%u", (unsigned)d.param_type);
        break;
    }
    case 24: { // GPS_RAW_INT
        mavlink_gps_raw_int_t d;
        mavlink_msg_gps_raw_int_decode(&raw, &d);
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        display_kvf("fix_type", "%u", (unsigned)d.fix_type);
        display_kvf("lat (degE7)", "%d", (int)d.lat);
        display_kvf("lon (degE7)", "%d", (int)d.lon);
        display_kvf("alt (mm)", "%d", (int)d.alt);
        display_kvf("eph", "%u", (unsigned)d.eph);
        display_kvf("epv", "%u", (unsigned)d.epv);
        display_kvf("vel (cm/s)", "%u", (unsigned)d.vel);
        display_kvf("cog (cdeg)", "%u", (unsigned)d.cog);
        display_kvf("satellites_visible", "%u", (unsigned)d.satellites_visible);
        display_kvf("alt_ellipsoid (mm)", "%d", (int)d.alt_ellipsoid);
        display_kvf("h_acc (mm)", "%u", (unsigned)d.h_acc);
        display_kvf("v_acc (mm)", "%u", (unsigned)d.v_acc);
        display_kvf("vel_acc (mm/s)", "%u", (unsigned)d.vel_acc);
        display_kvf("hdg_acc (degE5)", "%u", (unsigned)d.hdg_acc);
        display_kvf("yaw (cdeg)", "%u", (unsigned)d.yaw);
        break;
    }
    case 25: { // GPS_STATUS
        mavlink_gps_status_t d;
        mavlink_msg_gps_status_decode(&raw, &d);
        display_kvf("satellites_visible", "%u", (unsigned)d.satellites_visible);
        display_kv("satellite_prn", "[20 elements]");
        display_kv("satellite_used", "[20 elements]");
        display_kv("satellite_elevation (deg)", "[20 elements]");
        display_kv("satellite_azimuth (deg)", "[20 elements]");
        display_kv("satellite_snr (dB)", "[20 elements]");
        break;
    }
    case 26: { // SCALED_IMU
        mavlink_scaled_imu_t d;
        mavlink_msg_scaled_imu_decode(&raw, &d);
        display_kvf("time_boot_ms (ms)", "%u", (unsigned)d.time_boot_ms);
        display_kvf("xacc (mG)", "%d", (int)d.xacc);
        display_kvf("yacc (mG)", "%d", (int)d.yacc);
        display_kvf("zacc (mG)", "%d", (int)d.zacc);
        display_kvf("xgyro (mrad/s)", "%d", (int)d.xgyro);
        display_kvf("ygyro (mrad/s)", "%d", (int)d.ygyro);
        display_kvf("zgyro (mrad/s)", "%d", (int)d.zgyro);
        display_kvf("xmag (mgauss)", "%d", (int)d.xmag);
        display_kvf("ymag (mgauss)", "%d", (int)d.ymag);
        display_kvf("zmag (mgauss)", "%d", (int)d.zmag);
        display_kvf("temperature (cdegC)", "%d", (int)d.temperature);
        break;
    }
    case 27: { // RAW_IMU
        mavlink_raw_imu_t d;
        mavlink_msg_raw_imu_decode(&raw, &d);
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        display_kvf("xacc", "%d", (int)d.xacc);
        display_kvf("yacc", "%d", (int)d.yacc);
        display_kvf("zacc", "%d", (int)d.zacc);
        display_kvf("xgyro", "%d", (int)d.xgyro);
        display_kvf("ygyro", "%d", (int)d.ygyro);
        display_kvf("zgyro", "%d", (int)d.zgyro);
        display_kvf("xmag", "%d", (int)d.xmag);
        display_kvf("ymag", "%d", (int)d.ymag);
        display_kvf("zmag", "%d", (int)d.zmag);
        display_kvf("id", "%u", (unsigned)d.id);
        display_kvf("temperature (cdegC)", "%d", (int)d.temperature);
        break;
    }
    case 28: { // RAW_PRESSURE
        mavlink_raw_pressure_t d;
        mavlink_msg_raw_pressure_decode(&raw, &d);
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        display_kvf("press_abs", "%d", (int)d.press_abs);
        display_kvf("press_diff1", "%d", (int)d.press_diff1);
        display_kvf("press_diff2", "%d", (int)d.press_diff2);
        display_kvf("temperature", "%d", (int)d.temperature);
        break;
    }
    case 29: { // SCALED_PRESSURE
        mavlink_scaled_pressure_t d;
        mavlink_msg_scaled_pressure_decode(&raw, &d);
        display_kvf("time_boot_ms (ms)", "%u", (unsigned)d.time_boot_ms);
        display_kvf("press_abs (hPa)", "%.4g", (double)d.press_abs);
        display_kvf("press_diff (hPa)", "%.4g", (double)d.press_diff);
        display_kvf("temperature (cdegC)", "%d", (int)d.temperature);
        display_kvf("temperature_press_diff (cdegC)", "%d", (int)d.temperature_press_diff);
        break;
    }
    case 30: { // ATTITUDE
        mavlink_attitude_t d;
        mavlink_msg_attitude_decode(&raw, &d);
        display_kvf("time_boot_ms (ms)", "%u", (unsigned)d.time_boot_ms);
        display_kvf("roll (rad)", "%.4g", (double)d.roll);
        display_kvf("pitch (rad)", "%.4g", (double)d.pitch);
        display_kvf("yaw (rad)", "%.4g", (double)d.yaw);
        display_kvf("rollspeed (rad/s)", "%.4g", (double)d.rollspeed);
        display_kvf("pitchspeed (rad/s)", "%.4g", (double)d.pitchspeed);
        display_kvf("yawspeed (rad/s)", "%.4g", (double)d.yawspeed);
        break;
    }
    case 31: { // ATTITUDE_QUATERNION
        mavlink_attitude_quaternion_t d;
        mavlink_msg_attitude_quaternion_decode(&raw, &d);
        display_kvf("time_boot_ms (ms)", "%u", (unsigned)d.time_boot_ms);
        display_kvf("q1", "%.4g", (double)d.q1);
        display_kvf("q2", "%.4g", (double)d.q2);
        display_kvf("q3", "%.4g", (double)d.q3);
        display_kvf("q4", "%.4g", (double)d.q4);
        display_kvf("rollspeed (rad/s)", "%.4g", (double)d.rollspeed);
        display_kvf("pitchspeed (rad/s)", "%.4g", (double)d.pitchspeed);
        display_kvf("yawspeed (rad/s)", "%.4g", (double)d.yawspeed);
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%.4g" : "%.4g", (double)d.repr_offset_q[_i]);
            (void)_n; display_kv("repr_offset_q", _arr);
        }
        break;
    }
    case 32: { // LOCAL_POSITION_NED
        mavlink_local_position_ned_t d;
        mavlink_msg_local_position_ned_decode(&raw, &d);
        display_kvf("time_boot_ms (ms)", "%u", (unsigned)d.time_boot_ms);
        display_kvf("x (m)", "%.4g", (double)d.x);
        display_kvf("y (m)", "%.4g", (double)d.y);
        display_kvf("z (m)", "%.4g", (double)d.z);
        display_kvf("vx (m/s)", "%.4g", (double)d.vx);
        display_kvf("vy (m/s)", "%.4g", (double)d.vy);
        display_kvf("vz (m/s)", "%.4g", (double)d.vz);
        break;
    }
    case 33: { // GLOBAL_POSITION_INT
        mavlink_global_position_int_t d;
        mavlink_msg_global_position_int_decode(&raw, &d);
        display_kvf("time_boot_ms (ms)", "%u", (unsigned)d.time_boot_ms);
        display_kvf("lat (degE7)", "%d", (int)d.lat);
        display_kvf("lon (degE7)", "%d", (int)d.lon);
        display_kvf("alt (mm)", "%d", (int)d.alt);
        display_kvf("relative_alt (mm)", "%d", (int)d.relative_alt);
        display_kvf("vx (cm/s)", "%d", (int)d.vx);
        display_kvf("vy (cm/s)", "%d", (int)d.vy);
        display_kvf("vz (cm/s)", "%d", (int)d.vz);
        display_kvf("hdg (cdeg)", "%u", (unsigned)d.hdg);
        break;
    }
    case 34: { // RC_CHANNELS_SCALED
        mavlink_rc_channels_scaled_t d;
        mavlink_msg_rc_channels_scaled_decode(&raw, &d);
        display_kvf("time_boot_ms (ms)", "%u", (unsigned)d.time_boot_ms);
        display_kvf("port", "%u", (unsigned)d.port);
        display_kvf("chan1_scaled", "%d", (int)d.chan1_scaled);
        display_kvf("chan2_scaled", "%d", (int)d.chan2_scaled);
        display_kvf("chan3_scaled", "%d", (int)d.chan3_scaled);
        display_kvf("chan4_scaled", "%d", (int)d.chan4_scaled);
        display_kvf("chan5_scaled", "%d", (int)d.chan5_scaled);
        display_kvf("chan6_scaled", "%d", (int)d.chan6_scaled);
        display_kvf("chan7_scaled", "%d", (int)d.chan7_scaled);
        display_kvf("chan8_scaled", "%d", (int)d.chan8_scaled);
        display_kvf("rssi", "%u", (unsigned)d.rssi);
        break;
    }
    case 35: { // RC_CHANNELS_RAW
        mavlink_rc_channels_raw_t d;
        mavlink_msg_rc_channels_raw_decode(&raw, &d);
        display_kvf("time_boot_ms (ms)", "%u", (unsigned)d.time_boot_ms);
        display_kvf("port", "%u", (unsigned)d.port);
        display_kvf("chan1_raw (us)", "%u", (unsigned)d.chan1_raw);
        display_kvf("chan2_raw (us)", "%u", (unsigned)d.chan2_raw);
        display_kvf("chan3_raw (us)", "%u", (unsigned)d.chan3_raw);
        display_kvf("chan4_raw (us)", "%u", (unsigned)d.chan4_raw);
        display_kvf("chan5_raw (us)", "%u", (unsigned)d.chan5_raw);
        display_kvf("chan6_raw (us)", "%u", (unsigned)d.chan6_raw);
        display_kvf("chan7_raw (us)", "%u", (unsigned)d.chan7_raw);
        display_kvf("chan8_raw (us)", "%u", (unsigned)d.chan8_raw);
        display_kvf("rssi", "%u", (unsigned)d.rssi);
        break;
    }
    case 36: { // SERVO_OUTPUT_RAW
        mavlink_servo_output_raw_t d;
        mavlink_msg_servo_output_raw_decode(&raw, &d);
        display_kvf("time_usec (us)", "%u", (unsigned)d.time_usec);
        display_kvf("port", "%u", (unsigned)d.port);
        display_kvf("servo1_raw (us)", "%u", (unsigned)d.servo1_raw);
        display_kvf("servo2_raw (us)", "%u", (unsigned)d.servo2_raw);
        display_kvf("servo3_raw (us)", "%u", (unsigned)d.servo3_raw);
        display_kvf("servo4_raw (us)", "%u", (unsigned)d.servo4_raw);
        display_kvf("servo5_raw (us)", "%u", (unsigned)d.servo5_raw);
        display_kvf("servo6_raw (us)", "%u", (unsigned)d.servo6_raw);
        display_kvf("servo7_raw (us)", "%u", (unsigned)d.servo7_raw);
        display_kvf("servo8_raw (us)", "%u", (unsigned)d.servo8_raw);
        display_kvf("servo9_raw (us)", "%u", (unsigned)d.servo9_raw);
        display_kvf("servo10_raw (us)", "%u", (unsigned)d.servo10_raw);
        display_kvf("servo11_raw (us)", "%u", (unsigned)d.servo11_raw);
        display_kvf("servo12_raw (us)", "%u", (unsigned)d.servo12_raw);
        display_kvf("servo13_raw (us)", "%u", (unsigned)d.servo13_raw);
        display_kvf("servo14_raw (us)", "%u", (unsigned)d.servo14_raw);
        display_kvf("servo15_raw (us)", "%u", (unsigned)d.servo15_raw);
        display_kvf("servo16_raw (us)", "%u", (unsigned)d.servo16_raw);
        break;
    }
    case 37: { // MISSION_REQUEST_PARTIAL_LIST
        mavlink_mission_request_partial_list_t d;
        mavlink_msg_mission_request_partial_list_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("start_index", "%d", (int)d.start_index);
        display_kvf("end_index", "%d", (int)d.end_index);
        display_kvf("mission_type", "%u", (unsigned)d.mission_type);
        break;
    }
    case 38: { // MISSION_WRITE_PARTIAL_LIST
        mavlink_mission_write_partial_list_t d;
        mavlink_msg_mission_write_partial_list_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("start_index", "%d", (int)d.start_index);
        display_kvf("end_index", "%d", (int)d.end_index);
        display_kvf("mission_type", "%u", (unsigned)d.mission_type);
        break;
    }
    case 39: { // MISSION_ITEM
        mavlink_mission_item_t d;
        mavlink_msg_mission_item_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("seq", "%u", (unsigned)d.seq);
        display_kvf("frame", "%u", (unsigned)d.frame);
        display_kvf("command", "%u", (unsigned)d.command);
        display_kvf("current", "%u", (unsigned)d.current);
        display_kvf("autocontinue", "%u", (unsigned)d.autocontinue);
        display_kvf("param1", "%.4g", (double)d.param1);
        display_kvf("param2", "%.4g", (double)d.param2);
        display_kvf("param3", "%.4g", (double)d.param3);
        display_kvf("param4", "%.4g", (double)d.param4);
        display_kvf("x", "%.4g", (double)d.x);
        display_kvf("y", "%.4g", (double)d.y);
        display_kvf("z", "%.4g", (double)d.z);
        display_kvf("mission_type", "%u", (unsigned)d.mission_type);
        break;
    }
    case 40: { // MISSION_REQUEST
        mavlink_mission_request_t d;
        mavlink_msg_mission_request_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("seq", "%u", (unsigned)d.seq);
        display_kvf("mission_type", "%u", (unsigned)d.mission_type);
        break;
    }
    case 41: { // MISSION_SET_CURRENT
        mavlink_mission_set_current_t d;
        mavlink_msg_mission_set_current_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("seq", "%u", (unsigned)d.seq);
        break;
    }
    case 42: { // MISSION_CURRENT
        mavlink_mission_current_t d;
        mavlink_msg_mission_current_decode(&raw, &d);
        display_kvf("seq", "%u", (unsigned)d.seq);
        display_kvf("total", "%u", (unsigned)d.total);
        display_kvf("mission_state", "%u", (unsigned)d.mission_state);
        display_kvf("mission_mode", "%u", (unsigned)d.mission_mode);
        display_kvf("mission_id", "%u", (unsigned)d.mission_id);
        display_kvf("fence_id", "%u", (unsigned)d.fence_id);
        display_kvf("rally_points_id", "%u", (unsigned)d.rally_points_id);
        break;
    }
    case 43: { // MISSION_REQUEST_LIST
        mavlink_mission_request_list_t d;
        mavlink_msg_mission_request_list_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("mission_type", "%u", (unsigned)d.mission_type);
        break;
    }
    case 44: { // MISSION_COUNT
        mavlink_mission_count_t d;
        mavlink_msg_mission_count_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("count", "%u", (unsigned)d.count);
        display_kvf("mission_type", "%u", (unsigned)d.mission_type);
        display_kvf("opaque_id", "%u", (unsigned)d.opaque_id);
        break;
    }
    case 45: { // MISSION_CLEAR_ALL
        mavlink_mission_clear_all_t d;
        mavlink_msg_mission_clear_all_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("mission_type", "%u", (unsigned)d.mission_type);
        break;
    }
    case 46: { // MISSION_ITEM_REACHED
        mavlink_mission_item_reached_t d;
        mavlink_msg_mission_item_reached_decode(&raw, &d);
        display_kvf("seq", "%u", (unsigned)d.seq);
        break;
    }
    case 47: { // MISSION_ACK
        mavlink_mission_ack_t d;
        mavlink_msg_mission_ack_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("type", "%u", (unsigned)d.type);
        display_kvf("mission_type", "%u", (unsigned)d.mission_type);
        display_kvf("opaque_id", "%u", (unsigned)d.opaque_id);
        break;
    }
    case 48: { // SET_GPS_GLOBAL_ORIGIN
        mavlink_set_gps_global_origin_t d;
        mavlink_msg_set_gps_global_origin_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("latitude (degE7)", "%d", (int)d.latitude);
        display_kvf("longitude (degE7)", "%d", (int)d.longitude);
        display_kvf("altitude (mm)", "%d", (int)d.altitude);
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        break;
    }
    case 49: { // GPS_GLOBAL_ORIGIN
        mavlink_gps_global_origin_t d;
        mavlink_msg_gps_global_origin_decode(&raw, &d);
        display_kvf("latitude (degE7)", "%d", (int)d.latitude);
        display_kvf("longitude (degE7)", "%d", (int)d.longitude);
        display_kvf("altitude (mm)", "%d", (int)d.altitude);
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        break;
    }
    case 50: { // PARAM_MAP_RC
        mavlink_param_map_rc_t d;
        mavlink_msg_param_map_rc_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        {
            char _s[17];
            memcpy(_s, d.param_id, 16);
            _s[16] = '\0';
            display_kv("param_id", _s);
        }
        display_kvf("param_index", "%d", (int)d.param_index);
        display_kvf("parameter_rc_channel_index", "%u", (unsigned)d.parameter_rc_channel_index);
        display_kvf("param_value0", "%.4g", (double)d.param_value0);
        display_kvf("scale", "%.4g", (double)d.scale);
        display_kvf("param_value_min", "%.4g", (double)d.param_value_min);
        display_kvf("param_value_max", "%.4g", (double)d.param_value_max);
        break;
    }
    case 51: { // MISSION_REQUEST_INT
        mavlink_mission_request_int_t d;
        mavlink_msg_mission_request_int_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("seq", "%u", (unsigned)d.seq);
        display_kvf("mission_type", "%u", (unsigned)d.mission_type);
        break;
    }
    case 54: { // SAFETY_SET_ALLOWED_AREA
        mavlink_safety_set_allowed_area_t d;
        mavlink_msg_safety_set_allowed_area_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("frame", "%u", (unsigned)d.frame);
        display_kvf("p1x (m)", "%.4g", (double)d.p1x);
        display_kvf("p1y (m)", "%.4g", (double)d.p1y);
        display_kvf("p1z (m)", "%.4g", (double)d.p1z);
        display_kvf("p2x (m)", "%.4g", (double)d.p2x);
        display_kvf("p2y (m)", "%.4g", (double)d.p2y);
        display_kvf("p2z (m)", "%.4g", (double)d.p2z);
        break;
    }
    case 55: { // SAFETY_ALLOWED_AREA
        mavlink_safety_allowed_area_t d;
        mavlink_msg_safety_allowed_area_decode(&raw, &d);
        display_kvf("frame", "%u", (unsigned)d.frame);
        display_kvf("p1x (m)", "%.4g", (double)d.p1x);
        display_kvf("p1y (m)", "%.4g", (double)d.p1y);
        display_kvf("p1z (m)", "%.4g", (double)d.p1z);
        display_kvf("p2x (m)", "%.4g", (double)d.p2x);
        display_kvf("p2y (m)", "%.4g", (double)d.p2y);
        display_kvf("p2z (m)", "%.4g", (double)d.p2z);
        break;
    }
    case 61: { // ATTITUDE_QUATERNION_COV
        mavlink_attitude_quaternion_cov_t d;
        mavlink_msg_attitude_quaternion_cov_decode(&raw, &d);
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%.4g" : "%.4g", (double)d.q[_i]);
            (void)_n; display_kv("q", _arr);
        }
        display_kvf("rollspeed (rad/s)", "%.4g", (double)d.rollspeed);
        display_kvf("pitchspeed (rad/s)", "%.4g", (double)d.pitchspeed);
        display_kvf("yawspeed (rad/s)", "%.4g", (double)d.yawspeed);
        {
            char _arr[220]; int _n = 0;
            for (int _i = 0; _i < 9; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%.4g" : "%.4g", (double)d.covariance[_i]);
            (void)_n; display_kv("covariance", _arr);
        }
        break;
    }
    case 62: { // NAV_CONTROLLER_OUTPUT
        mavlink_nav_controller_output_t d;
        mavlink_msg_nav_controller_output_decode(&raw, &d);
        display_kvf("nav_roll (deg)", "%.4g", (double)d.nav_roll);
        display_kvf("nav_pitch (deg)", "%.4g", (double)d.nav_pitch);
        display_kvf("nav_bearing (deg)", "%d", (int)d.nav_bearing);
        display_kvf("target_bearing (deg)", "%d", (int)d.target_bearing);
        display_kvf("wp_dist (m)", "%u", (unsigned)d.wp_dist);
        display_kvf("alt_error (m)", "%.4g", (double)d.alt_error);
        display_kvf("aspd_error (m/s)", "%.4g", (double)d.aspd_error);
        display_kvf("xtrack_error (m)", "%.4g", (double)d.xtrack_error);
        break;
    }
    case 63: { // GLOBAL_POSITION_INT_COV
        mavlink_global_position_int_cov_t d;
        mavlink_msg_global_position_int_cov_decode(&raw, &d);
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        display_kvf("estimator_type", "%u", (unsigned)d.estimator_type);
        display_kvf("lat (degE7)", "%d", (int)d.lat);
        display_kvf("lon (degE7)", "%d", (int)d.lon);
        display_kvf("alt (mm)", "%d", (int)d.alt);
        display_kvf("relative_alt (mm)", "%d", (int)d.relative_alt);
        display_kvf("vx (m/s)", "%.4g", (double)d.vx);
        display_kvf("vy (m/s)", "%.4g", (double)d.vy);
        display_kvf("vz (m/s)", "%.4g", (double)d.vz);
        display_kv("covariance", "[36 elements]");
        break;
    }
    case 64: { // LOCAL_POSITION_NED_COV
        mavlink_local_position_ned_cov_t d;
        mavlink_msg_local_position_ned_cov_decode(&raw, &d);
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        display_kvf("estimator_type", "%u", (unsigned)d.estimator_type);
        display_kvf("x (m)", "%.4g", (double)d.x);
        display_kvf("y (m)", "%.4g", (double)d.y);
        display_kvf("z (m)", "%.4g", (double)d.z);
        display_kvf("vx (m/s)", "%.4g", (double)d.vx);
        display_kvf("vy (m/s)", "%.4g", (double)d.vy);
        display_kvf("vz (m/s)", "%.4g", (double)d.vz);
        display_kvf("ax (m/s/s)", "%.4g", (double)d.ax);
        display_kvf("ay (m/s/s)", "%.4g", (double)d.ay);
        display_kvf("az (m/s/s)", "%.4g", (double)d.az);
        display_kv("covariance", "[45 elements]");
        break;
    }
    case 65: { // RC_CHANNELS
        mavlink_rc_channels_t d;
        mavlink_msg_rc_channels_decode(&raw, &d);
        display_kvf("time_boot_ms (ms)", "%u", (unsigned)d.time_boot_ms);
        display_kvf("chancount", "%u", (unsigned)d.chancount);
        display_kvf("chan1_raw (us)", "%u", (unsigned)d.chan1_raw);
        display_kvf("chan2_raw (us)", "%u", (unsigned)d.chan2_raw);
        display_kvf("chan3_raw (us)", "%u", (unsigned)d.chan3_raw);
        display_kvf("chan4_raw (us)", "%u", (unsigned)d.chan4_raw);
        display_kvf("chan5_raw (us)", "%u", (unsigned)d.chan5_raw);
        display_kvf("chan6_raw (us)", "%u", (unsigned)d.chan6_raw);
        display_kvf("chan7_raw (us)", "%u", (unsigned)d.chan7_raw);
        display_kvf("chan8_raw (us)", "%u", (unsigned)d.chan8_raw);
        display_kvf("chan9_raw (us)", "%u", (unsigned)d.chan9_raw);
        display_kvf("chan10_raw (us)", "%u", (unsigned)d.chan10_raw);
        display_kvf("chan11_raw (us)", "%u", (unsigned)d.chan11_raw);
        display_kvf("chan12_raw (us)", "%u", (unsigned)d.chan12_raw);
        display_kvf("chan13_raw (us)", "%u", (unsigned)d.chan13_raw);
        display_kvf("chan14_raw (us)", "%u", (unsigned)d.chan14_raw);
        display_kvf("chan15_raw (us)", "%u", (unsigned)d.chan15_raw);
        display_kvf("chan16_raw (us)", "%u", (unsigned)d.chan16_raw);
        display_kvf("chan17_raw (us)", "%u", (unsigned)d.chan17_raw);
        display_kvf("chan18_raw (us)", "%u", (unsigned)d.chan18_raw);
        display_kvf("rssi", "%u", (unsigned)d.rssi);
        break;
    }
    case 66: { // REQUEST_DATA_STREAM
        mavlink_request_data_stream_t d;
        mavlink_msg_request_data_stream_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("req_stream_id", "%u", (unsigned)d.req_stream_id);
        display_kvf("req_message_rate (Hz)", "%u", (unsigned)d.req_message_rate);
        display_kvf("start_stop", "%u", (unsigned)d.start_stop);
        break;
    }
    case 67: { // DATA_STREAM
        mavlink_data_stream_t d;
        mavlink_msg_data_stream_decode(&raw, &d);
        display_kvf("stream_id", "%u", (unsigned)d.stream_id);
        display_kvf("message_rate (Hz)", "%u", (unsigned)d.message_rate);
        display_kvf("on_off", "%u", (unsigned)d.on_off);
        break;
    }
    case 69: { // MANUAL_CONTROL
        mavlink_manual_control_t d;
        mavlink_msg_manual_control_decode(&raw, &d);
        display_kvf("target", "%u", (unsigned)d.target);
        display_kvf("x", "%d", (int)d.x);
        display_kvf("y", "%d", (int)d.y);
        display_kvf("z", "%d", (int)d.z);
        display_kvf("r", "%d", (int)d.r);
        display_kvf("buttons", "%u", (unsigned)d.buttons);
        display_kvf("buttons2", "%u", (unsigned)d.buttons2);
        display_kvf("enabled_extensions", "%u", (unsigned)d.enabled_extensions);
        display_kvf("s", "%d", (int)d.s);
        display_kvf("t", "%d", (int)d.t);
        display_kvf("aux1", "%d", (int)d.aux1);
        display_kvf("aux2", "%d", (int)d.aux2);
        display_kvf("aux3", "%d", (int)d.aux3);
        display_kvf("aux4", "%d", (int)d.aux4);
        display_kvf("aux5", "%d", (int)d.aux5);
        display_kvf("aux6", "%d", (int)d.aux6);
        break;
    }
    case 70: { // RC_CHANNELS_OVERRIDE
        mavlink_rc_channels_override_t d;
        mavlink_msg_rc_channels_override_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("chan1_raw (us)", "%u", (unsigned)d.chan1_raw);
        display_kvf("chan2_raw (us)", "%u", (unsigned)d.chan2_raw);
        display_kvf("chan3_raw (us)", "%u", (unsigned)d.chan3_raw);
        display_kvf("chan4_raw (us)", "%u", (unsigned)d.chan4_raw);
        display_kvf("chan5_raw (us)", "%u", (unsigned)d.chan5_raw);
        display_kvf("chan6_raw (us)", "%u", (unsigned)d.chan6_raw);
        display_kvf("chan7_raw (us)", "%u", (unsigned)d.chan7_raw);
        display_kvf("chan8_raw (us)", "%u", (unsigned)d.chan8_raw);
        display_kvf("chan9_raw (us)", "%u", (unsigned)d.chan9_raw);
        display_kvf("chan10_raw (us)", "%u", (unsigned)d.chan10_raw);
        display_kvf("chan11_raw (us)", "%u", (unsigned)d.chan11_raw);
        display_kvf("chan12_raw (us)", "%u", (unsigned)d.chan12_raw);
        display_kvf("chan13_raw (us)", "%u", (unsigned)d.chan13_raw);
        display_kvf("chan14_raw (us)", "%u", (unsigned)d.chan14_raw);
        display_kvf("chan15_raw (us)", "%u", (unsigned)d.chan15_raw);
        display_kvf("chan16_raw (us)", "%u", (unsigned)d.chan16_raw);
        display_kvf("chan17_raw (us)", "%u", (unsigned)d.chan17_raw);
        display_kvf("chan18_raw (us)", "%u", (unsigned)d.chan18_raw);
        break;
    }
    case 73: { // MISSION_ITEM_INT
        mavlink_mission_item_int_t d;
        mavlink_msg_mission_item_int_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("seq", "%u", (unsigned)d.seq);
        display_kvf("frame", "%u", (unsigned)d.frame);
        display_kvf("command", "%u", (unsigned)d.command);
        display_kvf("current", "%u", (unsigned)d.current);
        display_kvf("autocontinue", "%u", (unsigned)d.autocontinue);
        display_kvf("param1", "%.4g", (double)d.param1);
        display_kvf("param2", "%.4g", (double)d.param2);
        display_kvf("param3", "%.4g", (double)d.param3);
        display_kvf("param4", "%.4g", (double)d.param4);
        display_kvf("x", "%d", (int)d.x);
        display_kvf("y", "%d", (int)d.y);
        display_kvf("z", "%.4g", (double)d.z);
        display_kvf("mission_type", "%u", (unsigned)d.mission_type);
        break;
    }
    case 74: { // VFR_HUD
        mavlink_vfr_hud_t d;
        mavlink_msg_vfr_hud_decode(&raw, &d);
        display_kvf("airspeed (m/s)", "%.4g", (double)d.airspeed);
        display_kvf("groundspeed (m/s)", "%.4g", (double)d.groundspeed);
        display_kvf("heading (deg)", "%d", (int)d.heading);
        display_kvf("throttle (%)", "%u", (unsigned)d.throttle);
        display_kvf("alt (m)", "%.4g", (double)d.alt);
        display_kvf("climb (m/s)", "%.4g", (double)d.climb);
        break;
    }
    case 75: { // COMMAND_INT
        mavlink_command_int_t d;
        mavlink_msg_command_int_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("frame", "%u", (unsigned)d.frame);
        display_kvf("command", "%u", (unsigned)d.command);
        display_kvf("current", "%u", (unsigned)d.current);
        display_kvf("autocontinue", "%u", (unsigned)d.autocontinue);
        display_kvf("param1", "%.4g", (double)d.param1);
        display_kvf("param2", "%.4g", (double)d.param2);
        display_kvf("param3", "%.4g", (double)d.param3);
        display_kvf("param4", "%.4g", (double)d.param4);
        display_kvf("x", "%d", (int)d.x);
        display_kvf("y", "%d", (int)d.y);
        display_kvf("z", "%.4g", (double)d.z);
        break;
    }
    case 76: { // COMMAND_LONG
        mavlink_command_long_t d;
        mavlink_msg_command_long_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("command", "%u", (unsigned)d.command);
        display_kvf("confirmation", "%u", (unsigned)d.confirmation);
        display_kvf("param1", "%.4g", (double)d.param1);
        display_kvf("param2", "%.4g", (double)d.param2);
        display_kvf("param3", "%.4g", (double)d.param3);
        display_kvf("param4", "%.4g", (double)d.param4);
        display_kvf("param5", "%.4g", (double)d.param5);
        display_kvf("param6", "%.4g", (double)d.param6);
        display_kvf("param7", "%.4g", (double)d.param7);
        break;
    }
    case 77: { // COMMAND_ACK
        mavlink_command_ack_t d;
        mavlink_msg_command_ack_decode(&raw, &d);
        display_kvf("command", "%u", (unsigned)d.command);
        display_kvf("result", "%u", (unsigned)d.result);
        display_kvf("progress (%)", "%u", (unsigned)d.progress);
        display_kvf("result_param2", "%d", (int)d.result_param2);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        break;
    }
    case 80: { // COMMAND_CANCEL
        mavlink_command_cancel_t d;
        mavlink_msg_command_cancel_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("command", "%u", (unsigned)d.command);
        break;
    }
    case 81: { // MANUAL_SETPOINT
        mavlink_manual_setpoint_t d;
        mavlink_msg_manual_setpoint_decode(&raw, &d);
        display_kvf("time_boot_ms (ms)", "%u", (unsigned)d.time_boot_ms);
        display_kvf("roll (rad/s)", "%.4g", (double)d.roll);
        display_kvf("pitch (rad/s)", "%.4g", (double)d.pitch);
        display_kvf("yaw (rad/s)", "%.4g", (double)d.yaw);
        display_kvf("thrust", "%.4g", (double)d.thrust);
        display_kvf("mode_switch", "%u", (unsigned)d.mode_switch);
        display_kvf("manual_override_switch", "%u", (unsigned)d.manual_override_switch);
        break;
    }
    case 82: { // SET_ATTITUDE_TARGET
        mavlink_set_attitude_target_t d;
        mavlink_msg_set_attitude_target_decode(&raw, &d);
        display_kvf("time_boot_ms (ms)", "%u", (unsigned)d.time_boot_ms);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("type_mask", "%u", (unsigned)d.type_mask);
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%.4g" : "%.4g", (double)d.q[_i]);
            (void)_n; display_kv("q", _arr);
        }
        display_kvf("body_roll_rate (rad/s)", "%.4g", (double)d.body_roll_rate);
        display_kvf("body_pitch_rate (rad/s)", "%.4g", (double)d.body_pitch_rate);
        display_kvf("body_yaw_rate (rad/s)", "%.4g", (double)d.body_yaw_rate);
        display_kvf("thrust", "%.4g", (double)d.thrust);
        {
            char _arr[76]; int _n = 0;
            for (int _i = 0; _i < 3; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%.4g" : "%.4g", (double)d.thrust_body[_i]);
            (void)_n; display_kv("thrust_body", _arr);
        }
        break;
    }
    case 83: { // ATTITUDE_TARGET
        mavlink_attitude_target_t d;
        mavlink_msg_attitude_target_decode(&raw, &d);
        display_kvf("time_boot_ms (ms)", "%u", (unsigned)d.time_boot_ms);
        display_kvf("type_mask", "%u", (unsigned)d.type_mask);
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%.4g" : "%.4g", (double)d.q[_i]);
            (void)_n; display_kv("q", _arr);
        }
        display_kvf("body_roll_rate (rad/s)", "%.4g", (double)d.body_roll_rate);
        display_kvf("body_pitch_rate (rad/s)", "%.4g", (double)d.body_pitch_rate);
        display_kvf("body_yaw_rate (rad/s)", "%.4g", (double)d.body_yaw_rate);
        display_kvf("thrust", "%.4g", (double)d.thrust);
        break;
    }
    case 84: { // SET_POSITION_TARGET_LOCAL_NED
        mavlink_set_position_target_local_ned_t d;
        mavlink_msg_set_position_target_local_ned_decode(&raw, &d);
        display_kvf("time_boot_ms (ms)", "%u", (unsigned)d.time_boot_ms);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("coordinate_frame", "%u", (unsigned)d.coordinate_frame);
        display_kvf("type_mask", "%u", (unsigned)d.type_mask);
        display_kvf("x (m)", "%.4g", (double)d.x);
        display_kvf("y (m)", "%.4g", (double)d.y);
        display_kvf("z (m)", "%.4g", (double)d.z);
        display_kvf("vx (m/s)", "%.4g", (double)d.vx);
        display_kvf("vy (m/s)", "%.4g", (double)d.vy);
        display_kvf("vz (m/s)", "%.4g", (double)d.vz);
        display_kvf("afx (m/s/s)", "%.4g", (double)d.afx);
        display_kvf("afy (m/s/s)", "%.4g", (double)d.afy);
        display_kvf("afz (m/s/s)", "%.4g", (double)d.afz);
        display_kvf("yaw (rad)", "%.4g", (double)d.yaw);
        display_kvf("yaw_rate (rad/s)", "%.4g", (double)d.yaw_rate);
        break;
    }
    case 85: { // POSITION_TARGET_LOCAL_NED
        mavlink_position_target_local_ned_t d;
        mavlink_msg_position_target_local_ned_decode(&raw, &d);
        display_kvf("time_boot_ms (ms)", "%u", (unsigned)d.time_boot_ms);
        display_kvf("coordinate_frame", "%u", (unsigned)d.coordinate_frame);
        display_kvf("type_mask", "%u", (unsigned)d.type_mask);
        display_kvf("x (m)", "%.4g", (double)d.x);
        display_kvf("y (m)", "%.4g", (double)d.y);
        display_kvf("z (m)", "%.4g", (double)d.z);
        display_kvf("vx (m/s)", "%.4g", (double)d.vx);
        display_kvf("vy (m/s)", "%.4g", (double)d.vy);
        display_kvf("vz (m/s)", "%.4g", (double)d.vz);
        display_kvf("afx (m/s/s)", "%.4g", (double)d.afx);
        display_kvf("afy (m/s/s)", "%.4g", (double)d.afy);
        display_kvf("afz (m/s/s)", "%.4g", (double)d.afz);
        display_kvf("yaw (rad)", "%.4g", (double)d.yaw);
        display_kvf("yaw_rate (rad/s)", "%.4g", (double)d.yaw_rate);
        break;
    }
    case 86: { // SET_POSITION_TARGET_GLOBAL_INT
        mavlink_set_position_target_global_int_t d;
        mavlink_msg_set_position_target_global_int_decode(&raw, &d);
        display_kvf("time_boot_ms (ms)", "%u", (unsigned)d.time_boot_ms);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("coordinate_frame", "%u", (unsigned)d.coordinate_frame);
        display_kvf("type_mask", "%u", (unsigned)d.type_mask);
        display_kvf("lat_int (degE7)", "%d", (int)d.lat_int);
        display_kvf("lon_int (degE7)", "%d", (int)d.lon_int);
        display_kvf("alt (m)", "%.4g", (double)d.alt);
        display_kvf("vx (m/s)", "%.4g", (double)d.vx);
        display_kvf("vy (m/s)", "%.4g", (double)d.vy);
        display_kvf("vz (m/s)", "%.4g", (double)d.vz);
        display_kvf("afx (m/s/s)", "%.4g", (double)d.afx);
        display_kvf("afy (m/s/s)", "%.4g", (double)d.afy);
        display_kvf("afz (m/s/s)", "%.4g", (double)d.afz);
        display_kvf("yaw (rad)", "%.4g", (double)d.yaw);
        display_kvf("yaw_rate (rad/s)", "%.4g", (double)d.yaw_rate);
        break;
    }
    case 87: { // POSITION_TARGET_GLOBAL_INT
        mavlink_position_target_global_int_t d;
        mavlink_msg_position_target_global_int_decode(&raw, &d);
        display_kvf("time_boot_ms (ms)", "%u", (unsigned)d.time_boot_ms);
        display_kvf("coordinate_frame", "%u", (unsigned)d.coordinate_frame);
        display_kvf("type_mask", "%u", (unsigned)d.type_mask);
        display_kvf("lat_int (degE7)", "%d", (int)d.lat_int);
        display_kvf("lon_int (degE7)", "%d", (int)d.lon_int);
        display_kvf("alt (m)", "%.4g", (double)d.alt);
        display_kvf("vx (m/s)", "%.4g", (double)d.vx);
        display_kvf("vy (m/s)", "%.4g", (double)d.vy);
        display_kvf("vz (m/s)", "%.4g", (double)d.vz);
        display_kvf("afx (m/s/s)", "%.4g", (double)d.afx);
        display_kvf("afy (m/s/s)", "%.4g", (double)d.afy);
        display_kvf("afz (m/s/s)", "%.4g", (double)d.afz);
        display_kvf("yaw (rad)", "%.4g", (double)d.yaw);
        display_kvf("yaw_rate (rad/s)", "%.4g", (double)d.yaw_rate);
        break;
    }
    case 89: { // LOCAL_POSITION_NED_SYSTEM_GLOBAL_OFFSET
        mavlink_local_position_ned_system_global_offset_t d;
        mavlink_msg_local_position_ned_system_global_offset_decode(&raw, &d);
        display_kvf("time_boot_ms (ms)", "%u", (unsigned)d.time_boot_ms);
        display_kvf("x (m)", "%.4g", (double)d.x);
        display_kvf("y (m)", "%.4g", (double)d.y);
        display_kvf("z (m)", "%.4g", (double)d.z);
        display_kvf("roll (rad)", "%.4g", (double)d.roll);
        display_kvf("pitch (rad)", "%.4g", (double)d.pitch);
        display_kvf("yaw (rad)", "%.4g", (double)d.yaw);
        break;
    }
    case 90: { // HIL_STATE
        mavlink_hil_state_t d;
        mavlink_msg_hil_state_decode(&raw, &d);
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        display_kvf("roll (rad)", "%.4g", (double)d.roll);
        display_kvf("pitch (rad)", "%.4g", (double)d.pitch);
        display_kvf("yaw (rad)", "%.4g", (double)d.yaw);
        display_kvf("rollspeed (rad/s)", "%.4g", (double)d.rollspeed);
        display_kvf("pitchspeed (rad/s)", "%.4g", (double)d.pitchspeed);
        display_kvf("yawspeed (rad/s)", "%.4g", (double)d.yawspeed);
        display_kvf("lat (degE7)", "%d", (int)d.lat);
        display_kvf("lon (degE7)", "%d", (int)d.lon);
        display_kvf("alt (mm)", "%d", (int)d.alt);
        display_kvf("vx (cm/s)", "%d", (int)d.vx);
        display_kvf("vy (cm/s)", "%d", (int)d.vy);
        display_kvf("vz (cm/s)", "%d", (int)d.vz);
        display_kvf("xacc (mG)", "%d", (int)d.xacc);
        display_kvf("yacc (mG)", "%d", (int)d.yacc);
        display_kvf("zacc (mG)", "%d", (int)d.zacc);
        break;
    }
    case 91: { // HIL_CONTROLS
        mavlink_hil_controls_t d;
        mavlink_msg_hil_controls_decode(&raw, &d);
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        display_kvf("roll_ailerons", "%.4g", (double)d.roll_ailerons);
        display_kvf("pitch_elevator", "%.4g", (double)d.pitch_elevator);
        display_kvf("yaw_rudder", "%.4g", (double)d.yaw_rudder);
        display_kvf("throttle", "%.4g", (double)d.throttle);
        display_kvf("aux1", "%.4g", (double)d.aux1);
        display_kvf("aux2", "%.4g", (double)d.aux2);
        display_kvf("aux3", "%.4g", (double)d.aux3);
        display_kvf("aux4", "%.4g", (double)d.aux4);
        display_kvf("mode", "%u", (unsigned)d.mode);
        display_kvf("nav_mode", "%u", (unsigned)d.nav_mode);
        break;
    }
    case 92: { // HIL_RC_INPUTS_RAW
        mavlink_hil_rc_inputs_raw_t d;
        mavlink_msg_hil_rc_inputs_raw_decode(&raw, &d);
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        display_kvf("chan1_raw (us)", "%u", (unsigned)d.chan1_raw);
        display_kvf("chan2_raw (us)", "%u", (unsigned)d.chan2_raw);
        display_kvf("chan3_raw (us)", "%u", (unsigned)d.chan3_raw);
        display_kvf("chan4_raw (us)", "%u", (unsigned)d.chan4_raw);
        display_kvf("chan5_raw (us)", "%u", (unsigned)d.chan5_raw);
        display_kvf("chan6_raw (us)", "%u", (unsigned)d.chan6_raw);
        display_kvf("chan7_raw (us)", "%u", (unsigned)d.chan7_raw);
        display_kvf("chan8_raw (us)", "%u", (unsigned)d.chan8_raw);
        display_kvf("chan9_raw (us)", "%u", (unsigned)d.chan9_raw);
        display_kvf("chan10_raw (us)", "%u", (unsigned)d.chan10_raw);
        display_kvf("chan11_raw (us)", "%u", (unsigned)d.chan11_raw);
        display_kvf("chan12_raw (us)", "%u", (unsigned)d.chan12_raw);
        display_kvf("rssi", "%u", (unsigned)d.rssi);
        break;
    }
    case 93: { // HIL_ACTUATOR_CONTROLS
        mavlink_hil_actuator_controls_t d;
        mavlink_msg_hil_actuator_controls_decode(&raw, &d);
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        {
            char _arr[388]; int _n = 0;
            for (int _i = 0; _i < 16; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%.4g" : "%.4g", (double)d.controls[_i]);
            (void)_n; display_kv("controls", _arr);
        }
        display_kvf("mode", "%u", (unsigned)d.mode);
        display_kvf("flags", "%llu", (unsigned long long)d.flags);
        break;
    }
    case 100: { // OPTICAL_FLOW
        mavlink_optical_flow_t d;
        mavlink_msg_optical_flow_decode(&raw, &d);
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        display_kvf("sensor_id", "%u", (unsigned)d.sensor_id);
        display_kvf("flow_x (dpix)", "%d", (int)d.flow_x);
        display_kvf("flow_y (dpix)", "%d", (int)d.flow_y);
        display_kvf("flow_comp_m_x (m/s)", "%.4g", (double)d.flow_comp_m_x);
        display_kvf("flow_comp_m_y (m/s)", "%.4g", (double)d.flow_comp_m_y);
        display_kvf("quality", "%u", (unsigned)d.quality);
        display_kvf("ground_distance (m)", "%.4g", (double)d.ground_distance);
        display_kvf("flow_rate_x (rad/s)", "%.4g", (double)d.flow_rate_x);
        display_kvf("flow_rate_y (rad/s)", "%.4g", (double)d.flow_rate_y);
        break;
    }
    case 101: { // GLOBAL_VISION_POSITION_ESTIMATE
        mavlink_global_vision_position_estimate_t d;
        mavlink_msg_global_vision_position_estimate_decode(&raw, &d);
        display_kvf("usec (us)", "%llu", (unsigned long long)d.usec);
        display_kvf("x (m)", "%.4g", (double)d.x);
        display_kvf("y (m)", "%.4g", (double)d.y);
        display_kvf("z (m)", "%.4g", (double)d.z);
        display_kvf("roll (rad)", "%.4g", (double)d.roll);
        display_kvf("pitch (rad)", "%.4g", (double)d.pitch);
        display_kvf("yaw (rad)", "%.4g", (double)d.yaw);
        display_kv("covariance", "[21 elements]");
        display_kvf("reset_counter", "%u", (unsigned)d.reset_counter);
        break;
    }
    case 102: { // VISION_POSITION_ESTIMATE
        mavlink_vision_position_estimate_t d;
        mavlink_msg_vision_position_estimate_decode(&raw, &d);
        display_kvf("usec (us)", "%llu", (unsigned long long)d.usec);
        display_kvf("x (m)", "%.4g", (double)d.x);
        display_kvf("y (m)", "%.4g", (double)d.y);
        display_kvf("z (m)", "%.4g", (double)d.z);
        display_kvf("roll (rad)", "%.4g", (double)d.roll);
        display_kvf("pitch (rad)", "%.4g", (double)d.pitch);
        display_kvf("yaw (rad)", "%.4g", (double)d.yaw);
        display_kv("covariance", "[21 elements]");
        display_kvf("reset_counter", "%u", (unsigned)d.reset_counter);
        break;
    }
    case 103: { // VISION_SPEED_ESTIMATE
        mavlink_vision_speed_estimate_t d;
        mavlink_msg_vision_speed_estimate_decode(&raw, &d);
        display_kvf("usec (us)", "%llu", (unsigned long long)d.usec);
        display_kvf("x (m/s)", "%.4g", (double)d.x);
        display_kvf("y (m/s)", "%.4g", (double)d.y);
        display_kvf("z (m/s)", "%.4g", (double)d.z);
        {
            char _arr[220]; int _n = 0;
            for (int _i = 0; _i < 9; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%.4g" : "%.4g", (double)d.covariance[_i]);
            (void)_n; display_kv("covariance", _arr);
        }
        display_kvf("reset_counter", "%u", (unsigned)d.reset_counter);
        break;
    }
    case 104: { // VICON_POSITION_ESTIMATE
        mavlink_vicon_position_estimate_t d;
        mavlink_msg_vicon_position_estimate_decode(&raw, &d);
        display_kvf("usec (us)", "%llu", (unsigned long long)d.usec);
        display_kvf("x (m)", "%.4g", (double)d.x);
        display_kvf("y (m)", "%.4g", (double)d.y);
        display_kvf("z (m)", "%.4g", (double)d.z);
        display_kvf("roll (rad)", "%.4g", (double)d.roll);
        display_kvf("pitch (rad)", "%.4g", (double)d.pitch);
        display_kvf("yaw (rad)", "%.4g", (double)d.yaw);
        display_kv("covariance", "[21 elements]");
        break;
    }
    case 105: { // HIGHRES_IMU
        mavlink_highres_imu_t d;
        mavlink_msg_highres_imu_decode(&raw, &d);
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        display_kvf("xacc (m/s/s)", "%.4g", (double)d.xacc);
        display_kvf("yacc (m/s/s)", "%.4g", (double)d.yacc);
        display_kvf("zacc (m/s/s)", "%.4g", (double)d.zacc);
        display_kvf("xgyro (rad/s)", "%.4g", (double)d.xgyro);
        display_kvf("ygyro (rad/s)", "%.4g", (double)d.ygyro);
        display_kvf("zgyro (rad/s)", "%.4g", (double)d.zgyro);
        display_kvf("xmag (gauss)", "%.4g", (double)d.xmag);
        display_kvf("ymag (gauss)", "%.4g", (double)d.ymag);
        display_kvf("zmag (gauss)", "%.4g", (double)d.zmag);
        display_kvf("abs_pressure (hPa)", "%.4g", (double)d.abs_pressure);
        display_kvf("diff_pressure (hPa)", "%.4g", (double)d.diff_pressure);
        display_kvf("pressure_alt", "%.4g", (double)d.pressure_alt);
        display_kvf("temperature (degC)", "%.4g", (double)d.temperature);
        display_kvf("fields_updated", "%u", (unsigned)d.fields_updated);
        display_kvf("id", "%u", (unsigned)d.id);
        break;
    }
    case 106: { // OPTICAL_FLOW_RAD
        mavlink_optical_flow_rad_t d;
        mavlink_msg_optical_flow_rad_decode(&raw, &d);
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        display_kvf("sensor_id", "%u", (unsigned)d.sensor_id);
        display_kvf("integration_time_us (us)", "%u", (unsigned)d.integration_time_us);
        display_kvf("integrated_x (rad)", "%.4g", (double)d.integrated_x);
        display_kvf("integrated_y (rad)", "%.4g", (double)d.integrated_y);
        display_kvf("integrated_xgyro (rad)", "%.4g", (double)d.integrated_xgyro);
        display_kvf("integrated_ygyro (rad)", "%.4g", (double)d.integrated_ygyro);
        display_kvf("integrated_zgyro (rad)", "%.4g", (double)d.integrated_zgyro);
        display_kvf("temperature (cdegC)", "%d", (int)d.temperature);
        display_kvf("quality", "%u", (unsigned)d.quality);
        display_kvf("time_delta_distance_us (us)", "%u", (unsigned)d.time_delta_distance_us);
        display_kvf("distance (m)", "%.4g", (double)d.distance);
        break;
    }
    case 107: { // HIL_SENSOR
        mavlink_hil_sensor_t d;
        mavlink_msg_hil_sensor_decode(&raw, &d);
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        display_kvf("xacc (m/s/s)", "%.4g", (double)d.xacc);
        display_kvf("yacc (m/s/s)", "%.4g", (double)d.yacc);
        display_kvf("zacc (m/s/s)", "%.4g", (double)d.zacc);
        display_kvf("xgyro (rad/s)", "%.4g", (double)d.xgyro);
        display_kvf("ygyro (rad/s)", "%.4g", (double)d.ygyro);
        display_kvf("zgyro (rad/s)", "%.4g", (double)d.zgyro);
        display_kvf("xmag (gauss)", "%.4g", (double)d.xmag);
        display_kvf("ymag (gauss)", "%.4g", (double)d.ymag);
        display_kvf("zmag (gauss)", "%.4g", (double)d.zmag);
        display_kvf("abs_pressure (hPa)", "%.4g", (double)d.abs_pressure);
        display_kvf("diff_pressure (hPa)", "%.4g", (double)d.diff_pressure);
        display_kvf("pressure_alt", "%.4g", (double)d.pressure_alt);
        display_kvf("temperature (degC)", "%.4g", (double)d.temperature);
        display_kvf("fields_updated", "%u", (unsigned)d.fields_updated);
        display_kvf("id", "%u", (unsigned)d.id);
        break;
    }
    case 108: { // SIM_STATE
        mavlink_sim_state_t d;
        mavlink_msg_sim_state_decode(&raw, &d);
        display_kvf("q1", "%.4g", (double)d.q1);
        display_kvf("q2", "%.4g", (double)d.q2);
        display_kvf("q3", "%.4g", (double)d.q3);
        display_kvf("q4", "%.4g", (double)d.q4);
        display_kvf("roll (rad)", "%.4g", (double)d.roll);
        display_kvf("pitch (rad)", "%.4g", (double)d.pitch);
        display_kvf("yaw (rad)", "%.4g", (double)d.yaw);
        display_kvf("xacc (m/s/s)", "%.4g", (double)d.xacc);
        display_kvf("yacc (m/s/s)", "%.4g", (double)d.yacc);
        display_kvf("zacc (m/s/s)", "%.4g", (double)d.zacc);
        display_kvf("xgyro (rad/s)", "%.4g", (double)d.xgyro);
        display_kvf("ygyro (rad/s)", "%.4g", (double)d.ygyro);
        display_kvf("zgyro (rad/s)", "%.4g", (double)d.zgyro);
        display_kvf("lat (deg)", "%.4g", (double)d.lat);
        display_kvf("lon (deg)", "%.4g", (double)d.lon);
        display_kvf("alt (m)", "%.4g", (double)d.alt);
        display_kvf("std_dev_horz", "%.4g", (double)d.std_dev_horz);
        display_kvf("std_dev_vert", "%.4g", (double)d.std_dev_vert);
        display_kvf("vn (m/s)", "%.4g", (double)d.vn);
        display_kvf("ve (m/s)", "%.4g", (double)d.ve);
        display_kvf("vd (m/s)", "%.4g", (double)d.vd);
        display_kvf("lat_int (degE7)", "%d", (int)d.lat_int);
        display_kvf("lon_int (degE7)", "%d", (int)d.lon_int);
        break;
    }
    case 109: { // RADIO_STATUS
        mavlink_radio_status_t d;
        mavlink_msg_radio_status_decode(&raw, &d);
        display_kvf("rssi", "%u", (unsigned)d.rssi);
        display_kvf("remrssi", "%u", (unsigned)d.remrssi);
        display_kvf("txbuf (%)", "%u", (unsigned)d.txbuf);
        display_kvf("noise", "%u", (unsigned)d.noise);
        display_kvf("remnoise", "%u", (unsigned)d.remnoise);
        display_kvf("rxerrors", "%u", (unsigned)d.rxerrors);
        display_kvf("fixed", "%u", (unsigned)d.fixed);
        break;
    }
    case 110: { // FILE_TRANSFER_PROTOCOL
        mavlink_file_transfer_protocol_t d;
        mavlink_msg_file_transfer_protocol_decode(&raw, &d);
        display_kvf("target_network", "%u", (unsigned)d.target_network);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kv("payload", "[251 elements]");
        break;
    }
    case 111: { // TIMESYNC
        mavlink_timesync_t d;
        mavlink_msg_timesync_decode(&raw, &d);
        display_kvf("tc1 (ns)", "%lld", (long long)d.tc1);
        display_kvf("ts1 (ns)", "%lld", (long long)d.ts1);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        break;
    }
    case 112: { // CAMERA_TRIGGER
        mavlink_camera_trigger_t d;
        mavlink_msg_camera_trigger_decode(&raw, &d);
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        display_kvf("seq", "%u", (unsigned)d.seq);
        break;
    }
    case 113: { // HIL_GPS
        mavlink_hil_gps_t d;
        mavlink_msg_hil_gps_decode(&raw, &d);
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        display_kvf("fix_type", "%u", (unsigned)d.fix_type);
        display_kvf("lat (degE7)", "%d", (int)d.lat);
        display_kvf("lon (degE7)", "%d", (int)d.lon);
        display_kvf("alt (mm)", "%d", (int)d.alt);
        display_kvf("eph", "%u", (unsigned)d.eph);
        display_kvf("epv", "%u", (unsigned)d.epv);
        display_kvf("vel (cm/s)", "%u", (unsigned)d.vel);
        display_kvf("vn (cm/s)", "%d", (int)d.vn);
        display_kvf("ve (cm/s)", "%d", (int)d.ve);
        display_kvf("vd (cm/s)", "%d", (int)d.vd);
        display_kvf("cog (cdeg)", "%u", (unsigned)d.cog);
        display_kvf("satellites_visible", "%u", (unsigned)d.satellites_visible);
        display_kvf("id", "%u", (unsigned)d.id);
        display_kvf("yaw (cdeg)", "%u", (unsigned)d.yaw);
        break;
    }
    case 114: { // HIL_OPTICAL_FLOW
        mavlink_hil_optical_flow_t d;
        mavlink_msg_hil_optical_flow_decode(&raw, &d);
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        display_kvf("sensor_id", "%u", (unsigned)d.sensor_id);
        display_kvf("integration_time_us (us)", "%u", (unsigned)d.integration_time_us);
        display_kvf("integrated_x (rad)", "%.4g", (double)d.integrated_x);
        display_kvf("integrated_y (rad)", "%.4g", (double)d.integrated_y);
        display_kvf("integrated_xgyro (rad)", "%.4g", (double)d.integrated_xgyro);
        display_kvf("integrated_ygyro (rad)", "%.4g", (double)d.integrated_ygyro);
        display_kvf("integrated_zgyro (rad)", "%.4g", (double)d.integrated_zgyro);
        display_kvf("temperature (cdegC)", "%d", (int)d.temperature);
        display_kvf("quality", "%u", (unsigned)d.quality);
        display_kvf("time_delta_distance_us (us)", "%u", (unsigned)d.time_delta_distance_us);
        display_kvf("distance (m)", "%.4g", (double)d.distance);
        break;
    }
    case 115: { // HIL_STATE_QUATERNION
        mavlink_hil_state_quaternion_t d;
        mavlink_msg_hil_state_quaternion_decode(&raw, &d);
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%.4g" : "%.4g", (double)d.attitude_quaternion[_i]);
            (void)_n; display_kv("attitude_quaternion", _arr);
        }
        display_kvf("rollspeed (rad/s)", "%.4g", (double)d.rollspeed);
        display_kvf("pitchspeed (rad/s)", "%.4g", (double)d.pitchspeed);
        display_kvf("yawspeed (rad/s)", "%.4g", (double)d.yawspeed);
        display_kvf("lat (degE7)", "%d", (int)d.lat);
        display_kvf("lon (degE7)", "%d", (int)d.lon);
        display_kvf("alt (mm)", "%d", (int)d.alt);
        display_kvf("vx (cm/s)", "%d", (int)d.vx);
        display_kvf("vy (cm/s)", "%d", (int)d.vy);
        display_kvf("vz (cm/s)", "%d", (int)d.vz);
        display_kvf("ind_airspeed (cm/s)", "%u", (unsigned)d.ind_airspeed);
        display_kvf("true_airspeed (cm/s)", "%u", (unsigned)d.true_airspeed);
        display_kvf("xacc (mG)", "%d", (int)d.xacc);
        display_kvf("yacc (mG)", "%d", (int)d.yacc);
        display_kvf("zacc (mG)", "%d", (int)d.zacc);
        break;
    }
    case 116: { // SCALED_IMU2
        mavlink_scaled_imu2_t d;
        mavlink_msg_scaled_imu2_decode(&raw, &d);
        display_kvf("time_boot_ms (ms)", "%u", (unsigned)d.time_boot_ms);
        display_kvf("xacc (mG)", "%d", (int)d.xacc);
        display_kvf("yacc (mG)", "%d", (int)d.yacc);
        display_kvf("zacc (mG)", "%d", (int)d.zacc);
        display_kvf("xgyro (mrad/s)", "%d", (int)d.xgyro);
        display_kvf("ygyro (mrad/s)", "%d", (int)d.ygyro);
        display_kvf("zgyro (mrad/s)", "%d", (int)d.zgyro);
        display_kvf("xmag (mgauss)", "%d", (int)d.xmag);
        display_kvf("ymag (mgauss)", "%d", (int)d.ymag);
        display_kvf("zmag (mgauss)", "%d", (int)d.zmag);
        display_kvf("temperature (cdegC)", "%d", (int)d.temperature);
        break;
    }
    case 117: { // LOG_REQUEST_LIST
        mavlink_log_request_list_t d;
        mavlink_msg_log_request_list_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("start", "%u", (unsigned)d.start);
        display_kvf("end", "%u", (unsigned)d.end);
        break;
    }
    case 118: { // LOG_ENTRY
        mavlink_log_entry_t d;
        mavlink_msg_log_entry_decode(&raw, &d);
        display_kvf("id", "%u", (unsigned)d.id);
        display_kvf("num_logs", "%u", (unsigned)d.num_logs);
        display_kvf("last_log_num", "%u", (unsigned)d.last_log_num);
        display_kvf("time_utc (s)", "%u", (unsigned)d.time_utc);
        display_kvf("size (bytes)", "%u", (unsigned)d.size);
        break;
    }
    case 119: { // LOG_REQUEST_DATA
        mavlink_log_request_data_t d;
        mavlink_msg_log_request_data_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("id", "%u", (unsigned)d.id);
        display_kvf("ofs", "%u", (unsigned)d.ofs);
        display_kvf("count (bytes)", "%u", (unsigned)d.count);
        break;
    }
    case 120: { // LOG_DATA
        mavlink_log_data_t d;
        mavlink_msg_log_data_decode(&raw, &d);
        display_kvf("id", "%u", (unsigned)d.id);
        display_kvf("ofs", "%u", (unsigned)d.ofs);
        display_kvf("count (bytes)", "%u", (unsigned)d.count);
        display_kv("data", "[90 elements]");
        break;
    }
    case 121: { // LOG_ERASE
        mavlink_log_erase_t d;
        mavlink_msg_log_erase_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        break;
    }
    case 122: { // LOG_REQUEST_END
        mavlink_log_request_end_t d;
        mavlink_msg_log_request_end_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        break;
    }
    case 123: { // GPS_INJECT_DATA
        mavlink_gps_inject_data_t d;
        mavlink_msg_gps_inject_data_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("len (bytes)", "%u", (unsigned)d.len);
        display_kv("data", "[110 elements]");
        break;
    }
    case 124: { // GPS2_RAW
        mavlink_gps2_raw_t d;
        mavlink_msg_gps2_raw_decode(&raw, &d);
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        display_kvf("fix_type", "%u", (unsigned)d.fix_type);
        display_kvf("lat (degE7)", "%d", (int)d.lat);
        display_kvf("lon (degE7)", "%d", (int)d.lon);
        display_kvf("alt (mm)", "%d", (int)d.alt);
        display_kvf("eph", "%u", (unsigned)d.eph);
        display_kvf("epv", "%u", (unsigned)d.epv);
        display_kvf("vel (cm/s)", "%u", (unsigned)d.vel);
        display_kvf("cog (cdeg)", "%u", (unsigned)d.cog);
        display_kvf("satellites_visible", "%u", (unsigned)d.satellites_visible);
        display_kvf("dgps_numch", "%u", (unsigned)d.dgps_numch);
        display_kvf("dgps_age (ms)", "%u", (unsigned)d.dgps_age);
        display_kvf("yaw (cdeg)", "%u", (unsigned)d.yaw);
        display_kvf("alt_ellipsoid (mm)", "%d", (int)d.alt_ellipsoid);
        display_kvf("h_acc (mm)", "%u", (unsigned)d.h_acc);
        display_kvf("v_acc (mm)", "%u", (unsigned)d.v_acc);
        display_kvf("vel_acc (mm/s)", "%u", (unsigned)d.vel_acc);
        display_kvf("hdg_acc (degE5)", "%u", (unsigned)d.hdg_acc);
        break;
    }
    case 125: { // POWER_STATUS
        mavlink_power_status_t d;
        mavlink_msg_power_status_decode(&raw, &d);
        display_kvf("Vcc (mV)", "%u", (unsigned)d.Vcc);
        display_kvf("Vservo (mV)", "%u", (unsigned)d.Vservo);
        display_kvf("flags", "%u", (unsigned)d.flags);
        break;
    }
    case 126: { // SERIAL_CONTROL
        mavlink_serial_control_t d;
        mavlink_msg_serial_control_decode(&raw, &d);
        display_kvf("device", "%u", (unsigned)d.device);
        display_kvf("flags", "%u", (unsigned)d.flags);
        display_kvf("timeout (ms)", "%u", (unsigned)d.timeout);
        display_kvf("baudrate (bits/s)", "%u", (unsigned)d.baudrate);
        display_kvf("count (bytes)", "%u", (unsigned)d.count);
        display_kv("data", "[70 elements]");
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        break;
    }
    case 127: { // GPS_RTK
        mavlink_gps_rtk_t d;
        mavlink_msg_gps_rtk_decode(&raw, &d);
        display_kvf("time_last_baseline_ms (ms)", "%u", (unsigned)d.time_last_baseline_ms);
        display_kvf("rtk_receiver_id", "%u", (unsigned)d.rtk_receiver_id);
        display_kvf("wn", "%u", (unsigned)d.wn);
        display_kvf("tow (ms)", "%u", (unsigned)d.tow);
        display_kvf("rtk_health", "%u", (unsigned)d.rtk_health);
        display_kvf("rtk_rate (Hz)", "%u", (unsigned)d.rtk_rate);
        display_kvf("nsats", "%u", (unsigned)d.nsats);
        display_kvf("baseline_coords_type", "%u", (unsigned)d.baseline_coords_type);
        display_kvf("baseline_a_mm (mm)", "%d", (int)d.baseline_a_mm);
        display_kvf("baseline_b_mm (mm)", "%d", (int)d.baseline_b_mm);
        display_kvf("baseline_c_mm (mm)", "%d", (int)d.baseline_c_mm);
        display_kvf("accuracy", "%u", (unsigned)d.accuracy);
        display_kvf("iar_num_hypotheses", "%d", (int)d.iar_num_hypotheses);
        break;
    }
    case 128: { // GPS2_RTK
        mavlink_gps2_rtk_t d;
        mavlink_msg_gps2_rtk_decode(&raw, &d);
        display_kvf("time_last_baseline_ms (ms)", "%u", (unsigned)d.time_last_baseline_ms);
        display_kvf("rtk_receiver_id", "%u", (unsigned)d.rtk_receiver_id);
        display_kvf("wn", "%u", (unsigned)d.wn);
        display_kvf("tow (ms)", "%u", (unsigned)d.tow);
        display_kvf("rtk_health", "%u", (unsigned)d.rtk_health);
        display_kvf("rtk_rate (Hz)", "%u", (unsigned)d.rtk_rate);
        display_kvf("nsats", "%u", (unsigned)d.nsats);
        display_kvf("baseline_coords_type", "%u", (unsigned)d.baseline_coords_type);
        display_kvf("baseline_a_mm (mm)", "%d", (int)d.baseline_a_mm);
        display_kvf("baseline_b_mm (mm)", "%d", (int)d.baseline_b_mm);
        display_kvf("baseline_c_mm (mm)", "%d", (int)d.baseline_c_mm);
        display_kvf("accuracy", "%u", (unsigned)d.accuracy);
        display_kvf("iar_num_hypotheses", "%d", (int)d.iar_num_hypotheses);
        break;
    }
    case 129: { // SCALED_IMU3
        mavlink_scaled_imu3_t d;
        mavlink_msg_scaled_imu3_decode(&raw, &d);
        display_kvf("time_boot_ms (ms)", "%u", (unsigned)d.time_boot_ms);
        display_kvf("xacc (mG)", "%d", (int)d.xacc);
        display_kvf("yacc (mG)", "%d", (int)d.yacc);
        display_kvf("zacc (mG)", "%d", (int)d.zacc);
        display_kvf("xgyro (mrad/s)", "%d", (int)d.xgyro);
        display_kvf("ygyro (mrad/s)", "%d", (int)d.ygyro);
        display_kvf("zgyro (mrad/s)", "%d", (int)d.zgyro);
        display_kvf("xmag (mgauss)", "%d", (int)d.xmag);
        display_kvf("ymag (mgauss)", "%d", (int)d.ymag);
        display_kvf("zmag (mgauss)", "%d", (int)d.zmag);
        display_kvf("temperature (cdegC)", "%d", (int)d.temperature);
        break;
    }
    case 130: { // DATA_TRANSMISSION_HANDSHAKE
        mavlink_data_transmission_handshake_t d;
        mavlink_msg_data_transmission_handshake_decode(&raw, &d);
        display_kvf("type", "%u", (unsigned)d.type);
        display_kvf("size (bytes)", "%u", (unsigned)d.size);
        display_kvf("width", "%u", (unsigned)d.width);
        display_kvf("height", "%u", (unsigned)d.height);
        display_kvf("packets", "%u", (unsigned)d.packets);
        display_kvf("payload (bytes)", "%u", (unsigned)d.payload);
        display_kvf("jpg_quality (%)", "%u", (unsigned)d.jpg_quality);
        break;
    }
    case 131: { // ENCAPSULATED_DATA
        mavlink_encapsulated_data_t d;
        mavlink_msg_encapsulated_data_decode(&raw, &d);
        display_kvf("seqnr", "%u", (unsigned)d.seqnr);
        display_kv("data", "[253 elements]");
        break;
    }
    case 132: { // DISTANCE_SENSOR
        mavlink_distance_sensor_t d;
        mavlink_msg_distance_sensor_decode(&raw, &d);
        display_kvf("time_boot_ms (ms)", "%u", (unsigned)d.time_boot_ms);
        display_kvf("min_distance (cm)", "%u", (unsigned)d.min_distance);
        display_kvf("max_distance (cm)", "%u", (unsigned)d.max_distance);
        display_kvf("current_distance (cm)", "%u", (unsigned)d.current_distance);
        display_kvf("type", "%u", (unsigned)d.type);
        display_kvf("id", "%u", (unsigned)d.id);
        display_kvf("orientation", "%u", (unsigned)d.orientation);
        display_kvf("covariance (cm^2)", "%u", (unsigned)d.covariance);
        display_kvf("horizontal_fov (rad)", "%.4g", (double)d.horizontal_fov);
        display_kvf("vertical_fov (rad)", "%.4g", (double)d.vertical_fov);
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%.4g" : "%.4g", (double)d.quaternion[_i]);
            (void)_n; display_kv("quaternion", _arr);
        }
        display_kvf("signal_quality (%)", "%u", (unsigned)d.signal_quality);
        break;
    }
    case 133: { // TERRAIN_REQUEST
        mavlink_terrain_request_t d;
        mavlink_msg_terrain_request_decode(&raw, &d);
        display_kvf("lat (degE7)", "%d", (int)d.lat);
        display_kvf("lon (degE7)", "%d", (int)d.lon);
        display_kvf("grid_spacing (m)", "%u", (unsigned)d.grid_spacing);
        display_kvf("mask", "0x%07x", (unsigned long long)d.mask);
        break;
    }
    case 134: { // TERRAIN_DATA
        mavlink_terrain_data_t d;
        mavlink_msg_terrain_data_decode(&raw, &d);
        display_kvf("lat (degE7)", "%d", (int)d.lat);
        display_kvf("lon (degE7)", "%d", (int)d.lon);
        display_kvf("grid_spacing (m)", "%u", (unsigned)d.grid_spacing);
        display_kvf("gridbit", "%u", (unsigned)d.gridbit);
        {
            char _arr[388]; int _n = 0;
            for (int _i = 0; _i < 16; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%d" : "%d", (int)d.data[_i]);
            (void)_n; display_kv("data (m)", _arr);
        }
        break;
    }
    case 135: { // TERRAIN_CHECK
        mavlink_terrain_check_t d;
        mavlink_msg_terrain_check_decode(&raw, &d);
        display_kvf("lat (degE7)", "%d", (int)d.lat);
        display_kvf("lon (degE7)", "%d", (int)d.lon);
        break;
    }
    case 136: { // TERRAIN_REPORT
        mavlink_terrain_report_t d;
        mavlink_msg_terrain_report_decode(&raw, &d);
        display_kvf("lat (degE7)", "%d", (int)d.lat);
        display_kvf("lon (degE7)", "%d", (int)d.lon);
        display_kvf("spacing", "%u", (unsigned)d.spacing);
        display_kvf("terrain_height (m)", "%.4g", (double)d.terrain_height);
        display_kvf("current_height (m)", "%.4g", (double)d.current_height);
        display_kvf("pending", "%u", (unsigned)d.pending);
        display_kvf("loaded", "%u", (unsigned)d.loaded);
        break;
    }
    case 137: { // SCALED_PRESSURE2
        mavlink_scaled_pressure2_t d;
        mavlink_msg_scaled_pressure2_decode(&raw, &d);
        display_kvf("time_boot_ms (ms)", "%u", (unsigned)d.time_boot_ms);
        display_kvf("press_abs (hPa)", "%.4g", (double)d.press_abs);
        display_kvf("press_diff (hPa)", "%.4g", (double)d.press_diff);
        display_kvf("temperature (cdegC)", "%d", (int)d.temperature);
        display_kvf("temperature_press_diff (cdegC)", "%d", (int)d.temperature_press_diff);
        break;
    }
    case 138: { // ATT_POS_MOCAP
        mavlink_att_pos_mocap_t d;
        mavlink_msg_att_pos_mocap_decode(&raw, &d);
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%.4g" : "%.4g", (double)d.q[_i]);
            (void)_n; display_kv("q", _arr);
        }
        display_kvf("x (m)", "%.4g", (double)d.x);
        display_kvf("y (m)", "%.4g", (double)d.y);
        display_kvf("z (m)", "%.4g", (double)d.z);
        display_kv("covariance", "[21 elements]");
        break;
    }
    case 139: { // SET_ACTUATOR_CONTROL_TARGET
        mavlink_set_actuator_control_target_t d;
        mavlink_msg_set_actuator_control_target_decode(&raw, &d);
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        display_kvf("group_mlx", "%u", (unsigned)d.group_mlx);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        {
            char _arr[196]; int _n = 0;
            for (int _i = 0; _i < 8; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%.4g" : "%.4g", (double)d.controls[_i]);
            (void)_n; display_kv("controls", _arr);
        }
        break;
    }
    case 140: { // ACTUATOR_CONTROL_TARGET
        mavlink_actuator_control_target_t d;
        mavlink_msg_actuator_control_target_decode(&raw, &d);
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        display_kvf("group_mlx", "%u", (unsigned)d.group_mlx);
        {
            char _arr[196]; int _n = 0;
            for (int _i = 0; _i < 8; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%.4g" : "%.4g", (double)d.controls[_i]);
            (void)_n; display_kv("controls", _arr);
        }
        break;
    }
    case 141: { // ALTITUDE
        mavlink_altitude_t d;
        mavlink_msg_altitude_decode(&raw, &d);
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        display_kvf("altitude_monotonic (m)", "%.4g", (double)d.altitude_monotonic);
        display_kvf("altitude_amsl (m)", "%.4g", (double)d.altitude_amsl);
        display_kvf("altitude_local (m)", "%.4g", (double)d.altitude_local);
        display_kvf("altitude_relative (m)", "%.4g", (double)d.altitude_relative);
        display_kvf("altitude_terrain (m)", "%.4g", (double)d.altitude_terrain);
        display_kvf("bottom_clearance (m)", "%.4g", (double)d.bottom_clearance);
        break;
    }
    case 142: { // RESOURCE_REQUEST
        mavlink_resource_request_t d;
        mavlink_msg_resource_request_decode(&raw, &d);
        display_kvf("request_id", "%u", (unsigned)d.request_id);
        display_kvf("uri_type", "%u", (unsigned)d.uri_type);
        display_kv("uri", "[120 elements]");
        display_kvf("transfer_type", "%u", (unsigned)d.transfer_type);
        display_kv("storage", "[120 elements]");
        break;
    }
    case 143: { // SCALED_PRESSURE3
        mavlink_scaled_pressure3_t d;
        mavlink_msg_scaled_pressure3_decode(&raw, &d);
        display_kvf("time_boot_ms (ms)", "%u", (unsigned)d.time_boot_ms);
        display_kvf("press_abs (hPa)", "%.4g", (double)d.press_abs);
        display_kvf("press_diff (hPa)", "%.4g", (double)d.press_diff);
        display_kvf("temperature (cdegC)", "%d", (int)d.temperature);
        display_kvf("temperature_press_diff (cdegC)", "%d", (int)d.temperature_press_diff);
        break;
    }
    case 144: { // FOLLOW_TARGET
        mavlink_follow_target_t d;
        mavlink_msg_follow_target_decode(&raw, &d);
        display_kvf("timestamp (ms)", "%llu", (unsigned long long)d.timestamp);
        display_kvf("est_capabilities", "%u", (unsigned)d.est_capabilities);
        display_kvf("lat (degE7)", "%d", (int)d.lat);
        display_kvf("lon (degE7)", "%d", (int)d.lon);
        display_kvf("alt (m)", "%.4g", (double)d.alt);
        {
            char _arr[76]; int _n = 0;
            for (int _i = 0; _i < 3; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%.4g" : "%.4g", (double)d.vel[_i]);
            (void)_n; display_kv("vel (m/s)", _arr);
        }
        {
            char _arr[76]; int _n = 0;
            for (int _i = 0; _i < 3; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%.4g" : "%.4g", (double)d.acc[_i]);
            (void)_n; display_kv("acc (m/s/s)", _arr);
        }
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%.4g" : "%.4g", (double)d.attitude_q[_i]);
            (void)_n; display_kv("attitude_q", _arr);
        }
        {
            char _arr[76]; int _n = 0;
            for (int _i = 0; _i < 3; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%.4g" : "%.4g", (double)d.rates[_i]);
            (void)_n; display_kv("rates", _arr);
        }
        {
            char _arr[76]; int _n = 0;
            for (int _i = 0; _i < 3; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%.4g" : "%.4g", (double)d.position_cov[_i]);
            (void)_n; display_kv("position_cov", _arr);
        }
        display_kvf("custom_state", "%llu", (unsigned long long)d.custom_state);
        break;
    }
    case 146: { // CONTROL_SYSTEM_STATE
        mavlink_control_system_state_t d;
        mavlink_msg_control_system_state_decode(&raw, &d);
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        display_kvf("x_acc (m/s/s)", "%.4g", (double)d.x_acc);
        display_kvf("y_acc (m/s/s)", "%.4g", (double)d.y_acc);
        display_kvf("z_acc (m/s/s)", "%.4g", (double)d.z_acc);
        display_kvf("x_vel (m/s)", "%.4g", (double)d.x_vel);
        display_kvf("y_vel (m/s)", "%.4g", (double)d.y_vel);
        display_kvf("z_vel (m/s)", "%.4g", (double)d.z_vel);
        display_kvf("x_pos (m)", "%.4g", (double)d.x_pos);
        display_kvf("y_pos (m)", "%.4g", (double)d.y_pos);
        display_kvf("z_pos (m)", "%.4g", (double)d.z_pos);
        display_kvf("airspeed (m/s)", "%.4g", (double)d.airspeed);
        {
            char _arr[76]; int _n = 0;
            for (int _i = 0; _i < 3; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%.4g" : "%.4g", (double)d.vel_variance[_i]);
            (void)_n; display_kv("vel_variance", _arr);
        }
        {
            char _arr[76]; int _n = 0;
            for (int _i = 0; _i < 3; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%.4g" : "%.4g", (double)d.pos_variance[_i]);
            (void)_n; display_kv("pos_variance", _arr);
        }
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%.4g" : "%.4g", (double)d.q[_i]);
            (void)_n; display_kv("q", _arr);
        }
        display_kvf("roll_rate (rad/s)", "%.4g", (double)d.roll_rate);
        display_kvf("pitch_rate (rad/s)", "%.4g", (double)d.pitch_rate);
        display_kvf("yaw_rate (rad/s)", "%.4g", (double)d.yaw_rate);
        break;
    }
    case 147: { // BATTERY_STATUS
        mavlink_battery_status_t d;
        mavlink_msg_battery_status_decode(&raw, &d);
        display_kvf("id", "%u", (unsigned)d.id);
        display_kvf("battery_function", "%u", (unsigned)d.battery_function);
        display_kvf("type", "%u", (unsigned)d.type);
        display_kvf("temperature (cdegC)", "%d", (int)d.temperature);
        {
            char _arr[244]; int _n = 0;
            for (int _i = 0; _i < 10; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.voltages[_i]);
            (void)_n; display_kv("voltages (mV)", _arr);
        }
        display_kvf("current_battery (cA)", "%d", (int)d.current_battery);
        display_kvf("current_consumed (mAh)", "%d", (int)d.current_consumed);
        display_kvf("energy_consumed (hJ)", "%d", (int)d.energy_consumed);
        display_kvf("battery_remaining (%)", "%d", (int)d.battery_remaining);
        display_kvf("time_remaining (s)", "%d", (int)d.time_remaining);
        display_kvf("charge_state", "%u", (unsigned)d.charge_state);
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.voltages_ext[_i]);
            (void)_n; display_kv("voltages_ext (mV)", _arr);
        }
        display_kvf("mode", "%u", (unsigned)d.mode);
        display_kvf("fault_bitmask", "%u", (unsigned)d.fault_bitmask);
        break;
    }
    case 148: { // AUTOPILOT_VERSION
        mavlink_autopilot_version_t d;
        mavlink_msg_autopilot_version_decode(&raw, &d);
        display_kvf("capabilities", "%llu", (unsigned long long)d.capabilities);
        display_kvf("flight_sw_version", "%u", (unsigned)d.flight_sw_version);
        display_kvf("middleware_sw_version", "%u", (unsigned)d.middleware_sw_version);
        display_kvf("os_sw_version", "%u", (unsigned)d.os_sw_version);
        display_kvf("board_version", "%u", (unsigned)d.board_version);
        {
            char _arr[196]; int _n = 0;
            for (int _i = 0; _i < 8; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.flight_custom_version[_i]);
            (void)_n; display_kv("flight_custom_version", _arr);
        }
        {
            char _arr[196]; int _n = 0;
            for (int _i = 0; _i < 8; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.middleware_custom_version[_i]);
            (void)_n; display_kv("middleware_custom_version", _arr);
        }
        {
            char _arr[196]; int _n = 0;
            for (int _i = 0; _i < 8; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.os_custom_version[_i]);
            (void)_n; display_kv("os_custom_version", _arr);
        }
        display_kvf("vendor_id", "%u", (unsigned)d.vendor_id);
        display_kvf("product_id", "%u", (unsigned)d.product_id);
        display_kvf("uid", "%llu", (unsigned long long)d.uid);
        display_kv("uid2", "[18 elements]");
        break;
    }
    case 149: { // LANDING_TARGET
        mavlink_landing_target_t d;
        mavlink_msg_landing_target_decode(&raw, &d);
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        display_kvf("target_num", "%u", (unsigned)d.target_num);
        display_kvf("frame", "%u", (unsigned)d.frame);
        display_kvf("angle_x (rad)", "%.4g", (double)d.angle_x);
        display_kvf("angle_y (rad)", "%.4g", (double)d.angle_y);
        display_kvf("distance (m)", "%.4g", (double)d.distance);
        display_kvf("size_x (rad)", "%.4g", (double)d.size_x);
        display_kvf("size_y (rad)", "%.4g", (double)d.size_y);
        display_kvf("x (m)", "%.4g", (double)d.x);
        display_kvf("y (m)", "%.4g", (double)d.y);
        display_kvf("z (m)", "%.4g", (double)d.z);
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%.4g" : "%.4g", (double)d.q[_i]);
            (void)_n; display_kv("q", _arr);
        }
        display_kvf("type", "%u", (unsigned)d.type);
        display_kvf("position_valid", "%u", (unsigned)d.position_valid);
        break;
    }
    case 150: { // SENSOR_OFFSETS
        mavlink_sensor_offsets_t d;
        mavlink_msg_sensor_offsets_decode(&raw, &d);
        display_kvf("mag_ofs_x", "%d", (int)d.mag_ofs_x);
        display_kvf("mag_ofs_y", "%d", (int)d.mag_ofs_y);
        display_kvf("mag_ofs_z", "%d", (int)d.mag_ofs_z);
        display_kvf("mag_declination (rad)", "%.4g", (double)d.mag_declination);
        display_kvf("raw_press", "%d", (int)d.raw_press);
        display_kvf("raw_temp", "%d", (int)d.raw_temp);
        display_kvf("gyro_cal_x", "%.4g", (double)d.gyro_cal_x);
        display_kvf("gyro_cal_y", "%.4g", (double)d.gyro_cal_y);
        display_kvf("gyro_cal_z", "%.4g", (double)d.gyro_cal_z);
        display_kvf("accel_cal_x", "%.4g", (double)d.accel_cal_x);
        display_kvf("accel_cal_y", "%.4g", (double)d.accel_cal_y);
        display_kvf("accel_cal_z", "%.4g", (double)d.accel_cal_z);
        break;
    }
    case 151: { // SET_MAG_OFFSETS
        mavlink_set_mag_offsets_t d;
        mavlink_msg_set_mag_offsets_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("mag_ofs_x", "%d", (int)d.mag_ofs_x);
        display_kvf("mag_ofs_y", "%d", (int)d.mag_ofs_y);
        display_kvf("mag_ofs_z", "%d", (int)d.mag_ofs_z);
        break;
    }
    case 152: { // MEMINFO
        mavlink_meminfo_t d;
        mavlink_msg_meminfo_decode(&raw, &d);
        display_kvf("brkval", "%u", (unsigned)d.brkval);
        display_kvf("freemem (bytes)", "%u", (unsigned)d.freemem);
        display_kvf("freemem32 (bytes)", "%u", (unsigned)d.freemem32);
        break;
    }
    case 153: { // AP_ADC
        mavlink_ap_adc_t d;
        mavlink_msg_ap_adc_decode(&raw, &d);
        display_kvf("adc1", "%u", (unsigned)d.adc1);
        display_kvf("adc2", "%u", (unsigned)d.adc2);
        display_kvf("adc3", "%u", (unsigned)d.adc3);
        display_kvf("adc4", "%u", (unsigned)d.adc4);
        display_kvf("adc5", "%u", (unsigned)d.adc5);
        display_kvf("adc6", "%u", (unsigned)d.adc6);
        break;
    }
    case 154: { // DIGICAM_CONFIGURE
        mavlink_digicam_configure_t d;
        mavlink_msg_digicam_configure_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("mode", "%u", (unsigned)d.mode);
        display_kvf("shutter_speed", "%u", (unsigned)d.shutter_speed);
        display_kvf("aperture", "%u", (unsigned)d.aperture);
        display_kvf("iso", "%u", (unsigned)d.iso);
        display_kvf("exposure_type", "%u", (unsigned)d.exposure_type);
        display_kvf("command_id", "%u", (unsigned)d.command_id);
        display_kvf("engine_cut_off (ds)", "%u", (unsigned)d.engine_cut_off);
        display_kvf("extra_param", "%u", (unsigned)d.extra_param);
        display_kvf("extra_value", "%.4g", (double)d.extra_value);
        break;
    }
    case 155: { // DIGICAM_CONTROL
        mavlink_digicam_control_t d;
        mavlink_msg_digicam_control_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("session", "%u", (unsigned)d.session);
        display_kvf("zoom_pos", "%u", (unsigned)d.zoom_pos);
        display_kvf("zoom_step", "%d", (int)d.zoom_step);
        display_kvf("focus_lock", "%u", (unsigned)d.focus_lock);
        display_kvf("shot", "%u", (unsigned)d.shot);
        display_kvf("command_id", "%u", (unsigned)d.command_id);
        display_kvf("extra_param", "%u", (unsigned)d.extra_param);
        display_kvf("extra_value", "%.4g", (double)d.extra_value);
        break;
    }
    case 156: { // MOUNT_CONFIGURE
        mavlink_mount_configure_t d;
        mavlink_msg_mount_configure_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("mount_mode", "%u", (unsigned)d.mount_mode);
        display_kvf("stab_roll", "%u", (unsigned)d.stab_roll);
        display_kvf("stab_pitch", "%u", (unsigned)d.stab_pitch);
        display_kvf("stab_yaw", "%u", (unsigned)d.stab_yaw);
        break;
    }
    case 157: { // MOUNT_CONTROL
        mavlink_mount_control_t d;
        mavlink_msg_mount_control_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("input_a", "%d", (int)d.input_a);
        display_kvf("input_b", "%d", (int)d.input_b);
        display_kvf("input_c", "%d", (int)d.input_c);
        display_kvf("save_position", "%u", (unsigned)d.save_position);
        break;
    }
    case 158: { // MOUNT_STATUS
        mavlink_mount_status_t d;
        mavlink_msg_mount_status_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("pointing_a (cdeg)", "%d", (int)d.pointing_a);
        display_kvf("pointing_b (cdeg)", "%d", (int)d.pointing_b);
        display_kvf("pointing_c (cdeg)", "%d", (int)d.pointing_c);
        display_kvf("mount_mode", "%u", (unsigned)d.mount_mode);
        break;
    }
    case 160: { // FENCE_POINT
        mavlink_fence_point_t d;
        mavlink_msg_fence_point_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("idx", "%u", (unsigned)d.idx);
        display_kvf("count", "%u", (unsigned)d.count);
        display_kvf("lat (deg)", "%.4g", (double)d.lat);
        display_kvf("lng (deg)", "%.4g", (double)d.lng);
        break;
    }
    case 161: { // FENCE_FETCH_POINT
        mavlink_fence_fetch_point_t d;
        mavlink_msg_fence_fetch_point_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("idx", "%u", (unsigned)d.idx);
        break;
    }
    case 162: { // FENCE_STATUS
        mavlink_fence_status_t d;
        mavlink_msg_fence_status_decode(&raw, &d);
        display_kvf("breach_status", "%u", (unsigned)d.breach_status);
        display_kvf("breach_count", "%u", (unsigned)d.breach_count);
        display_kvf("breach_type", "%u", (unsigned)d.breach_type);
        display_kvf("breach_time (ms)", "%u", (unsigned)d.breach_time);
        display_kvf("breach_mitigation", "%u", (unsigned)d.breach_mitigation);
        break;
    }
    case 163: { // AHRS
        mavlink_ahrs_t d;
        mavlink_msg_ahrs_decode(&raw, &d);
        display_kvf("omegaIx (rad/s)", "%.4g", (double)d.omegaIx);
        display_kvf("omegaIy (rad/s)", "%.4g", (double)d.omegaIy);
        display_kvf("omegaIz (rad/s)", "%.4g", (double)d.omegaIz);
        display_kvf("accel_weight", "%.4g", (double)d.accel_weight);
        display_kvf("renorm_val", "%.4g", (double)d.renorm_val);
        display_kvf("error_rp", "%.4g", (double)d.error_rp);
        display_kvf("error_yaw", "%.4g", (double)d.error_yaw);
        break;
    }
    case 164: { // SIMSTATE
        mavlink_simstate_t d;
        mavlink_msg_simstate_decode(&raw, &d);
        display_kvf("roll (rad)", "%.4g", (double)d.roll);
        display_kvf("pitch (rad)", "%.4g", (double)d.pitch);
        display_kvf("yaw (rad)", "%.4g", (double)d.yaw);
        display_kvf("xacc (m/s/s)", "%.4g", (double)d.xacc);
        display_kvf("yacc (m/s/s)", "%.4g", (double)d.yacc);
        display_kvf("zacc (m/s/s)", "%.4g", (double)d.zacc);
        display_kvf("xgyro (rad/s)", "%.4g", (double)d.xgyro);
        display_kvf("ygyro (rad/s)", "%.4g", (double)d.ygyro);
        display_kvf("zgyro (rad/s)", "%.4g", (double)d.zgyro);
        display_kvf("lat (degE7)", "%d", (int)d.lat);
        display_kvf("lng (degE7)", "%d", (int)d.lng);
        break;
    }
    case 165: { // HWSTATUS
        mavlink_hwstatus_t d;
        mavlink_msg_hwstatus_decode(&raw, &d);
        display_kvf("Vcc (mV)", "%u", (unsigned)d.Vcc);
        display_kvf("I2Cerr", "%u", (unsigned)d.I2Cerr);
        break;
    }
    case 166: { // RADIO
        mavlink_radio_t d;
        mavlink_msg_radio_decode(&raw, &d);
        display_kvf("rssi", "%u", (unsigned)d.rssi);
        display_kvf("remrssi", "%u", (unsigned)d.remrssi);
        display_kvf("txbuf (%)", "%u", (unsigned)d.txbuf);
        display_kvf("noise", "%u", (unsigned)d.noise);
        display_kvf("remnoise", "%u", (unsigned)d.remnoise);
        display_kvf("rxerrors", "%u", (unsigned)d.rxerrors);
        display_kvf("fixed", "%u", (unsigned)d.fixed);
        break;
    }
    case 167: { // LIMITS_STATUS
        mavlink_limits_status_t d;
        mavlink_msg_limits_status_decode(&raw, &d);
        display_kvf("limits_state", "%u", (unsigned)d.limits_state);
        display_kvf("last_trigger (ms)", "%u", (unsigned)d.last_trigger);
        display_kvf("last_action (ms)", "%u", (unsigned)d.last_action);
        display_kvf("last_recovery (ms)", "%u", (unsigned)d.last_recovery);
        display_kvf("last_clear (ms)", "%u", (unsigned)d.last_clear);
        display_kvf("breach_count", "%u", (unsigned)d.breach_count);
        display_kvf("mods_enabled", "%u", (unsigned)d.mods_enabled);
        display_kvf("mods_required", "%u", (unsigned)d.mods_required);
        display_kvf("mods_triggered", "%u", (unsigned)d.mods_triggered);
        break;
    }
    case 168: { // WIND
        mavlink_wind_t d;
        mavlink_msg_wind_decode(&raw, &d);
        display_kvf("direction (deg)", "%.4g", (double)d.direction);
        display_kvf("speed (m/s)", "%.4g", (double)d.speed);
        display_kvf("speed_z (m/s)", "%.4g", (double)d.speed_z);
        break;
    }
    case 169: { // DATA16
        mavlink_data16_t d;
        mavlink_msg_data16_decode(&raw, &d);
        display_kvf("type", "%u", (unsigned)d.type);
        display_kvf("len (bytes)", "%u", (unsigned)d.len);
        {
            char _arr[388]; int _n = 0;
            for (int _i = 0; _i < 16; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.data[_i]);
            (void)_n; display_kv("data", _arr);
        }
        break;
    }
    case 170: { // DATA32
        mavlink_data32_t d;
        mavlink_msg_data32_decode(&raw, &d);
        display_kvf("type", "%u", (unsigned)d.type);
        display_kvf("len (bytes)", "%u", (unsigned)d.len);
        display_kv("data", "[32 elements]");
        break;
    }
    case 171: { // DATA64
        mavlink_data64_t d;
        mavlink_msg_data64_decode(&raw, &d);
        display_kvf("type", "%u", (unsigned)d.type);
        display_kvf("len (bytes)", "%u", (unsigned)d.len);
        display_kv("data", "[64 elements]");
        break;
    }
    case 172: { // DATA96
        mavlink_data96_t d;
        mavlink_msg_data96_decode(&raw, &d);
        display_kvf("type", "%u", (unsigned)d.type);
        display_kvf("len (bytes)", "%u", (unsigned)d.len);
        display_kv("data", "[96 elements]");
        break;
    }
    case 173: { // RANGEFINDER
        mavlink_rangefinder_t d;
        mavlink_msg_rangefinder_decode(&raw, &d);
        display_kvf("distance (m)", "%.4g", (double)d.distance);
        display_kvf("voltage (V)", "%.4g", (double)d.voltage);
        break;
    }
    case 174: { // AIRSPEED_AUTOCAL
        mavlink_airspeed_autocal_t d;
        mavlink_msg_airspeed_autocal_decode(&raw, &d);
        display_kvf("vx (m/s)", "%.4g", (double)d.vx);
        display_kvf("vy (m/s)", "%.4g", (double)d.vy);
        display_kvf("vz (m/s)", "%.4g", (double)d.vz);
        display_kvf("diff_pressure (Pa)", "%.4g", (double)d.diff_pressure);
        display_kvf("EAS2TAS", "%.4g", (double)d.EAS2TAS);
        display_kvf("ratio", "%.4g", (double)d.ratio);
        display_kvf("state_x", "%.4g", (double)d.state_x);
        display_kvf("state_y", "%.4g", (double)d.state_y);
        display_kvf("state_z", "%.4g", (double)d.state_z);
        display_kvf("Pax", "%.4g", (double)d.Pax);
        display_kvf("Pby", "%.4g", (double)d.Pby);
        display_kvf("Pcz", "%.4g", (double)d.Pcz);
        break;
    }
    case 175: { // RALLY_POINT
        mavlink_rally_point_t d;
        mavlink_msg_rally_point_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("idx", "%u", (unsigned)d.idx);
        display_kvf("count", "%u", (unsigned)d.count);
        display_kvf("lat (degE7)", "%d", (int)d.lat);
        display_kvf("lng (degE7)", "%d", (int)d.lng);
        display_kvf("alt (m)", "%d", (int)d.alt);
        display_kvf("break_alt (m)", "%d", (int)d.break_alt);
        display_kvf("land_dir (cdeg)", "%u", (unsigned)d.land_dir);
        display_kvf("flags", "%u", (unsigned)d.flags);
        break;
    }
    case 176: { // RALLY_FETCH_POINT
        mavlink_rally_fetch_point_t d;
        mavlink_msg_rally_fetch_point_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("idx", "%u", (unsigned)d.idx);
        break;
    }
    case 177: { // COMPASSMOT_STATUS
        mavlink_compassmot_status_t d;
        mavlink_msg_compassmot_status_decode(&raw, &d);
        display_kvf("throttle (d%)", "%u", (unsigned)d.throttle);
        display_kvf("current (A)", "%.4g", (double)d.current);
        display_kvf("interference (%)", "%u", (unsigned)d.interference);
        display_kvf("CompensationX", "%.4g", (double)d.CompensationX);
        display_kvf("CompensationY", "%.4g", (double)d.CompensationY);
        display_kvf("CompensationZ", "%.4g", (double)d.CompensationZ);
        break;
    }
    case 178: { // AHRS2
        mavlink_ahrs2_t d;
        mavlink_msg_ahrs2_decode(&raw, &d);
        display_kvf("roll (rad)", "%.4g", (double)d.roll);
        display_kvf("pitch (rad)", "%.4g", (double)d.pitch);
        display_kvf("yaw (rad)", "%.4g", (double)d.yaw);
        display_kvf("altitude (m)", "%.4g", (double)d.altitude);
        display_kvf("lat (degE7)", "%d", (int)d.lat);
        display_kvf("lng (degE7)", "%d", (int)d.lng);
        break;
    }
    case 179: { // CAMERA_STATUS
        mavlink_camera_status_t d;
        mavlink_msg_camera_status_decode(&raw, &d);
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("cam_idx", "%u", (unsigned)d.cam_idx);
        display_kvf("img_idx", "%u", (unsigned)d.img_idx);
        display_kvf("event_id", "%u", (unsigned)d.event_id);
        display_kvf("p1", "%.4g", (double)d.p1);
        display_kvf("p2", "%.4g", (double)d.p2);
        display_kvf("p3", "%.4g", (double)d.p3);
        display_kvf("p4", "%.4g", (double)d.p4);
        break;
    }
    case 180: { // CAMERA_FEEDBACK
        mavlink_camera_feedback_t d;
        mavlink_msg_camera_feedback_decode(&raw, &d);
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("cam_idx", "%u", (unsigned)d.cam_idx);
        display_kvf("img_idx", "%u", (unsigned)d.img_idx);
        display_kvf("lat (degE7)", "%d", (int)d.lat);
        display_kvf("lng (degE7)", "%d", (int)d.lng);
        display_kvf("alt_msl (m)", "%.4g", (double)d.alt_msl);
        display_kvf("alt_rel (m)", "%.4g", (double)d.alt_rel);
        display_kvf("roll (deg)", "%.4g", (double)d.roll);
        display_kvf("pitch (deg)", "%.4g", (double)d.pitch);
        display_kvf("yaw (deg)", "%.4g", (double)d.yaw);
        display_kvf("foc_len (mm)", "%.4g", (double)d.foc_len);
        display_kvf("flags", "%u", (unsigned)d.flags);
        display_kvf("completed_captures", "%u", (unsigned)d.completed_captures);
        break;
    }
    case 181: { // BATTERY2
        mavlink_battery2_t d;
        mavlink_msg_battery2_decode(&raw, &d);
        display_kvf("voltage (mV)", "%u", (unsigned)d.voltage);
        display_kvf("current_battery (cA)", "%d", (int)d.current_battery);
        break;
    }
    case 182: { // AHRS3
        mavlink_ahrs3_t d;
        mavlink_msg_ahrs3_decode(&raw, &d);
        display_kvf("roll (rad)", "%.4g", (double)d.roll);
        display_kvf("pitch (rad)", "%.4g", (double)d.pitch);
        display_kvf("yaw (rad)", "%.4g", (double)d.yaw);
        display_kvf("altitude (m)", "%.4g", (double)d.altitude);
        display_kvf("lat (degE7)", "%d", (int)d.lat);
        display_kvf("lng (degE7)", "%d", (int)d.lng);
        display_kvf("v1", "%.4g", (double)d.v1);
        display_kvf("v2", "%.4g", (double)d.v2);
        display_kvf("v3", "%.4g", (double)d.v3);
        display_kvf("v4", "%.4g", (double)d.v4);
        break;
    }
    case 183: { // AUTOPILOT_VERSION_REQUEST
        mavlink_autopilot_version_request_t d;
        mavlink_msg_autopilot_version_request_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        break;
    }
    case 184: { // REMOTE_LOG_DATA_BLOCK
        mavlink_remote_log_data_block_t d;
        mavlink_msg_remote_log_data_block_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("seqno", "%u", (unsigned)d.seqno);
        display_kv("data", "[200 elements]");
        break;
    }
    case 185: { // REMOTE_LOG_BLOCK_STATUS
        mavlink_remote_log_block_status_t d;
        mavlink_msg_remote_log_block_status_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("seqno", "%u", (unsigned)d.seqno);
        display_kvf("status", "%u", (unsigned)d.status);
        break;
    }
    case 186: { // LED_CONTROL
        mavlink_led_control_t d;
        mavlink_msg_led_control_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("instance", "%u", (unsigned)d.instance);
        display_kvf("pattern", "%u", (unsigned)d.pattern);
        display_kvf("custom_len", "%u", (unsigned)d.custom_len);
        display_kv("custom_bytes", "[24 elements]");
        break;
    }
    case 191: { // MAG_CAL_PROGRESS
        mavlink_mag_cal_progress_t d;
        mavlink_msg_mag_cal_progress_decode(&raw, &d);
        display_kvf("compass_id", "%u", (unsigned)d.compass_id);
        display_kvf("cal_mask", "%u", (unsigned)d.cal_mask);
        display_kvf("cal_status", "%u", (unsigned)d.cal_status);
        display_kvf("attempt", "%u", (unsigned)d.attempt);
        display_kvf("completion_pct (%)", "%u", (unsigned)d.completion_pct);
        {
            char _arr[244]; int _n = 0;
            for (int _i = 0; _i < 10; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.completion_mask[_i]);
            (void)_n; display_kv("completion_mask", _arr);
        }
        display_kvf("direction_x", "%.4g", (double)d.direction_x);
        display_kvf("direction_y", "%.4g", (double)d.direction_y);
        display_kvf("direction_z", "%.4g", (double)d.direction_z);
        break;
    }
    case 192: { // MAG_CAL_REPORT
        mavlink_mag_cal_report_t d;
        mavlink_msg_mag_cal_report_decode(&raw, &d);
        display_kvf("compass_id", "%u", (unsigned)d.compass_id);
        display_kvf("cal_mask", "%u", (unsigned)d.cal_mask);
        display_kvf("cal_status", "%u", (unsigned)d.cal_status);
        display_kvf("autosaved", "%u", (unsigned)d.autosaved);
        display_kvf("fitness (mgauss)", "%.4g", (double)d.fitness);
        display_kvf("ofs_x", "%.4g", (double)d.ofs_x);
        display_kvf("ofs_y", "%.4g", (double)d.ofs_y);
        display_kvf("ofs_z", "%.4g", (double)d.ofs_z);
        display_kvf("diag_x", "%.4g", (double)d.diag_x);
        display_kvf("diag_y", "%.4g", (double)d.diag_y);
        display_kvf("diag_z", "%.4g", (double)d.diag_z);
        display_kvf("offdiag_x", "%.4g", (double)d.offdiag_x);
        display_kvf("offdiag_y", "%.4g", (double)d.offdiag_y);
        display_kvf("offdiag_z", "%.4g", (double)d.offdiag_z);
        display_kvf("orientation_confidence", "%.4g", (double)d.orientation_confidence);
        display_kvf("old_orientation", "%u", (unsigned)d.old_orientation);
        display_kvf("new_orientation", "%u", (unsigned)d.new_orientation);
        display_kvf("scale_factor", "%.4g", (double)d.scale_factor);
        break;
    }
    case 193: { // EKF_STATUS_REPORT
        mavlink_ekf_status_report_t d;
        mavlink_msg_ekf_status_report_decode(&raw, &d);
        display_kvf("flags", "%u", (unsigned)d.flags);
        display_kvf("velocity_variance", "%.4g", (double)d.velocity_variance);
        display_kvf("pos_horiz_variance", "%.4g", (double)d.pos_horiz_variance);
        display_kvf("pos_vert_variance", "%.4g", (double)d.pos_vert_variance);
        display_kvf("compass_variance", "%.4g", (double)d.compass_variance);
        display_kvf("terrain_alt_variance", "%.4g", (double)d.terrain_alt_variance);
        display_kvf("airspeed_variance", "%.4g", (double)d.airspeed_variance);
        break;
    }
    case 194: { // PID_TUNING
        mavlink_pid_tuning_t d;
        mavlink_msg_pid_tuning_decode(&raw, &d);
        display_kvf("axis", "%u", (unsigned)d.axis);
        display_kvf("desired", "%.4g", (double)d.desired);
        display_kvf("achieved", "%.4g", (double)d.achieved);
        display_kvf("FF", "%.4g", (double)d.FF);
        display_kvf("P", "%.4g", (double)d.P);
        display_kvf("I", "%.4g", (double)d.I);
        display_kvf("D", "%.4g", (double)d.D);
        display_kvf("SRate", "%.4g", (double)d.SRate);
        display_kvf("PDmod", "%.4g", (double)d.PDmod);
        break;
    }
    case 195: { // DEEPSTALL
        mavlink_deepstall_t d;
        mavlink_msg_deepstall_decode(&raw, &d);
        display_kvf("landing_lat (degE7)", "%d", (int)d.landing_lat);
        display_kvf("landing_lon (degE7)", "%d", (int)d.landing_lon);
        display_kvf("path_lat (degE7)", "%d", (int)d.path_lat);
        display_kvf("path_lon (degE7)", "%d", (int)d.path_lon);
        display_kvf("arc_entry_lat (degE7)", "%d", (int)d.arc_entry_lat);
        display_kvf("arc_entry_lon (degE7)", "%d", (int)d.arc_entry_lon);
        display_kvf("altitude (m)", "%.4g", (double)d.altitude);
        display_kvf("expected_travel_distance (m)", "%.4g", (double)d.expected_travel_distance);
        display_kvf("cross_track_error (m)", "%.4g", (double)d.cross_track_error);
        display_kvf("stage", "%u", (unsigned)d.stage);
        break;
    }
    case 200: { // GIMBAL_REPORT
        mavlink_gimbal_report_t d;
        mavlink_msg_gimbal_report_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("delta_time (s)", "%.4g", (double)d.delta_time);
        display_kvf("delta_angle_x (rad)", "%.4g", (double)d.delta_angle_x);
        display_kvf("delta_angle_y (rad)", "%.4g", (double)d.delta_angle_y);
        display_kvf("delta_angle_z (rad)", "%.4g", (double)d.delta_angle_z);
        display_kvf("delta_velocity_x (m/s)", "%.4g", (double)d.delta_velocity_x);
        display_kvf("delta_velocity_y (m/s)", "%.4g", (double)d.delta_velocity_y);
        display_kvf("delta_velocity_z (m/s)", "%.4g", (double)d.delta_velocity_z);
        display_kvf("joint_roll (rad)", "%.4g", (double)d.joint_roll);
        display_kvf("joint_el (rad)", "%.4g", (double)d.joint_el);
        display_kvf("joint_az (rad)", "%.4g", (double)d.joint_az);
        break;
    }
    case 201: { // GIMBAL_CONTROL
        mavlink_gimbal_control_t d;
        mavlink_msg_gimbal_control_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("demanded_rate_x (rad/s)", "%.4g", (double)d.demanded_rate_x);
        display_kvf("demanded_rate_y (rad/s)", "%.4g", (double)d.demanded_rate_y);
        display_kvf("demanded_rate_z (rad/s)", "%.4g", (double)d.demanded_rate_z);
        break;
    }
    case 214: { // GIMBAL_TORQUE_CMD_REPORT
        mavlink_gimbal_torque_cmd_report_t d;
        mavlink_msg_gimbal_torque_cmd_report_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("rl_torque_cmd", "%d", (int)d.rl_torque_cmd);
        display_kvf("el_torque_cmd", "%d", (int)d.el_torque_cmd);
        display_kvf("az_torque_cmd", "%d", (int)d.az_torque_cmd);
        break;
    }
    case 215: { // GOPRO_HEARTBEAT
        mavlink_gopro_heartbeat_t d;
        mavlink_msg_gopro_heartbeat_decode(&raw, &d);
        display_kvf("status", "%u", (unsigned)d.status);
        display_kvf("capture_mode", "%u", (unsigned)d.capture_mode);
        display_kvf("flags", "%u", (unsigned)d.flags);
        break;
    }
    case 216: { // GOPRO_GET_REQUEST
        mavlink_gopro_get_request_t d;
        mavlink_msg_gopro_get_request_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("cmd_id", "%u", (unsigned)d.cmd_id);
        break;
    }
    case 217: { // GOPRO_GET_RESPONSE
        mavlink_gopro_get_response_t d;
        mavlink_msg_gopro_get_response_decode(&raw, &d);
        display_kvf("cmd_id", "%u", (unsigned)d.cmd_id);
        display_kvf("status", "%u", (unsigned)d.status);
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.value[_i]);
            (void)_n; display_kv("value", _arr);
        }
        break;
    }
    case 218: { // GOPRO_SET_REQUEST
        mavlink_gopro_set_request_t d;
        mavlink_msg_gopro_set_request_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("cmd_id", "%u", (unsigned)d.cmd_id);
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.value[_i]);
            (void)_n; display_kv("value", _arr);
        }
        break;
    }
    case 219: { // GOPRO_SET_RESPONSE
        mavlink_gopro_set_response_t d;
        mavlink_msg_gopro_set_response_decode(&raw, &d);
        display_kvf("cmd_id", "%u", (unsigned)d.cmd_id);
        display_kvf("status", "%u", (unsigned)d.status);
        break;
    }
    case 225: { // EFI_STATUS
        mavlink_efi_status_t d;
        mavlink_msg_efi_status_decode(&raw, &d);
        display_kvf("health", "%u", (unsigned)d.health);
        display_kvf("ecu_index", "%.4g", (double)d.ecu_index);
        display_kvf("rpm", "%.4g", (double)d.rpm);
        display_kvf("fuel_consumed (cm^3)", "%.4g", (double)d.fuel_consumed);
        display_kvf("fuel_flow (cm^3/min)", "%.4g", (double)d.fuel_flow);
        display_kvf("engine_load (%)", "%.4g", (double)d.engine_load);
        display_kvf("throttle_position (%)", "%.4g", (double)d.throttle_position);
        display_kvf("spark_dwell_time (ms)", "%.4g", (double)d.spark_dwell_time);
        display_kvf("barometric_pressure (kPa)", "%.4g", (double)d.barometric_pressure);
        display_kvf("intake_manifold_pressure (kPa)", "%.4g", (double)d.intake_manifold_pressure);
        display_kvf("intake_manifold_temperature (degC)", "%.4g", (double)d.intake_manifold_temperature);
        display_kvf("cylinder_head_temperature (degC)", "%.4g", (double)d.cylinder_head_temperature);
        display_kvf("ignition_timing (deg)", "%.4g", (double)d.ignition_timing);
        display_kvf("injection_time (ms)", "%.4g", (double)d.injection_time);
        display_kvf("exhaust_gas_temperature (degC)", "%.4g", (double)d.exhaust_gas_temperature);
        display_kvf("throttle_out (%)", "%.4g", (double)d.throttle_out);
        display_kvf("pt_compensation", "%.4g", (double)d.pt_compensation);
        display_kvf("ignition_voltage (V)", "%.4g", (double)d.ignition_voltage);
        display_kvf("fuel_pressure (kPa)", "%.4g", (double)d.fuel_pressure);
        break;
    }
    case 226: { // RPM
        mavlink_rpm_t d;
        mavlink_msg_rpm_decode(&raw, &d);
        display_kvf("rpm1", "%.4g", (double)d.rpm1);
        display_kvf("rpm2", "%.4g", (double)d.rpm2);
        break;
    }
    case 230: { // ESTIMATOR_STATUS
        mavlink_estimator_status_t d;
        mavlink_msg_estimator_status_decode(&raw, &d);
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        display_kvf("flags", "%u", (unsigned)d.flags);
        display_kvf("vel_ratio", "%.4g", (double)d.vel_ratio);
        display_kvf("pos_horiz_ratio", "%.4g", (double)d.pos_horiz_ratio);
        display_kvf("pos_vert_ratio", "%.4g", (double)d.pos_vert_ratio);
        display_kvf("mag_ratio", "%.4g", (double)d.mag_ratio);
        display_kvf("hagl_ratio", "%.4g", (double)d.hagl_ratio);
        display_kvf("tas_ratio", "%.4g", (double)d.tas_ratio);
        display_kvf("pos_horiz_accuracy (m)", "%.4g", (double)d.pos_horiz_accuracy);
        display_kvf("pos_vert_accuracy (m)", "%.4g", (double)d.pos_vert_accuracy);
        break;
    }
    case 231: { // WIND_COV
        mavlink_wind_cov_t d;
        mavlink_msg_wind_cov_decode(&raw, &d);
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        display_kvf("wind_x (m/s)", "%.4g", (double)d.wind_x);
        display_kvf("wind_y (m/s)", "%.4g", (double)d.wind_y);
        display_kvf("wind_z (m/s)", "%.4g", (double)d.wind_z);
        display_kvf("var_horiz (m/s)", "%.4g", (double)d.var_horiz);
        display_kvf("var_vert (m/s)", "%.4g", (double)d.var_vert);
        display_kvf("wind_alt (m)", "%.4g", (double)d.wind_alt);
        display_kvf("horiz_accuracy (m/s)", "%.4g", (double)d.horiz_accuracy);
        display_kvf("vert_accuracy (m/s)", "%.4g", (double)d.vert_accuracy);
        break;
    }
    case 232: { // GPS_INPUT
        mavlink_gps_input_t d;
        mavlink_msg_gps_input_decode(&raw, &d);
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        display_kvf("gps_id", "%u", (unsigned)d.gps_id);
        display_kvf("ignore_flags", "%u", (unsigned)d.ignore_flags);
        display_kvf("time_week_ms (ms)", "%u", (unsigned)d.time_week_ms);
        display_kvf("time_week", "%u", (unsigned)d.time_week);
        display_kvf("fix_type", "%u", (unsigned)d.fix_type);
        display_kvf("lat (degE7)", "%d", (int)d.lat);
        display_kvf("lon (degE7)", "%d", (int)d.lon);
        display_kvf("alt (m)", "%.4g", (double)d.alt);
        display_kvf("hdop", "%.4g", (double)d.hdop);
        display_kvf("vdop", "%.4g", (double)d.vdop);
        display_kvf("vn (m/s)", "%.4g", (double)d.vn);
        display_kvf("ve (m/s)", "%.4g", (double)d.ve);
        display_kvf("vd (m/s)", "%.4g", (double)d.vd);
        display_kvf("speed_accuracy (m/s)", "%.4g", (double)d.speed_accuracy);
        display_kvf("horiz_accuracy (m)", "%.4g", (double)d.horiz_accuracy);
        display_kvf("vert_accuracy (m)", "%.4g", (double)d.vert_accuracy);
        display_kvf("satellites_visible", "%u", (unsigned)d.satellites_visible);
        display_kvf("yaw (cdeg)", "%u", (unsigned)d.yaw);
        break;
    }
    case 233: { // GPS_RTCM_DATA
        mavlink_gps_rtcm_data_t d;
        mavlink_msg_gps_rtcm_data_decode(&raw, &d);
        display_kvf("flags", "%u", (unsigned)d.flags);
        display_kvf("len (bytes)", "%u", (unsigned)d.len);
        display_kv("data", "[180 elements]");
        break;
    }
    case 234: { // HIGH_LATENCY
        mavlink_high_latency_t d;
        mavlink_msg_high_latency_decode(&raw, &d);
        display_kvf("base_mode", "%u", (unsigned)d.base_mode);
        display_kvf("custom_mode", "%u", (unsigned)d.custom_mode);
        display_kvf("landed_state", "%u", (unsigned)d.landed_state);
        display_kvf("roll (cdeg)", "%d", (int)d.roll);
        display_kvf("pitch (cdeg)", "%d", (int)d.pitch);
        display_kvf("heading (cdeg)", "%u", (unsigned)d.heading);
        display_kvf("throttle (%)", "%d", (int)d.throttle);
        display_kvf("heading_sp (cdeg)", "%d", (int)d.heading_sp);
        display_kvf("latitude (degE7)", "%d", (int)d.latitude);
        display_kvf("longitude (degE7)", "%d", (int)d.longitude);
        display_kvf("altitude_amsl (m)", "%d", (int)d.altitude_amsl);
        display_kvf("altitude_sp (m)", "%d", (int)d.altitude_sp);
        display_kvf("airspeed (m/s)", "%u", (unsigned)d.airspeed);
        display_kvf("airspeed_sp (m/s)", "%u", (unsigned)d.airspeed_sp);
        display_kvf("groundspeed (m/s)", "%u", (unsigned)d.groundspeed);
        display_kvf("climb_rate (m/s)", "%d", (int)d.climb_rate);
        display_kvf("gps_nsat", "%u", (unsigned)d.gps_nsat);
        display_kvf("gps_fix_type", "%u", (unsigned)d.gps_fix_type);
        display_kvf("battery_remaining (%)", "%u", (unsigned)d.battery_remaining);
        display_kvf("temperature (degC)", "%d", (int)d.temperature);
        display_kvf("temperature_air (degC)", "%d", (int)d.temperature_air);
        display_kvf("failsafe", "%u", (unsigned)d.failsafe);
        display_kvf("wp_num", "%u", (unsigned)d.wp_num);
        display_kvf("wp_distance (m)", "%u", (unsigned)d.wp_distance);
        break;
    }
    case 235: { // HIGH_LATENCY2
        mavlink_high_latency2_t d;
        mavlink_msg_high_latency2_decode(&raw, &d);
        display_kvf("timestamp (ms)", "%u", (unsigned)d.timestamp);
        display_kvf("type", "%u", (unsigned)d.type);
        display_kvf("autopilot", "%u", (unsigned)d.autopilot);
        display_kvf("custom_mode", "%u", (unsigned)d.custom_mode);
        display_kvf("latitude (degE7)", "%d", (int)d.latitude);
        display_kvf("longitude (degE7)", "%d", (int)d.longitude);
        display_kvf("altitude (m)", "%d", (int)d.altitude);
        display_kvf("target_altitude (m)", "%d", (int)d.target_altitude);
        display_kvf("heading (deg/2)", "%u", (unsigned)d.heading);
        display_kvf("target_heading (deg/2)", "%u", (unsigned)d.target_heading);
        display_kvf("target_distance (dam)", "%u", (unsigned)d.target_distance);
        display_kvf("throttle (%)", "%u", (unsigned)d.throttle);
        display_kvf("airspeed (m/s*5)", "%u", (unsigned)d.airspeed);
        display_kvf("airspeed_sp (m/s*5)", "%u", (unsigned)d.airspeed_sp);
        display_kvf("groundspeed (m/s*5)", "%u", (unsigned)d.groundspeed);
        display_kvf("windspeed (m/s*5)", "%u", (unsigned)d.windspeed);
        display_kvf("wind_heading (deg/2)", "%u", (unsigned)d.wind_heading);
        display_kvf("eph (dm)", "%u", (unsigned)d.eph);
        display_kvf("epv (dm)", "%u", (unsigned)d.epv);
        display_kvf("temperature_air (degC)", "%d", (int)d.temperature_air);
        display_kvf("climb_rate (dm/s)", "%d", (int)d.climb_rate);
        display_kvf("battery (%)", "%d", (int)d.battery);
        display_kvf("wp_num", "%u", (unsigned)d.wp_num);
        display_kvf("failure_flags", "%u", (unsigned)d.failure_flags);
        display_kvf("custom0", "%d", (int)d.custom0);
        display_kvf("custom1", "%d", (int)d.custom1);
        display_kvf("custom2", "%d", (int)d.custom2);
        break;
    }
    case 241: { // VIBRATION
        mavlink_vibration_t d;
        mavlink_msg_vibration_decode(&raw, &d);
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        display_kvf("vibration_x", "%.4g", (double)d.vibration_x);
        display_kvf("vibration_y", "%.4g", (double)d.vibration_y);
        display_kvf("vibration_z", "%.4g", (double)d.vibration_z);
        display_kvf("clipping_0", "%u", (unsigned)d.clipping_0);
        display_kvf("clipping_1", "%u", (unsigned)d.clipping_1);
        display_kvf("clipping_2", "%u", (unsigned)d.clipping_2);
        break;
    }
    case 242: { // HOME_POSITION
        mavlink_home_position_t d;
        mavlink_msg_home_position_decode(&raw, &d);
        display_kvf("latitude (degE7)", "%d", (int)d.latitude);
        display_kvf("longitude (degE7)", "%d", (int)d.longitude);
        display_kvf("altitude (mm)", "%d", (int)d.altitude);
        display_kvf("x (m)", "%.4g", (double)d.x);
        display_kvf("y (m)", "%.4g", (double)d.y);
        display_kvf("z (m)", "%.4g", (double)d.z);
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%.4g" : "%.4g", (double)d.q[_i]);
            (void)_n; display_kv("q", _arr);
        }
        display_kvf("approach_x (m)", "%.4g", (double)d.approach_x);
        display_kvf("approach_y (m)", "%.4g", (double)d.approach_y);
        display_kvf("approach_z (m)", "%.4g", (double)d.approach_z);
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        break;
    }
    case 243: { // SET_HOME_POSITION
        mavlink_set_home_position_t d;
        mavlink_msg_set_home_position_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("latitude (degE7)", "%d", (int)d.latitude);
        display_kvf("longitude (degE7)", "%d", (int)d.longitude);
        display_kvf("altitude (mm)", "%d", (int)d.altitude);
        display_kvf("x (m)", "%.4g", (double)d.x);
        display_kvf("y (m)", "%.4g", (double)d.y);
        display_kvf("z (m)", "%.4g", (double)d.z);
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%.4g" : "%.4g", (double)d.q[_i]);
            (void)_n; display_kv("q", _arr);
        }
        display_kvf("approach_x (m)", "%.4g", (double)d.approach_x);
        display_kvf("approach_y (m)", "%.4g", (double)d.approach_y);
        display_kvf("approach_z (m)", "%.4g", (double)d.approach_z);
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        break;
    }
    case 244: { // MESSAGE_INTERVAL
        mavlink_message_interval_t d;
        mavlink_msg_message_interval_decode(&raw, &d);
        display_kvf("message_id", "%u", (unsigned)d.message_id);
        display_kvf("interval_us (us)", "%d", (int)d.interval_us);
        break;
    }
    case 245: { // EXTENDED_SYS_STATE
        mavlink_extended_sys_state_t d;
        mavlink_msg_extended_sys_state_decode(&raw, &d);
        display_kvf("vtol_state", "%u", (unsigned)d.vtol_state);
        display_kvf("landed_state", "%u", (unsigned)d.landed_state);
        break;
    }
    case 246: { // ADSB_VEHICLE
        mavlink_adsb_vehicle_t d;
        mavlink_msg_adsb_vehicle_decode(&raw, &d);
        display_kvf("ICAO_address", "%u", (unsigned)d.ICAO_address);
        display_kvf("lat (degE7)", "%d", (int)d.lat);
        display_kvf("lon (degE7)", "%d", (int)d.lon);
        display_kvf("altitude_type", "%u", (unsigned)d.altitude_type);
        display_kvf("altitude (mm)", "%d", (int)d.altitude);
        display_kvf("heading (cdeg)", "%u", (unsigned)d.heading);
        display_kvf("hor_velocity (cm/s)", "%u", (unsigned)d.hor_velocity);
        display_kvf("ver_velocity (cm/s)", "%d", (int)d.ver_velocity);
        {
            char _s[10];
            memcpy(_s, d.callsign, 9);
            _s[9] = '\0';
            display_kv("callsign", _s);
        }
        display_kvf("emitter_type", "%u", (unsigned)d.emitter_type);
        display_kvf("tslc (s)", "%u", (unsigned)d.tslc);
        display_kvf("flags", "%u", (unsigned)d.flags);
        display_kvf("squawk", "%u", (unsigned)d.squawk);
        break;
    }
    case 247: { // COLLISION
        mavlink_collision_t d;
        mavlink_msg_collision_decode(&raw, &d);
        display_kvf("src", "%u", (unsigned)d.src);
        display_kvf("id", "%u", (unsigned)d.id);
        display_kvf("action", "%u", (unsigned)d.action);
        display_kvf("threat_level", "%u", (unsigned)d.threat_level);
        display_kvf("time_to_minimum_delta (s)", "%.4g", (double)d.time_to_minimum_delta);
        display_kvf("altitude_minimum_delta (m)", "%.4g", (double)d.altitude_minimum_delta);
        display_kvf("horizontal_minimum_delta (m)", "%.4g", (double)d.horizontal_minimum_delta);
        break;
    }
    case 248: { // V2_EXTENSION
        mavlink_v2_extension_t d;
        mavlink_msg_v2_extension_decode(&raw, &d);
        display_kvf("target_network", "%u", (unsigned)d.target_network);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("message_type", "%u", (unsigned)d.message_type);
        display_kv("payload", "[249 elements]");
        break;
    }
    case 249: { // MEMORY_VECT
        mavlink_memory_vect_t d;
        mavlink_msg_memory_vect_decode(&raw, &d);
        display_kvf("address", "%u", (unsigned)d.address);
        display_kvf("ver", "%u", (unsigned)d.ver);
        display_kvf("type", "%u", (unsigned)d.type);
        display_kv("value", "[32 elements]");
        break;
    }
    case 250: { // DEBUG_VECT
        mavlink_debug_vect_t d;
        mavlink_msg_debug_vect_decode(&raw, &d);
        {
            char _s[11];
            memcpy(_s, d.name, 10);
            _s[10] = '\0';
            display_kv("name", _s);
        }
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        display_kvf("x", "%.4g", (double)d.x);
        display_kvf("y", "%.4g", (double)d.y);
        display_kvf("z", "%.4g", (double)d.z);
        break;
    }
    case 251: { // NAMED_VALUE_FLOAT
        mavlink_named_value_float_t d;
        mavlink_msg_named_value_float_decode(&raw, &d);
        display_kvf("time_boot_ms (ms)", "%u", (unsigned)d.time_boot_ms);
        {
            char _s[11];
            memcpy(_s, d.name, 10);
            _s[10] = '\0';
            display_kv("name", _s);
        }
        display_kvf("value", "%.4g", (double)d.value);
        break;
    }
    case 252: { // NAMED_VALUE_INT
        mavlink_named_value_int_t d;
        mavlink_msg_named_value_int_decode(&raw, &d);
        display_kvf("time_boot_ms (ms)", "%u", (unsigned)d.time_boot_ms);
        {
            char _s[11];
            memcpy(_s, d.name, 10);
            _s[10] = '\0';
            display_kv("name", _s);
        }
        display_kvf("value", "%d", (int)d.value);
        break;
    }
    case 253: { // STATUSTEXT
        mavlink_statustext_t d;
        mavlink_msg_statustext_decode(&raw, &d);
        display_kvf("severity", "%u", (unsigned)d.severity);
        {
            char _s[51];
            memcpy(_s, d.text, 50);
            _s[50] = '\0';
            display_kv("text", _s);
        }
        display_kvf("id", "%u", (unsigned)d.id);
        display_kvf("chunk_seq", "%u", (unsigned)d.chunk_seq);
        break;
    }
    case 254: { // DEBUG
        mavlink_debug_t d;
        mavlink_msg_debug_decode(&raw, &d);
        display_kvf("time_boot_ms (ms)", "%u", (unsigned)d.time_boot_ms);
        display_kvf("ind", "%u", (unsigned)d.ind);
        display_kvf("value", "%.4g", (double)d.value);
        break;
    }
    case 256: { // SETUP_SIGNING
        mavlink_setup_signing_t d;
        mavlink_msg_setup_signing_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kv("secret_key", "[32 elements]");
        display_kvf("initial_timestamp", "%llu", (unsigned long long)d.initial_timestamp);
        break;
    }
    case 257: { // BUTTON_CHANGE
        mavlink_button_change_t d;
        mavlink_msg_button_change_decode(&raw, &d);
        display_kvf("time_boot_ms (ms)", "%u", (unsigned)d.time_boot_ms);
        display_kvf("last_change_ms (ms)", "%u", (unsigned)d.last_change_ms);
        display_kvf("state", "%u", (unsigned)d.state);
        break;
    }
    case 258: { // PLAY_TUNE
        mavlink_play_tune_t d;
        mavlink_msg_play_tune_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        {
            char _s[31];
            memcpy(_s, d.tune, 30);
            _s[30] = '\0';
            display_kv("tune", _s);
        }
        {
            char _s[201];
            memcpy(_s, d.tune2, 200);
            _s[200] = '\0';
            display_kv("tune2", _s);
        }
        break;
    }
    case 259: { // CAMERA_INFORMATION
        mavlink_camera_information_t d;
        mavlink_msg_camera_information_decode(&raw, &d);
        display_kvf("time_boot_ms (ms)", "%u", (unsigned)d.time_boot_ms);
        display_kv("vendor_name", "[32 elements]");
        display_kv("model_name", "[32 elements]");
        display_kvf("firmware_version", "%u", (unsigned)d.firmware_version);
        display_kvf("focal_length (mm)", "%.4g", (double)d.focal_length);
        display_kvf("sensor_size_h (mm)", "%.4g", (double)d.sensor_size_h);
        display_kvf("sensor_size_v (mm)", "%.4g", (double)d.sensor_size_v);
        display_kvf("resolution_h (pix)", "%u", (unsigned)d.resolution_h);
        display_kvf("resolution_v (pix)", "%u", (unsigned)d.resolution_v);
        display_kvf("lens_id", "%u", (unsigned)d.lens_id);
        display_kvf("flags", "%u", (unsigned)d.flags);
        display_kvf("cam_definition_version", "%u", (unsigned)d.cam_definition_version);
        {
            char _s[141];
            memcpy(_s, d.cam_definition_uri, 140);
            _s[140] = '\0';
            display_kv("cam_definition_uri", _s);
        }
        display_kvf("gimbal_device_id", "%u", (unsigned)d.gimbal_device_id);
        display_kvf("camera_device_id", "%u", (unsigned)d.camera_device_id);
        break;
    }
    case 260: { // CAMERA_SETTINGS
        mavlink_camera_settings_t d;
        mavlink_msg_camera_settings_decode(&raw, &d);
        display_kvf("time_boot_ms (ms)", "%u", (unsigned)d.time_boot_ms);
        display_kvf("mode_id", "%u", (unsigned)d.mode_id);
        display_kvf("zoomLevel", "%.4g", (double)d.zoomLevel);
        display_kvf("focusLevel", "%.4g", (double)d.focusLevel);
        display_kvf("camera_device_id", "%u", (unsigned)d.camera_device_id);
        break;
    }
    case 261: { // STORAGE_INFORMATION
        mavlink_storage_information_t d;
        mavlink_msg_storage_information_decode(&raw, &d);
        display_kvf("time_boot_ms (ms)", "%u", (unsigned)d.time_boot_ms);
        display_kvf("storage_id", "%u", (unsigned)d.storage_id);
        display_kvf("storage_count", "%u", (unsigned)d.storage_count);
        display_kvf("status", "%u", (unsigned)d.status);
        display_kvf("total_capacity (MiB)", "%.4g", (double)d.total_capacity);
        display_kvf("used_capacity (MiB)", "%.4g", (double)d.used_capacity);
        display_kvf("available_capacity (MiB)", "%.4g", (double)d.available_capacity);
        display_kvf("read_speed (MiB/s)", "%.4g", (double)d.read_speed);
        display_kvf("write_speed (MiB/s)", "%.4g", (double)d.write_speed);
        display_kvf("type", "%u", (unsigned)d.type);
        {
            char _s[33];
            memcpy(_s, d.name, 32);
            _s[32] = '\0';
            display_kv("name", _s);
        }
        display_kvf("storage_usage", "%u", (unsigned)d.storage_usage);
        break;
    }
    case 262: { // CAMERA_CAPTURE_STATUS
        mavlink_camera_capture_status_t d;
        mavlink_msg_camera_capture_status_decode(&raw, &d);
        display_kvf("time_boot_ms (ms)", "%u", (unsigned)d.time_boot_ms);
        display_kvf("image_status", "%u", (unsigned)d.image_status);
        display_kvf("video_status", "%u", (unsigned)d.video_status);
        display_kvf("image_interval (s)", "%.4g", (double)d.image_interval);
        display_kvf("recording_time_ms (ms)", "%u", (unsigned)d.recording_time_ms);
        display_kvf("available_capacity (MiB)", "%.4g", (double)d.available_capacity);
        display_kvf("image_count", "%d", (int)d.image_count);
        display_kvf("camera_device_id", "%u", (unsigned)d.camera_device_id);
        break;
    }
    case 263: { // CAMERA_IMAGE_CAPTURED
        mavlink_camera_image_captured_t d;
        mavlink_msg_camera_image_captured_decode(&raw, &d);
        display_kvf("time_boot_ms (ms)", "%u", (unsigned)d.time_boot_ms);
        display_kvf("time_utc (us)", "%llu", (unsigned long long)d.time_utc);
        display_kvf("camera_id", "%u", (unsigned)d.camera_id);
        display_kvf("lat (degE7)", "%d", (int)d.lat);
        display_kvf("lon (degE7)", "%d", (int)d.lon);
        display_kvf("alt (mm)", "%d", (int)d.alt);
        display_kvf("relative_alt (mm)", "%d", (int)d.relative_alt);
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%.4g" : "%.4g", (double)d.q[_i]);
            (void)_n; display_kv("q", _arr);
        }
        display_kvf("image_index", "%d", (int)d.image_index);
        display_kvf("capture_result", "%d", (int)d.capture_result);
        {
            char _s[206];
            memcpy(_s, d.file_url, 205);
            _s[205] = '\0';
            display_kv("file_url", _s);
        }
        break;
    }
    case 264: { // FLIGHT_INFORMATION
        mavlink_flight_information_t d;
        mavlink_msg_flight_information_decode(&raw, &d);
        display_kvf("time_boot_ms (ms)", "%u", (unsigned)d.time_boot_ms);
        display_kvf("arming_time_utc (us)", "%llu", (unsigned long long)d.arming_time_utc);
        display_kvf("takeoff_time_utc (us)", "%llu", (unsigned long long)d.takeoff_time_utc);
        display_kvf("flight_uuid", "%llu", (unsigned long long)d.flight_uuid);
        display_kvf("landing_time (ms)", "%u", (unsigned)d.landing_time);
        break;
    }
    case 265: { // MOUNT_ORIENTATION
        mavlink_mount_orientation_t d;
        mavlink_msg_mount_orientation_decode(&raw, &d);
        display_kvf("time_boot_ms (ms)", "%u", (unsigned)d.time_boot_ms);
        display_kvf("roll (deg)", "%.4g", (double)d.roll);
        display_kvf("pitch (deg)", "%.4g", (double)d.pitch);
        display_kvf("yaw (deg)", "%.4g", (double)d.yaw);
        display_kvf("yaw_absolute (deg)", "%.4g", (double)d.yaw_absolute);
        break;
    }
    case 266: { // LOGGING_DATA
        mavlink_logging_data_t d;
        mavlink_msg_logging_data_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("sequence", "%u", (unsigned)d.sequence);
        display_kvf("length (bytes)", "%u", (unsigned)d.length);
        display_kvf("first_message_offset (bytes)", "%u", (unsigned)d.first_message_offset);
        display_kv("data", "[249 elements]");
        break;
    }
    case 267: { // LOGGING_DATA_ACKED
        mavlink_logging_data_acked_t d;
        mavlink_msg_logging_data_acked_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("sequence", "%u", (unsigned)d.sequence);
        display_kvf("length (bytes)", "%u", (unsigned)d.length);
        display_kvf("first_message_offset (bytes)", "%u", (unsigned)d.first_message_offset);
        display_kv("data", "[249 elements]");
        break;
    }
    case 268: { // LOGGING_ACK
        mavlink_logging_ack_t d;
        mavlink_msg_logging_ack_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("sequence", "%u", (unsigned)d.sequence);
        break;
    }
    case 269: { // VIDEO_STREAM_INFORMATION
        mavlink_video_stream_information_t d;
        mavlink_msg_video_stream_information_decode(&raw, &d);
        display_kvf("stream_id", "%u", (unsigned)d.stream_id);
        display_kvf("count", "%u", (unsigned)d.count);
        display_kvf("type", "%u", (unsigned)d.type);
        display_kvf("flags", "%u", (unsigned)d.flags);
        display_kvf("framerate (Hz)", "%.4g", (double)d.framerate);
        display_kvf("resolution_h (pix)", "%u", (unsigned)d.resolution_h);
        display_kvf("resolution_v (pix)", "%u", (unsigned)d.resolution_v);
        display_kvf("bitrate (bits/s)", "%u", (unsigned)d.bitrate);
        display_kvf("rotation (deg)", "%u", (unsigned)d.rotation);
        display_kvf("hfov (deg)", "%u", (unsigned)d.hfov);
        {
            char _s[33];
            memcpy(_s, d.name, 32);
            _s[32] = '\0';
            display_kv("name", _s);
        }
        {
            char _s[161];
            memcpy(_s, d.uri, 160);
            _s[160] = '\0';
            display_kv("uri", _s);
        }
        display_kvf("encoding", "%u", (unsigned)d.encoding);
        display_kvf("camera_device_id", "%u", (unsigned)d.camera_device_id);
        break;
    }
    case 270: { // VIDEO_STREAM_STATUS
        mavlink_video_stream_status_t d;
        mavlink_msg_video_stream_status_decode(&raw, &d);
        display_kvf("stream_id", "%u", (unsigned)d.stream_id);
        display_kvf("flags", "%u", (unsigned)d.flags);
        display_kvf("framerate (Hz)", "%.4g", (double)d.framerate);
        display_kvf("resolution_h (pix)", "%u", (unsigned)d.resolution_h);
        display_kvf("resolution_v (pix)", "%u", (unsigned)d.resolution_v);
        display_kvf("bitrate (bits/s)", "%u", (unsigned)d.bitrate);
        display_kvf("rotation (deg)", "%u", (unsigned)d.rotation);
        display_kvf("hfov (deg)", "%u", (unsigned)d.hfov);
        display_kvf("camera_device_id", "%u", (unsigned)d.camera_device_id);
        break;
    }
    case 271: { // CAMERA_FOV_STATUS
        mavlink_camera_fov_status_t d;
        mavlink_msg_camera_fov_status_decode(&raw, &d);
        display_kvf("time_boot_ms (ms)", "%u", (unsigned)d.time_boot_ms);
        display_kvf("lat_camera (degE7)", "%d", (int)d.lat_camera);
        display_kvf("lon_camera (degE7)", "%d", (int)d.lon_camera);
        display_kvf("alt_camera (mm)", "%d", (int)d.alt_camera);
        display_kvf("lat_image (degE7)", "%d", (int)d.lat_image);
        display_kvf("lon_image (degE7)", "%d", (int)d.lon_image);
        display_kvf("alt_image (mm)", "%d", (int)d.alt_image);
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%.4g" : "%.4g", (double)d.q[_i]);
            (void)_n; display_kv("q", _arr);
        }
        display_kvf("hfov (deg)", "%.4g", (double)d.hfov);
        display_kvf("vfov (deg)", "%.4g", (double)d.vfov);
        display_kvf("camera_device_id", "%u", (unsigned)d.camera_device_id);
        break;
    }
    case 275: { // CAMERA_TRACKING_IMAGE_STATUS
        mavlink_camera_tracking_image_status_t d;
        mavlink_msg_camera_tracking_image_status_decode(&raw, &d);
        display_kvf("tracking_status", "%u", (unsigned)d.tracking_status);
        display_kvf("tracking_mode", "%u", (unsigned)d.tracking_mode);
        display_kvf("target_data", "%u", (unsigned)d.target_data);
        display_kvf("point_x", "%.4g", (double)d.point_x);
        display_kvf("point_y", "%.4g", (double)d.point_y);
        display_kvf("radius", "%.4g", (double)d.radius);
        display_kvf("rec_top_x", "%.4g", (double)d.rec_top_x);
        display_kvf("rec_top_y", "%.4g", (double)d.rec_top_y);
        display_kvf("rec_bottom_x", "%.4g", (double)d.rec_bottom_x);
        display_kvf("rec_bottom_y", "%.4g", (double)d.rec_bottom_y);
        display_kvf("camera_device_id", "%u", (unsigned)d.camera_device_id);
        break;
    }
    case 276: { // CAMERA_TRACKING_GEO_STATUS
        mavlink_camera_tracking_geo_status_t d;
        mavlink_msg_camera_tracking_geo_status_decode(&raw, &d);
        display_kvf("tracking_status", "%u", (unsigned)d.tracking_status);
        display_kvf("lat (degE7)", "%d", (int)d.lat);
        display_kvf("lon (degE7)", "%d", (int)d.lon);
        display_kvf("alt (m)", "%.4g", (double)d.alt);
        display_kvf("h_acc (m)", "%.4g", (double)d.h_acc);
        display_kvf("v_acc (m)", "%.4g", (double)d.v_acc);
        display_kvf("vel_n (m/s)", "%.4g", (double)d.vel_n);
        display_kvf("vel_e (m/s)", "%.4g", (double)d.vel_e);
        display_kvf("vel_d (m/s)", "%.4g", (double)d.vel_d);
        display_kvf("vel_acc (m/s)", "%.4g", (double)d.vel_acc);
        display_kvf("dist (m)", "%.4g", (double)d.dist);
        display_kvf("hdg (rad)", "%.4g", (double)d.hdg);
        display_kvf("hdg_acc (rad)", "%.4g", (double)d.hdg_acc);
        display_kvf("camera_device_id", "%u", (unsigned)d.camera_device_id);
        break;
    }
    case 277: { // CAMERA_THERMAL_RANGE
        mavlink_camera_thermal_range_t d;
        mavlink_msg_camera_thermal_range_decode(&raw, &d);
        display_kvf("time_boot_ms (ms)", "%u", (unsigned)d.time_boot_ms);
        display_kvf("stream_id", "%u", (unsigned)d.stream_id);
        display_kvf("camera_device_id", "%u", (unsigned)d.camera_device_id);
        display_kvf("max (degC)", "%.4g", (double)d.max);
        display_kvf("max_point_x", "%.4g", (double)d.max_point_x);
        display_kvf("max_point_y", "%.4g", (double)d.max_point_y);
        display_kvf("min (degC)", "%.4g", (double)d.min);
        display_kvf("min_point_x", "%.4g", (double)d.min_point_x);
        display_kvf("min_point_y", "%.4g", (double)d.min_point_y);
        break;
    }
    case 280: { // GIMBAL_MANAGER_INFORMATION
        mavlink_gimbal_manager_information_t d;
        mavlink_msg_gimbal_manager_information_decode(&raw, &d);
        display_kvf("time_boot_ms (ms)", "%u", (unsigned)d.time_boot_ms);
        display_kvf("cap_flags", "%u", (unsigned)d.cap_flags);
        display_kvf("gimbal_device_id", "%u", (unsigned)d.gimbal_device_id);
        display_kvf("roll_min (rad)", "%.4g", (double)d.roll_min);
        display_kvf("roll_max (rad)", "%.4g", (double)d.roll_max);
        display_kvf("pitch_min (rad)", "%.4g", (double)d.pitch_min);
        display_kvf("pitch_max (rad)", "%.4g", (double)d.pitch_max);
        display_kvf("yaw_min (rad)", "%.4g", (double)d.yaw_min);
        display_kvf("yaw_max (rad)", "%.4g", (double)d.yaw_max);
        break;
    }
    case 281: { // GIMBAL_MANAGER_STATUS
        mavlink_gimbal_manager_status_t d;
        mavlink_msg_gimbal_manager_status_decode(&raw, &d);
        display_kvf("time_boot_ms (ms)", "%u", (unsigned)d.time_boot_ms);
        display_kvf("flags", "%u", (unsigned)d.flags);
        display_kvf("gimbal_device_id", "%u", (unsigned)d.gimbal_device_id);
        display_kvf("primary_control_sysid", "%u", (unsigned)d.primary_control_sysid);
        display_kvf("primary_control_compid", "%u", (unsigned)d.primary_control_compid);
        display_kvf("secondary_control_sysid", "%u", (unsigned)d.secondary_control_sysid);
        display_kvf("secondary_control_compid", "%u", (unsigned)d.secondary_control_compid);
        break;
    }
    case 282: { // GIMBAL_MANAGER_SET_ATTITUDE
        mavlink_gimbal_manager_set_attitude_t d;
        mavlink_msg_gimbal_manager_set_attitude_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("flags", "%u", (unsigned)d.flags);
        display_kvf("gimbal_device_id", "%u", (unsigned)d.gimbal_device_id);
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%.4g" : "%.4g", (double)d.q[_i]);
            (void)_n; display_kv("q", _arr);
        }
        display_kvf("angular_velocity_x (rad/s)", "%.4g", (double)d.angular_velocity_x);
        display_kvf("angular_velocity_y (rad/s)", "%.4g", (double)d.angular_velocity_y);
        display_kvf("angular_velocity_z (rad/s)", "%.4g", (double)d.angular_velocity_z);
        break;
    }
    case 283: { // GIMBAL_DEVICE_INFORMATION
        mavlink_gimbal_device_information_t d;
        mavlink_msg_gimbal_device_information_decode(&raw, &d);
        display_kvf("time_boot_ms (ms)", "%u", (unsigned)d.time_boot_ms);
        {
            char _s[33];
            memcpy(_s, d.vendor_name, 32);
            _s[32] = '\0';
            display_kv("vendor_name", _s);
        }
        {
            char _s[33];
            memcpy(_s, d.model_name, 32);
            _s[32] = '\0';
            display_kv("model_name", _s);
        }
        {
            char _s[33];
            memcpy(_s, d.custom_name, 32);
            _s[32] = '\0';
            display_kv("custom_name", _s);
        }
        display_kvf("firmware_version", "%u", (unsigned)d.firmware_version);
        display_kvf("hardware_version", "%u", (unsigned)d.hardware_version);
        display_kvf("uid", "%llu", (unsigned long long)d.uid);
        display_kvf("cap_flags", "%u", (unsigned)d.cap_flags);
        display_kvf("custom_cap_flags", "%u", (unsigned)d.custom_cap_flags);
        display_kvf("roll_min (rad)", "%.4g", (double)d.roll_min);
        display_kvf("roll_max (rad)", "%.4g", (double)d.roll_max);
        display_kvf("pitch_min (rad)", "%.4g", (double)d.pitch_min);
        display_kvf("pitch_max (rad)", "%.4g", (double)d.pitch_max);
        display_kvf("yaw_min (rad)", "%.4g", (double)d.yaw_min);
        display_kvf("yaw_max (rad)", "%.4g", (double)d.yaw_max);
        display_kvf("gimbal_device_id", "%u", (unsigned)d.gimbal_device_id);
        display_kvf("cap_flags2", "%u", (unsigned)d.cap_flags2);
        break;
    }
    case 284: { // GIMBAL_DEVICE_SET_ATTITUDE
        mavlink_gimbal_device_set_attitude_t d;
        mavlink_msg_gimbal_device_set_attitude_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("flags", "%u", (unsigned)d.flags);
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%.4g" : "%.4g", (double)d.q[_i]);
            (void)_n; display_kv("q", _arr);
        }
        display_kvf("angular_velocity_x (rad/s)", "%.4g", (double)d.angular_velocity_x);
        display_kvf("angular_velocity_y (rad/s)", "%.4g", (double)d.angular_velocity_y);
        display_kvf("angular_velocity_z (rad/s)", "%.4g", (double)d.angular_velocity_z);
        break;
    }
    case 285: { // GIMBAL_DEVICE_ATTITUDE_STATUS
        mavlink_gimbal_device_attitude_status_t d;
        mavlink_msg_gimbal_device_attitude_status_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("time_boot_ms (ms)", "%u", (unsigned)d.time_boot_ms);
        display_kvf("flags", "%u", (unsigned)d.flags);
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%.4g" : "%.4g", (double)d.q[_i]);
            (void)_n; display_kv("q", _arr);
        }
        display_kvf("angular_velocity_x (rad/s)", "%.4g", (double)d.angular_velocity_x);
        display_kvf("angular_velocity_y (rad/s)", "%.4g", (double)d.angular_velocity_y);
        display_kvf("angular_velocity_z (rad/s)", "%.4g", (double)d.angular_velocity_z);
        display_kvf("failure_flags", "%u", (unsigned)d.failure_flags);
        display_kvf("delta_yaw (rad)", "%.4g", (double)d.delta_yaw);
        display_kvf("delta_yaw_velocity (rad/s)", "%.4g", (double)d.delta_yaw_velocity);
        display_kvf("gimbal_device_id", "%u", (unsigned)d.gimbal_device_id);
        break;
    }
    case 286: { // AUTOPILOT_STATE_FOR_GIMBAL_DEVICE
        mavlink_autopilot_state_for_gimbal_device_t d;
        mavlink_msg_autopilot_state_for_gimbal_device_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("time_boot_us (us)", "%llu", (unsigned long long)d.time_boot_us);
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%.4g" : "%.4g", (double)d.q[_i]);
            (void)_n; display_kv("q", _arr);
        }
        display_kvf("q_estimated_delay_us (us)", "%u", (unsigned)d.q_estimated_delay_us);
        display_kvf("vx (m/s)", "%.4g", (double)d.vx);
        display_kvf("vy (m/s)", "%.4g", (double)d.vy);
        display_kvf("vz (m/s)", "%.4g", (double)d.vz);
        display_kvf("v_estimated_delay_us (us)", "%u", (unsigned)d.v_estimated_delay_us);
        display_kvf("feed_forward_angular_velocity_z (rad/s)", "%.4g", (double)d.feed_forward_angular_velocity_z);
        display_kvf("estimator_status", "%u", (unsigned)d.estimator_status);
        display_kvf("landed_state", "%u", (unsigned)d.landed_state);
        display_kvf("angular_velocity_z (rad/s)", "%.4g", (double)d.angular_velocity_z);
        break;
    }
    case 287: { // GIMBAL_MANAGER_SET_PITCHYAW
        mavlink_gimbal_manager_set_pitchyaw_t d;
        mavlink_msg_gimbal_manager_set_pitchyaw_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("flags", "%u", (unsigned)d.flags);
        display_kvf("gimbal_device_id", "%u", (unsigned)d.gimbal_device_id);
        display_kvf("pitch (rad)", "%.4g", (double)d.pitch);
        display_kvf("yaw (rad)", "%.4g", (double)d.yaw);
        display_kvf("pitch_rate (rad/s)", "%.4g", (double)d.pitch_rate);
        display_kvf("yaw_rate (rad/s)", "%.4g", (double)d.yaw_rate);
        break;
    }
    case 288: { // GIMBAL_MANAGER_SET_MANUAL_CONTROL
        mavlink_gimbal_manager_set_manual_control_t d;
        mavlink_msg_gimbal_manager_set_manual_control_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("flags", "%u", (unsigned)d.flags);
        display_kvf("gimbal_device_id", "%u", (unsigned)d.gimbal_device_id);
        display_kvf("pitch", "%.4g", (double)d.pitch);
        display_kvf("yaw", "%.4g", (double)d.yaw);
        display_kvf("pitch_rate", "%.4g", (double)d.pitch_rate);
        display_kvf("yaw_rate", "%.4g", (double)d.yaw_rate);
        break;
    }
    case 290: { // ESC_INFO
        mavlink_esc_info_t d;
        mavlink_msg_esc_info_decode(&raw, &d);
        display_kvf("index", "%u", (unsigned)d.index);
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        display_kvf("counter", "%u", (unsigned)d.counter);
        display_kvf("count", "%u", (unsigned)d.count);
        display_kvf("connection_type", "%u", (unsigned)d.connection_type);
        display_kvf("info", "%u", (unsigned)d.info);
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.failure_flags[_i]);
            (void)_n; display_kv("failure_flags", _arr);
        }
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.error_count[_i]);
            (void)_n; display_kv("error_count", _arr);
        }
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%d" : "%d", (int)d.temperature[_i]);
            (void)_n; display_kv("temperature (cdegC)", _arr);
        }
        break;
    }
    case 291: { // ESC_STATUS
        mavlink_esc_status_t d;
        mavlink_msg_esc_status_decode(&raw, &d);
        display_kvf("index", "%u", (unsigned)d.index);
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%d" : "%d", (int)d.rpm[_i]);
            (void)_n; display_kv("rpm (rpm)", _arr);
        }
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%.4g" : "%.4g", (double)d.voltage[_i]);
            (void)_n; display_kv("voltage (V)", _arr);
        }
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%.4g" : "%.4g", (double)d.current[_i]);
            (void)_n; display_kv("current (A)", _arr);
        }
        break;
    }
    case 295: { // AIRSPEED
        mavlink_airspeed_t d;
        mavlink_msg_airspeed_decode(&raw, &d);
        display_kvf("id", "%u", (unsigned)d.id);
        display_kvf("airspeed (m/s)", "%.4g", (double)d.airspeed);
        display_kvf("temperature (cdegC)", "%d", (int)d.temperature);
        display_kvf("raw_press (hPa)", "%.4g", (double)d.raw_press);
        display_kvf("flags", "%u", (unsigned)d.flags);
        break;
    }
    case 296: { // GLOBAL_POSITION_SENSOR
        mavlink_global_position_sensor_t d;
        mavlink_msg_global_position_sensor_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("id", "%u", (unsigned)d.id);
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        display_kvf("processing_time (us)", "%u", (unsigned)d.processing_time);
        display_kvf("source", "%u", (unsigned)d.source);
        display_kvf("flags", "%u", (unsigned)d.flags);
        display_kvf("lat (degE7)", "%d", (int)d.lat);
        display_kvf("lon (degE7)", "%d", (int)d.lon);
        display_kvf("alt_ellipsoid (m)", "%.4g", (double)d.alt_ellipsoid);
        display_kvf("alt (m)", "%.4g", (double)d.alt);
        display_kvf("eph (m)", "%.4g", (double)d.eph);
        display_kvf("epv (m)", "%.4g", (double)d.epv);
        break;
    }
    case 299: { // WIFI_CONFIG_AP
        mavlink_wifi_config_ap_t d;
        mavlink_msg_wifi_config_ap_decode(&raw, &d);
        {
            char _s[33];
            memcpy(_s, d.ssid, 32);
            _s[32] = '\0';
            display_kv("ssid", _s);
        }
        {
            char _s[65];
            memcpy(_s, d.password, 64);
            _s[64] = '\0';
            display_kv("password", _s);
        }
        display_kvf("mode", "%d", (int)d.mode);
        display_kvf("response", "%d", (int)d.response);
        break;
    }
    case 300: { // PROTOCOL_VERSION
        mavlink_protocol_version_t d;
        mavlink_msg_protocol_version_decode(&raw, &d);
        display_kvf("version", "%u", (unsigned)d.version);
        display_kvf("min_version", "%u", (unsigned)d.min_version);
        display_kvf("max_version", "%u", (unsigned)d.max_version);
        {
            char _arr[196]; int _n = 0;
            for (int _i = 0; _i < 8; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.spec_version_hash[_i]);
            (void)_n; display_kv("spec_version_hash", _arr);
        }
        {
            char _arr[196]; int _n = 0;
            for (int _i = 0; _i < 8; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.library_version_hash[_i]);
            (void)_n; display_kv("library_version_hash", _arr);
        }
        break;
    }
    case 301: { // AIS_VESSEL
        mavlink_ais_vessel_t d;
        mavlink_msg_ais_vessel_decode(&raw, &d);
        display_kvf("MMSI", "%u", (unsigned)d.MMSI);
        display_kvf("lat (degE7)", "%d", (int)d.lat);
        display_kvf("lon (degE7)", "%d", (int)d.lon);
        display_kvf("COG (cdeg)", "%u", (unsigned)d.COG);
        display_kvf("heading (cdeg)", "%u", (unsigned)d.heading);
        display_kvf("velocity (cm/s)", "%u", (unsigned)d.velocity);
        display_kvf("turn_rate (ddeg/s)", "%d", (int)d.turn_rate);
        display_kvf("navigational_status", "%u", (unsigned)d.navigational_status);
        display_kvf("type", "%u", (unsigned)d.type);
        display_kvf("dimension_bow (m)", "%u", (unsigned)d.dimension_bow);
        display_kvf("dimension_stern (m)", "%u", (unsigned)d.dimension_stern);
        display_kvf("dimension_port (m)", "%u", (unsigned)d.dimension_port);
        display_kvf("dimension_starboard (m)", "%u", (unsigned)d.dimension_starboard);
        {
            char _s[8];
            memcpy(_s, d.callsign, 7);
            _s[7] = '\0';
            display_kv("callsign", _s);
        }
        {
            char _s[21];
            memcpy(_s, d.name, 20);
            _s[20] = '\0';
            display_kv("name", _s);
        }
        display_kvf("tslc (s)", "%u", (unsigned)d.tslc);
        display_kvf("flags", "%u", (unsigned)d.flags);
        break;
    }
    case 310: { // UAVCAN_NODE_STATUS
        mavlink_uavcan_node_status_t d;
        mavlink_msg_uavcan_node_status_decode(&raw, &d);
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        display_kvf("uptime_sec (s)", "%u", (unsigned)d.uptime_sec);
        display_kvf("health", "%u", (unsigned)d.health);
        display_kvf("mode", "%u", (unsigned)d.mode);
        display_kvf("sub_mode", "%u", (unsigned)d.sub_mode);
        display_kvf("vendor_specific_status_code", "%u", (unsigned)d.vendor_specific_status_code);
        break;
    }
    case 311: { // UAVCAN_NODE_INFO
        mavlink_uavcan_node_info_t d;
        mavlink_msg_uavcan_node_info_decode(&raw, &d);
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        display_kvf("uptime_sec (s)", "%u", (unsigned)d.uptime_sec);
        {
            char _s[81];
            memcpy(_s, d.name, 80);
            _s[80] = '\0';
            display_kv("name", _s);
        }
        display_kvf("hw_version_major", "%u", (unsigned)d.hw_version_major);
        display_kvf("hw_version_minor", "%u", (unsigned)d.hw_version_minor);
        {
            char _arr[388]; int _n = 0;
            for (int _i = 0; _i < 16; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.hw_unique_id[_i]);
            (void)_n; display_kv("hw_unique_id", _arr);
        }
        display_kvf("sw_version_major", "%u", (unsigned)d.sw_version_major);
        display_kvf("sw_version_minor", "%u", (unsigned)d.sw_version_minor);
        display_kvf("sw_vcs_commit", "%u", (unsigned)d.sw_vcs_commit);
        break;
    }
    case 320: { // PARAM_EXT_REQUEST_READ
        mavlink_param_ext_request_read_t d;
        mavlink_msg_param_ext_request_read_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        {
            char _s[17];
            memcpy(_s, d.param_id, 16);
            _s[16] = '\0';
            display_kv("param_id", _s);
        }
        display_kvf("param_index", "%d", (int)d.param_index);
        break;
    }
    case 321: { // PARAM_EXT_REQUEST_LIST
        mavlink_param_ext_request_list_t d;
        mavlink_msg_param_ext_request_list_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        break;
    }
    case 322: { // PARAM_EXT_VALUE
        mavlink_param_ext_value_t d;
        mavlink_msg_param_ext_value_decode(&raw, &d);
        {
            char _s[17];
            memcpy(_s, d.param_id, 16);
            _s[16] = '\0';
            display_kv("param_id", _s);
        }
        {
            char _s[129];
            memcpy(_s, d.param_value, 128);
            _s[128] = '\0';
            display_kv("param_value", _s);
        }
        display_kvf("param_type", "%u", (unsigned)d.param_type);
        display_kvf("param_count", "%u", (unsigned)d.param_count);
        display_kvf("param_index", "%u", (unsigned)d.param_index);
        break;
    }
    case 323: { // PARAM_EXT_SET
        mavlink_param_ext_set_t d;
        mavlink_msg_param_ext_set_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        {
            char _s[17];
            memcpy(_s, d.param_id, 16);
            _s[16] = '\0';
            display_kv("param_id", _s);
        }
        {
            char _s[129];
            memcpy(_s, d.param_value, 128);
            _s[128] = '\0';
            display_kv("param_value", _s);
        }
        display_kvf("param_type", "%u", (unsigned)d.param_type);
        break;
    }
    case 324: { // PARAM_EXT_ACK
        mavlink_param_ext_ack_t d;
        mavlink_msg_param_ext_ack_decode(&raw, &d);
        {
            char _s[17];
            memcpy(_s, d.param_id, 16);
            _s[16] = '\0';
            display_kv("param_id", _s);
        }
        {
            char _s[129];
            memcpy(_s, d.param_value, 128);
            _s[128] = '\0';
            display_kv("param_value", _s);
        }
        display_kvf("param_type", "%u", (unsigned)d.param_type);
        display_kvf("param_result", "%u", (unsigned)d.param_result);
        break;
    }
    case 330: { // OBSTACLE_DISTANCE
        mavlink_obstacle_distance_t d;
        mavlink_msg_obstacle_distance_decode(&raw, &d);
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        display_kvf("sensor_type", "%u", (unsigned)d.sensor_type);
        display_kv("distances (cm)", "[72 elements]");
        display_kvf("increment (deg)", "%u", (unsigned)d.increment);
        display_kvf("min_distance (cm)", "%u", (unsigned)d.min_distance);
        display_kvf("max_distance (cm)", "%u", (unsigned)d.max_distance);
        display_kvf("increment_f (deg)", "%.4g", (double)d.increment_f);
        display_kvf("angle_offset (deg)", "%.4g", (double)d.angle_offset);
        display_kvf("frame", "%u", (unsigned)d.frame);
        break;
    }
    case 331: { // ODOMETRY
        mavlink_odometry_t d;
        mavlink_msg_odometry_decode(&raw, &d);
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        display_kvf("frame_id", "%u", (unsigned)d.frame_id);
        display_kvf("child_frame_id", "%u", (unsigned)d.child_frame_id);
        display_kvf("x (m)", "%.4g", (double)d.x);
        display_kvf("y (m)", "%.4g", (double)d.y);
        display_kvf("z (m)", "%.4g", (double)d.z);
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%.4g" : "%.4g", (double)d.q[_i]);
            (void)_n; display_kv("q", _arr);
        }
        display_kvf("vx (m/s)", "%.4g", (double)d.vx);
        display_kvf("vy (m/s)", "%.4g", (double)d.vy);
        display_kvf("vz (m/s)", "%.4g", (double)d.vz);
        display_kvf("rollspeed (rad/s)", "%.4g", (double)d.rollspeed);
        display_kvf("pitchspeed (rad/s)", "%.4g", (double)d.pitchspeed);
        display_kvf("yawspeed (rad/s)", "%.4g", (double)d.yawspeed);
        display_kv("pose_covariance", "[21 elements]");
        display_kv("velocity_covariance", "[21 elements]");
        display_kvf("reset_counter", "%u", (unsigned)d.reset_counter);
        display_kvf("estimator_type", "%u", (unsigned)d.estimator_type);
        display_kvf("quality (%)", "%d", (int)d.quality);
        break;
    }
    case 332: { // TRAJECTORY_REPRESENTATION_WAYPOINTS
        mavlink_trajectory_representation_waypoints_t d;
        mavlink_msg_trajectory_representation_waypoints_decode(&raw, &d);
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        display_kvf("valid_points", "%u", (unsigned)d.valid_points);
        {
            char _arr[124]; int _n = 0;
            for (int _i = 0; _i < 5; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%.4g" : "%.4g", (double)d.pos_x[_i]);
            (void)_n; display_kv("pos_x (m)", _arr);
        }
        {
            char _arr[124]; int _n = 0;
            for (int _i = 0; _i < 5; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%.4g" : "%.4g", (double)d.pos_y[_i]);
            (void)_n; display_kv("pos_y (m)", _arr);
        }
        {
            char _arr[124]; int _n = 0;
            for (int _i = 0; _i < 5; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%.4g" : "%.4g", (double)d.pos_z[_i]);
            (void)_n; display_kv("pos_z (m)", _arr);
        }
        {
            char _arr[124]; int _n = 0;
            for (int _i = 0; _i < 5; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%.4g" : "%.4g", (double)d.vel_x[_i]);
            (void)_n; display_kv("vel_x (m/s)", _arr);
        }
        {
            char _arr[124]; int _n = 0;
            for (int _i = 0; _i < 5; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%.4g" : "%.4g", (double)d.vel_y[_i]);
            (void)_n; display_kv("vel_y (m/s)", _arr);
        }
        {
            char _arr[124]; int _n = 0;
            for (int _i = 0; _i < 5; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%.4g" : "%.4g", (double)d.vel_z[_i]);
            (void)_n; display_kv("vel_z (m/s)", _arr);
        }
        {
            char _arr[124]; int _n = 0;
            for (int _i = 0; _i < 5; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%.4g" : "%.4g", (double)d.acc_x[_i]);
            (void)_n; display_kv("acc_x (m/s/s)", _arr);
        }
        {
            char _arr[124]; int _n = 0;
            for (int _i = 0; _i < 5; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%.4g" : "%.4g", (double)d.acc_y[_i]);
            (void)_n; display_kv("acc_y (m/s/s)", _arr);
        }
        {
            char _arr[124]; int _n = 0;
            for (int _i = 0; _i < 5; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%.4g" : "%.4g", (double)d.acc_z[_i]);
            (void)_n; display_kv("acc_z (m/s/s)", _arr);
        }
        {
            char _arr[124]; int _n = 0;
            for (int _i = 0; _i < 5; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%.4g" : "%.4g", (double)d.pos_yaw[_i]);
            (void)_n; display_kv("pos_yaw (rad)", _arr);
        }
        {
            char _arr[124]; int _n = 0;
            for (int _i = 0; _i < 5; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%.4g" : "%.4g", (double)d.vel_yaw[_i]);
            (void)_n; display_kv("vel_yaw (rad/s)", _arr);
        }
        {
            char _arr[124]; int _n = 0;
            for (int _i = 0; _i < 5; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.command[_i]);
            (void)_n; display_kv("command", _arr);
        }
        break;
    }
    case 333: { // TRAJECTORY_REPRESENTATION_BEZIER
        mavlink_trajectory_representation_bezier_t d;
        mavlink_msg_trajectory_representation_bezier_decode(&raw, &d);
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        display_kvf("valid_points", "%u", (unsigned)d.valid_points);
        {
            char _arr[124]; int _n = 0;
            for (int _i = 0; _i < 5; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%.4g" : "%.4g", (double)d.pos_x[_i]);
            (void)_n; display_kv("pos_x (m)", _arr);
        }
        {
            char _arr[124]; int _n = 0;
            for (int _i = 0; _i < 5; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%.4g" : "%.4g", (double)d.pos_y[_i]);
            (void)_n; display_kv("pos_y (m)", _arr);
        }
        {
            char _arr[124]; int _n = 0;
            for (int _i = 0; _i < 5; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%.4g" : "%.4g", (double)d.pos_z[_i]);
            (void)_n; display_kv("pos_z (m)", _arr);
        }
        {
            char _arr[124]; int _n = 0;
            for (int _i = 0; _i < 5; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%.4g" : "%.4g", (double)d.delta[_i]);
            (void)_n; display_kv("delta (s)", _arr);
        }
        {
            char _arr[124]; int _n = 0;
            for (int _i = 0; _i < 5; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%.4g" : "%.4g", (double)d.pos_yaw[_i]);
            (void)_n; display_kv("pos_yaw (rad)", _arr);
        }
        break;
    }
    case 334: { // CELLULAR_STATUS
        mavlink_cellular_status_t d;
        mavlink_msg_cellular_status_decode(&raw, &d);
        display_kvf("status", "%u", (unsigned)d.status);
        display_kvf("failure_reason", "%u", (unsigned)d.failure_reason);
        display_kvf("type", "%u", (unsigned)d.type);
        display_kvf("quality", "%u", (unsigned)d.quality);
        display_kvf("mcc", "%u", (unsigned)d.mcc);
        display_kvf("mnc", "%u", (unsigned)d.mnc);
        display_kvf("lac", "%u", (unsigned)d.lac);
        break;
    }
    case 335: { // ISBD_LINK_STATUS
        mavlink_isbd_link_status_t d;
        mavlink_msg_isbd_link_status_decode(&raw, &d);
        display_kvf("timestamp (us)", "%llu", (unsigned long long)d.timestamp);
        display_kvf("last_heartbeat (us)", "%llu", (unsigned long long)d.last_heartbeat);
        display_kvf("failed_sessions", "%u", (unsigned)d.failed_sessions);
        display_kvf("successful_sessions", "%u", (unsigned)d.successful_sessions);
        display_kvf("signal_quality", "%u", (unsigned)d.signal_quality);
        display_kvf("ring_pending", "%u", (unsigned)d.ring_pending);
        display_kvf("tx_session_pending", "%u", (unsigned)d.tx_session_pending);
        display_kvf("rx_session_pending", "%u", (unsigned)d.rx_session_pending);
        break;
    }
    case 336: { // CELLULAR_CONFIG
        mavlink_cellular_config_t d;
        mavlink_msg_cellular_config_decode(&raw, &d);
        display_kvf("enable_lte", "%u", (unsigned)d.enable_lte);
        display_kvf("enable_pin", "%u", (unsigned)d.enable_pin);
        {
            char _s[17];
            memcpy(_s, d.pin, 16);
            _s[16] = '\0';
            display_kv("pin", _s);
        }
        {
            char _s[17];
            memcpy(_s, d.new_pin, 16);
            _s[16] = '\0';
            display_kv("new_pin", _s);
        }
        {
            char _s[33];
            memcpy(_s, d.apn, 32);
            _s[32] = '\0';
            display_kv("apn", _s);
        }
        {
            char _s[17];
            memcpy(_s, d.puk, 16);
            _s[16] = '\0';
            display_kv("puk", _s);
        }
        display_kvf("roaming", "%u", (unsigned)d.roaming);
        display_kvf("response", "%u", (unsigned)d.response);
        break;
    }
    case 339: { // RAW_RPM
        mavlink_raw_rpm_t d;
        mavlink_msg_raw_rpm_decode(&raw, &d);
        display_kvf("index", "%u", (unsigned)d.index);
        display_kvf("frequency (rpm)", "%.4g", (double)d.frequency);
        break;
    }
    case 340: { // UTM_GLOBAL_POSITION
        mavlink_utm_global_position_t d;
        mavlink_msg_utm_global_position_decode(&raw, &d);
        display_kvf("time (us)", "%llu", (unsigned long long)d.time);
        display_kv("uas_id", "[18 elements]");
        display_kvf("lat (degE7)", "%d", (int)d.lat);
        display_kvf("lon (degE7)", "%d", (int)d.lon);
        display_kvf("alt (mm)", "%d", (int)d.alt);
        display_kvf("relative_alt (mm)", "%d", (int)d.relative_alt);
        display_kvf("vx (cm/s)", "%d", (int)d.vx);
        display_kvf("vy (cm/s)", "%d", (int)d.vy);
        display_kvf("vz (cm/s)", "%d", (int)d.vz);
        display_kvf("h_acc (mm)", "%u", (unsigned)d.h_acc);
        display_kvf("v_acc (mm)", "%u", (unsigned)d.v_acc);
        display_kvf("vel_acc (cm/s)", "%u", (unsigned)d.vel_acc);
        display_kvf("next_lat (degE7)", "%d", (int)d.next_lat);
        display_kvf("next_lon (degE7)", "%d", (int)d.next_lon);
        display_kvf("next_alt (mm)", "%d", (int)d.next_alt);
        display_kvf("update_rate (cs)", "%u", (unsigned)d.update_rate);
        display_kvf("flight_state", "%u", (unsigned)d.flight_state);
        display_kvf("flags", "%u", (unsigned)d.flags);
        break;
    }
    case 345: { // PARAM_ERROR
        mavlink_param_error_t d;
        mavlink_msg_param_error_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        {
            char _s[17];
            memcpy(_s, d.param_id, 16);
            _s[16] = '\0';
            display_kv("param_id", _s);
        }
        display_kvf("param_index", "%d", (int)d.param_index);
        display_kvf("error", "%u", (unsigned)d.error);
        break;
    }
    case 350: { // DEBUG_FLOAT_ARRAY
        mavlink_debug_float_array_t d;
        mavlink_msg_debug_float_array_decode(&raw, &d);
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        {
            char _s[11];
            memcpy(_s, d.name, 10);
            _s[10] = '\0';
            display_kv("name", _s);
        }
        display_kvf("array_id", "%u", (unsigned)d.array_id);
        display_kv("data", "[58 elements]");
        break;
    }
    case 360: { // ORBIT_EXECUTION_STATUS
        mavlink_orbit_execution_status_t d;
        mavlink_msg_orbit_execution_status_decode(&raw, &d);
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        display_kvf("radius (m)", "%.4g", (double)d.radius);
        display_kvf("frame", "%u", (unsigned)d.frame);
        display_kvf("x", "%d", (int)d.x);
        display_kvf("y", "%d", (int)d.y);
        display_kvf("z (m)", "%.4g", (double)d.z);
        break;
    }
    case 361: { // FIGURE_EIGHT_EXECUTION_STATUS
        mavlink_figure_eight_execution_status_t d;
        mavlink_msg_figure_eight_execution_status_decode(&raw, &d);
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        display_kvf("major_radius (m)", "%.4g", (double)d.major_radius);
        display_kvf("minor_radius (m)", "%.4g", (double)d.minor_radius);
        display_kvf("orientation (rad)", "%.4g", (double)d.orientation);
        display_kvf("frame", "%u", (unsigned)d.frame);
        display_kvf("x", "%d", (int)d.x);
        display_kvf("y", "%d", (int)d.y);
        display_kvf("z (m)", "%.4g", (double)d.z);
        break;
    }
    case 370: { // SMART_BATTERY_INFO
        mavlink_smart_battery_info_t d;
        mavlink_msg_smart_battery_info_decode(&raw, &d);
        display_kvf("id", "%u", (unsigned)d.id);
        display_kvf("battery_function", "%u", (unsigned)d.battery_function);
        display_kvf("type", "%u", (unsigned)d.type);
        display_kvf("capacity_full_specification (mAh)", "%d", (int)d.capacity_full_specification);
        display_kvf("capacity_full (mAh)", "%d", (int)d.capacity_full);
        display_kvf("cycle_count", "%u", (unsigned)d.cycle_count);
        {
            char _s[17];
            memcpy(_s, d.serial_number, 16);
            _s[16] = '\0';
            display_kv("serial_number", _s);
        }
        {
            char _s[51];
            memcpy(_s, d.device_name, 50);
            _s[50] = '\0';
            display_kv("device_name", _s);
        }
        display_kvf("weight (g)", "%u", (unsigned)d.weight);
        display_kvf("discharge_minimum_voltage (mV)", "%u", (unsigned)d.discharge_minimum_voltage);
        display_kvf("charging_minimum_voltage (mV)", "%u", (unsigned)d.charging_minimum_voltage);
        display_kvf("resting_minimum_voltage (mV)", "%u", (unsigned)d.resting_minimum_voltage);
        display_kvf("charging_maximum_voltage (mV)", "%u", (unsigned)d.charging_maximum_voltage);
        display_kvf("cells_in_series", "%u", (unsigned)d.cells_in_series);
        display_kvf("discharge_maximum_current (mA)", "%u", (unsigned)d.discharge_maximum_current);
        display_kvf("discharge_maximum_burst_current (mA)", "%u", (unsigned)d.discharge_maximum_burst_current);
        {
            char _s[12];
            memcpy(_s, d.manufacture_date, 11);
            _s[11] = '\0';
            display_kv("manufacture_date", _s);
        }
        break;
    }
    case 371: { // FUEL_STATUS
        mavlink_fuel_status_t d;
        mavlink_msg_fuel_status_decode(&raw, &d);
        display_kvf("id", "%u", (unsigned)d.id);
        display_kvf("maximum_fuel", "%.4g", (double)d.maximum_fuel);
        display_kvf("consumed_fuel", "%.4g", (double)d.consumed_fuel);
        display_kvf("remaining_fuel", "%.4g", (double)d.remaining_fuel);
        display_kvf("percent_remaining (%)", "%u", (unsigned)d.percent_remaining);
        display_kvf("flow_rate", "%.4g", (double)d.flow_rate);
        display_kvf("temperature (K)", "%.4g", (double)d.temperature);
        display_kvf("fuel_type", "%u", (unsigned)d.fuel_type);
        break;
    }
    case 372: { // BATTERY_INFO
        mavlink_battery_info_t d;
        mavlink_msg_battery_info_decode(&raw, &d);
        display_kvf("id", "%u", (unsigned)d.id);
        display_kvf("battery_function", "%u", (unsigned)d.battery_function);
        display_kvf("type", "%u", (unsigned)d.type);
        display_kvf("state_of_health (%)", "%u", (unsigned)d.state_of_health);
        display_kvf("cells_in_series", "%u", (unsigned)d.cells_in_series);
        display_kvf("cycle_count", "%u", (unsigned)d.cycle_count);
        display_kvf("weight (g)", "%u", (unsigned)d.weight);
        display_kvf("discharge_minimum_voltage (V)", "%.4g", (double)d.discharge_minimum_voltage);
        display_kvf("charging_minimum_voltage (V)", "%.4g", (double)d.charging_minimum_voltage);
        display_kvf("resting_minimum_voltage (V)", "%.4g", (double)d.resting_minimum_voltage);
        display_kvf("charging_maximum_voltage (V)", "%.4g", (double)d.charging_maximum_voltage);
        display_kvf("charging_maximum_current (A)", "%.4g", (double)d.charging_maximum_current);
        display_kvf("nominal_voltage (V)", "%.4g", (double)d.nominal_voltage);
        display_kvf("discharge_maximum_current (A)", "%.4g", (double)d.discharge_maximum_current);
        display_kvf("discharge_maximum_burst_current (A)", "%.4g", (double)d.discharge_maximum_burst_current);
        display_kvf("design_capacity (Ah)", "%.4g", (double)d.design_capacity);
        display_kvf("full_charge_capacity (Ah)", "%.4g", (double)d.full_charge_capacity);
        {
            char _s[10];
            memcpy(_s, d.manufacture_date, 9);
            _s[9] = '\0';
            display_kv("manufacture_date", _s);
        }
        {
            char _s[33];
            memcpy(_s, d.serial_number, 32);
            _s[32] = '\0';
            display_kv("serial_number", _s);
        }
        {
            char _s[51];
            memcpy(_s, d.name, 50);
            _s[50] = '\0';
            display_kv("name", _s);
        }
        break;
    }
    case 373: { // GENERATOR_STATUS
        mavlink_generator_status_t d;
        mavlink_msg_generator_status_decode(&raw, &d);
        display_kvf("status", "%llu", (unsigned long long)d.status);
        display_kvf("generator_speed (rpm)", "%u", (unsigned)d.generator_speed);
        display_kvf("battery_current (A)", "%.4g", (double)d.battery_current);
        display_kvf("load_current (A)", "%.4g", (double)d.load_current);
        display_kvf("power_generated (W)", "%.4g", (double)d.power_generated);
        display_kvf("bus_voltage (V)", "%.4g", (double)d.bus_voltage);
        display_kvf("rectifier_temperature (degC)", "%d", (int)d.rectifier_temperature);
        display_kvf("bat_current_setpoint (A)", "%.4g", (double)d.bat_current_setpoint);
        display_kvf("generator_temperature (degC)", "%d", (int)d.generator_temperature);
        display_kvf("runtime (s)", "%u", (unsigned)d.runtime);
        display_kvf("time_until_maintenance (s)", "%d", (int)d.time_until_maintenance);
        break;
    }
    case 375: { // ACTUATOR_OUTPUT_STATUS
        mavlink_actuator_output_status_t d;
        mavlink_msg_actuator_output_status_decode(&raw, &d);
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        display_kvf("active", "%u", (unsigned)d.active);
        display_kv("actuator", "[32 elements]");
        break;
    }
    case 376: { // RELAY_STATUS
        mavlink_relay_status_t d;
        mavlink_msg_relay_status_decode(&raw, &d);
        display_kvf("time_boot_ms (ms)", "%u", (unsigned)d.time_boot_ms);
        display_kvf("on", "%u", (unsigned)d.on);
        display_kvf("present", "%u", (unsigned)d.present);
        break;
    }
    case 380: { // TIME_ESTIMATE_TO_TARGET
        mavlink_time_estimate_to_target_t d;
        mavlink_msg_time_estimate_to_target_decode(&raw, &d);
        display_kvf("safe_return (s)", "%d", (int)d.safe_return);
        display_kvf("land (s)", "%d", (int)d.land);
        display_kvf("mission_next_item (s)", "%d", (int)d.mission_next_item);
        display_kvf("mission_end (s)", "%d", (int)d.mission_end);
        display_kvf("commanded_action (s)", "%d", (int)d.commanded_action);
        break;
    }
    case 385: { // TUNNEL
        mavlink_tunnel_t d;
        mavlink_msg_tunnel_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("payload_type", "%u", (unsigned)d.payload_type);
        display_kvf("payload_length", "%u", (unsigned)d.payload_length);
        display_kv("payload", "[128 elements]");
        break;
    }
    case 386: { // CAN_FRAME
        mavlink_can_frame_t d;
        mavlink_msg_can_frame_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("bus", "%u", (unsigned)d.bus);
        display_kvf("len", "%u", (unsigned)d.len);
        display_kvf("id", "%u", (unsigned)d.id);
        {
            char _arr[196]; int _n = 0;
            for (int _i = 0; _i < 8; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.data[_i]);
            (void)_n; display_kv("data", _arr);
        }
        break;
    }
    case 387: { // CANFD_FRAME
        mavlink_canfd_frame_t d;
        mavlink_msg_canfd_frame_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("bus", "%u", (unsigned)d.bus);
        display_kvf("len", "%u", (unsigned)d.len);
        display_kvf("id", "%u", (unsigned)d.id);
        display_kv("data", "[64 elements]");
        break;
    }
    case 388: { // CAN_FILTER_MODIFY
        mavlink_can_filter_modify_t d;
        mavlink_msg_can_filter_modify_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("bus", "%u", (unsigned)d.bus);
        display_kvf("operation", "%u", (unsigned)d.operation);
        display_kvf("num_ids", "%u", (unsigned)d.num_ids);
        {
            char _arr[388]; int _n = 0;
            for (int _i = 0; _i < 16; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.ids[_i]);
            (void)_n; display_kv("ids", _arr);
        }
        break;
    }
    case 390: { // ONBOARD_COMPUTER_STATUS
        mavlink_onboard_computer_status_t d;
        mavlink_msg_onboard_computer_status_decode(&raw, &d);
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        display_kvf("uptime (ms)", "%u", (unsigned)d.uptime);
        display_kvf("type", "%u", (unsigned)d.type);
        {
            char _arr[196]; int _n = 0;
            for (int _i = 0; _i < 8; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.cpu_cores[_i]);
            (void)_n; display_kv("cpu_cores", _arr);
        }
        {
            char _arr[244]; int _n = 0;
            for (int _i = 0; _i < 10; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.cpu_combined[_i]);
            (void)_n; display_kv("cpu_combined", _arr);
        }
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.gpu_cores[_i]);
            (void)_n; display_kv("gpu_cores", _arr);
        }
        {
            char _arr[244]; int _n = 0;
            for (int _i = 0; _i < 10; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.gpu_combined[_i]);
            (void)_n; display_kv("gpu_combined", _arr);
        }
        display_kvf("temperature_board (degC)", "%d", (int)d.temperature_board);
        {
            char _arr[196]; int _n = 0;
            for (int _i = 0; _i < 8; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%d" : "%d", (int)d.temperature_core[_i]);
            (void)_n; display_kv("temperature_core (degC)", _arr);
        }
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%d" : "%d", (int)d.fan_speed[_i]);
            (void)_n; display_kv("fan_speed (rpm)", _arr);
        }
        display_kvf("ram_usage (MiB)", "%u", (unsigned)d.ram_usage);
        display_kvf("ram_total (MiB)", "%u", (unsigned)d.ram_total);
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.storage_type[_i]);
            (void)_n; display_kv("storage_type", _arr);
        }
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.storage_usage[_i]);
            (void)_n; display_kv("storage_usage (MiB)", _arr);
        }
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.storage_total[_i]);
            (void)_n; display_kv("storage_total (MiB)", _arr);
        }
        {
            char _arr[148]; int _n = 0;
            for (int _i = 0; _i < 6; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.link_type[_i]);
            (void)_n; display_kv("link_type", _arr);
        }
        {
            char _arr[148]; int _n = 0;
            for (int _i = 0; _i < 6; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.link_tx_rate[_i]);
            (void)_n; display_kv("link_tx_rate (KiB/s)", _arr);
        }
        {
            char _arr[148]; int _n = 0;
            for (int _i = 0; _i < 6; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.link_rx_rate[_i]);
            (void)_n; display_kv("link_rx_rate (KiB/s)", _arr);
        }
        {
            char _arr[148]; int _n = 0;
            for (int _i = 0; _i < 6; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.link_tx_max[_i]);
            (void)_n; display_kv("link_tx_max (KiB/s)", _arr);
        }
        {
            char _arr[148]; int _n = 0;
            for (int _i = 0; _i < 6; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.link_rx_max[_i]);
            (void)_n; display_kv("link_rx_max (KiB/s)", _arr);
        }
        display_kvf("status_flags", "%u", (unsigned)d.status_flags);
        break;
    }
    case 395: { // COMPONENT_INFORMATION
        mavlink_component_information_t d;
        mavlink_msg_component_information_decode(&raw, &d);
        display_kvf("time_boot_ms (ms)", "%u", (unsigned)d.time_boot_ms);
        display_kvf("general_metadata_file_crc", "%u", (unsigned)d.general_metadata_file_crc);
        {
            char _s[101];
            memcpy(_s, d.general_metadata_uri, 100);
            _s[100] = '\0';
            display_kv("general_metadata_uri", _s);
        }
        display_kvf("peripherals_metadata_file_crc", "%u", (unsigned)d.peripherals_metadata_file_crc);
        {
            char _s[101];
            memcpy(_s, d.peripherals_metadata_uri, 100);
            _s[100] = '\0';
            display_kv("peripherals_metadata_uri", _s);
        }
        break;
    }
    case 396: { // COMPONENT_INFORMATION_BASIC
        mavlink_component_information_basic_t d;
        mavlink_msg_component_information_basic_decode(&raw, &d);
        display_kvf("time_boot_ms (ms)", "%u", (unsigned)d.time_boot_ms);
        display_kvf("capabilities", "%llu", (unsigned long long)d.capabilities);
        display_kvf("time_manufacture_s (s)", "%u", (unsigned)d.time_manufacture_s);
        {
            char _s[33];
            memcpy(_s, d.vendor_name, 32);
            _s[32] = '\0';
            display_kv("vendor_name", _s);
        }
        {
            char _s[33];
            memcpy(_s, d.model_name, 32);
            _s[32] = '\0';
            display_kv("model_name", _s);
        }
        {
            char _s[25];
            memcpy(_s, d.software_version, 24);
            _s[24] = '\0';
            display_kv("software_version", _s);
        }
        {
            char _s[25];
            memcpy(_s, d.hardware_version, 24);
            _s[24] = '\0';
            display_kv("hardware_version", _s);
        }
        {
            char _s[33];
            memcpy(_s, d.serial_number, 32);
            _s[32] = '\0';
            display_kv("serial_number", _s);
        }
        break;
    }
    case 397: { // COMPONENT_METADATA
        mavlink_component_metadata_t d;
        mavlink_msg_component_metadata_decode(&raw, &d);
        display_kvf("time_boot_ms (ms)", "%u", (unsigned)d.time_boot_ms);
        display_kvf("file_crc", "%u", (unsigned)d.file_crc);
        {
            char _s[101];
            memcpy(_s, d.uri, 100);
            _s[100] = '\0';
            display_kv("uri", _s);
        }
        break;
    }
    case 400: { // PLAY_TUNE_V2
        mavlink_play_tune_v2_t d;
        mavlink_msg_play_tune_v2_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("format", "%u", (unsigned)d.format);
        {
            char _s[249];
            memcpy(_s, d.tune, 248);
            _s[248] = '\0';
            display_kv("tune", _s);
        }
        break;
    }
    case 401: { // SUPPORTED_TUNES
        mavlink_supported_tunes_t d;
        mavlink_msg_supported_tunes_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("format", "%u", (unsigned)d.format);
        break;
    }
    case 410: { // EVENT
        mavlink_event_t d;
        mavlink_msg_event_decode(&raw, &d);
        display_kvf("destination_component", "%u", (unsigned)d.destination_component);
        display_kvf("destination_system", "%u", (unsigned)d.destination_system);
        display_kvf("id", "%u", (unsigned)d.id);
        display_kvf("event_time_boot_ms (ms)", "%u", (unsigned)d.event_time_boot_ms);
        display_kvf("sequence", "%u", (unsigned)d.sequence);
        display_kvf("log_levels", "%u", (unsigned)d.log_levels);
        display_kv("arguments", "[40 elements]");
        break;
    }
    case 411: { // CURRENT_EVENT_SEQUENCE
        mavlink_current_event_sequence_t d;
        mavlink_msg_current_event_sequence_decode(&raw, &d);
        display_kvf("sequence", "%u", (unsigned)d.sequence);
        display_kvf("flags", "%u", (unsigned)d.flags);
        break;
    }
    case 412: { // REQUEST_EVENT
        mavlink_request_event_t d;
        mavlink_msg_request_event_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("first_sequence", "%u", (unsigned)d.first_sequence);
        display_kvf("last_sequence", "%u", (unsigned)d.last_sequence);
        break;
    }
    case 413: { // RESPONSE_EVENT_ERROR
        mavlink_response_event_error_t d;
        mavlink_msg_response_event_error_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("sequence", "%u", (unsigned)d.sequence);
        display_kvf("sequence_oldest_available", "%u", (unsigned)d.sequence_oldest_available);
        display_kvf("reason", "%u", (unsigned)d.reason);
        break;
    }
    case 435: { // AVAILABLE_MODES
        mavlink_available_modes_t d;
        mavlink_msg_available_modes_decode(&raw, &d);
        display_kvf("number_modes", "%u", (unsigned)d.number_modes);
        display_kvf("mode_index", "%u", (unsigned)d.mode_index);
        display_kvf("standard_mode", "%u", (unsigned)d.standard_mode);
        display_kvf("custom_mode", "%u", (unsigned)d.custom_mode);
        display_kvf("properties", "%u", (unsigned)d.properties);
        {
            char _s[36];
            memcpy(_s, d.mode_name, 35);
            _s[35] = '\0';
            display_kv("mode_name", _s);
        }
        break;
    }
    case 436: { // CURRENT_MODE
        mavlink_current_mode_t d;
        mavlink_msg_current_mode_decode(&raw, &d);
        display_kvf("standard_mode", "%u", (unsigned)d.standard_mode);
        display_kvf("custom_mode", "%u", (unsigned)d.custom_mode);
        display_kvf("intended_custom_mode", "%u", (unsigned)d.intended_custom_mode);
        break;
    }
    case 437: { // AVAILABLE_MODES_MONITOR
        mavlink_available_modes_monitor_t d;
        mavlink_msg_available_modes_monitor_decode(&raw, &d);
        display_kvf("seq", "%u", (unsigned)d.seq);
        break;
    }
    case 440: { // ILLUMINATOR_STATUS
        mavlink_illuminator_status_t d;
        mavlink_msg_illuminator_status_decode(&raw, &d);
        display_kvf("uptime_ms (ms)", "%u", (unsigned)d.uptime_ms);
        display_kvf("enable", "%u", (unsigned)d.enable);
        display_kvf("mode_bitmask", "%u", (unsigned)d.mode_bitmask);
        display_kvf("error_status", "%u", (unsigned)d.error_status);
        display_kvf("mode", "%u", (unsigned)d.mode);
        display_kvf("brightness (%)", "%.4g", (double)d.brightness);
        display_kvf("strobe_period (s)", "%.4g", (double)d.strobe_period);
        display_kvf("strobe_duty_cycle (%)", "%.4g", (double)d.strobe_duty_cycle);
        display_kvf("temp_c", "%.4g", (double)d.temp_c);
        display_kvf("min_strobe_period (s)", "%.4g", (double)d.min_strobe_period);
        display_kvf("max_strobe_period (s)", "%.4g", (double)d.max_strobe_period);
        break;
    }
    case 9000: { // WHEEL_DISTANCE
        mavlink_wheel_distance_t d;
        mavlink_msg_wheel_distance_decode(&raw, &d);
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        display_kvf("count", "%u", (unsigned)d.count);
        {
            char _arr[388]; int _n = 0;
            for (int _i = 0; _i < 16; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%.6g" : "%.6g", d.distance[_i]);
            (void)_n; display_kv("distance (m)", _arr);
        }
        break;
    }
    case 9005: { // WINCH_STATUS
        mavlink_winch_status_t d;
        mavlink_msg_winch_status_decode(&raw, &d);
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        display_kvf("line_length (m)", "%.4g", (double)d.line_length);
        display_kvf("speed (m/s)", "%.4g", (double)d.speed);
        display_kvf("tension (kg)", "%.4g", (double)d.tension);
        display_kvf("voltage (V)", "%.4g", (double)d.voltage);
        display_kvf("current (A)", "%.4g", (double)d.current);
        display_kvf("temperature (degC)", "%d", (int)d.temperature);
        display_kvf("status", "%u", (unsigned)d.status);
        break;
    }
    case 10001: { // UAVIONIX_ADSB_OUT_CFG
        mavlink_uavionix_adsb_out_cfg_t d;
        mavlink_msg_uavionix_adsb_out_cfg_decode(&raw, &d);
        display_kvf("ICAO", "%u", (unsigned)d.ICAO);
        {
            char _s[10];
            memcpy(_s, d.callsign, 9);
            _s[9] = '\0';
            display_kv("callsign", _s);
        }
        display_kvf("emitterType", "%u", (unsigned)d.emitterType);
        display_kvf("aircraftSize", "%u", (unsigned)d.aircraftSize);
        display_kvf("gpsOffsetLat", "%u", (unsigned)d.gpsOffsetLat);
        display_kvf("gpsOffsetLon", "%u", (unsigned)d.gpsOffsetLon);
        display_kvf("stallSpeed (cm/s)", "%u", (unsigned)d.stallSpeed);
        display_kvf("rfSelect", "%u", (unsigned)d.rfSelect);
        break;
    }
    case 10002: { // UAVIONIX_ADSB_OUT_DYNAMIC
        mavlink_uavionix_adsb_out_dynamic_t d;
        mavlink_msg_uavionix_adsb_out_dynamic_decode(&raw, &d);
        display_kvf("utcTime (s)", "%u", (unsigned)d.utcTime);
        display_kvf("gpsLat (degE7)", "%d", (int)d.gpsLat);
        display_kvf("gpsLon (degE7)", "%d", (int)d.gpsLon);
        display_kvf("gpsAlt (mm)", "%d", (int)d.gpsAlt);
        display_kvf("gpsFix", "%u", (unsigned)d.gpsFix);
        display_kvf("numSats", "%u", (unsigned)d.numSats);
        display_kvf("baroAltMSL (mbar)", "%d", (int)d.baroAltMSL);
        display_kvf("accuracyHor (mm)", "%u", (unsigned)d.accuracyHor);
        display_kvf("accuracyVert (cm)", "%u", (unsigned)d.accuracyVert);
        display_kvf("accuracyVel (mm/s)", "%u", (unsigned)d.accuracyVel);
        display_kvf("velVert (cm/s)", "%d", (int)d.velVert);
        display_kvf("velNS (cm/s)", "%d", (int)d.velNS);
        display_kvf("VelEW (cm/s)", "%d", (int)d.VelEW);
        display_kvf("emergencyStatus", "%u", (unsigned)d.emergencyStatus);
        display_kvf("state", "%u", (unsigned)d.state);
        display_kvf("squawk", "%u", (unsigned)d.squawk);
        break;
    }
    case 10003: { // UAVIONIX_ADSB_TRANSCEIVER_HEALTH_REPORT
        mavlink_uavionix_adsb_transceiver_health_report_t d;
        mavlink_msg_uavionix_adsb_transceiver_health_report_decode(&raw, &d);
        display_kvf("rfHealth", "%u", (unsigned)d.rfHealth);
        break;
    }
    case 10004: { // UAVIONIX_ADSB_OUT_CFG_REGISTRATION
        mavlink_uavionix_adsb_out_cfg_registration_t d;
        mavlink_msg_uavionix_adsb_out_cfg_registration_decode(&raw, &d);
        {
            char _s[10];
            memcpy(_s, d.registration, 9);
            _s[9] = '\0';
            display_kv("registration", _s);
        }
        break;
    }
    case 10005: { // UAVIONIX_ADSB_OUT_CFG_FLIGHTID
        mavlink_uavionix_adsb_out_cfg_flightid_t d;
        mavlink_msg_uavionix_adsb_out_cfg_flightid_decode(&raw, &d);
        {
            char _s[10];
            memcpy(_s, d.flight_id, 9);
            _s[9] = '\0';
            display_kv("flight_id", _s);
        }
        break;
    }
    case 10006: { // UAVIONIX_ADSB_GET
        mavlink_uavionix_adsb_get_t d;
        mavlink_msg_uavionix_adsb_get_decode(&raw, &d);
        display_kvf("ReqMessageId", "%u", (unsigned)d.ReqMessageId);
        break;
    }
    case 10007: { // UAVIONIX_ADSB_OUT_CONTROL
        mavlink_uavionix_adsb_out_control_t d;
        mavlink_msg_uavionix_adsb_out_control_decode(&raw, &d);
        display_kvf("state", "%u", (unsigned)d.state);
        display_kvf("baroAltMSL (mbar)", "%d", (int)d.baroAltMSL);
        display_kvf("squawk", "%u", (unsigned)d.squawk);
        display_kvf("emergencyStatus", "%u", (unsigned)d.emergencyStatus);
        {
            char _s[9];
            memcpy(_s, d.flight_id, 8);
            _s[8] = '\0';
            display_kv("flight_id", _s);
        }
        display_kvf("x_bit", "%u", (unsigned)d.x_bit);
        break;
    }
    case 10008: { // UAVIONIX_ADSB_OUT_STATUS
        mavlink_uavionix_adsb_out_status_t d;
        mavlink_msg_uavionix_adsb_out_status_decode(&raw, &d);
        display_kvf("state", "%u", (unsigned)d.state);
        display_kvf("squawk", "%u", (unsigned)d.squawk);
        display_kvf("NIC_NACp", "%u", (unsigned)d.NIC_NACp);
        display_kvf("boardTemp", "%u", (unsigned)d.boardTemp);
        display_kvf("fault", "%u", (unsigned)d.fault);
        {
            char _s[9];
            memcpy(_s, d.flight_id, 8);
            _s[8] = '\0';
            display_kv("flight_id", _s);
        }
        break;
    }
    case 10151: { // LOWEHEISER_GOV_EFI
        mavlink_loweheiser_gov_efi_t d;
        mavlink_msg_loweheiser_gov_efi_decode(&raw, &d);
        display_kvf("volt_batt (V)", "%.4g", (double)d.volt_batt);
        display_kvf("curr_batt (A)", "%.4g", (double)d.curr_batt);
        display_kvf("curr_gen (A)", "%.4g", (double)d.curr_gen);
        display_kvf("curr_rot (A)", "%.4g", (double)d.curr_rot);
        display_kvf("fuel_level (l)", "%.4g", (double)d.fuel_level);
        display_kvf("throttle (%)", "%.4g", (double)d.throttle);
        display_kvf("runtime (s)", "%u", (unsigned)d.runtime);
        display_kvf("until_maintenance (s)", "%d", (int)d.until_maintenance);
        display_kvf("rectifier_temp (degC)", "%.4g", (double)d.rectifier_temp);
        display_kvf("generator_temp (degC)", "%.4g", (double)d.generator_temp);
        display_kvf("efi_batt (V)", "%.4g", (double)d.efi_batt);
        display_kvf("efi_rpm (rpm)", "%.4g", (double)d.efi_rpm);
        display_kvf("efi_pw (ms)", "%.4g", (double)d.efi_pw);
        display_kvf("efi_fuel_flow", "%.4g", (double)d.efi_fuel_flow);
        display_kvf("efi_fuel_consumed (l)", "%.4g", (double)d.efi_fuel_consumed);
        display_kvf("efi_baro (kPa)", "%.4g", (double)d.efi_baro);
        display_kvf("efi_mat (degC)", "%.4g", (double)d.efi_mat);
        display_kvf("efi_clt (degC)", "%.4g", (double)d.efi_clt);
        display_kvf("efi_tps (%)", "%.4g", (double)d.efi_tps);
        display_kvf("efi_exhaust_gas_temperature (degC)", "%.4g", (double)d.efi_exhaust_gas_temperature);
        display_kvf("efi_index", "%u", (unsigned)d.efi_index);
        display_kvf("generator_status", "%u", (unsigned)d.generator_status);
        display_kvf("efi_status", "%u", (unsigned)d.efi_status);
        break;
    }
    case 11000: { // DEVICE_OP_READ
        mavlink_device_op_read_t d;
        mavlink_msg_device_op_read_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("request_id", "%u", (unsigned)d.request_id);
        display_kvf("bustype", "%u", (unsigned)d.bustype);
        display_kvf("bus", "%u", (unsigned)d.bus);
        display_kvf("address", "%u", (unsigned)d.address);
        {
            char _s[41];
            memcpy(_s, d.busname, 40);
            _s[40] = '\0';
            display_kv("busname", _s);
        }
        display_kvf("regstart", "%u", (unsigned)d.regstart);
        display_kvf("count", "%u", (unsigned)d.count);
        display_kvf("bank", "%u", (unsigned)d.bank);
        break;
    }
    case 11001: { // DEVICE_OP_READ_REPLY
        mavlink_device_op_read_reply_t d;
        mavlink_msg_device_op_read_reply_decode(&raw, &d);
        display_kvf("request_id", "%u", (unsigned)d.request_id);
        display_kvf("result", "%u", (unsigned)d.result);
        display_kvf("regstart", "%u", (unsigned)d.regstart);
        display_kvf("count", "%u", (unsigned)d.count);
        display_kv("data", "[128 elements]");
        display_kvf("bank", "%u", (unsigned)d.bank);
        break;
    }
    case 11002: { // DEVICE_OP_WRITE
        mavlink_device_op_write_t d;
        mavlink_msg_device_op_write_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("request_id", "%u", (unsigned)d.request_id);
        display_kvf("bustype", "%u", (unsigned)d.bustype);
        display_kvf("bus", "%u", (unsigned)d.bus);
        display_kvf("address", "%u", (unsigned)d.address);
        {
            char _s[41];
            memcpy(_s, d.busname, 40);
            _s[40] = '\0';
            display_kv("busname", _s);
        }
        display_kvf("regstart", "%u", (unsigned)d.regstart);
        display_kvf("count", "%u", (unsigned)d.count);
        display_kv("data", "[128 elements]");
        display_kvf("bank", "%u", (unsigned)d.bank);
        break;
    }
    case 11003: { // DEVICE_OP_WRITE_REPLY
        mavlink_device_op_write_reply_t d;
        mavlink_msg_device_op_write_reply_decode(&raw, &d);
        display_kvf("request_id", "%u", (unsigned)d.request_id);
        display_kvf("result", "%u", (unsigned)d.result);
        break;
    }
    case 11004: { // SECURE_COMMAND
        mavlink_secure_command_t d;
        mavlink_msg_secure_command_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("sequence", "%u", (unsigned)d.sequence);
        display_kvf("operation", "%u", (unsigned)d.operation);
        display_kvf("data_length", "%u", (unsigned)d.data_length);
        display_kvf("sig_length", "%u", (unsigned)d.sig_length);
        display_kv("data", "[220 elements]");
        break;
    }
    case 11005: { // SECURE_COMMAND_REPLY
        mavlink_secure_command_reply_t d;
        mavlink_msg_secure_command_reply_decode(&raw, &d);
        display_kvf("sequence", "%u", (unsigned)d.sequence);
        display_kvf("operation", "%u", (unsigned)d.operation);
        display_kvf("result", "%u", (unsigned)d.result);
        display_kvf("data_length", "%u", (unsigned)d.data_length);
        display_kv("data", "[220 elements]");
        break;
    }
    case 11010: { // ADAP_TUNING
        mavlink_adap_tuning_t d;
        mavlink_msg_adap_tuning_decode(&raw, &d);
        display_kvf("axis", "%u", (unsigned)d.axis);
        display_kvf("desired (deg/s)", "%.4g", (double)d.desired);
        display_kvf("achieved (deg/s)", "%.4g", (double)d.achieved);
        display_kvf("error", "%.4g", (double)d.error);
        display_kvf("theta", "%.4g", (double)d.theta);
        display_kvf("omega", "%.4g", (double)d.omega);
        display_kvf("sigma", "%.4g", (double)d.sigma);
        display_kvf("theta_dot", "%.4g", (double)d.theta_dot);
        display_kvf("omega_dot", "%.4g", (double)d.omega_dot);
        display_kvf("sigma_dot", "%.4g", (double)d.sigma_dot);
        display_kvf("f", "%.4g", (double)d.f);
        display_kvf("f_dot", "%.4g", (double)d.f_dot);
        display_kvf("u", "%.4g", (double)d.u);
        break;
    }
    case 11011: { // VISION_POSITION_DELTA
        mavlink_vision_position_delta_t d;
        mavlink_msg_vision_position_delta_decode(&raw, &d);
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        display_kvf("time_delta_usec (us)", "%llu", (unsigned long long)d.time_delta_usec);
        {
            char _arr[76]; int _n = 0;
            for (int _i = 0; _i < 3; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%.4g" : "%.4g", (double)d.angle_delta[_i]);
            (void)_n; display_kv("angle_delta (rad)", _arr);
        }
        {
            char _arr[76]; int _n = 0;
            for (int _i = 0; _i < 3; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%.4g" : "%.4g", (double)d.position_delta[_i]);
            (void)_n; display_kv("position_delta (m)", _arr);
        }
        display_kvf("confidence (%)", "%.4g", (double)d.confidence);
        break;
    }
    case 11020: { // AOA_SSA
        mavlink_aoa_ssa_t d;
        mavlink_msg_aoa_ssa_decode(&raw, &d);
        display_kvf("time_usec (us)", "%llu", (unsigned long long)d.time_usec);
        display_kvf("AOA (deg)", "%.4g", (double)d.AOA);
        display_kvf("SSA (deg)", "%.4g", (double)d.SSA);
        break;
    }
    case 11030: { // ESC_TELEMETRY_1_TO_4
        mavlink_esc_telemetry_1_to_4_t d;
        mavlink_msg_esc_telemetry_1_to_4_decode(&raw, &d);
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.temperature[_i]);
            (void)_n; display_kv("temperature (degC)", _arr);
        }
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.voltage[_i]);
            (void)_n; display_kv("voltage (cV)", _arr);
        }
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.current[_i]);
            (void)_n; display_kv("current (cA)", _arr);
        }
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.totalcurrent[_i]);
            (void)_n; display_kv("totalcurrent (mAh)", _arr);
        }
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.rpm[_i]);
            (void)_n; display_kv("rpm (rpm)", _arr);
        }
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.count[_i]);
            (void)_n; display_kv("count", _arr);
        }
        break;
    }
    case 11031: { // ESC_TELEMETRY_5_TO_8
        mavlink_esc_telemetry_5_to_8_t d;
        mavlink_msg_esc_telemetry_5_to_8_decode(&raw, &d);
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.temperature[_i]);
            (void)_n; display_kv("temperature (degC)", _arr);
        }
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.voltage[_i]);
            (void)_n; display_kv("voltage (cV)", _arr);
        }
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.current[_i]);
            (void)_n; display_kv("current (cA)", _arr);
        }
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.totalcurrent[_i]);
            (void)_n; display_kv("totalcurrent (mAh)", _arr);
        }
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.rpm[_i]);
            (void)_n; display_kv("rpm (rpm)", _arr);
        }
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.count[_i]);
            (void)_n; display_kv("count", _arr);
        }
        break;
    }
    case 11032: { // ESC_TELEMETRY_9_TO_12
        mavlink_esc_telemetry_9_to_12_t d;
        mavlink_msg_esc_telemetry_9_to_12_decode(&raw, &d);
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.temperature[_i]);
            (void)_n; display_kv("temperature (degC)", _arr);
        }
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.voltage[_i]);
            (void)_n; display_kv("voltage (cV)", _arr);
        }
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.current[_i]);
            (void)_n; display_kv("current (cA)", _arr);
        }
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.totalcurrent[_i]);
            (void)_n; display_kv("totalcurrent (mAh)", _arr);
        }
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.rpm[_i]);
            (void)_n; display_kv("rpm (rpm)", _arr);
        }
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.count[_i]);
            (void)_n; display_kv("count", _arr);
        }
        break;
    }
    case 11033: { // OSD_PARAM_CONFIG
        mavlink_osd_param_config_t d;
        mavlink_msg_osd_param_config_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("request_id", "%u", (unsigned)d.request_id);
        display_kvf("osd_screen", "%u", (unsigned)d.osd_screen);
        display_kvf("osd_index", "%u", (unsigned)d.osd_index);
        {
            char _s[17];
            memcpy(_s, d.param_id, 16);
            _s[16] = '\0';
            display_kv("param_id", _s);
        }
        display_kvf("config_type", "%u", (unsigned)d.config_type);
        display_kvf("min_value", "%.4g", (double)d.min_value);
        display_kvf("max_value", "%.4g", (double)d.max_value);
        display_kvf("increment", "%.4g", (double)d.increment);
        break;
    }
    case 11034: { // OSD_PARAM_CONFIG_REPLY
        mavlink_osd_param_config_reply_t d;
        mavlink_msg_osd_param_config_reply_decode(&raw, &d);
        display_kvf("request_id", "%u", (unsigned)d.request_id);
        display_kvf("result", "%u", (unsigned)d.result);
        break;
    }
    case 11035: { // OSD_PARAM_SHOW_CONFIG
        mavlink_osd_param_show_config_t d;
        mavlink_msg_osd_param_show_config_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("request_id", "%u", (unsigned)d.request_id);
        display_kvf("osd_screen", "%u", (unsigned)d.osd_screen);
        display_kvf("osd_index", "%u", (unsigned)d.osd_index);
        break;
    }
    case 11036: { // OSD_PARAM_SHOW_CONFIG_REPLY
        mavlink_osd_param_show_config_reply_t d;
        mavlink_msg_osd_param_show_config_reply_decode(&raw, &d);
        display_kvf("request_id", "%u", (unsigned)d.request_id);
        display_kvf("result", "%u", (unsigned)d.result);
        {
            char _s[17];
            memcpy(_s, d.param_id, 16);
            _s[16] = '\0';
            display_kv("param_id", _s);
        }
        display_kvf("config_type", "%u", (unsigned)d.config_type);
        display_kvf("min_value", "%.4g", (double)d.min_value);
        display_kvf("max_value", "%.4g", (double)d.max_value);
        display_kvf("increment", "%.4g", (double)d.increment);
        break;
    }
    case 11037: { // OBSTACLE_DISTANCE_3D
        mavlink_obstacle_distance_3d_t d;
        mavlink_msg_obstacle_distance_3d_decode(&raw, &d);
        display_kvf("time_boot_ms (ms)", "%u", (unsigned)d.time_boot_ms);
        display_kvf("sensor_type", "%u", (unsigned)d.sensor_type);
        display_kvf("frame", "%u", (unsigned)d.frame);
        display_kvf("obstacle_id", "%u", (unsigned)d.obstacle_id);
        display_kvf("x (m)", "%.4g", (double)d.x);
        display_kvf("y (m)", "%.4g", (double)d.y);
        display_kvf("z (m)", "%.4g", (double)d.z);
        display_kvf("min_distance (m)", "%.4g", (double)d.min_distance);
        display_kvf("max_distance (m)", "%.4g", (double)d.max_distance);
        break;
    }
    case 11038: { // WATER_DEPTH
        mavlink_water_depth_t d;
        mavlink_msg_water_depth_decode(&raw, &d);
        display_kvf("time_boot_ms (ms)", "%u", (unsigned)d.time_boot_ms);
        display_kvf("id", "%u", (unsigned)d.id);
        display_kvf("healthy", "%u", (unsigned)d.healthy);
        display_kvf("lat (degE7)", "%d", (int)d.lat);
        display_kvf("lng (degE7)", "%d", (int)d.lng);
        display_kvf("alt (m)", "%.4g", (double)d.alt);
        display_kvf("roll (rad)", "%.4g", (double)d.roll);
        display_kvf("pitch (rad)", "%.4g", (double)d.pitch);
        display_kvf("yaw (rad)", "%.4g", (double)d.yaw);
        display_kvf("distance (m)", "%.4g", (double)d.distance);
        display_kvf("temperature (degC)", "%.4g", (double)d.temperature);
        break;
    }
    case 11039: { // MCU_STATUS
        mavlink_mcu_status_t d;
        mavlink_msg_mcu_status_decode(&raw, &d);
        display_kvf("id", "%u", (unsigned)d.id);
        display_kvf("MCU_temperature (cdegC)", "%d", (int)d.MCU_temperature);
        display_kvf("MCU_voltage (mV)", "%u", (unsigned)d.MCU_voltage);
        display_kvf("MCU_voltage_min (mV)", "%u", (unsigned)d.MCU_voltage_min);
        display_kvf("MCU_voltage_max (mV)", "%u", (unsigned)d.MCU_voltage_max);
        break;
    }
    case 11040: { // ESC_TELEMETRY_13_TO_16
        mavlink_esc_telemetry_13_to_16_t d;
        mavlink_msg_esc_telemetry_13_to_16_decode(&raw, &d);
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.temperature[_i]);
            (void)_n; display_kv("temperature (degC)", _arr);
        }
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.voltage[_i]);
            (void)_n; display_kv("voltage (cV)", _arr);
        }
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.current[_i]);
            (void)_n; display_kv("current (cA)", _arr);
        }
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.totalcurrent[_i]);
            (void)_n; display_kv("totalcurrent (mAh)", _arr);
        }
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.rpm[_i]);
            (void)_n; display_kv("rpm (rpm)", _arr);
        }
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.count[_i]);
            (void)_n; display_kv("count", _arr);
        }
        break;
    }
    case 11041: { // ESC_TELEMETRY_17_TO_20
        mavlink_esc_telemetry_17_to_20_t d;
        mavlink_msg_esc_telemetry_17_to_20_decode(&raw, &d);
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.temperature[_i]);
            (void)_n; display_kv("temperature (degC)", _arr);
        }
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.voltage[_i]);
            (void)_n; display_kv("voltage (cV)", _arr);
        }
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.current[_i]);
            (void)_n; display_kv("current (cA)", _arr);
        }
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.totalcurrent[_i]);
            (void)_n; display_kv("totalcurrent (mAh)", _arr);
        }
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.rpm[_i]);
            (void)_n; display_kv("rpm (rpm)", _arr);
        }
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.count[_i]);
            (void)_n; display_kv("count", _arr);
        }
        break;
    }
    case 11042: { // ESC_TELEMETRY_21_TO_24
        mavlink_esc_telemetry_21_to_24_t d;
        mavlink_msg_esc_telemetry_21_to_24_decode(&raw, &d);
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.temperature[_i]);
            (void)_n; display_kv("temperature (degC)", _arr);
        }
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.voltage[_i]);
            (void)_n; display_kv("voltage (cV)", _arr);
        }
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.current[_i]);
            (void)_n; display_kv("current (cA)", _arr);
        }
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.totalcurrent[_i]);
            (void)_n; display_kv("totalcurrent (mAh)", _arr);
        }
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.rpm[_i]);
            (void)_n; display_kv("rpm (rpm)", _arr);
        }
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.count[_i]);
            (void)_n; display_kv("count", _arr);
        }
        break;
    }
    case 11043: { // ESC_TELEMETRY_25_TO_28
        mavlink_esc_telemetry_25_to_28_t d;
        mavlink_msg_esc_telemetry_25_to_28_decode(&raw, &d);
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.temperature[_i]);
            (void)_n; display_kv("temperature (degC)", _arr);
        }
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.voltage[_i]);
            (void)_n; display_kv("voltage (cV)", _arr);
        }
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.current[_i]);
            (void)_n; display_kv("current (cA)", _arr);
        }
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.totalcurrent[_i]);
            (void)_n; display_kv("totalcurrent (mAh)", _arr);
        }
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.rpm[_i]);
            (void)_n; display_kv("rpm (rpm)", _arr);
        }
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.count[_i]);
            (void)_n; display_kv("count", _arr);
        }
        break;
    }
    case 11044: { // ESC_TELEMETRY_29_TO_32
        mavlink_esc_telemetry_29_to_32_t d;
        mavlink_msg_esc_telemetry_29_to_32_decode(&raw, &d);
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.temperature[_i]);
            (void)_n; display_kv("temperature (degC)", _arr);
        }
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.voltage[_i]);
            (void)_n; display_kv("voltage (cV)", _arr);
        }
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.current[_i]);
            (void)_n; display_kv("current (cA)", _arr);
        }
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.totalcurrent[_i]);
            (void)_n; display_kv("totalcurrent (mAh)", _arr);
        }
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.rpm[_i]);
            (void)_n; display_kv("rpm (rpm)", _arr);
        }
        {
            char _arr[100]; int _n = 0;
            for (int _i = 0; _i < 4; ++_i)
                _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,
                               _i ? ",%u" : "%u", (unsigned)d.count[_i]);
            (void)_n; display_kv("count", _arr);
        }
        break;
    }
    case 11060: { // NAMED_VALUE_STRING
        mavlink_named_value_string_t d;
        mavlink_msg_named_value_string_decode(&raw, &d);
        display_kvf("time_boot_ms (ms)", "%u", (unsigned)d.time_boot_ms);
        {
            char _s[11];
            memcpy(_s, d.name, 10);
            _s[10] = '\0';
            display_kv("name", _s);
        }
        {
            char _s[65];
            memcpy(_s, d.value, 64);
            _s[64] = '\0';
            display_kv("value", _s);
        }
        break;
    }
    case 12900: { // OPEN_DRONE_ID_BASIC_ID
        mavlink_open_drone_id_basic_id_t d;
        mavlink_msg_open_drone_id_basic_id_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kv("id_or_mac", "[20 elements]");
        display_kvf("id_type", "%u", (unsigned)d.id_type);
        display_kvf("ua_type", "%u", (unsigned)d.ua_type);
        display_kv("uas_id", "[20 elements]");
        break;
    }
    case 12901: { // OPEN_DRONE_ID_LOCATION
        mavlink_open_drone_id_location_t d;
        mavlink_msg_open_drone_id_location_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kv("id_or_mac", "[20 elements]");
        display_kvf("status", "%u", (unsigned)d.status);
        display_kvf("direction (cdeg)", "%u", (unsigned)d.direction);
        display_kvf("speed_horizontal (cm/s)", "%u", (unsigned)d.speed_horizontal);
        display_kvf("speed_vertical (cm/s)", "%d", (int)d.speed_vertical);
        display_kvf("latitude (degE7)", "%d", (int)d.latitude);
        display_kvf("longitude (degE7)", "%d", (int)d.longitude);
        display_kvf("altitude_barometric (m)", "%.4g", (double)d.altitude_barometric);
        display_kvf("altitude_geodetic (m)", "%.4g", (double)d.altitude_geodetic);
        display_kvf("height_reference", "%u", (unsigned)d.height_reference);
        display_kvf("height (m)", "%.4g", (double)d.height);
        display_kvf("horizontal_accuracy", "%u", (unsigned)d.horizontal_accuracy);
        display_kvf("vertical_accuracy", "%u", (unsigned)d.vertical_accuracy);
        display_kvf("barometer_accuracy", "%u", (unsigned)d.barometer_accuracy);
        display_kvf("speed_accuracy", "%u", (unsigned)d.speed_accuracy);
        display_kvf("timestamp (s)", "%.4g", (double)d.timestamp);
        display_kvf("timestamp_accuracy", "%u", (unsigned)d.timestamp_accuracy);
        break;
    }
    case 12902: { // OPEN_DRONE_ID_AUTHENTICATION
        mavlink_open_drone_id_authentication_t d;
        mavlink_msg_open_drone_id_authentication_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kv("id_or_mac", "[20 elements]");
        display_kvf("authentication_type", "%u", (unsigned)d.authentication_type);
        display_kvf("data_page", "%u", (unsigned)d.data_page);
        display_kvf("last_page_index", "%u", (unsigned)d.last_page_index);
        display_kvf("length (bytes)", "%u", (unsigned)d.length);
        display_kvf("timestamp (s)", "%u", (unsigned)d.timestamp);
        display_kv("authentication_data", "[23 elements]");
        break;
    }
    case 12903: { // OPEN_DRONE_ID_SELF_ID
        mavlink_open_drone_id_self_id_t d;
        mavlink_msg_open_drone_id_self_id_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kv("id_or_mac", "[20 elements]");
        display_kvf("description_type", "%u", (unsigned)d.description_type);
        {
            char _s[24];
            memcpy(_s, d.description, 23);
            _s[23] = '\0';
            display_kv("description", _s);
        }
        break;
    }
    case 12904: { // OPEN_DRONE_ID_SYSTEM
        mavlink_open_drone_id_system_t d;
        mavlink_msg_open_drone_id_system_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kv("id_or_mac", "[20 elements]");
        display_kvf("operator_location_type", "%u", (unsigned)d.operator_location_type);
        display_kvf("classification_type", "%u", (unsigned)d.classification_type);
        display_kvf("operator_latitude (degE7)", "%d", (int)d.operator_latitude);
        display_kvf("operator_longitude (degE7)", "%d", (int)d.operator_longitude);
        display_kvf("area_count", "%u", (unsigned)d.area_count);
        display_kvf("area_radius (m)", "%u", (unsigned)d.area_radius);
        display_kvf("area_ceiling (m)", "%.4g", (double)d.area_ceiling);
        display_kvf("area_floor (m)", "%.4g", (double)d.area_floor);
        display_kvf("category_eu", "%u", (unsigned)d.category_eu);
        display_kvf("class_eu", "%u", (unsigned)d.class_eu);
        display_kvf("operator_altitude_geo (m)", "%.4g", (double)d.operator_altitude_geo);
        display_kvf("timestamp (s)", "%u", (unsigned)d.timestamp);
        break;
    }
    case 12905: { // OPEN_DRONE_ID_OPERATOR_ID
        mavlink_open_drone_id_operator_id_t d;
        mavlink_msg_open_drone_id_operator_id_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kv("id_or_mac", "[20 elements]");
        display_kvf("operator_id_type", "%u", (unsigned)d.operator_id_type);
        {
            char _s[21];
            memcpy(_s, d.operator_id, 20);
            _s[20] = '\0';
            display_kv("operator_id", _s);
        }
        break;
    }
    case 12915: { // OPEN_DRONE_ID_MESSAGE_PACK
        mavlink_open_drone_id_message_pack_t d;
        mavlink_msg_open_drone_id_message_pack_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kv("id_or_mac", "[20 elements]");
        display_kvf("single_message_size (bytes)", "%u", (unsigned)d.single_message_size);
        display_kvf("msg_pack_size", "%u", (unsigned)d.msg_pack_size);
        display_kv("messages", "[225 elements]");
        break;
    }
    case 12918: { // OPEN_DRONE_ID_ARM_STATUS
        mavlink_open_drone_id_arm_status_t d;
        mavlink_msg_open_drone_id_arm_status_decode(&raw, &d);
        display_kvf("status", "%u", (unsigned)d.status);
        {
            char _s[51];
            memcpy(_s, d.error, 50);
            _s[50] = '\0';
            display_kv("error", _s);
        }
        break;
    }
    case 12919: { // OPEN_DRONE_ID_SYSTEM_UPDATE
        mavlink_open_drone_id_system_update_t d;
        mavlink_msg_open_drone_id_system_update_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("operator_latitude (degE7)", "%d", (int)d.operator_latitude);
        display_kvf("operator_longitude (degE7)", "%d", (int)d.operator_longitude);
        display_kvf("operator_altitude_geo (m)", "%.4g", (double)d.operator_altitude_geo);
        display_kvf("timestamp (s)", "%u", (unsigned)d.timestamp);
        break;
    }
    case 12920: { // HYGROMETER_SENSOR
        mavlink_hygrometer_sensor_t d;
        mavlink_msg_hygrometer_sensor_decode(&raw, &d);
        display_kvf("id", "%u", (unsigned)d.id);
        display_kvf("temperature (cdegC)", "%d", (int)d.temperature);
        display_kvf("humidity (c%)", "%u", (unsigned)d.humidity);
        break;
    }
    case 42000: { // ICAROUS_HEARTBEAT
        mavlink_icarous_heartbeat_t d;
        mavlink_msg_icarous_heartbeat_decode(&raw, &d);
        display_kvf("status", "%u", (unsigned)d.status);
        break;
    }
    case 42001: { // ICAROUS_KINEMATIC_BANDS
        mavlink_icarous_kinematic_bands_t d;
        mavlink_msg_icarous_kinematic_bands_decode(&raw, &d);
        display_kvf("numBands", "%d", (int)d.numBands);
        display_kvf("type1", "%u", (unsigned)d.type1);
        display_kvf("min1 (deg)", "%.4g", (double)d.min1);
        display_kvf("max1 (deg)", "%.4g", (double)d.max1);
        display_kvf("type2", "%u", (unsigned)d.type2);
        display_kvf("min2 (deg)", "%.4g", (double)d.min2);
        display_kvf("max2 (deg)", "%.4g", (double)d.max2);
        display_kvf("type3", "%u", (unsigned)d.type3);
        display_kvf("min3 (deg)", "%.4g", (double)d.min3);
        display_kvf("max3 (deg)", "%.4g", (double)d.max3);
        display_kvf("type4", "%u", (unsigned)d.type4);
        display_kvf("min4 (deg)", "%.4g", (double)d.min4);
        display_kvf("max4 (deg)", "%.4g", (double)d.max4);
        display_kvf("type5", "%u", (unsigned)d.type5);
        display_kvf("min5 (deg)", "%.4g", (double)d.min5);
        display_kvf("max5 (deg)", "%.4g", (double)d.max5);
        break;
    }
    case 50001: { // CUBEPILOT_RAW_RC
        mavlink_cubepilot_raw_rc_t d;
        mavlink_msg_cubepilot_raw_rc_decode(&raw, &d);
        display_kv("rc_raw", "[32 elements]");
        break;
    }
    case 50002: { // HERELINK_VIDEO_STREAM_INFORMATION
        mavlink_herelink_video_stream_information_t d;
        mavlink_msg_herelink_video_stream_information_decode(&raw, &d);
        display_kvf("camera_id", "%u", (unsigned)d.camera_id);
        display_kvf("status", "%u", (unsigned)d.status);
        display_kvf("framerate (Hz)", "%.4g", (double)d.framerate);
        display_kvf("resolution_h (pix)", "%u", (unsigned)d.resolution_h);
        display_kvf("resolution_v (pix)", "%u", (unsigned)d.resolution_v);
        display_kvf("bitrate (bits/s)", "%u", (unsigned)d.bitrate);
        display_kvf("rotation (deg)", "%u", (unsigned)d.rotation);
        {
            char _s[231];
            memcpy(_s, d.uri, 230);
            _s[230] = '\0';
            display_kv("uri", _s);
        }
        break;
    }
    case 50003: { // HERELINK_TELEM
        mavlink_herelink_telem_t d;
        mavlink_msg_herelink_telem_decode(&raw, &d);
        display_kvf("rssi", "%u", (unsigned)d.rssi);
        display_kvf("snr", "%d", (int)d.snr);
        display_kvf("rf_freq", "%u", (unsigned)d.rf_freq);
        display_kvf("link_bw", "%u", (unsigned)d.link_bw);
        display_kvf("link_rate", "%u", (unsigned)d.link_rate);
        display_kvf("cpu_temp", "%d", (int)d.cpu_temp);
        display_kvf("board_temp", "%d", (int)d.board_temp);
        break;
    }
    case 50004: { // CUBEPILOT_FIRMWARE_UPDATE_START
        mavlink_cubepilot_firmware_update_start_t d;
        mavlink_msg_cubepilot_firmware_update_start_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("size (bytes)", "%u", (unsigned)d.size);
        display_kvf("crc", "%u", (unsigned)d.crc);
        break;
    }
    case 50005: { // CUBEPILOT_FIRMWARE_UPDATE_RESP
        mavlink_cubepilot_firmware_update_resp_t d;
        mavlink_msg_cubepilot_firmware_update_resp_decode(&raw, &d);
        display_kvf("target_system", "%u", (unsigned)d.target_system);
        display_kvf("target_component", "%u", (unsigned)d.target_component);
        display_kvf("offset (bytes)", "%u", (unsigned)d.offset);
        break;
    }
    case 52000: { // AIRLINK_AUTH
        mavlink_airlink_auth_t d;
        mavlink_msg_airlink_auth_decode(&raw, &d);
        {
            char _s[51];
            memcpy(_s, d.login, 50);
            _s[50] = '\0';
            display_kv("login", _s);
        }
        {
            char _s[51];
            memcpy(_s, d.password, 50);
            _s[50] = '\0';
            display_kv("password", _s);
        }
        break;
    }
    case 52001: { // AIRLINK_AUTH_RESPONSE
        mavlink_airlink_auth_response_t d;
        mavlink_msg_airlink_auth_response_decode(&raw, &d);
        display_kvf("resp_type", "%u", (unsigned)d.resp_type);
        break;
    }
    default:
        display_kvf("id", "%u (unknown message)", (unsigned)id);
        break;
    }
}
