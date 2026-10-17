# 10 — A foliage stroke is an undo step
folder: editor
after: 07, 09
decisions: 0168, 0379, 0442

## Change
0442 points 3 and 4, the editor's store calls and the stroke an undo step carries. Nothing
stamps foliage yet (card 11).

- `editor/src/models.h`, `editor/src/models.c` — over `3d/models_foliage.h`, each nothing for a
  path that is no loaded landscape:
  - `voe_editor_models_scatter(models, path, const voe_3d_foliage_brush *, x, z, seconds,
    uint64_t *seed, voe_assets_landscape_copy *added)` and `voe_editor_models_erase(models,
    path, brush, x, z, voe_assets_landscape_copy *removed, uint32_t room)`, returning counts;
  - `voe_editor_models_put_copies(models, path, copies, count, bool add)`;
  - `voe_editor_models_kind_set(models, path, index, const char *model)`.
  The header is at 57 of 60 lines: its landscape paragraph names foliage in a phrase within
  the cap; the rest goes above the declarations. Save already writes copies (the writer
  carries them).
- `editor/src/strokes.h`, `editor/src/strokes.c` — a stroke is heights, paint or foliage: a
  `foliage` flag, `erase` (the stroke removed rather than added), and its `copies` and
  `copy_count`, its own malloced copy; `voe_editor_stroke_new_foliage(path, bool erase, const
  voe_assets_landscape_copy *, uint32_t count)`; `voe_editor_stroke_apply` for foliage puts the
  copies back through `voe_editor_models_put_copies`: added forward and removed backward for a
  scatter, the other way for an erase; `_destroy` frees them. Header: a stroke may carry the
  copies it added or removed, put back by value (0442 point 4).
- `editor/src/undo.h` — its stroke paragraph says a stroke may be a foliage stroke; `undo.c`
  needs no change if it only calls `voe_editor_stroke_apply` and `_destroy`.
- `editor/src/src.md` — entries for `strokes.*` and `models.*` mention foliage.

The editor has no tests; the folder's build is the check here and the human's steps at card 14.

## Done when
The editor builds, and `grep -n "put_copies" editor/src/strokes.c editor/src/models.c` prints a
line for each file.
