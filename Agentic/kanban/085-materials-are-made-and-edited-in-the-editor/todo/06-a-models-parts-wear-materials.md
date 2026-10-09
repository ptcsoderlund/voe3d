# 06 — A model's parts wear materials
folder: 3d
after: 05
decisions: 0168, 0399

## Change
0399 point 6 for models.

- `3d/include/3d/model_component.h` — `VOE_3D_MODEL_MATERIALS` 8, and the model row gains
  `materials`, `F(char, materials, CHAR, VOE_3D_MODEL_MATERIALS, VOE_3D_MODEL_PATH)`: eight paths
  for the first eight parts in the store's bake order, empty by default and in a file without them.
  Header points: which part a string names, that an empty or unloaded path keeps the file's own
  material, and that a landscape row's ground ignores them (its layers are 086's).
- `3d/src/model_component.c` — the drain's NUL correction covers each of the eight strings as it
  does `path`.
- `3d/src/draw_group.c`, `3d/src/draw_system.c`, `3d/src/draw_bounce.c` — wherever a model part's
  record is chosen (its `material` or, fading, its `faded`), part `i < 8` whose `materials[i]` names a
  loaded material (`voe_3d_draw_material_named`, card 05) takes that entry's part material, and its
  twin `faded` while fading. Not `3d/src/draw_terrain.c`.

Update `3d/3d.md`'s `model_component.h` line.

Test: add to `3d/tests/draw_material.c` a model row of `3d/tests/model_data.inc`'s model whose
`materials[0]` names the red unlit material, reading red where the model covers the picture's centre;
and to `3d/tests/model_component.c` a check that an unterminated `materials[3]` is corrected.

## Done when
`ctest --test-dir build/debug -R '^3d/(draw_material|model_component|draw_model_fade)$'` and
`ctest --test-dir build/debug -R '^game/'` pass.
