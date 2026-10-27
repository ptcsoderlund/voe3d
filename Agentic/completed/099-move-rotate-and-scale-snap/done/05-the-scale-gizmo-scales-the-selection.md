# 05 — The scale gizmo scales the selection
folder: editor
after: 04
decisions: 0168, 0482, 0205, 0274, 0204

## Change
0482 point 2 in the editor: in Scale mode the pointer hovers and grabs the scale gizmo's handles,
and a drag submits the entity's new scale as a whole transform, the way a move or a turn is.
Undo needs nothing: a drag counted in `moved` is one step (0204).

- `editor/src/gizmo.h` — `voe_editor_gizmo` gains `voe_math_float3 scale_start`, the entity's
  world scale at the press (`kept` already holds its world rotation then). Header points: the
  scale gizmo stands on the entity's own axes (0482 point 1); a scale drag is the world scale at
  the press times `voe_3d_gizmo_scales_factor` of the grab and now, on the held axis or all
  three for the centre, written back relative to the parent as a move is; `moved` counts a scale
  that changed.
- `editor/src/gizmo.c` — with `gizmo_kind` SCALES:
  - hover: `voe_3d_gizmo_scales_hit` with the entity's world rotation as `turn`;
  - press: `voe_3d_gizmo_scales_grab` into `grab`, `scale_start` and `kept` kept;
  - drag: the gizmo at `start` with `kept` as `turn`, grab now; a refused grab keeps the scale
    it had; the world transform's scale replaced and submitted through the path moves and turns
    already take.
  `voe_editor_gizmo_marked` answers the held or hovered scale handle as for the others.
- `editor/src/src.md` — the `gizmo.h` and `gizmo.c` entries name the scale drag.

## Done when
The editor builds, and `grep -n "voe_3d_gizmo_scales_factor" editor/src/gizmo.c` prints a line.
