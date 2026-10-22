# 76 — A copy is placed at its kind's base size
folder: 3d
after: 75
decisions: 0168, 0442, 0459, 0460

## Change
0460 point 3: a copy's placed scale is its kind's base size × its own, and the store sets a
kind's size and places that kind's copies again.

- `3d/include/3d/foliage.h`, `3d/src/foliage.c` — `voe_3d_foliage_place` scales uniformly by
  `voe_assets_landscape_kind_size(landscape, copy.kind)` × the copy's scale / 1000. Scatter,
  erase and put unchanged (a scatter still stores 1 ± vary). Header point: the PLACED ON THE
  GROUND paragraph says the scale is the kind's base size times the copy's own.
- `3d/include/3d/models_foliage.h`, `3d/src/models_foliage.c` —
  - `void voe_3d_models_landscape_kind_size_set(voe_3d_models *, const char *path, uint32_t
    index, uint16_t size)`: kind `index` of `path`'s grid takes `size` (thousandths, 0 or
    VOE_ASSETS_LANDSCAPE_KIND_SIZE_MIN–_MAX, asserted; index below kind_count, asserted); when
    it differs, every copy of that kind placed again into the entry's placed transforms (one
    walk of the copies), the entry marked edited and the store's `changes` raised as a kind
    set raises it. Nothing for a path that is no loaded landscape; the same size, nothing.
  - `voe_3d_models_landscape_kind_set` adding a kind (index == kind_count) gives it size 0;
    replacing a kind's model keeps its size.
  - Header: the "marks the entry edited" paragraph names a size change.
- `3d/src/models_landscape.c` — the load's grid copy carries `kind_sizes` with the kinds (read
  the header and the comment above the grid copy; add it if only `kinds` is copied).
- `3d/tests/foliage.c` — new case: two copies alike but for kind 0 and 1, `kind_sizes[1]` 2000:
  kind 1's placed scale is twice kind 0's, position and rotation equal.
- `3d/tests/models_landscape.c` — new cases on the loaded landscape with one kind and two
  copies: `_kind_size_set(…, 0, 2000)` doubles both placed scales, leaves their positions and
  rotations, and marks the entry edited; set back to 0 gives the first transforms exactly; a
  `_kind_set` adding kind 1 leaves `kind_sizes[1]` 0; a file with `size0=2000` loads with the
  copies placed at twice their scale.
- `3d/3d.md`, `3d/src/src.md`, `3d/tests/tests.md` — the foliage and models_foliage entries
  name the base size.

## Done when
Tests `3d/foliage` and `3d/models_landscape` pass.
