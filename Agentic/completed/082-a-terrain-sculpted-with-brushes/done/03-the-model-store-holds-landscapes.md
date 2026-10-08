# 03 — The model store holds landscapes
folder: 3d
after: 02
decisions: 0168, 0379

## Change
A `.landscape` path is a model store entry of 16 chunk parts that a brush changes live (0379 points 2,
4, 6). Uses `3d/landscape.h` (card 02) and `assets/landscape.h`.

- `3d/include/3d/models.h`:
  - `voe_3d_model_entry` gains `voe_assets_landscape *landscape` (NULL for any other file, owned by the
    entry) and `bool edited` (changed since loaded or `_landscape_saved`).
  - `_load` reads a path ending `.landscape`, any case, with `voe_assets_landscape_read`, then as below.
  - `[[nodiscard]] bool voe_3d_models_load_landscape(models, device, const char *path, uint64_t stamp,
    const voe_assets_landscape *, voe_base_error *)` — the grid copied into the entry; one part per
    chunk (`voe_3d_landscape_chunk`), all wearing one material of 0379 point 2 and one blended twin as
    `faded`; the shape empty; replaces and fails as `_load` does.
  - `voe_3d_landscape_rect voe_3d_models_landscape_brush(models, const char *path, const voe_3d_brush *,
    float x, float z, float seconds, voe_base_arena *scratch)` — one stamp; its chunks marked dirty and
    the entry edited; empty rect for a path that is no loaded landscape.
  - `void voe_3d_models_landscape_put(models, const char *path, voe_3d_landscape_rect, const float
    *values)` — the rect's heights, row-major, written; dirty and edited as a brush.
  - `void voe_3d_models_landscape_frame(models, device, voe_base_arena *scratch)` — after the frame's
    begin: each dirty chunk built in scratch and made transient, its part drawing that this frame; a
    refused one keeps its static geometry and says so on stderr.
  - `[[nodiscard]] bool voe_3d_models_landscape_settle(models, device, scratch, voe_base_error *)` —
    between frames: each dirty chunk uploaded static again, its old static destroyed, dirt cleared;
    true at once with nothing dirty.
  - `void voe_3d_models_landscape_saved(models, const char *path)` — clears `edited`.
  - `void voe_3d_models_rename(models, const char *from, const char *to)` — an entry whose path is
    `from` or begins `from/` takes `to` in its place.
  - `VOE_3D_LANDSCAPE_TRANSIENT_VERTICES`, `_INDICES`, `_RANGES` — one whole 512-cell landscape's chunks,
    what a program adds to its transient capacities.
  - Header points, added to the store's: a landscape entry and why it wears one material; why its
    shape is empty; transient while dirty and static again when settled, and why (0379 point 4); the
    static ids kept apart from the parts so a frame's transient never loses them.
- `3d/src/models.c` — the extension's branch in `_load`, the landscape's memory freed on replace, clear
  and destroy. The new calls go in a new `3d/src/models_landscape.c`; when the store's struct is private
  to models.c it moves to a new internal `3d/src/models_store.h` both include.
- New `3d/tests/models_landscape.c` (headless device, as `3d/tests/models.c` makes one) —
  `a_landscape_loads_as_sixteen_parts`, `a_brush_marks_it_edited_and_the_frame_draws_transient`,
  `settle_makes_static_and_clears_the_dirt`, `put_writes_the_rect`, `rename_moves_the_entry`.
- `3d/3d.md`'s `models.h` entry, `3d/src/src.md` and `3d/tests/tests.md` updated or gain entries.

## Done when
`ctest --test-dir build/debug -R '^3d/models'` passes, the five tests above among them.
