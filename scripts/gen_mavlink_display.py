#!/usr/bin/env python3
"""
Generate mavlink_display_generated.cpp and .hpp from MAVLink XML definitions.
Parses ardupilotmega.xml transitively (which pulls in common.xml, minimal.xml, etc.)

Usage: gen_mavlink_display.py <xml_dir> <out_cpp> <out_hpp>
"""

import xml.etree.ElementTree as ET
import sys
import os

XML_DIR = ''
seen_files = set()
messages = {}  # id (int) -> {'name': str, 'fields': [...]}


def parse_xml(filename):
    """Recursively parse a MAVLink XML file and all its <include> dependencies."""
    if filename in seen_files:
        return
    seen_files.add(filename)

    filepath = os.path.join(XML_DIR, filename)
    if not os.path.exists(filepath):
        return  # silently skip missing optional vendor includes

    try:
        tree = ET.parse(filepath)
    except ET.ParseError as e:
        print(f'Warning: failed to parse {filepath}: {e}', file=sys.stderr)
        return

    root = tree.getroot()

    # Depth-first include processing (matches the official MAVLink generator order)
    for inc in root.findall('include'):
        if inc.text:
            parse_xml(inc.text.strip())

    msgs_elem = root.find('messages')
    if msgs_elem is None:
        return

    for msg in msgs_elem.findall('message'):
        msg_id_str = msg.get('id')
        if not msg_id_str:
            continue
        msg_id   = int(msg_id_str)
        msg_name = msg.get('name', f'MSG_{msg_id}')

        fields = []
        for child in msg:
            if child.tag != 'field':
                continue
            fields.append({
                'type':         child.get('type', 'uint8_t'),
                'name':         child.get('name', '_unknown'),
                'units':        child.get('units', ''),
                'print_format': child.get('print_format', ''),
            })

        messages[msg_id] = {'name': msg_name, 'fields': fields}


# Message IDs that are purely GCS→FC uplink (commands, requests, configuration).
# Excluded from the field catalog so the picker only shows receivable telemetry.
UPLINK_ONLY_IDS = {
    # Operator control / mode
    5,   # CHANGE_OPERATOR_CONTROL
    6,   # CHANGE_OPERATOR_CONTROL_ACK
    11,  # SET_MODE
    # Parameter protocol — GCS initiates these
    20,  # PARAM_REQUEST_READ
    21,  # PARAM_REQUEST_LIST
    23,  # PARAM_SET
    # Mission protocol — GCS→FC upload/management
    37,  # MISSION_REQUEST_PARTIAL_LIST
    38,  # MISSION_WRITE_PARTIAL_LIST
    39,  # MISSION_ITEM
    40,  # MISSION_REQUEST
    41,  # MISSION_SET_CURRENT
    43,  # MISSION_REQUEST_LIST
    45,  # MISSION_CLEAR_ALL
    48,  # SET_GPS_GLOBAL_ORIGIN
    50,  # PARAM_MAP_RC
    51,  # MISSION_REQUEST_INT
    54,  # SAFETY_SET_ALLOWED_AREA
    # Stream / data requests
    66,  # REQUEST_DATA_STREAM
    # Manual / override inputs
    69,  # MANUAL_CONTROL
    70,  # RC_CHANNELS_OVERRIDE
    # Commands (GCS sends these)
    73,  # MISSION_ITEM_INT  (upload direction)
    75,  # COMMAND_INT
    76,  # COMMAND_LONG
    80,  # COMMAND_CANCEL
    81,  # MANUAL_SETPOINT
    # Setpoint / target messages (GCS→FC)
    82,  # SET_ATTITUDE_TARGET
    84,  # SET_POSITION_TARGET_LOCAL_NED
    86,  # SET_POSITION_TARGET_GLOBAL_INT
    # Log control
    117, # LOG_REQUEST_LIST
    119, # LOG_REQUEST_DATA
    121, # LOG_ERASE
    122, # LOG_REQUEST_END
    # Misc uplink
    123, # GPS_INJECT_DATA
    126, # SERIAL_CONTROL
    139, # SET_ACTUATOR_CONTROL_TARGET
    142, # RESOURCE_REQUEST
    151, # SET_MAG_OFFSETS
    154, # DIGICAM_CONFIGURE
    155, # DIGICAM_CONTROL
    156, # MOUNT_CONFIGURE
    157, # MOUNT_CONTROL
    160, # FENCE_POINT (upload)
    161, # FENCE_FETCH_POINT
    183, # AUTOPILOT_VERSION_REQUEST
    184, # REMOTE_LOG_DATA_BLOCK
    186, # LED_CONTROL
    218, # GOPRO_SET_REQUEST
    243, # SET_HOME_POSITION
    244, # MESSAGE_INTERVAL (request, not report)
    248, # V2_EXTENSION
    256, # SETUP_SIGNING
    258, # PLAY_TUNE
    # Gimbal setpoints
    282, # GIMBAL_MANAGER_SET_ATTITUDE
    284, # GIMBAL_DEVICE_SET_ATTITUDE
    287, # GIMBAL_MANAGER_SET_PITCHYAW
    288, # GIMBAL_MANAGER_SET_MANUAL_CONTROL
    # Config / provisioning
    299, # WIFI_CONFIG_AP
    300, # PROTOCOL_VERSION
    320, # PARAM_EXT_REQUEST_READ
    321, # PARAM_EXT_REQUEST_LIST
    323, # PARAM_EXT_SET
    336, # CELLULAR_CONFIG
    388, # CAN_FILTER_MODIFY
    400, # PLAY_TUNE_V2
    412, # REQUEST_EVENT
    # Vendor / ArduPilot uplink
    11002, # DEVICE_OP_WRITE
    11004, # SECURE_COMMAND
    11033, # OSD_PARAM_CONFIG
    11035, # OSD_PARAM_SHOW_CONFIG
    50004, # CUBEPILOT_FIRMWARE_UPDATE_START
    # HIL / simulation — inputs to FC in SITL mode, not streamable telemetry
    90,  # HIL_STATE
    91,  # HIL_CONTROLS
    92,  # HIL_RC_INPUTS_RAW
    93,  # HIL_ACTUATOR_CONTROLS
    107, # HIL_SENSOR
    108, # SIM_STATE
    113, # HIL_GPS
    114, # HIL_OPTICAL_FLOW
    115, # HIL_STATE_QUATERNION
}

