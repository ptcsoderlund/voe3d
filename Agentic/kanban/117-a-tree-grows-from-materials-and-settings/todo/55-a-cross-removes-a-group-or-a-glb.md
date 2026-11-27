# 55 — A cross removes a group or a glb
folder: editor
after: 54
decisions: 0168, 0520, 0521, 0516, 0517
read: feature.md

## Change
`editor/src/inspector_foliage.h`, `inspector_foliage.c`: a × at the right end of each group's title row,
and of each glb's title row unless it is its group's only glb (0521 point 5). Hover text says what it
removes: the group and its copies, or the glb, whose spots the group's other glbs then fill. A click asks
for the removal; it does not pick or fold. No confirm; Delete is not bound to it.

`editor/src/sculpt.h`, `sculpt.c`:
- a removal ask beside the look and grow asks: `remove_asked`, its group and glb, set by the Foliage read
  and cleared each read;
- `voe_editor_sculpt_read` settles it as it settles the other asks, calling `voe_editor_foliage_remove`
  on the brush's landscape with the project it gives its other undo steps;
- a removal drops any look or grow ask of that same read, since their indices are stale.

Header points: the ×s and what each removes, that undo brings it back; the asks. Entries in
`editor/src/src.md`.

This is the feature's last card: `## How to test` steps 1 to 13 of `feature.md` are the human's.

## Done when
`cmake --build --preset debug --target voe_editor` exits 0, and
`grep -c 'remove_asked' editor/src/sculpt.c` prints 1 or more.

The human, `feature.md` How to test steps 1 to 13, among them:
- the hill opens as before, each kind a group of its one glb;
- a group of four glbs mixes; share 3 comes up about three times as often, share 0 never;
- a glb's × lets the others fill its spots; a group's × leaves the other groups' trees in their spots;
  undo, redo, save and reopen each as step 8 says;
- a seventeenth group is refused; LODs swap per glb under 114's budget; Play and Ship look the same.
