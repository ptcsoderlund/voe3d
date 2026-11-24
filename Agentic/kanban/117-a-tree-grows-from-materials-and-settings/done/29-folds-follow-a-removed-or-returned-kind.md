# 29 — Folds follow a removed or returned kind
folder: editor
after: 28
decisions: 0168, 0516, 0517

## Change
`editor/src/folds.h` and `editor/src/folds.c`. A kind's fold is named `kind <k> <path>`
(`voe_editor_folds_kind_name`). There is no rename call yet. Two new functions, each writing the settings
once:
- `void voe_editor_folds_kind_removed(voe_editor_folds *, const char *landscape, uint32_t kind)`: drops
  kind `kind`'s fold of that landscape, and renames each later kind's fold one index down.
- `void voe_editor_folds_kind_returned(voe_editor_folds *, const char *landscape, uint32_t kind)`:
  renames each fold from `kind` up one index, from the top down so none overwrites another. Kind `kind`
  is left with no fold, so it shows open (0517 point 5).

Folds of other landscapes, and other sections, are untouched. Header points: both calls, and that a
removal and its undo leave every other kind's fold as it was. Index entries in `editor/src/src.md`.

## Done when
`cmake --build --preset debug --target voe_editor` exits 0, and
`grep -c 'voe_editor_folds_kind_re' editor/src/folds.h` prints 2.
