---
title: Swarm view
description: Every vehicle at once, and commands to several of them.
---

**SWARM**, at the right end of the centre view's mode buttons, gives the whole
window below the topbar to the fleet. It is the one place a command goes to
several vehicles at once. Press it again — it reads **EXIT SWARM** — to go back
to the view you came from.

There is no cap on how many vehicles the fleet holds. The ceiling is MAVLink's
254 system IDs per link and, in practice, the machine's CPU: fifty SITL copters
on one link run without trouble.

  ![SWARM view: vehicle cards on the left, FLIGHT, telemetry and event log across the top, map and video below](../../../../../screens/swarm.png)

- **A card per vehicle**, down the left: its number and sysid, armed state,
  attitude, MODE, BATT, GPS, ALT and GS, and the EKF variance bars beneath.
  The column is as wide as the fleet needs — one column for a few aircraft,
  more as the fleet grows, up to half the screen, after which it scrolls
- **Checking and selecting are separate.** The checkbox in a card's corner
  decides whether a command reaches that vehicle; **ALL**, **NONE** and
  **INVERT** above the cards act on the checkboxes only. A click on the card
  body selects the vehicle instead — green frame, and the topbar, telemetry
  tiles and GO HERE follow it — so looking at an aircraft never adds it to a
  batch command. A double click opens the card large, with ASL, airspeed,
  throttle, flight stack and IDs added; a click on it, or **ESC**, puts it back
- **FLIGHT acts on the checked vehicles.** Its header lists who it reaches
  (`FLIGHT → #1 #2 #3`). **TAKEOFF** and the mode grid send to all of them; a
  mode is lit when every checked vehicle is already in it, and a mixed
  ArduPilot/PX4 set gets one grid per stack. Each card shows its own
  vehicle's answer to the last command, so the one aircraft that refused is
  easy to find
- **The topbar's arming buttons act on the checked vehicles too**: **ILK
  HIGH** / **ILK LOW** for motor interlock (PX4 vehicles in the set are skipped
  and logged) and **ARM** / **DISARM**, which name every vehicle they will
  reach and ask before sending
- **Batch commands are retried.** A command sent to more than one vehicle at
  once goes out again to each vehicle that has not answered within a second,
  up to three tries in all; a vehicle still silent after the third is named in
  the event log (`sys7: no ACK for command 176 after 3 tries`). This is the only
  thing the GCS ever resends on its own: a command to a single vehicle goes
  out once
- **Telemetry and event log** across the top: the telemetry tiles of the
  selected vehicle, and the whole fleet's event log
- **Map, and video beside it.** The map fills the space below, with checked
  vehicles ringed. The **VIDEO** toggle at its top-right corner splits that
  space into map and feed, while there is room for both — see
  [Screen Sizes](../screen-sizes/)
