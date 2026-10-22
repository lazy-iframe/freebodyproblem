---
title: Quick start
description: Connect to a vehicle and find your way around.
---

1. Launch `freebodyproblem`
2. Click any key to dismiss splash screen
3. Select connection type in left sidebar (Serial/TCP/UDP)
4. Configure transport parameters:
   - **Serial**: select port from dropdown, set baud rate
   - **TCP**: enter IP and port (e.g., `192.168.1.100:5760`)
   - **UDP**: bind address and port (default `0.0.0.0:14550`)
5. Click **CONNECT**
6. Telemetry will auto-configure and display within seconds

Connecting again does not replace the link you already have — it adds another.
Open as many as you need; each appears in the **LINKS** list below the connect
form with its status, how many vehicles have been heard on it, and a button to
drop that one link.

## Switching Vehicles
Every vehicle discovered on every link shows up in the switcher. Click the
**callsign chip** in the topbar — the one reading `(1) SYS1·1` — and pick from
the list; it names each vehicle's sysid and the link it arrived on, so two
aircraft that both report sysid 1 can still be told apart. The chip only grows
a caret when there is more than one vehicle, so nothing changes when flying one.

The `(1)` is the GCS's own number for that vehicle, handed out on discovery,
lowest free number first. A sysid is what an aircraft calls itself and is not
usable as a name — two airframes fresh off the bench both answer to 1 — so the
number is what the chip, the switcher rows and the symbols on the map all agree
on. Numbers are freed when a vehicle goes, so a fleet of three reads 1, 2, 3
rather than climbing all session.

Everything follows the selection: telemetry, parameters, mission, radio,
sensors, the inspector and the map. Panel state does not — a calibration or a
mission you were editing stays with the aircraft it belongs to and is waiting
when you switch back. Calibrations keep running on vehicles you are not looking
at, and only the selected vehicle makes sound.
