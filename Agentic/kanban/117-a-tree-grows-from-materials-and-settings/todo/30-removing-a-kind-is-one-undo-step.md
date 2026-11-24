# 30 — Removing a kind is one undo step
folder: editor
after: 29
decisions: 0168, 0516, 0517, 0442, 0496, 0502

## Change
`editor/src/file_steps.h` and `editor/src/file_steps.c`: `voe_editor_landscape_step` gains
`uint32_t removed`, the kind the step removes, `VOE_ASSETS_LANDSCAPE_KINDS` for none. `_new` sets none.
Its only caller today is `disk_take.c`.

New `editor/src/kind_removal.h` and `editor/src/kind_removal.c`:
- `void voe_editor_kind_removal_follow(voe_editor_scene *, const char *path, uint32_t kind, bool
  forward)`. Forward: `voe_editor_folds_kind_removed`, and the brush's `sculpt.picked` loses bit `kind`
  with the higher bits moved down one. Back: `_kind_returned`, and the higher bits move up one, leaving
  bit `kind` clear.
- `void voe_editor_kind_remove(voe_editor_scene *, voe_editor_models *, voe_editor_undo *,
  voe_editor_project *, const char *path, uint32_t kind, voe_base_arena *scratch)`. It:
  - copies `voe_editor_models_landscape_held` into scratch, as an exact `voe_assets_landscape_resample`
    at its own cells;
  - calls `voe_assets_landscape_kind_remove` on the copy;
  - makes `voe_editor_landscape_step_new(path, held, copy)` with `removed` = kind;
  - applies it forward and follows forward;
  - hands it to `voe_editor_undo_landscape`.
  Nothing for a path that is not a held landscape, or a kind not below its `kind_count`.

`editor/src/undo.c` and `editor/src/undo.h`: where a landscape step is taken (back or forward), a step
whose `removed` is a kind also calls `voe_editor_kind_removal_follow` with the scene `undo_take` already
has. The header of `voe_editor_undo_landscape` says it takes removals as well as disk takes.

Header points: undo and redo move folds and picked kinds with the kinds; the step holds two whole
landscapes. Index entries in `editor/src/src.md`.

## Meaning
- Scout: the landscape step holds whole landscapes and is used today only for disk takes. Folds are never
  undone otherwise.
- Not settled: that `voe_editor_models_landscape_take` rebuilds the store's per-kind state (kind models,
  grown copies) for a landscape whose kind count fell. The coder confirms it in its header or blocks.

## Done when
`cmake --build --preset debug --target voe_editor` exits 0, and
`grep -c 'voe_editor_kind_removal_follow' editor/src/undo.c` prints 1 or more.