# Base C type -> (cast prefix, printf format string)
TYPE_FMT = {
    'uint8_t':   ('(unsigned)',           '%u'),
    'uint16_t':  ('(unsigned)',           '%u'),
    'uint32_t':  ('(unsigned)',           '%u'),
    'uint64_t':  ('(unsigned long long)', '%llu'),
    'int8_t':    ('(int)',                '%d'),
    'int16_t':   ('(int)',                '%d'),
    'int32_t':   ('(int)',                '%d'),
    'int64_t':   ('(long long)',          '%lld'),
    'float':     ('(double)',             '%.4g'),
    'double':    ('',                     '%.6g'),
    'char':      ('',                     '%c'),
}


def parse_type(type_str):
    """Returns (base_type: str, array_len: int)."""
    type_str = type_str.strip()
    if '[' in type_str:
        base = type_str[:type_str.index('[')].strip()
        n    = int(type_str[type_str.index('[')+1 : type_str.index(']')])
        return base, n
    return type_str, 0


def gen_field_lines(fname, ftype_str, funits, fprint_fmt, svar, ind='        '):
    """Return a list of C++ source lines to display one field."""
    base_type, arr_len = parse_type(ftype_str)
    label = f'{fname} ({funits})' if funits else fname

    lines = []

    if arr_len > 0 and base_type == 'char':
        # char[N]: copy to null-terminated local, display as string
        lines += [
            f'{ind}{{',
            f'{ind}    char _s[{arr_len + 1}];',
            f'{ind}    memcpy(_s, {svar}.{fname}, {arr_len});',
            f'{ind}    _s[{arr_len}] = \'\\0\';',
            f'{ind}    display_kv("{label}", _s);',
            f'{ind}}}',
        ]

    elif arr_len > 0:
        if arr_len > 16:
            # Too large to expand inline — show element count
            lines.append(f'{ind}display_kv("{label}", "[{arr_len} elements]");')
        else:
            cast, scalar_fmt = TYPE_FMT.get(base_type, ('(unsigned)', '%u'))
            buf_sz = arr_len * 24 + 4
            lines += [
                f'{ind}{{',
                f'{ind}    char _arr[{buf_sz}]; int _n = 0;',
                f'{ind}    for (int _i = 0; _i < {arr_len}; ++_i)',
                f'{ind}        _n += snprintf(_arr + _n, (int)sizeof(_arr) - _n,',
                f'{ind}                       _i ? ",{scalar_fmt}" : "{scalar_fmt}", {cast}{svar}.{fname}[_i]);',
                f'{ind}    (void)_n; display_kv("{label}", _arr);',
                f'{ind}}}',
            ]

    else:
        # Scalar — pick cast and format
        if fprint_fmt:
            fmt = fprint_fmt
            if '64' in base_type:
                cast = '(unsigned long long)' if 'uint' in base_type else '(long long)'
            elif base_type in ('float', 'double'):
                cast = '(double)'
            else:
                cast = '(unsigned)' if 'uint' in base_type else '(int)'
        else:
            cast, fmt = TYPE_FMT.get(base_type, ('(unsigned)', '%u'))

        val = f'{cast}{svar}.{fname}' if cast else f'{svar}.{fname}'
        lines.append(f'{ind}display_kvf("{label}", "{fmt}", {val});')

    return lines


