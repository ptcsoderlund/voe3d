# 08 — Drags snap, and Ctrl flips them
folder: editor
after: 07
decisions: 0168, 0482, 0438
read: feature.md

## Change
0482 points 4 and 6: a move, turn or scale drag rounds what it submits while snapping is on
XOR Ctrl is held, read every frame of the drag. The Inspector is not touched.

- `editor/src/gizmo.h` — `voe_editor_gizmo_read` gains `bool flip`, Ctrl's level this frame.
  Header paragraph SNAP: whether a frame snaps is `scene->snap.on != flip`; what is rounded is
  the world value the drag writes, by `3d/gizmo_snap.h` (0482 point 4): the position on the
  world grid along the held handle's axes, the swept angle in whole steps from the press, the
  scale on multiples; a typed Inspector value never comes here.
- `editor/src/gizmo.c` — in the drag, before the transform is submitted:
  `voe_3d_gizmo_snap_position(position, held, move)`, the swept angle through
  `voe_3d_gizmo_snap_angle` with the step in radians, the new world scale through
  `voe_3d_gizmo_snap_scale`. Unchanged values still submit nothing and count nothing.
- `editor/src/frame_pointer.c` — passes `input->control` as `flip`.
- `editor/src/frame_pointer.h` — step 6 of the order says the gizmo reads Ctrl as snap's flip.
- `editor/src/src.md` — the `gizmo` entries name snapping.

## Done when
The editor builds, and `grep -n "voe_3d_gizmo_snap_angle" editor/src/gizmo.c` prints a line.
Then the human, in the editor, walks `## How to test` in `feature.md`, steps 1 to 7.
