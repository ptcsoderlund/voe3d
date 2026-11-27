# 54 — Removing a group or a glb is one undo step
folder: editor
after: 53
decisions: 0168, 0520, 0521, 0516, 0517, 0442, 0496, 0502

## Change
`editor/src/file_steps.h`, `file_steps.c`: `voe_editor_landscape_step` gains what it removes: nothing, a
group, or one glb of a group (its group and glb). `_new` sets nothing. Its only caller today is
`disk_take.c`.

New `editor/src/foliage_removal.h`, `foliage_removal.c`:
- `voe_editor_foliage_removal_follow(scene, path, the step's removal, bool forward)`. A group: forward,
  `voe_editor_folds_group_removed` and the brush's `sculpt.picked` loses that bit with higher bits moved
  down one; back, `_group_returned` and the higher bits move up, that bit left clear. A glb:
  `_glb_removed` or `_glb_returned`; picked bits stay.
- `voe_editor_foliage_remove(scene, models, undo, project, path, group, glb, scratch)`, glb equal to
  `VOE_ASSETS_LANDSCAPE_GROUP_GLBS` meaning the whole group. It copies
  `voe_editor_models_landscape_held` into scratch as an exact `voe_assets_landscape_resample` at its own
  cells, calls `voe_assets_landscape_group_remove` or `_glb_remove` on the copy, makes
  `voe_editor_landscape_step_new(path, held, copy)` with the removal set, applies it forward, follows
  forward, and hands it to `voe_editor_undo_landscape`. Nothing for a path not held, a group or glb out of
  range, or a group's only glb.

`editor/src/undo.c`, `undo.h`: where a landscape step is taken, back or forward, a step that removes calls
`voe_editor_foliage_removal_follow` with the scene `undo_take` has. `voe_editor_undo_landscape`'s header
says it takes removals as well as disk takes.

Header points: undo and redo move folds and picked groups with the groups; the step holds two whole
landscapes. Entries in `editor/src/src.md`.

## Meaning
- Scout: the landscape step holds whole landscapes and is used today only for disk takes. Folds are never
  undone otherwise.
- Open, settled by 0520 and 0521: a removed glb's copies stay and pick among the rest.
- Not settled: that `voe_editor_models_landscape_take` rebuilds the store's per-group and per-glb state
  (models, grown copies) for a landscape whose groups or glbs fell. The coder confirms it in its header
  or blocks.

## Done when
`cmake --build --preset debug --target voe_editor` exits 0, and
`grep -c 'voe_editor_foliage_removal_follow' editor/src/undo.c` prints 1 or more.
