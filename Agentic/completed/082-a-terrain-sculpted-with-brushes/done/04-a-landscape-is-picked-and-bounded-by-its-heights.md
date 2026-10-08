# 04 — A landscape is picked and bounded by its heights
folder: 3d
after: 03
decisions: 0168, 0379

## Change
A model row wearing a loaded landscape is met by a ray and has a size, though its shape is empty (0379
point 2). So a click on the hill selects it, an Assets drop lands on it, and the brush finds its ground.

- `3d/include/3d/pick.h` / `3d/src/pick.c` — the walk over model rows tests a landscape entry with
  `voe_3d_landscape_ray` (3d/landscape.h), the ray moved into the row's own space the way a model's
  triangles are tested, its distance compared as theirs is. New
  `bool voe_3d_pick_landscape(const voe_ecs_world *, const voe_3d_models *, voe_ecs_entity,
  <pick.h's ray type>, voe_3d_landscape_hit *)` with `voe_3d_landscape_hit` — `float x, z` in the
  grid's own space and `float distance` along the ray; false for an entity with no loaded landscape or a
  miss. Header: a landscape is met by its heights, not triangles.
- `3d/include/3d/bounds.h` / `3d/src/bounds.c` — a landscape entry counts its `voe_3d_landscape_box`
  through the row's transform as a model counts its vertices. Header phrase added.
- `3d/tests/pick.c` gains `pick_meets_a_landscape` and `pick_landscape_answers_its_own_space`;
  `3d/tests/bounds.c` gains `bounds_hold_a_landscape`. Each makes its landscape with
  `voe_3d_models_load_landscape` on a headless device as those files' model tests do.
- `3d/3d.md`'s `pick.h` and `bounds.h` entries, and `src.md`'s, take the landscape in a phrase.

## Done when
`ctest --test-dir build/debug -R '^3d/(pick|bounds)$'` passes with the three tests above.
