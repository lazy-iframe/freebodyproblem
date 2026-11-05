---
title: The map
description: The moving map, the fleet on it, GO HERE, and the map in 3D.
---

The map draws every vehicle that has a position, not only the one the panels
are following. Each aircraft carries a label above it — `(2) SYS 1`, the GCS's
number then the sysid it reports — and under that its height above the terrain
data, red and saying **BELOW TERRAIN** when it is inside it. The selected
aircraft keeps full colour and a heading line run out to the edge of the panel,
while the rest are dimmed with a short stub so three aircraft do not lay three
dotted lines across the imagery.

![MAP: five vehicles on the map, the selected one with its heading line and the parameter fence ring](../../../../../screens/cap3.png)

Behind each aircraft is its **trail**, recorded off the live stream rather than
off what is on screen: an aircraft that flew while you were watching another one
still has its track when you switch back. Points are thinned to one every 3 m,
so a vehicle standing still does not smudge, and capped at 6000 — about 18 km of
ground track. Each vehicle's **home**, from `HOME_POSITION`, is drawn as a
helipad: an H inside its circle, in green so it reads as neither the aircraft
nor an instruction to it.

**CENTER** — the map follows the selected aircraft until you drag it. The
**CENTER** button in the map's top-left corner, lit while following, puts the
map back on the aircraft and keeps it there. It follows whichever vehicle is
selected, so after switching vehicles it centres on the new one.

**GO HERE** — click anywhere on the map for a guided position target; a drag
still pans. In [3D](#3d-view) the click starts a pick of the point's place and
then its height. The small menu at the pointer shows the coordinates under the
cursor (click them to copy, as signed decimal degrees) and an altitude, which
starts at the height the vehicle is already holding, and **GO** sends it as
`SET_POSITION_TARGET_GLOBAL_INT`. It only sends while the vehicle is in GUIDED,
which is the only mode that acts on one; out of GUIDED the button is disabled
and says why, rather than sending into silence. The menu also checks the
straight line from the vehicle to the point against the terrain. The target
stays drawn as a ringed crosshair joined to the aircraft by a line until
**Clear target** in the map's right-click menu, which also carries **Clear
track** for the trail.

**MAP FULL** — with the map alone in the centre view, the map button relabels
itself. Press it and the map takes the right sidebar's width and the plugin
rail's column; the left sidebar stays, because the tabs are how a mission is
planned and the map is what it is planned on. Attitude and the event log come
back as a semi-transparent block in the map's top-right corner, small enough to
read the map past. Click either half and it grows to the size it had in the
sidebar, text and all, until you click somewhere else; it always comes back
small the next time the map goes fullscreen. **ESC** or **EXIT FULL** restores.

![MAP FULL: the map takes the right sidebar, with attitude and the event log in its corner](../../../../../screens/cap2.png)

## 3D view

**3D**, under **CENTER** in the map's top-left corner, stands the map on the
terrain. Drag to orbit, right-drag to pan, wheel to zoom; the camera keeps
above the ground between it and what it looks at, so a valley never turns the
view into rock. The button under **3D** — **×1.5** to begin with — steps the
vertical exaggeration through 1, 1.5 and 2, and **TEX** swaps the imagery for
the bare elevation mesh. The view needs OpenGL 3.3; where the driver lacks it,
the button says so and stays off.

![The map in 3D: a quadcopter in GUIDED over terrain, its body axes and pink line to the ground, labelled with its name and height above the terrain, and a GO HERE target with its own line to the ground](../../../../../screens/map_3d.png)

- **Aircraft** are drawn as models of their type — multicopter, plane or
  helicopter — at their reported altitude and attitude, white with the livery
  in red and navigation lights red to port, green to starboard. Propellers and
  rotors turn while the vehicle is armed. Each carries its body axes (X forward
  in amber, Y right in green, Z down in blue), a pink line down to the ground
  under it, and above it its name and height above the terrain. Any part of an
  aircraft inside the terrain shows through the ground as an amber cage, and
  its label says **BELOW TERRAIN**.
- **The mission** is drawn as its path with a translucent blue band hanging
  from it to the ground, so how high each leg runs over the terrain — and where
  the ground rises to meet it — reads from any angle. Legs too close to the
  terrain are marked as in the mission tab.
- **The geofence** stands as walls: green for inclusion zones and the
  parameter cylinder, red for exclusion zones, up to `FENCE_ALT_MAX` above home
  where a ceiling is set. A fence that is not enabled is not drawn.
- **Placing a point** takes two clicks: the first on the ground fixes where,
  then the point rides its vertical line with the pointer and the second click
  sets its height. A mission item's **LOC?** works this way in 3D, and so does
  **GO HERE**, which then opens its menu with that height filled in. Fence
  drawing and editing are flat-map work and leave 3D.
- **EDIT** works in 3D. Drag a waypoint's marker to move it over the ground at
  the same height, or drag its vertical line up and down to change only its
  height. A drag anywhere else still orbits.

The terrain comes from AWS Terrain Tiles (SRTM, ±25 m) unless another
`{z}/{x}/{y}` PNG source is set in **Settings → TERRAIN**, which also sets the
clearance a planned path must keep from it.
