# 04 — The store holds a landscape's foliage
folder: 3d
after: 03
decisions: 0168, 0442

## Change
0442 points 1–3, the store's half: a landscape entry owns its kinds and copies with room for
the cap, and each copy's placed transform, kept standing on the heights.

- `3d/include/3d/models_foliage.h` — new public header (`models.h`'s header is at its cap):
  the store's foliage calls, each nothing for a path that is no loaded landscape, each marking
  the entry edited when it changes a copy:
  - `voe_3d_models_landscape_scatter(models, path, const voe_3d_foliage_brush *, x, z,
    seconds, uint64_t *seed, voe_assets_landscape_copy *added)` and
    `voe_3d_models_landscape_erase(models, path, brush, x, z, removed, room)`, returning the
    count, over `3d/foliage.h`'s with the entry's placed transforms.
  - `voe_3d_models_landscape_put_copies(models, path, copies, count, bool add)`: undo's.
  - `voe_3d_models_landscape_kind_set(models, path, index, const char *model)`: kind `index`
    takes the path; `index == kind_count` (below VOE_ASSETS_LANDSCAPE_KINDS) adds one; asserts
    past that and on a path of VOE_ASSETS_LANDSCAPE_KIND_PATH bytes or more.
  - `const voe_scene_transform *voe_3d_models_landscape_placed(const voe_3d_models *, const
    voe_3d_model_entry *)`: the entry's copies' placed transforms, `entry->landscape->copy_count`
    of them; NULL for an entry that is no loaded landscape.
  - Header points: the room held per landscape (VOE_ASSETS_LANDSCAPE_COPIES copies and
    transforms, about 4 MB), why placed transforms are kept and when they are placed again.
- `3d/src/models_store.h` — the held entry gains `voe_scene_transform *placed`.
- `3d/src/models_foliage.c` — new, with its header: the calls above.
- `3d/src/models_landscape.c` — the load (`_load_landscape`, and a `.landscape` through
  `_load`): the grid copy's `copies` gets room for the cap in the entry's arena, the given ones
  copied in, kinds carried, every copy placed. `_landscape_brush` with heights changed and
  `_landscape_put` call `voe_3d_foliage_replace` over the rect. The rename: a kind path equal to
  `from` or under `from/` follows, as a layer's does (in `models.c` if the rename lives there).
- `3d/tests/models_landscape.c` — new cases on a loaded 8-cell landscape whose file names one
  kind and two copies: `_placed` gives two transforms on the ground; a RAISE brush under copy 0
  raises its placed y and leaves copy 1's; `_put` of the old heights puts it back; a scatter
  with the kind picked adds copies, marks the entry edited and `_placed` grows with it; erase
  then `_put_copies` add restores the count; `_kind_set` at index 1 adds a kind; `_rename` of
  the kind's folder follows its path.
- `3d/3d.md`, `3d/src/src.md`, `3d/tests/tests.md` — entries for the new files; the
  `models_landscape.c` entries mention foliage.

## Done when
Tests `3d/models_landscape` and `3d/foliage` pass.
