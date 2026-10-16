# 72 — Own LODs are measured and swap by their error
folder: 3d
after: 71
decisions: 0168, 0435, 0436

## Change
0435 point 3, 0436 point 6: a `.glb`'s own `_LOD<n>` levels get an error against level 0,
measured when the levels file is made, and swap by it as made ones do. Also the triangle
counts the LODs panel shows (card 75).

- `3d/src/model_levels.c`:
  - `_make`, own branch: bake level 0 and each own level k (`voe_3d_model_bake_create` with
    level k, `3d/src/model_bake.h`), error k = `voe_3d_measure_apart` (`3d/src/measure.h`,
    card 69) of the two. A level whose bake fails or holds no triangle ends the list, as an
    own level matching no part ends it in the store. The file: `own` set, level count the
    levels kept, part count 0, one error per level, the asked swaps.
  - `take_head`: an own file may have up to VOE_3D_MODEL_LEVELS levels, part count still 0.
  - New `voe_3d_model_levels_triangles` (below).
- `3d/include/3d/model_levels.h`:
  - `uint32_t voe_3d_model_levels_triangles(voe_base_arena *arena, const uint8_t *glb,
    size_t glb_size, const uint8_t *levels, size_t levels_size,
    uint32_t triangles[VOE_3D_MODEL_LEVELS], bool *own, bool *fresh)`: how many levels'
    triangles it wrote; `*fresh` whether `levels` read as a fresh file of this `.glb`.
    Own: each own level's bake, ending as `_make` ends. Made: level 0's bake, then a fresh
    file's index counts per level; level 0 alone when `levels` is NULL or stale. 0
    when the `.glb` will not parse or bake. Everything on `arena`, the caller rewinding.
  - Header comment: A FILE WITH ITS OWN LEVELS says they are measured both ways against
    level 0 and stored with their errors, no parts (replacing "never simplified, one
    level"); the layout's own case. Stay under 60 lines.
  - `voe_3d_model_levels.level_count`'s and `part_count`'s comments: own files now count
    their levels.
- `3d/src/models_levels.c`: `upload_own` bounds each level as `upload_made` does —
  0.002 × radius ÷ error ÷ `swaps[k]`, infinite for error nought, made non-increasing —
  when the fresh file is own and its level count is the bake's own count; otherwise 2^-k as
  now, with swaps 1. Its header paragraph says so.
- `3d/include/3d/models.h`: the `.glb` HAS LEVELS paragraph: own levels bounded by their
  measured error, 2^-k only without a fresh file.
- `3d/tests/model_levels.c`, new claims in its header's list:
  - claim 3 becomes: the own sphere's file reads `own`, 2 levels, `errors[1]` finite and above
    nought;
  - `_triangles` of the own sphere: 2, `own` true, each above nought and level 1 below level
    0; `fresh` false with NULL levels and true with its file; of the plain sphere with its
    defaults' file: its `level_count`, level 0 the sphere's
    triangles, level k the sum of its index counts ÷ 3, `own` false; with NULL levels: 1;
    of bytes that are no `.glb`: 0.
- `3d/tests/models_levels.c`: claim 3 stays (NULL levels, bound 0.5). New claim: the own
  sphere loaded with its made file, `level_count` 2, `levels[1].bound` 0.002 × radius ÷
  the file's `errors[1]` within 1e-6 relative; with `swaps[1]` 2 in its file, half that.
  Raise `CAPACITIES` if the new loads need room, said in the comment above it.
- `3d/tests/tests.md`, `3d/include/3d/3d.md`, `3d/src/src.md`: entries that say own levels
  are not measured or swap by 2^-k change.

## Done when
The tests `3d/model_levels` and `3d/models_levels` pass after the folder's build, and
`bash ~/.claude/skills/checks/scripts/checks.sh --header-lines 3d/include/3d/model_levels.h`
prints 60 or less.
