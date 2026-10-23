# 05 — The store keeps what each material grows
folder: 3d
after: 04
decisions: 0168, 0442, 0451, 0473, 0476

## Change
0476 points 4, 5 and 7's store side: a loaded landscape's grown copies, kept beside the file's
copies, remade when the paint, heights, a choice or a look changes. Changes a public call the
editor uses; the editor follows in card 07.

- `3d/src/models_store.h` — `entry_held` gains `grown` (copies) and `grown_placed`, room
  VOE_3D_FOLIAGE_GROWN each on the entry's `memory`, held at a landscape's load as `placed` is,
  and `grown_count`. The header's paragraph on placed transforms says grown copies sit beside
  them and are never the file's.
- `3d/src/models_landscape.c` — at a load, after the copies are placed: every kind grown over
  the whole grid (`voe_3d_foliage_grow`, foliage_grow.h); a paint stamp or `_put_paint`: every
  kind remade over its rect; a height stamp or `_put`: every kind remade over its rect, which
  places them on the new heights; reaching VOE_3D_FOLIAGE_GROWN said once on stderr per entry.
  A change to the grown copies raises the store's `changes`. Header and Constraints follow
  (about 8 MB a landscape now).
- `3d/include/3d/models_foliage.h`, `3d/src/models_foliage.c` —
  - `voe_3d_models_landscape_kind_size_set` is replaced by
    `void voe_3d_models_landscape_look_set(voe_3d_models *models, const char *path,
    uint32_t kind, voe_assets_landscape_look look)`: ranges asserted as card 01's; an equal look
    does nothing; else a new size places that kind's copies again, a new size or spread remakes
    its grown copies over the grid, and any change marks the entry edited and raises `changes`.
  - New `void voe_3d_models_landscape_grow_set(voe_3d_models *models, const char *path,
    uint32_t layer, uint32_t kind, uint16_t density)`: layer below layer_count, kind below
    kind_count, density up to GROW_MAX, asserted; equal does nothing; else sets the grid's
    `grows[layer][kind]`, remakes that kind over the grid, edits, raises `changes`.
  - New `voe_3d_models_grown` (`const voe_assets_landscape_copy *copies`,
    `const voe_scene_transform *placed`, `uint32_t count`) and
    `voe_3d_models_grown voe_3d_models_landscape_grown(const voe_3d_models *models,
    const voe_3d_model_entry *entry)`, zeroed for an entry that is no loaded landscape.
  - Header: GROWN COPIES ARE THE MATERIAL'S: scatter, erase and the copy puts touch only the
    file's copies (0473 point 4); when they are remade; why they are not saved (0476 point 3).
- `3d/tests/models_landscape.c` — new cases on a loaded two-layer landscape whose layer 1 grows
  kind 0: grown copies stand only where layer 1 is painted; a paint stamp of layer 1 on bare
  ground adds grown copies there, and `_put_paint` of the old weights gives back the old count;
  `_grow_set` to 0 leaves none, back to 40 the first count again; an erase stamp of kind 0 over
  them leaves the grown count; `_look_set` with only `shadowless` changed raises `changes` and
  keeps the count.
- `3d/tests/bounce_foliage.c` — `_kind_size_set` uses become `_look_set`.
- `3d/3d.md`, `3d/src/src.md` — the `models_foliage.h`, `models_store.h`,
  `models_landscape.c` and `models_foliage.c` entries say grown copies.

## Done when
The `3d/models_landscape` and `3d/bounce_foliage` tests pass.
