# 06 — A pass draws the sun marker and the rings
folder: 3d
decisions: 0168, 0274, 0205, 0223

## Change
Needs cards 04 and 05. Additive to `voe_3d_frame`: a zeroed field draws nothing new, so no
caller breaks.

- `3d/include/3d/draw_system.h`:
  - New `voe_3d_sun_marked`, fields as `voe_3d_camera_marked` (entity, material, colour,
    pixels, size), and `voe_3d_frame` gains `voe_3d_sun_marked sun` beside `marker`: drawn in
    the world layer inside the world's depth, as the camera marker; a zeroed, dead or
    light-less or transform-less entity draws nothing; sized from `VOE_3D_SUN_MARKER_*`, one
    more range and one more object.
  - `voe_3d_gizmoed` gains `bool rings`: false draws the move gizmo as now, true draws
    `voe_3d_gizmo_rings_quads` in the same two draws, at the same place in the order;
    `marked` names a ring then. Say the capacity is the larger of the two gizmos' counts.
  - `voe_3d_draw_system_frame`'s paragraph lists `sun` among the fields that come back zeroed;
    `_run`'s paragraph says when the sun marker is drawn.
- `3d/src/draw_marks.h`, `3d/src/draw_marks.c`: draw the sun marker beside the camera marker
  through `voe_3d_sun_marker_quads`; the gizmo branch picks arrows or rings by `rings`.
- `3d/src/draw_system.c`: only if `_frame` or `_run` must name the new field.
- `3d/tests/draw_system.c`: a frame marking a sun is one draw more than without; a ring gizmo
  on a cube shows through it, as the move gizmo's case does. Update its line in
  `3d/tests/tests.md`; `3d/src/src.md`'s `draw_marks.c` line names the sun and the rings.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_3d $(ninja -C build/debug -t
targets all | grep -oE "^voe_test_3d_[A-Za-z0-9_]+") && ctest --test-dir build/debug -R "^3d/"`
exits 0.
