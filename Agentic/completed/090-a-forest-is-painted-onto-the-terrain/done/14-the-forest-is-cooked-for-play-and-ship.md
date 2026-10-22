# 14 — The forest is cooked for Play and Ship
folder: editor
after: 13
decisions: 0168, 0379, 0442

## Change
0442 point 1, the editor's last half: Play and Ship cook each landscape's kinds and copies
into the table the game loads (card 08).

- `editor/src/game_tree_landscapes.c` — for a landscape with copies: its kind paths as `static
  const char *const landscape_<n>_kinds[]`, escaped as the layers' are, and its copies cooked by
  `voe_authoring_landscape_cook_copies` as `landscape_<n>_copies`; the table entry gains
  `landscape_<n>_kinds, <kind_count>, landscape_<n>_copies, <copy_count>` after the paint, and
  with no copies `NULL, 0, NULL, 0`. A landscape with no layers but copies still names its
  layers `NULL, 0, NULL` before them. The scratch is sized for the copies' text too (some 40
  bytes a copy). The header's example shows a forested entry and says the four fields.
- `editor/src/game_tree.h` (header at 59 of 60 lines) — its landscapes.c phrase names kinds and
  copies only if it fits; else the file's own header carries it.
- `editor/src/src.md` — the `game_tree_landscapes.c` entry mentions the forest.

`.landscape` is already walked by a rename (assets_walk.c), so kind paths follow as layers do.

## Done when
The editor builds, and `grep -n "cook_copies" editor/src/game_tree_landscapes.c` prints a line.

The human, on the example hill, How to test in full:
1. Select the hill. Drop two tree `.glb`s and a rock `.glb` on the Foliage section's add row,
   pick all three, press Scatter and paint the hill's lower slopes: trees and rocks appear,
   each a little different in size and turn.
2. Paint the whole valley with trees, many thousands. Fly over it on the laptop: it stays
   smooth. The Frame panel's Draws stays far below the number of trees.
3. Pick only the trees, press Erase and erase a clearing for the house: the trees go.
4. Press Raise and raise the ground under some trees: they stay standing on it.
5. Ctrl+Z the clearing, Ctrl+Y it back.
6. Look from the hilltop to the far valley: far trees are there, drawn with their simple LODs.
7. Save, close and reopen: the same forest. Press Play, then Ship and run the shipped game:
   the same forest in both.