NUMERIC_TYPES = {
    'uint8_t', 'uint16_t', 'uint32_t', 'uint64_t',
    'int8_t',  'int16_t',  'int32_t',  'int64_t',
    'float',   'double',
}
# Max array length to expand into indexed catalog entries (e.g. controls[0..3])
CATALOG_MAX_ARR = 4


def generate_catalog(out_cpp, out_hpp):
    """Generate mavlink_field_catalog_generated.{cpp,hpp} alongside the display files."""
    sorted_msgs = sorted(messages.items())

    # ── Header ────────────────────────────────────────────────────────────────
    with open(out_hpp, 'w') as f:
        f.write('// AUTO-GENERATED — do not edit manually.\n')
        f.write('// Regenerate: re-run CMake or call gen_mavlink_display.py directly.\n')
        f.write('#pragma once\n')
        f.write('#include <cstdint>\n')
        f.write('#include <cstddef>\n')
        f.write('#include <mavlink/ardupilotmega/mavlink.h>\n')
        f.write('\n')
        f.write('// Describes one numeric-readable field in a MAVLink message.\n')
        f.write('struct MavlinkFieldDesc {\n')
        f.write('    uint32_t    msg_id;\n')
        f.write('    const char* msg_name;   // e.g. "VFR_HUD"\n')
        f.write('    const char* field_name; // e.g. "airspeed" or "controls[0]"\n')
        f.write('    const char* label;      // e.g. "airspeed (m/s)"\n')
        f.write('};\n\n')
        f.write('// Flat catalog of all numeric scalar fields, sorted by msg_id.\n')
        f.write('extern const MavlinkFieldDesc kMavlinkFieldCatalog[];\n')
        f.write('extern const size_t           kMavlinkFieldCatalogCount;\n\n')
        f.write('// Read a single numeric field value from a decoded raw message.\n')
        f.write('// Returns true and sets *out_value on success.\n')
        f.write('bool mavlink_field_read(uint32_t msg_id, const char* field_name,\n')
        f.write('                        const mavlink_message_t& raw, double* out_value);\n')

    # ── Source ────────────────────────────────────────────────────────────────
    with open(out_cpp, 'w') as f:
        f.write('// AUTO-GENERATED — do not edit manually.\n')
        f.write('// Regenerate: re-run CMake or call gen_mavlink_display.py directly.\n')
        f.write('#include "mavlink_field_catalog_generated.hpp"\n')
        f.write('#include <cstring>\n\n')

        # Collect catalog entries (skip uplink-only messages)
        catalog = []  # (msg_id, msg_name, field_name, label)
        for msg_id, msg in sorted_msgs:
            if msg_id in UPLINK_ONLY_IDS:
                continue
            name = msg['name']
            for field in msg['fields']:
                base_type, arr_len = parse_type(field['type'])
                if base_type not in NUMERIC_TYPES:
                    continue
                units = field['units']
                fname = field['name']
                if arr_len == 0:
                    label = f'{fname} ({units})' if units else fname
                    catalog.append((msg_id, name, fname, label))
                elif arr_len <= CATALOG_MAX_ARR:
                    for i in range(arr_len):
                        iname = f'{fname}[{i}]'
                        label = f'{iname} ({units})' if units else iname
                        catalog.append((msg_id, name, iname, label))

        # Catalog array
        f.write(f'const MavlinkFieldDesc kMavlinkFieldCatalog[] = {{\n')
        for msg_id, msg_name, field_name, label in catalog:
            # Escape any quotes in strings
            fn_esc  = field_name.replace('"', '\\"')
            lbl_esc = label.replace('"', '\\"')
            f.write(f'    {{ {msg_id}, "{msg_name}", "{fn_esc}", "{lbl_esc}" }},\n')
        f.write('};\n\n')
        f.write(f'const size_t kMavlinkFieldCatalogCount = {len(catalog)};\n\n')

        # Read function
        f.write('bool mavlink_field_read(uint32_t msg_id, const char* field_name,\n')
        f.write('                        const mavlink_message_t& raw, double* out_value)\n{\n')
        f.write('    switch (msg_id) {\n')

        for msg_id, msg in sorted_msgs:
            if msg_id in UPLINK_ONLY_IDS:
                continue
            name   = msg['name']
            lower  = name.lower()
            # Collect numeric fields for this message
            num_fields = []
            for field in msg['fields']:
                base_type, arr_len = parse_type(field['type'])
                if base_type not in NUMERIC_TYPES:
                    continue
                num_fields.append((field['name'], base_type, arr_len))

            if not num_fields:
                continue

            f.write(f'    case {msg_id}: {{\n')
            f.write(f'        mavlink_{lower}_t d;\n')
            f.write(f'        mavlink_msg_{lower}_decode(&raw, &d);\n')
            for fname, base_type, arr_len in num_fields:
                if arr_len == 0:
                    f.write(f'        if (strcmp(field_name, "{fname}") == 0)'
                            f' {{ *out_value = (double)d.{fname}; return true; }}\n')
                elif arr_len <= CATALOG_MAX_ARR:
                    for i in range(arr_len):
                        iname = f'{fname}[{i}]'
                        f.write(f'        if (strcmp(field_name, "{iname}") == 0)'
                                f' {{ *out_value = (double)d.{fname}[{i}]; return true; }}\n')
            f.write('        return false;\n')
            f.write('    }\n')

        f.write('    default: return false;\n')
        f.write('    }\n}\n')

    print(f'Generated {len(catalog)} field catalog entries → {os.path.basename(out_cpp)}',
          file=sys.stderr)


