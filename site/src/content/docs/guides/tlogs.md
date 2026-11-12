---
title: Telemetry logs
description: Every message to and from each vehicle, recorded to a .tlog.
---

Every vehicle gets a `.tlog` from the moment it is discovered until its link is
disconnected: each MAVLink frame it sent and each frame sent to it, exactly as it
crossed the link, stamped with the time it did. It is the format Mission Planner,
MAVProxy, QGroundControl and pymavlink (`mavlogdump.py`, MAVExplorer) read.

- One file per vehicle per connection, named for when it started and who it is:
  `2026-11-12_16-05-33_v1_sys1.tlog` is vehicle (1), system ID 1. A vehicle you
  have named carries the name too, `…_v1_sys1_alpha.tlog`, and a log being
  written is renamed when its vehicle is
- Written once a second, so a crash or a pulled plug loses at most the last second
- Turn it off or on in **SETTINGS → FILES → Telemetry logs**; it takes effect at once
- Nothing is ever deleted. A telemetry radio fills about 20 MB an hour and a
  fast UDP link several times that, so clear the folder out now and then

Logs go in the `tlogs` folder of the save folder, which **SETTINGS → FILES →
CHANGE** moves to anywhere you pick; **DEFAULT** puts it back. Video snapshots
go in `snapshots` beside it, and the parameter file browser starts there. A log
already being written stays where it is; the next one goes to the new folder.

| Platform | Default save folder |
|----------|--------|
| Linux    | `$XDG_DATA_HOME/freebodyproblem`, or `~/.local/share/freebodyproblem` |
| macOS    | `~/Documents/freebodyproblem` |
| Windows  | `Documents\freebodyproblem` |
