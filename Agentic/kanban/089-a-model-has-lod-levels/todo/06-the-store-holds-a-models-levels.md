# 06 — The model store holds a model's levels
folder: 3d
after: 05
decisions: 0168, 0425, 0426

## Change
A loaded `.glb` gets its levels on the card: its own (0425 point 4) or the made ones read from a
levels file (0426 point 1, `3d/include/3d/model_levels.h`). The store's room grows (0425 point 6).

- `3d/include/3d/models.h`:
  - new `voe_3d_model_level_parts`: `voe_render_geometry geometries[VOE_3D_MODEL_PARTS]`, one per
    level-0 part, a zeroed id (generation 0 is never handed out) where the level lacks the part;
    `float bound`, the screen size `s` the level is drawn under (0425 point 3).
  - `voe_3d_model_entry` gains `voe_3d_model_level_parts levels[VOE_3D_MODEL_LEVELS]`, `uint32_t
    level_count` (1 for every entry without levels, pictures, landscapes and materials too),
    `bool levels_own`, and the model-space bounding sphere `voe_math_float3 centre` and `float
    radius` (box centre, farthest level-0 vertex; nought for a non-`.glb`). `levels[0]` mirrors the
    parts' geometries with an infinite bound, so a draw reads `levels[k]` for any k.
  - new `voe_3d_models_load_levelled(models, device, path, stamp, bytes, size, const uint8_t
    *levels, size_t levels_size, error)`; `_load` becomes it with NULL levels. A levels file that
    is stale or names an index past its part loads level 0 alone; never a failure.
  - capacities: geometries 2050, vertices 3 M, indices 9 M; the room comment says why (0425
    point 6).
  - header: a paragraph on levels: own levels baked per level and joined to level 0's parts by
    material, the bound 2^-k; made ones uploaded compacted to the vertices they use, the bound
    0.002 × radius ÷ error (infinite for error nought), bounds made non-increasing; freed with the
    parts.
- `3d/src/models_levels.c` (new) and its declarations in `3d/src/models_store.h`: the upload of an
  entry's levels from the model, its level-0 bake and the levels bytes, and their free. Own: each
  level 1..n bakes with `voe_3d_model_bake_create(..., k, ...)`, each baked part uploaded as the
  geometry of the level-0 part of the same `material`, one with no match left out; an own level
  with no part at all ends the list. Made: `voe_3d_model_levels_read`, every index checked below its
  part's vertex count, each part's used vertices gathered with its indices renumbered, uploaded with
  `voe_render_geometry_create`. A device refusing room fails the load as a part's would.
- `3d/src/models.c`: `load_glb` takes the levels bytes, sets centre and radius from the bake, calls
  the above after the parts; the entry-new and give-back paths free level geometries k ≥ 1 wherever
  part geometries are freed (replace, failure, clear). Keep the additions small: 612 lines now.
- `3d/src/src.md`: the `models_levels.c` entry; `models_store.h`'s names it.
- `3d/tests/models_levels.c` (new), skipping without a graphics card as `3d/tests/models.c` does,
  its glb from `3d/tests/model_glb.inc` and levels from `voe_3d_model_levels_make`:
  1. a 64 × 32 sphere with its levels: `level_count` at least 3, level 1's part 0 a live id, bounds
     non-increasing, radius within 1 % of the sphere's;
  2. the same with NULL levels, and with those levels for a different glb: loaded, `level_count` 1;
  3. the "Ball_LOD1" sphere with NULL levels: `levels_own`, `level_count` 2, `levels[1].bound` 0.5;
  4. loaded again and cleared, then loaded once more: true (nothing leaked past the room).
  Header lists them. `3d/tests/tests.md`: the entry.

## Done when
The test `3d/models_levels` passes after the folder's build, and `3d/models` still passes.
