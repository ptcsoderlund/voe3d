# 13 — A selection turns by its rings
folder: editor
decisions: 0168, 0274, 0205, 0204, 0250

## Change
Needs cards 06 and 12. The mode is switched by card 14; here it is the flag and what it does.

- `editor/src/scene.h`, `editor/src/scene.c`: `voe_editor_scene` gains `bool rings` (false is
  move, zeroed is move) and `void voe_editor_scene_gizmo_switch(voe_editor_scene *)` that flips
  it. The header says the mode is the person's, not the project's: never saved, never undone.
- `editor/src/gizmo.h`, `editor/src/gizmo.c`: when `scene->rings` is set, hover is
  `voe_3d_gizmo_rings_hit`, a press on a ring takes `voe_3d_gizmo_rings_angle` as the grab
  angle and keeps the entity's rotation then; each frame of the drag the angle now against the
  grab angle turns that kept rotation about the ring's world axis
  (`voe_math_quat_from_axis_angle`, pre-multiplied, normalized), and the row read as it is with
  its rotation replaced is submitted as a whole transform; a refused angle keeps the last
  submitted. `moved` counts a rotation that changed, so a turn marks unsaved and is one undo
  step (0204). A switch of mode while a handle is held ends the drag. The struct gains the
  grab angle and the kept rotation. Rewrite the header's opening and the "measured from the
  press" paragraph to cover a turn as well as a move.
- `editor/src/view_passes.c`: the pass's `gizmo.rings` is `scene->rings`.
- `editor/src/src.md`: the `gizmo`, `scene` lines.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_editor` exits 0, and
`build/debug/editor/voe_editor examples/coin_game --capture <scratch>/rings.png` exits 0 with a
PNG written. Turning by hand is the walk-through's (card 15).
