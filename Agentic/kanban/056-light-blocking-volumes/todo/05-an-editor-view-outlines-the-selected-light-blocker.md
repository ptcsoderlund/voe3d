# 05 — An editor view outlines the selected light blocker
folder: 3d
after: 04
decisions: 0168, 0347

## Change
The selected blocker drawn as a box's lines, as the selected collider is (0347 point 5). Read
`3d/include/3d/collider_marker.h`, `3d/include/3d/light_blocker.h`, `3d/src/draw_marks.h`,
`3d/src/draw_marks.c`'s collider lines, `voe_3d_collider_marked` and `voe_3d_frame` in
`3d/include/3d/draw_system.h`, and the frame zeroing in `3d/src/draw_system.c`.

- `3d/include/3d/draw_system.h`: `voe_3d_frame` gains `voe_3d_collider_marked light_blocker` after
  `collider`. Comment points: only an editor's view sets it; the entity's blocker box from
  `voe_3d_light_blocker_shape` at the frame's lag, as the collider's lines are, behind the outline's
  clear in its colour; nothing for none or an entity with no blocker; one more range and object,
  sized from VOE_3D_COLLIDER_MARKER_VERTICES and _INDICES; a pool too small draws nothing. `_frame`
  zeroes it and its list names it; `_run`'s comment names it.
- `3d/src/draw_marks.h`, `3d/src/draw_marks.c`: `voe_3d_draw_marks_light_blocker`, the collider
  lines' call with the shape from `voe_3d_light_blocker_shape`; or one shared static helper both
  calls use, so the line drawing is written once.
- `3d/src/draw_system.c`: `_run` calls it right after the collider's lines; the zeroing in `_frame`.
- `3d/tests/light_blockers.c`: a case where the shape's box through `voe_3d_collider_marker_quads`
  from an off-axis eye gives 12 edges' quads (48 vertices); and a GPU case drawing a frame with
  `light_blocker` naming a blocker over an empty view, which reads the outline colour on a box edge's
  pixel, and with it zeroed reads none.
- `3d/3d.md`, `3d/src/src.md`, `3d/tests/tests.md`: the draw_system.h, draw_marks and test entries
  name the blocker's lines. Each at most 300 characters.

## Done when
The tests `3d/light_blockers`, `3d/collider_marker` and `3d/draw_system` pass after the folder's
build.
