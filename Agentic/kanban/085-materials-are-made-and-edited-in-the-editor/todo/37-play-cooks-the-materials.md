# 37 — Play cooks the materials
folder: editor
after: 08, 36
decisions: 0168, 0236, 0399

## Change
0399 point 7: the game tree's `materials.c`, as `landscapes.c` is written.

- New `editor/src/game_tree_materials.c` — beside `game_tree_landscapes.c` (read its header): every
  `.material` under `Assets/` (`game_tree_find.h`), read and parsed by `assets/material.h` in a
  rewound scratch, written as one `voe_game_material` initializer each (path and map strings escaped
  as `game_tree.c` escapes, floats as `%a` hex so nothing is lost), then `const voe_game_materials
  voe_game_materials_cooked`, `#include <game/materials.h>` first. One that will not parse is
  handled as a landscape that will not read is.
- `editor/src/game_tree.h` / `game_tree.c` — the tree's sixth file `materials.c`, compared before it
  is written as the others are; the header's file list and paragraph on it.

Update `editor/src/src.md` (`game_tree.h`, `game_tree.c`, the new file).

## Done when
`grep -n 'voe_game_materials_cooked' editor/src/game_tree_materials.c` finds it, and the editor
builds. Human: Play on a project with a `.material` writes `Build/game/materials.c` holding it and
builds.
