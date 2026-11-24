# 31 — A × on each kind's title removes it
folder: editor
after: 30
decisions: 0168, 0516, 0517

## Change
`editor/src/inspector_foliage.h` and `editor/src/inspector_foliage.c`: each kind's folding box in the
Foliage section gets a × at the right end of its title row. Its hover text says it removes the kind and
its copies. A click asks for the removal; it does not pick the kind or fold the box. No confirm. The
Delete key is not bound to it.

`editor/src/sculpt.h` and `editor/src/sculpt.c`:
- a removal ask beside the look and grow asks: `remove_asked` and `remove_kind`, set by the Foliage read
  and cleared each read;
- `voe_editor_sculpt_read` acts on it as it settles the other asks: it calls `voe_editor_kind_remove` on
  the brush's landscape, with the project it gives its other undo steps;
- a removal drops any look or grow ask of that same read, since their kind indices are stale.

Header points: the × and what it removes, that undo brings it back; the asks. Index entries in
`editor/src/src.md`.

This is the feature's last card: How to test step 15 of `feature.md` is the human's.

## Done when
`cmake --build --preset debug --target voe_editor` exits 0, and
`grep -c 'remove_asked' editor/src/sculpt.c` prints 1 or more.

The human, How to test step 15:
- in a landscape with a rock kind painted before a tree kind, press the rock kind's ×. Every rock goes;
  the landscape and other kinds stay; each tree is the same variation in the same spot, grown trees too;
- Ctrl+Z: the rock kind is back with every rock, unfolded; the other kinds' folds are as they were;
- Ctrl+Y, save, close and reopen: no rocks, and the trees are as before;
- Delete with an entity selected still deletes the entity.
