# 11 — Layers are followed, and cooked for Play and Ship
folder: editor
after: 07, 08, 10
decisions: 0168, 0379, 0402

## Change
0402 points 1 and 8, the editor's last half: a rename of a layer's material follows into the
`.landscape` files, and Play and Ship cook the layers and paint the game loads.

- `editor/src/assets_walk.c`, `editor/src/assets_walk.h` — `.landscape` joins `walked_extensions`
  (its layer paths are quoted, 0402 point 1); the header's WHAT IS WALKED names it.
- `editor/src/game_tree_landscapes.c` — for a landscape with layers: its paint cooked by
  `voe_authoring_landscape_cook_paint` as `landscape_<n>_paint` (paint all layer 0 when the file
  has none: build it in the scratch before cooking), its layer paths as a
  `static const char *const landscape_<n>_layers[]` array, escaped as the table's path is; the
  table entry gains `landscape_<n>_layers, <count>, landscape_<n>_paint`, and with no layers
  `NULL, 0, NULL`. The header's example shows a painted entry. The scratch may need to grow for
  the paint (8 bytes a point): size it from the cells, as a block size.
- `editor/src/game_tree.h` — its landscapes.c paragraph says layers and paint are cooked.
- `editor/src/src.md` — entries for `assets_walk.*` and `game_tree_landscapes.c` follow.

## Done when
The editor builds, and `grep -n '".landscape"' editor/src/assets_walk.c` and
`grep -n "cook_paint" editor/src/game_tree_landscapes.c` each print a line.

The human, on the example hill, How to test in full:
1. Create Dirt, Grass, Moss and Rock materials; drop each on the hill's Layers add row. The hill
   is all dirt.
2. Paint grass over the lower slopes and rock over the top; each shows where painted.
3. Paint moss at strength ~0.3 along the rock's edge: moss sits in the rock's cracks, no smear.
4. Fly up high: no tiled pattern across the grass.
5. Ctrl+Z one stroke, Ctrl+Y it back.
6. Change Grass's tint in the Inspector: the painted grass changes at once.
7. Rename the Grass material in Assets; Save, close, reopen: the same ground. Press Play, then
   Ship and run the shipped game: the same ground in both.
