# 52 — A box per glb, with its share
folder: editor
after: 51
decisions: 0168, 0520, 0521, 0516, 0517, 0442

## Change
`editor/src/inspector_foliage.h`, `inspector_foliage.c`: inside each group's box, a folding box per glb
titled by its file name (0521 point 6), holding every setting a kind had, plus a Share slider of whole
numbers 0 to `VOE_ASSETS_LANDSCAPE_SHARE_MAX`. A share edit is a look stroke as a spread edit is.

`editor/src/sculpt.h`, `sculpt.c`: what was per group and is a glb's (look asks, slider state) is per group
and glb, `_GROUPS` by `_GROUP_GLBS`; the look ask names the glb the slider belongs to.

`editor/src/folds.h`, `folds.c`: a glb's fold is `glb <G> <J> <path>`. New `_glb_removed` and
`_glb_returned` (group, glb) move later glbs' folds of that group as the group calls do for groups;
`_group_removed` and `_group_returned` also move the glb folds of the groups they move. Each writes the
settings once. A box with no fold shows open.

Header points: the glb box, the share. Entries in `editor/src/src.md`.

## Done when
`cmake --build --preset debug --target voe_editor` exits 0, and
`grep -c 'voe_editor_folds_glb_re' editor/src/folds.h` prints 2.
