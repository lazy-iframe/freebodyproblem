---
title: Mission & geofence
description: Plan missions and geofences on the map.
---

1. Navigate to **MISSION** tab in left sidebar
2. Click **FETCH** to download current mission from vehicle
3. Add items from the command grid. A command with a position (WAYPOINT,
   TAKEOFF, LAND, LOITER, ROI, ...) arms the map: its button reads **LOC?**
   until you click the map, and the item is added there. Press it again or
   **Esc** to cancel. A command without one (RTL, SPEED, DELAY, JUMP, ...) is
   added at once. **MORE...** holds the rest of the commands
4. Press **EDIT** (the full-width button above the grid) and drag any waypoint
   on the map to move it; press it again or **Esc** to go back to panning. In 3D,
   drag its marker to move it or its vertical line to change its height
5. Click an item in the list to expand or collapse it and adjust its
   altitude, parameters and command type. A newly added item opens expanded
   and the others collapse
6. Click **UPLOAD MISSION** to send the mission to the vehicle
7. Click **CLEAR** to empty the editor

New items take the altitude of the last item that has one, or 20 m. On
`GLOBAL_RELATIVE` an altitude can be negative, for a point below home. Leaving
the MISSION tab cancels a pending **LOC?** and turns **EDIT** off. With the map
in [3D](../map/#3d-view), **LOC?** takes two clicks: the ground for the place,
then the height along its vertical line.

**EDIT** works in [3D](../map/#3d-view) too, and the map stays in 3D:

- **Drag a waypoint's marker** to move it over the ground. It keeps its altitude, and the
  height above ground in the readout changes as the terrain does. The marker stays under
  the pointer from any camera angle.
- **Drag its vertical pink line** up or down to change only its altitude. It can go below the terrain data, which the data is not
  accurate enough to rule out: the readout and banner warn in red, and the part inside is drawn
  dotted. Its position does not change. If you are looking straight down,
  the banner asks you to tilt the view.

The readout beside the waypoint shows its altitude above home and above the ground, and the
list, the path and the terrain checks follow the drag. A waypoint keeps its frame: on
`GLOBAL_RELATIVE` the altitude is above home, on a terrain frame above the ground, so a
marker dragged on a terrain frame rises and falls with the ground. A drag anywhere else
still orbits, and a click beside a waypoint in EDIT does not start a **GO HERE**.

Each item's header shows its height above the terrain data under it, and each
leg is checked against the terrain along its length: one that comes within the
clearance set in **Settings → TERRAIN** (30 m by default) is marked amber, one
that goes into the ground red.

:::caution[A planning aid, not a guarantee]
Terrain heights are approximate (about ±25 m) and leave out buildings, trees, wires and
anything else on the ground, so a leg marked as clear can still hit something. The source
data is provided as is, and part of it (ETOPO1) is marked not to be used for navigation.
Check the route yourself.
:::

## Geofence
The parameter fence needs no editing here: set `FENCE_ENABLE`, `FENCE_TYPE`,
`FENCE_RADIUS` and `FENCE_ALT_MAX` in **PARAMS** (PX4: `GF_ACTION`,
`GF_MAX_HOR_DIST`, `GF_MAX_VER_DIST`) and the ring around home follows as the
vehicle echoes each one back. The **GEOFENCE** view of the MISSION tab
summarises it.

Uploaded zones are edited in that view:

1. Click **FETCH** to download the vehicle's current fence. Fetch before
   editing to keep its existing zones: an upload replaces the whole list
2. **POLYGON**: click the map for each vertex, then click the first vertex,
   press **Enter** or press the button (now **CLOSE**) to finish. **CIRCLE**:
   click the centre, then click again to set the radius. **Esc** cancels. A
   new zone is an inclusion zone (the vehicle must stay inside)
3. Click a zone in the list to switch it to **EXCLUSION** (the vehicle must
   stay out) or back, set a circle's radius, or type vertex coordinates.
   **EDIT** lets you drag polygon vertices and circle centres on the map
4. Click **UPLOAD FENCE**. **CLEAR** followed by **UPLOAD EMPTY FENCE** removes
   the vehicle's fence

On ArduPilot, uploaded zones are only enforced with `FENCE_ENABLE` set and the
Polygon bit (4) in `FENCE_TYPE`; the view warns when either is missing. A
Plane's fence return point (`MAV_CMD_NAV_FENCE_RETURN_POINT`) is kept through
a fetch and upload but not edited.
