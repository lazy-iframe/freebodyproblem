---
title: Configuration
description: Settings, files and where they live.
---

Settings are auto-saved to a per-user location, not the working directory:

| Platform | Path                                                                                           |
|----------|------------------------------------------------------------------------------------------------|
| Linux, macOS | `$XDG_CONFIG_HOME/freebodyproblem/settings.json`, or `~/.config/freebodyproblem/settings.json` |
| Windows  | `%APPDATA%\freebodyproblem\settings.json`                                                      |

If neither environment variable is set, it falls back to
`./freebodyproblem_settings.json` in the working directory.

Map tiles are cached separately, under
`$XDG_CACHE_HOME`/`~/.cache/freebodyproblem/tiles` on Linux and macOS, and
`%LOCALAPPDATA%\freebodyproblem\tiles` on Windows. Deleting that directory is
safe — tiles are refetched on demand.
Each map source caches in its own folder there, except Google with an API key, whose tiles are
not written to disk.

Telemetry logs and video snapshots go to the save folder, in `tlogs` and `snapshots`:
`$XDG_DATA_HOME`/`~/.local/share/freebodyproblem` on Linux, `~/Documents/freebodyproblem`
on macOS and `Documents\freebodyproblem` on Windows, unless `save_dir` names another.
Nothing clears that folder but you.

Settings contents:
- **Active theme**: Selected color scheme
- **Custom themes**: User-defined color palettes
- **Map source**: `tile_provider` (0 Esri, 1 OpenStreetMap, 2 Google, 3 Custom, 4 Sentinel-2, 5 NASA GIBS), `tile_kind` (0 satellite, 1 hybrid, 2 terrain), `tile_gibs_layer` and `tile_gibs_date`, `tile_key_esri`, `tile_key_google` and `tile_key_custom`, with `tile_url` and `tile_attribution` for the custom source. Keys are stored in plain text
- **GCS system ID**: `gcs_sysid`, 0 to 255, default 255. Set in **SETTINGS → MAVLINK**. On ArduPilot, set `MAV_GCS_SYSID` (`SYSID_MYGCS` on older firmware) to match, because joystick, RC override and GCS failsafe only answer to that ID
- **Files**: `save_dir`, the save folder, empty for the default, and `tlog_enabled`, default on. Both set in **SETTINGS → FILES**
- **Vehicle names**: `vehicle_names`, link and sysid (`udp:0.0.0.0:14550/1`) → the name given in the **CONNECTION** tab's **VEHICLES** list
- **Video URL**: Last used stream source
- **Joystick**: `joystick_profiles`, one per device GUID: calibration, stick and button bindings, virtual channels and RC override bindings
- **MCP server**: `mcp_enabled`, `mcp_port` (default 8737), `mcp_token`, `mcp_actions_enabled`, `mcp_auto_allow` (action name → runs without asking) and `mcp_instructions` — all set from the SETTINGS tab's **ASSISTANT (MCP)** section, and read by the `freebodyproblem-mcp` adapter for the port and token

Example `settings.json`:
```json
{
  "active_theme": "Tactical",
  "tile_provider": 3,
  "tile_url": "https://tiles.example.org/{z}/{x}/{y}.png?key={key}",
  "tile_key_custom": "your-key",
  "tile_attribution": "© Example Maps",
  "video_url": "udp://0.0.0.0:5600",
  "themes": {
    "Custom": {
      "bg": [0.05, 0.05, 0.08],
      "accent": [0.2, 0.8, 0.9],
      ...
    }
  }
}
```

## Performance Notes

- **Parallel per-vehicle work**: framing is cheap and stays on the link thread; the message dispatch, parameter table, mission state and protocol timers run per vehicle and overlap, so a slow conversation with one aircraft does not hold up another
- **One message copy at the hand-off**: a framed message is copied once into the owning vehicle's inbox. The inbox is bounded, and a vehicle that falls far enough behind to drop from it says so in the log rather than losing telemetry silently
- **Rate-capped snapshots**: each vehicle publishes its state at ~20 Hz rather than on every datagram, and the UI copies only the vehicle on screen
- **Telemetry logging off the hot path**: recording a frame is a clock read and a copy into memory, some tens of nanoseconds; one background thread writes every vehicle's log once a second, so a slow disk costs log records, never telemetry
- **Threaded tile fetching**: Map tiles load asynchronously without blocking the render loop
- **Exponential moving average rates**: Message rate display decays gracefully when telemetry stops
- **Parameter retransmit logic**: Automatically re-requests dropped PARAM_VALUE packets after 2s stall
