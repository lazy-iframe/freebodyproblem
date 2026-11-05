---
title: Limitations & plans
description: What is not there yet, and what is coming.
---

- **No firmware upload**: Use ArduPilot's `uploader.py` or Mission Planner for bootloader operations
- **No log download**: Access DataFlash logs via MAVLink file transfer in other tools
- **OS support**: Linux, Windows and macOS (Apple Silicon) are built and packaged by CI. Intel Macs have no prebuilt package, and the macOS app is not signed with a Developer ID, so its first launch has to be allowed by hand
- **No telemetry replay**: Live connections only; no `.tlog` or `.bin` file playback
- **No rally points**: the vehicle's rally list (`MAV_MISSION_TYPE_RALLY`) is neither downloaded nor edited
- **One vehicle in the tabs at a time**: the map draws the whole fleet and SWARM gives every vehicle a summary card, but the tabs — parameters, mission, radio, sensors, the inspector — and the telemetry tiles follow the selected vehicle only
- **Joystick**: flown only against ArduPilot SITL so far; PX4 and real aircraft are untested. Two identical sticks share one saved setup, and on Linux a switch already on shows as off until moved while both are connected
- **One vehicle per link per sysid**: an aircraft reachable over two links at once appears as two entries rather than being recognised as one. Merging them wants a real identity to key on (the board UID) and is not done yet
- **Display scaling on Windows and X11**: the UI is drawn in physical pixels there, so at 150% or 200% desktop scaling it comes out smaller than intended. Wayland and macOS hand the app scaled sizes and are not affected
- **Plugin rail on short screens**: below about 900 px of height the rail's ten buttons do not fit and the rail scrolls

## Future Plans

- Implementations of "Console" and "ESC" tabs, for the MAVLink console and for ESC configuration with motor test.
- Multi-vehicle UI: more of each aircraft's telemetry on the SWARM cards, so it can be watched without switching between them
- PX4: mission, parameters, RC configuration and sensor calibration (telemetry and modes are done)
- Video AI features

## Contributing

This is a personal project built for speed and clarity over feature completeness (only the features needed **during** the flight should be implemented). PRs welcome for:
- Bug fixes
- Performance improvements
- Additional MAVLink message handlers
- Telemetry graph overlays
- Other features which are essential during a flight

Please keep contributions focused on the core philosophy: fast, direct, minimal abstraction.
