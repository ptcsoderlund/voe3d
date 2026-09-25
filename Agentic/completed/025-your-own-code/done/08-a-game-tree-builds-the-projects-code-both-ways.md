# 08 — The editor's game tree builds a project's code, as the game and as a library
folder: editor
decisions: 0168, 0235, 0240, 0242

## Change
Points 4 and 8 of 0242 on the editor's side. Read the headers of `editor/src/game_tree.h`,
`editor/src/play.h`, `cmake/game.cmake` (card 03), `platform/process.h` (card 01),
`platform/folder.h` and `platform/file.h`.

- `editor/src/game_tree.h`, `editor/src/game_tree.c`:
  - `voe_editor_game_tree_kind` {GAME, LIBRARY}; `_configured`, `_configure` and `_build` take it:
    GAME is today's (`Build/debug`, target `game`); LIBRARY configures the same `Build/game` into
    `Build/editor` with `-DVOE_GAME_LIBRARY=ON` and builds the target `project`.
  - `const char *voe_editor_game_tree_library(folder, arena)` — `Build/editor/libproject.so`.
  - `const char *voe_editor_game_tree_log(folder, arena)` — `Build/build.log`.
  - `bool voe_editor_game_tree_has_code(folder, arena)` — `<folder>/Code/` lists a `.c`.
  - The written CMakeLists.txt sets `VOE_PROJECT_CODE` to the project's `Code/` before including
    game.cmake; scene.c is `#include "<name>.h"` for every `.h` in `Code/`, sorted by name, then
    the cooked text (the cook names project structs and keys, 0242 point 1). Both still written only
    when their bytes change.
  - The header: THE LAYOUT gains `Build/editor/`, `Build/build.log` and `Code/`; the two kinds.
- `editor/src/play.c`, `editor/src/play.h` — play passes GAME; `_start` empties the log (a zero-byte
  write) and every configure and build step sends its output there; the game itself keeps the shared
  output. A failed step's one stderr line names the log. The header's paragraph on a failed build
  says where the compiler's words are now.
- `editor/src/src.md` — game_tree's and play's entries if their sentence changes.

## Done when
1. `cmake --preset debug` exits 0; tool values from `build/debug/generated/editor/toolchain.h`.
2. `checks.sh --folder editor` prints `FINDINGS: 0`.
3. `grep -q VOE_PROJECT_CODE editor/src/game_tree.c && grep -q VOE_GAME_LIBRARY editor/src/game_tree.c`
   exits 0. What the tree builds is proven by card 09, the first to run it with code.
