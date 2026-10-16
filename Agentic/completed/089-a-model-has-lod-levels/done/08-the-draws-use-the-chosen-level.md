# 08 — The draws use the level the camera chooses
folder: 3d
after: 02, 07
decisions: 0168, 0425

## Change
0425 point 5: the camera's pass and its shadows draw the level the camera chooses for each model
row; the probe bounce's captures the last level; pick, outline and bounds keep level 0.

- `3d/src/draw_group.h` / `draw_group.c`: `voe_3d_draw_group_part_worn` gains `uint32_t level`
  (asserted below `level_count`); the worn part's `geometry` is `model->levels[level].geometries
  [part]`. Its comment says a zeroed geometry is a part this level lacks, which its callers skip.
- `3d/src/draw_system.c`: the model-row walk (around its `part_worn` call) asks
  `voe_3d_model_level(model, &drawn, frame->eye, &frame->view)` once per row and passes it; a part
  whose geometry has generation 0 is skipped. 774 lines: add only those lines; the header's
  paragraph on model rows names the level in a phrase.
- `3d/src/draw_bounce.h`: `voe_3d_draw_casters` gains `bool coarsest`: false draws each model row
  at the camera's level, true at its last; its comment says so and why (0331: the camera never
  changes the bounce).
- `3d/src/draw_shadows.c`: `draw_model_casters` and `voe_3d_draw_casters` take `coarsest`; the
  level per row as above or `level_count − 1`; zero geometries skipped. Its two calls (cascades,
  point shadows) pass false.
- `3d/src/draw_bounce.c`: its two `voe_3d_draw_casters` calls pass true; its two `part_worn`
  calls (the box grown from geometry, the casts check) pass 0.
- `3d/src/src.md`: the `draw_shadows.c` and `draw_bounce.h` entries name the level in a phrase.
- `3d/tests/draw_levels.c` (new), set up as `3d/tests/draw_model_fade.c` is (a camera, a sun that
  casts, `voe_3d_draw_system_run`), skipping without a graphics card as it does: one model row
  wearing a 64 × 32 sphere from `3d/tests/model_glb.inc`, loaded with
  `voe_3d_models_load_levelled` and levels from `voe_3d_model_levels_make`:
  1. the row 3 m before the camera: the frame's `voe_render_frame_triangle_count` is N;
  2. the row 400 m away: under N ÷ 4;
  3. the same sphere loaded with NULL levels, 400 m away: over four times check 2's count (a
     model without levels is drawn whole).
  Radius 1, on the camera's axis, so both distances are in view.
  Its header says what it shows. `3d/tests/tests.md`: the entry.

## Done when
The test `3d/draw_levels` passes after the folder's build, and `3d/draw_model_fade` and
`3d/draw_system` still pass.
