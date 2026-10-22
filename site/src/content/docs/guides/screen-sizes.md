---
title: Screen sizes
description: How the layout adapts from portrait to 32:9.
---

The layout is worked out from the screen and the font each frame, not from
fixed fractions of the window, so a laptop, a 4K screen and an ultrawide each
get a layout that uses their space rather than a stretched copy of one.

**Sidebars are as wide as their text.** The UI font is monospaced, so a column
is measured in characters: each sidebar has a minimum — its widest row that
cannot shrink, such as a parameter ID beside its value and WRITE — and a
maximum, the width it has on a 1080p screen. Past the maximum they stop
growing, and every extra pixel goes to the centre.

**MAP + VIDEO picks its arrangement from the space left.** While the centre is
no more than about one and a half times as wide as it is tall, the feed sits
above the map and the two get equal areas. Wider than that — an ultrawide, or
a wide screen with a short window — they go side by side and split the centre
in half: map on the left beside the mission tabs, video on the right beside
the plugin rail. The feed is always 16:9, so whatever its half does not fill
is letterbox; the map is never cut down to fit the feed.

| Screen          | MAP + VIDEO                         |
|-----------------|-------------------------------------|
| 1920×1080       | stacked                             |
| 2560×1440, 4K   | stacked, the centre taking the gain |
| 2560×1080       | side by side                        |
| 3440×1440       | side by side                        |
| 3840×1080 32:9  | side by side                        |

**Narrow screens get the fullscreens only.** When there is no room for both
sidebars at their minimum beside a usable centre — below about 1400 px wide,
so 1366×768 laptops and portrait screens — the right sidebar and the plugin
rail go and the mode buttons become **VIDEO FULL**, **MAP FULL** and **SWARM**.
MAP FULL keeps the left sidebar and brings attitude and the event log back as
the overlay in the map's corner; there is no windowed view to exit to, so ESC
does nothing. Widen the window and the view you had comes back.

**The topbar drops readouts rather than cutting them.** When the row does not
fit, whole readouts go in this order: RX, POSITION, HDG, GNSS, SPEED, ALT AGL,
then DATALINK; PWR and MODE stay longest. What is on the map or in a tab goes
first; what you fly on stays.

**SWARM can show the feed too.** A **VIDEO** toggle at the top-right of the
swarm map splits it into map and video by the same rules. It is offered only
while the map keeps a usable size beside it: on a 1080p screen that is up to
two columns of vehicle cards, on wider screens all of them. With more cards
than that the toggle disappears and the map has the space to itself; your
choice is kept for when the column narrows again.

On Wayland, sizes are in the desktop's scaled units, so a 4K screen at 200%
gets the 1080p layout at twice the sharpness. Windows and X11 do not
apply display scaling yet — see [Limitations](https://github.com/lazy-iframe/freebodyproblem#limitations).
