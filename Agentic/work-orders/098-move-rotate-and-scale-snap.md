# 098 — Move, rotate and scale snap

## What
Decision 0438. "I would like to have some snap movement when moving or rotating stuff", and scaling too. The top
bar has a Snap toggle and three steps: move 0.5 m, rotate 15°, scale 0.1, each one I can change. With Snap on,
dragging the move gizmo puts the thing on whole steps of the world grid, the rotate rings turn it in whole
steps, and the new scale gizmo puts its scale on whole steps. Holding Ctrl while I drag does the opposite for
that drag. There is a scale gizmo now: R goes Move → Rotate → Scale, an axis handle scales along that axis, the
centre handle scales evenly. Snap and the steps are remembered for me after a restart. Typing a value in the
Inspector is never snapped.

## Why
Lining things up by eye is slow and never exact; snapping makes walls meet and things stand level.

## How to test
1. Select a box. Press R until the top bar says Scale: a scale gizmo is on the box. Drag one axis: the box
   stretches along that axis only. Drag the centre: it grows evenly. Ctrl+Z undoes each drag.
2. Turn Snap on in the top bar. Move the box along X: its X in the Inspector goes 0.5, 1.0, 1.5… and nothing in
   between, even if it started at 0.37. Y and Z are left as they were.
3. Rotate it: it turns in 15° steps. Scale it: the scale goes in 0.1 steps.
4. Change the move step to 1 and rotate to 45: moving goes in whole metres, turning in 45° steps.
5. Hold Ctrl while dragging: no snapping during that drag. Turn Snap off and hold Ctrl: that drag snaps.
6. Type 0.37 into X in the Inspector: it stays 0.37.
7. Close and reopen the editor: Snap and my steps are as I left them. Open another project: the same.
