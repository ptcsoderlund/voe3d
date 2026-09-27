# 0274 — The sun is a marker, and a selection turns by rings
date: 2026-09-27
by: planner

## Decision
For 035:

1. **The sun marker is `3d`'s**, beside the camera marker: a circle of radius 0.25 m in the
   sun's own XY plane and an arrow 1 m along its −Z with a head, as line quads of a fixed pixel
   width under its position and rotation (scale ignored), in the world layer, unlit. It picks
   by a cube of half extent 0.25 m about its position, walked by `voe_3d_pick` beside the
   camera boxes. Colour as the camera marker (0223): the outline's when selected, the gizmo's
   rest colour otherwise. Only the editor's views ask for it.
2. **The rotate gizmo is `3d`'s arithmetic**, as the move gizmo is (0205): three rings about
   the world X, Y and Z through the entity's position, radius one shaft, 48 segments, drawn
   as camera-facing quads in the gizmo's two draws; the handles are X, Y and Z of
   `voe_3d_gizmo_handle`. A ray meets a ring where it crosses the ring's plane within the
   grip of its radius, nearest first; a grab answers the angle about the axis where the ray
   crosses that plane, refused when the ray lies in it.
3. **The editor turns by the angle swept since the press**: the rotation at the press,
   pre-multiplied by the turn about the world axis, submitted as a whole transform.
4. **One key, R, switches the gizmo between move and rotate** for whatever is selected; not
   while typing or flying, as the other shortcuts. The top bar shows `Move` or `Rotate` as a
   label after Preferences. The mode is the editor's, not saved, starts at move.

## Reasoning
World-axis rings match the move gizmo's world axes (0205); local axes are the later toggle
both headers already name. A toggle key and not two keys is what the feature asks for; R is
free and names rotate. A label rather than a button keeps the bar's one row of commands.
Picking the sun by a cube, not its lines, is how the camera marker picks (0223).

## Replaces
nothing. Extends 0205 and 0223.
