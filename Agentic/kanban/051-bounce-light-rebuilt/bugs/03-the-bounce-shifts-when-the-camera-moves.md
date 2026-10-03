# 03 — The bounce shifts when the camera moves

## Seen
"I took two screenshots. I only moved the camera a bit to the left. Notice how the darkness shifted on the green
tank and the pillar. This is in editor. Something is screen spaced, if not maybe it is being not drawn or
something when out of screen? I am only guessing."

`03-before.png` and `03-after.png` beside this file, taken about half a minute apart in the editor's scene view
of `examples/tank_game`. Nothing in the scene moved between them, only the camera. In `03-before.png` the green
tank's faces and the shaded sides of the red and tan pillars are nearly black. In `03-after.png` the same faces
are lit, much brighter, and the tank reads clearly green.

Asked whether it catches up when the camera stands still: "It stays dark even when camera is still. [...] No
wait, i see now. Its the distance. Things in the distance gets darker. I dont want that." So it is not a slow
refresh. Things far from the camera get no bounce, and moving the camera moves where that cut-off falls. 0329
expected this: its grid reaches about 24 m from the eye, and it named the edge showing in the tank game as a
later fix.

On cost: "The lights and shadows calculations are a bit slow. So if they have to recalculate everytime camera
moves we will be in trouble. It will never be done if thats the case. It seemed snappier before we prepared for
more lightbounces. Even when only doing one bounce."

Bug 02's check by hand passed: with several scene views open, while Satisfactory ran on another desktop, "editor
is smooth as butter".

This comes after bug 01 was fixed (0329 placed the probe volume by the eye alone). Turning the camera may no
longer change the bounce, but moving it still visibly does.

## Expected
Decided: 0331. The bounce grid covers the level, not the camera.

The bounce belongs to the world, as in bug 01 and 0328. Moving the camera, as well as turning it, leaves the
light on things that did not move unchanged. A world cache that follows the camera is allowed only if nobody can
see its edges or its refreshes (0328). A face does not go dark or light up because the eye came closer, moved
aside, or because something went off the screen. Things far away get the same bounce as things near: distance
from the camera never darkens anything. Only a light or a thing that moves changes the bounce, and only
around itself (051 point 8).

## How to reproduce
1. Open `examples/tank_game` in the editor with the sun at Bounces 1 or more.
2. In a scene view, look at the green tank and the two pillars beside it, with the tall tan block close to the
   camera on the right (as in `03-before.png`).
3. Move the camera a little to the left and back (as in `03-after.png`), without turning or touching anything
   else.
4. The shaded sides of the tank and the pillars change brightness with the camera. They should look the same
   from both places, and still the same after waiting a few seconds at each.
