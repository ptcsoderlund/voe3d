# 05 — A view keeps each copy's LOD
folder: 3d
after: 03
decisions: 0168, 0495, 0500, 0501

## Change
The per-view memory 0500 point 1 asks for, built as 0501 says. Nothing draws through it yet
(cards 06, 07).

- `3d/include/3d/model_levels.h`:
  - The choice struct gains `from` (the LOD drawing the pixels `level` does not); `shown` now is
    the share `level` draws, `from` the rest, 1 when settled.
  - Add `float voe_3d_model_level_size(model, placed, eye, view)`: s as the choice paragraph
    defines it, INFINITY for an eye inside the world sphere or w ≤ 0.
  - Add `uint32_t voe_3d_model_level_at(const voe_3d_model_entry *model, float s)`: the largest
    k below `level_count` with `levels[k].bound` > s, else 0.
  - `voe_3d_model_level` answers `_at(_size(...))`, unchanged in result. `voe_3d_model_level_choose`
    and its band stay until card 07; it sets `from` to `level` − 1.
- `3d/include/3d/lods_kept.h` (new) and `3d/src/lods_kept.c` (new), type `voe_3d_lods_kept`:
  - `VOE_3D_LODS_KEPT` (2^17 copies a frame), `VOE_3D_LODS_DISSOLVE` (0.25 s),
    `VOE_3D_LODS_BACK` (0.9).
  - `voe_3d_lods_kept *voe_3d_lods_kept_create(voe_base_arena *arena)`: one memory, empty.
  - `void voe_3d_lods_kept_begin(voe_3d_lods_kept *kept, double now)`: a new frame at `now` seconds;
    this frame's table becomes last frame's and the new one is empty by its stamp, nothing cleared.
  - `voe_3d_model_level_choice voe_3d_lods_kept_choose(voe_3d_lods_kept *kept, uint64_t key,
    model, placed, eye, view)`: with s from `_level_size`, D = `_level_at(s)`, D' = `_level_at(0.9 s)`;
    a key not chosen last frame (first sight), or a NULL `kept`, or no room, takes D whole; else with
    kept L: D > L swaps to D, D' < L swaps to D', else keeps L. A swap records `from` = L and the
    moment; while under 0.25 s since, `shown` = elapsed ÷ 0.25, else whole (`from` = `level`). A
    second choose of one key in a frame answers as the first.
  - `bool voe_3d_lods_kept_find(const voe_3d_lods_kept *kept, uint64_t key,
    voe_3d_model_level_choice *out)`: this frame's choice of `key`, else last frame's; false when
    neither.
  - `uint64_t voe_3d_lods_kept_row_key(voe_ecs_entity row)` and
    `uint64_t voe_3d_lods_kept_copy_key(voe_ecs_entity landscape, uint32_t kind, int32_t x,
    int32_t z)`: 0501 point 4, top bit set on copies.
  - Two open-addressed tables, each twice `VOE_3D_LODS_KEPT` slots, entries small (key, frame stamp,
    level, from, swap moment); the header comment covers ownership (caller keeps one per target,
    as `bounce_casters.h` does), the clock, first sight, the room and its size in bytes.
- `3d/tests/lods_kept.c` (new), each a test: `first_sight_takes_the_distance_level`,
  `creeping_back_keeps_the_coarser_level`, `ten_percent_nearer_swaps_back`,
  `a_swap_dissolves_over_a_quarter_second`, `settled_is_whole`, `unseen_a_frame_is_first_sight`,
  `two_chooses_in_a_frame_agree`, `null_memory_is_stateless`, `row_and_copy_keys_differ`, using a
  model entry with hand-set bounds as `3d/tests/model_levels.c` builds one.
- `3d/include/3d/3d.md`, `3d/src/src.md`, `3d/tests/tests.md`: entries for the new files.

## Meaning
- Holds: a copy swaps coarser at the bound and back only once 10% nearer; each copy keeps, per view,
  its LOD and the moment it last swapped; first seen takes its distance's LOD with no dissolve
  (0495 pt 2, 0500 pt 1).
- Gaps answered by 0501: copy identity, the clock, first sight.

## Done when
The tests `3d/lods_kept` and `3d/model_levels` pass.
