# 02 — The game tree knows a release build and the shipped folder
folder: editor
decisions: 0168, 0237, 0242, 0264

## Change
- `editor/src/game_tree.h` and `game_tree.c`:
  - A third kind, `VOE_EDITOR_GAME_TREE_RELEASE`: binary folder `<folder>/Build/release/`,
    configured with `-DCMAKE_BUILD_TYPE=Release` (GAME and LIBRARY keep Debug), built with the
    target `game`. `voe_editor_game_tree_configured`, `_configure` and `_build` take it.
  - The written `CMakeLists.txt` sets `VOE_GAME_NAME` to the project's name
    (`voe_editor_project_name`), escaped as a CMake argument the way the engine path is, before
    `game.cmake` is included.
  - `const char *voe_editor_game_tree_shipped(const char *folder, voe_base_arena *arena)` —
    `<folder>/Build/ship/<name>`, `<name>` the folder's last component
    (`voe_platform_path_name`).
  - `const char *const *voe_editor_game_tree_ship_clear(const char *folder, voe_base_arena *arena)`
    — the NULL-terminated list running the toolchain's CMake as `-E rm -rf <shipped>`.
  - `const char *const *voe_editor_game_tree_install(const char *folder, voe_base_arena *arena)` —
    `cmake --install <folder>/Build/release --prefix <shipped>`.
  - The header's layout and kinds paragraphs gain RELEASE, the shipped folder and why the old
    one is removed only after a good build (0264 point 3); the "build is Debug" paragraph says
    Ship's is Release.
- `editor/src/src.md` — the `game_tree.h` and `.c` entries mention the release kind and the
  shipped folder.

Nothing calls the new functions yet; card 03 does.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder editor` prints `FINDINGS: 0`.
2. `cmake --build --preset debug --target voe_editor`, then in `p=$(mktemp -d)` with
   `cp -r examples/coin_game/. $p` and `rm -rf $p/Build`:
   `build/debug/editor/voe_editor --capture $p/shot.png $p 2>$p/err` exits 0 and
   `grep -q VOE_GAME_NAME $p/Build/game/CMakeLists.txt` exits 0.
