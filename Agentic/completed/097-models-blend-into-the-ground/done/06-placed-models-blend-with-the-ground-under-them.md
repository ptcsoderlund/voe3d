# 06 — Placed models blend with the ground under them
folder: 3d
after: 03, 05
decisions: 0168, 0481, 0396, 0402, 0388

## Change
0481 points 2, 3 and 5 for model rows.

- `3d/src/draw_ground.h`, `3d/src/draw_ground.c` — new, internal:
  - `struct voe_3d_ground`: the landscape's model `row`, `heights`, `paint[2]`, `layers[8]` as a
    node record names them, `rows[3]` (camera-relative space to its own metres: the inverse,
    `voe_math_float4x4_inverse`, of the world place about the eye that `draw_system.c`'s
    landscape walk takes), `side`.
  - `struct voe_3d_grounds { const struct voe_3d_ground *grounds; uint32_t count; }`.
  - `voe_3d_draw_grounds(world, frame, arena)`: one per landscape row the run draws solid, its
    first VOE_3D_LANDSCAPES_DRAWN, off, hidden and fading left out, the paint and layers asked
    of the store as `draw_terrain.c` asks them; none with no store; only painted ones.
  - `voe_3d_draw_ground_under(grounds, at)`: the first whose square holds camera-relative
    point `at` in its own x and z; NULL with none.
  - `voe_3d_draw_ground_fill(ground, reach, object)`: `ground`, `paint`, `layers`,
    `ground_rows`, `terrain.w` and `ground_reach`, the reach clamped to 0.05–10 m; a reach not
    above nought or not a number fills nothing.
  - Header: the claims, 0481 points, an example as the other draw headers have.
- `3d/src/draw_models.h`, `3d/src/draw_models.c` — the walk takes `const struct voe_3d_grounds
  *`. A row with `blend_with_ground` and a ground under its world origin about the eye has
  every part's record filled with it at its `blend_height`; its solid parts are drawn after
  every other row's, inside one `ground blend` span opened only when there is one (render's
  `voe_render_frame_span_begin`, one span at a time); its blended or fading parts are held as
  ever, filled. The header's example and paragraphs follow.
- `3d/src/draw_terrain.c` — read only, for the paint and layer asks and the world place.
- `3d/src/draw_system.c` — `_run` builds the grounds on its scratch before the model walk and
  hands them to it.
- `3d/include/3d/draw_system.h` — `_run`'s comment, after the landscape-row sentence: a model
  row blending with the ground (0481) draws its solid parts in the pass's `ground blend` span,
  listed as `<pass>: ground blend`.
- `3d/src/src.md` — entries for `draw_ground.*`; `draw_models.*` follow.
- `3d/tests/draw_ground.c` — new, set up as `3d/tests/draw_terrain.c` sets up its device, store,
  landscape row and frame (read it): a 200 m painted landscape row at (100, 0, 50), the eye at
  the origin. Claims: one ground, held under (100, 5, 50) and nothing under (400, 0, 0); the
  filled record's `ground` and `paint[0]` not nought, `terrain.w` 200, `ground_rows` taking
  (100, 5, 50) to (0, 5, 0) within 1e-3; reach 20 filled as 10, reach 0 filling nothing. Skips
  without usable Vulkan as the others do.
- `3d/tests/tests.md` — an entry for it.

## Done when
`ctest --test-dir build/debug -R '^3d/draw_ground$'` passes, and the existing 3d draw tests
still pass.
