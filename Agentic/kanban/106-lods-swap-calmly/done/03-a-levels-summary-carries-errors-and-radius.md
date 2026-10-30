# 03 — A levels summary carries errors and the radius
folder: 3d
after: 02
decisions: 0168, 0500

## Change
The LODs panel will show Swap distance in metres (0500 point 3), which needs each LOD's error and
the model's radius beside its triangles. `voe_3d_model_levels_triangles` becomes a summary. Its
caller `game` is fixed by card 04; this card does not touch it.

- `3d/include/3d/model_levels.h`:
  - Add `voe_3d_model_levels_summary`: `count` (levels written, 0 when the `.glb` will not parse
    or bake), `triangles[VOE_3D_MODEL_LEVELS]`, `errors[VOE_3D_MODEL_LEVELS]` (a fresh file's,
    level 0's nought, all nought when not fresh), `radius` (level 0's bounding radius, the same
    sphere the store measures: box centre, farthest baked vertex), `own`, `fresh`.
  - Replace `voe_3d_model_levels_triangles` with `void voe_3d_model_levels_summarise(
    voe_base_arena *arena, const uint8_t *glb, size_t glb_size, const uint8_t *levels,
    size_t levels_size, voe_3d_model_levels_summary *out)`; its comment keeps the old one's rules
    for triangles and own/fresh and adds the errors and the radius.
- `3d/src/model_levels.c`: the function renamed and filling the new fields; the radius from the
  file's `radius_of` on level 0's bake, the errors from the fresh file `_read` returns.
- `3d/tests/model_levels.c`: the calls to `_triangles` use `_summarise`, checking the same counts,
  plus in one fresh case that `errors[k]` matches the read file's and `radius` is above nought;
  a stale case gives all errors nought.
- `3d/include/3d/3d.md`, `3d/tests/tests.md`: entries say summary.

## Done when
The test `3d/model_levels` passes and
`! grep -rq voe_3d_model_levels_triangles 3d` exits 0.
