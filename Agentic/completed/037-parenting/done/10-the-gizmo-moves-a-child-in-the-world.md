# 10 — The gizmo moves and turns a child in the world
folder: editor
decisions: 0168, 0281, 0205, 0274

## Change
Needs card 06. `3d` already stands the gizmo at the world place (card 04); the drag must work in
the same space and write back the row, which is relative to the parent.

- `editor/src/gizmo.c`, `editor/src/gizmo.h`:
  - Everything the drag measures from — the hover, the grab's `start` and `kept`, and the
    "did it change" comparisons — uses `voe_scene_transform_world(world, selected)` in place of
    the row (keep `voe_scene_transform_get` only as the "has a transform" test).
  - The moved or turned world transform is turned into the row with
    `voe_scene_transform_local(world, entity, placed)` just before it is submitted.
  - Header: the "A WHOLE TRANSFORM IS SUBMITTED" point says the drag is in world space and what
    is submitted is the row relative to the parent (0271), so a turned turret carries its barrel.
- `editor/src/view.c`: `voe_editor_views_focus_camera` focuses on the camera's world position.
- `editor/src/src.md`: the `gizmo.c` entry says a child's move is written back relative to its
  parent.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_editor` exits 0. The human's:
with a turret under a hull, the hull's arrows move both, the turret's rings turn the turret (and
a barrel under it) and leave the hull still, and the turret's arrows keep following the pointer.
