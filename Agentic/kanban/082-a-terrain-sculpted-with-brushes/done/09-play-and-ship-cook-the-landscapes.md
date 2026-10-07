# 09 — Play and Ship cook the landscapes
folder: editor
after: 06, 08
decisions: 0168, 0379

## Change
The game tree gains a fifth file, `landscapes.c`: every `.landscape` under `Assets/` as saved, cooked
(0379 point 7). Ends the configure failure card 08 left.

- New `editor/src/game_tree_find.h` / `game_tree_find.c` — game_tree.c's prefab finder (the
  `prefab_path*`, `prefab_folder*`, `prefabs_find` and path-order code) moved out unchanged in what it
  does, and taking the suffix (`.prefab`, `.landscape`) as a parameter: every file under `Assets/` with
  that ending in any case, hidden entries skipped, in byte order of path, with its explicit stack and
  depth. game_tree.c's prefabs use it. Header: what it finds and why a stack.
- New `editor/src/game_tree_landscapes.c`, declared in `game_tree.h` — the source of `landscapes.c`:
  `#include <game/landscapes.h>`; each found file read through `platform`'s file read in a scratch
  rewound after it, read by `voe_assets_landscape_read` and cooked by `voe_authoring_landscape_cook`
  (authoring/landscape_cook.h) as `landscape_<n>`; then `voe_game_landscapes_cooked` naming each by its
  project-relative path (`Assets/…`), its size as `%a`, its cells and its array; none is `{ NULL, 0 }`.
  A file that will not read or parse refuses the write naming it, as a bad prefab does.
- `editor/src/game_tree.c` — `game_files_write` writes `landscapes.c` beside `prefabs.c`, only when its
  bytes differ.
- `editor/src/game_tree.h` — the layout names five files and `landscapes.c`'s rule in a paragraph like
  prefabs.c's.
- `editor/src/src.md` — entries for the two new files and game_tree's updated.

## Done when
`grep -c voe_game_landscapes_cooked editor/src/game_tree_landscapes.c` prints 1 or more, and the folder's
check passes.
Human: in a project with a saved `Assets/Hill.landscape` (card 15 makes one; before it, any file in
0379's format), press Play: the game starts, and `Build/game/landscapes.c` names `Assets/Hill.landscape`.
