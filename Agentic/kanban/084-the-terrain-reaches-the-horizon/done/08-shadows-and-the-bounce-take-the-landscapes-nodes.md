# 08 — Shadows and the bounce take the landscape's nodes
folder: 3d
after: 07
decisions: 0168, 0389, 0396

## Change
0396 point 4: every pass of a frame draws the same nodes as the view.

- `3d/src/draw_shadows.c`: `draw_model_casters` hands a landscape row to draw_terrain (card 07)
  with the frame's eye, so each cascade and the point lights' pass draw its nodes; a landscape
  casts by its part's material as any part does. Header: one sentence that the ground casts by
  nodes.
- `3d/src/draw_bounce.c`: the casters drawn into captures and sun maps take the landscape the
  same way; the boxes it grows (lines ~174, ~242, ~339) take a landscape row's box from
  `voe_3d_landscape_lod_box` and the row's place, not from the grid geometry's unit cube.
- `3d/src/draw_terrain.h`/`.c`: only if a shadow pass needs a variant (no fade, no span).
- `3d/src/bounds.c`: `grow_landscape` uses `voe_3d_landscape_lod_box` through card 06's lookup
  instead of walking every height.
- New headless test `3d/tests/terrain_shadow.c` (entry in `3d/tests/tests.md`): a 512 m,
  256-cell landscape, flat but a 40 m ridge across its middle, a low sun behind the ridge: ground
  just behind the ridge is darker than ground in front of it in the same frame.

## Done when
`3d/tests/terrain_shadow.c` passes, and `3d/tests/shadows.c`, `3d/tests/bounce_scene.c` and
`3d/tests/bounds.c` still pass.
