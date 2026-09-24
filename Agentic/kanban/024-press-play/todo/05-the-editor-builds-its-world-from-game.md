# 05 — The editor builds every project's world through `game`
folder: editor
decisions: 0168, 0237

## Change
The editor's world and the game's are one list (0237 point 2). No behaviour changes.

- `editor/CMakeLists.txt` — `game` added to `voe_executable(editor DEPENDS …)`.
- `editor/src/project.c` — `world_new` and the defines only it used give way to
  `voe_game_world_new` from `<game/world.h>`; the file header's "`world_new()` registers the eight
  component types" names `game/world.h` instead. A `static_assert` that `VOE_EDITOR_SCENE_ROWS`
  (scene.h) is no more than `VOE_GAME_WORLD_AUTHORED`, with the reason (every row is an authored
  entity the world must hold).
- `editor/src/project.h` — `VOE_EDITOR_PROJECT_MAX_DRAWN` becomes `VOE_GAME_WORLD_MAX_DRAWN`; its
  comment says where the number now lives.
- `editor/src/interface.h` — in the Inspector paragraph, "project.c's world_new is the only place
  that decides it" names `game/world.h`; the numbers do not change.
- `editor/src/src.md` — the `project.c` entry drops "the one `world_new` every world is built by".

Read the headers of the files above and, in `project.c`, only `world_new`, the defines above it and
its callers.

## Done when
The Checks line of `CLAUDE.md` with `{folder}` = `editor` exits 0, and with `XDG_CONFIG_HOME` at an
empty `mktemp -d` folder `build/debug/editor/voe_editor --capture <that folder>/a.png --size
1280x720` exits 0 and writes the untitled cube as before.