def generate(xml_dir, out_cpp, out_hpp):
    global XML_DIR
    XML_DIR = xml_dir
    parse_xml('ardupilotmega.xml')

    sorted_msgs = sorted(messages.items())

    # ── Header ────────────────────────────────────────────────────────────────
    with open(out_hpp, 'w') as f:
        f.write('// AUTO-GENERATED — do not edit manually.\n')
        f.write('// Regenerate: re-run CMake or call gen_mavlink_display.py directly.\n')
        f.write('#pragma once\n')
        f.write('#include <cstdint>\n')
        f.write('#include <mavlink/ardupilotmega/mavlink.h>\n')
        f.write('\n')
        f.write('// Returns the human-readable name for a MAVLink message ID, or nullptr.\n')
        f.write('const char* mavlink_display_name(uint32_t id);\n')
        f.write('\n')
        f.write('// Renders all decoded fields for the given raw message as ImGui kv rows.\n')
        f.write('// Must be called inside an active ImGui window or child.\n')
        f.write('void mavlink_display_draw(uint32_t id, const mavlink_message_t& raw);\n')

    # ── Source ────────────────────────────────────────────────────────────────
    with open(out_cpp, 'w') as f:
        f.write('// AUTO-GENERATED — do not edit manually.\n')
        f.write('// Regenerate: re-run CMake or call gen_mavlink_display.py directly.\n')
        f.write('#include "mavlink_display_generated.hpp"\n')
        f.write('#include "mavlink_display_shim.hpp"\n')
        f.write('#include <cstdio>\n')
        f.write('#include <cstring>\n')
        f.write('\n')

        # Name lookup
        f.write('const char* mavlink_display_name(uint32_t id)\n{\n')
        f.write('    switch (id) {\n')
        for msg_id, msg in sorted_msgs:
            f.write(f'    case {msg_id}: return "{msg["name"]}";\n')
        f.write('    default: return nullptr;\n')
        f.write('    }\n}\n\n')

        # Decode + display
        f.write('void mavlink_display_draw(uint32_t id, const mavlink_message_t& raw)\n{\n')
        f.write('    switch (id) {\n')

        for msg_id, msg in sorted_msgs:
            name   = msg['name']
            fields = msg['fields']
            lower  = name.lower()

            f.write(f'    case {msg_id}: {{ // {name}\n')
            if fields:
                f.write(f'        mavlink_{lower}_t d;\n')
                f.write(f'        mavlink_msg_{lower}_decode(&raw, &d);\n')
                for field in fields:
                    for line in gen_field_lines(field['name'], field['type'],
                                               field['units'], field['print_format'], 'd'):
                        f.write(line + '\n')
            else:
                f.write(f'        display_kv("(no fields)", "");\n')
            f.write('        break;\n    }\n')

        f.write('    default:\n')
        f.write('        display_kvf("id", "%u (unknown message)", (unsigned)id);\n')
        f.write('        break;\n')
        f.write('    }\n}\n')

    print(f'Generated {len(messages)} message decoders → {os.path.basename(out_cpp)}',
          file=sys.stderr)

    # Derive catalog paths alongside the display files
    import os as _os
    base_dir       = _os.path.dirname(out_cpp)
    cat_cpp        = _os.path.join(base_dir, 'mavlink_field_catalog_generated.cpp')
    cat_hpp        = _os.path.join(base_dir, 'mavlink_field_catalog_generated.hpp')
    generate_catalog(cat_cpp, cat_hpp)


if __name__ == '__main__':
    if len(sys.argv) != 4:
        print(f'Usage: {sys.argv[0]} <xml_dir> <out_cpp> <out_hpp>', file=sys.stderr)
        sys.exit(1)
    generate(sys.argv[1], sys.argv[2], sys.argv[3])
