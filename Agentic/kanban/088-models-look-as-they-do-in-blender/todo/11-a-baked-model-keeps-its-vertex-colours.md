# 11 — A baked model keeps its vertex colours
folder: 3d
after: 06, 10
decisions: 0168, 0421

## Change
0421 point 1 on the model store's side: a primitive's `colours` (card 02) reach the vertex's
`colour_complement` (card 07).

- `3d/src/model_bake.c`: where a primitive's attributes are interleaved into `voe_render_vertex`,
  each colour channel c (clamped 0–1) is written as the complement (1 − c) × 65535, rounded; a
  primitive with no colours leaves the field zeroed (white). The header gains one sentence on it.
- `3d/src/model_bake.h`: the header says vertices carry the file's vertex colours.
- `3d/tests/model_bake.c` (new, no graphics card; include `../src/model_bake.h` as
  `3d/tests/models_landscape.c` includes its internal header): a `voe_assets_model` built in the test,
  one node, two primitives on one material, one with colours (opaque red, half-grey) and one without;
  the bake's vertices carry complements 0/65535/65535/0 and about 32767 for grey, and zeros for the
  primitive without.
- `3d/tests/tests.md`, `3d/src/src.md`: an entry for the test; the `model_bake` entries mention colours
  (each at most 300 characters).

## Done when
The test `3d/model_bake` passes after the folder's build.
