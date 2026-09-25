# 23 — The library configure names the editor's import library, and the library is `project.dll`
folder: editor
decisions: 0168, 0242, 0243, 0245

## Change
Points 3 and 4 of 0245, the editor's half. Read the headers of `voe_editor_toolchain` in
`cmake/voe.cmake`, `editor/src/game_tree.h` and `editor/src/code.h`.

- `cmake/voe.cmake`, `voe_editor_toolchain` — a new name `EDITOR_IMPORTS` in the list it writes
  to `toolchain.h`: on `WIN32`, the import library card 18's link writes,
  `${CMAKE_CURRENT_BINARY_DIR}/voe_editor${CMAKE_IMPORT_LIBRARY_SUFFIX}`; empty elsewhere. The
  function's header names it.
- `editor/src/game_tree.c` — the LIBRARY configure adds `VOE_EDITOR_IMPORTS` from
  `VOE_TOOLCHAIN_EDITOR_IMPORTS` through the same `define_add` the tools use, so it is left out
  when empty (Linux). `voe_editor_game_tree_library` returns `Build/editor/project.dll` on
  `_WIN32`, `Build/editor/libproject.so` else, chosen as `code.c` chooses its extension.
- `editor/src/game_tree.h` — the TWO KINDS paragraph: LIBRARY builds `project.dll` on Windows
  and links the editor's import library (0245); "Linux only" goes. The comment on
  `voe_editor_game_tree_library` and on the configure name both.
- `editor/src/src.md` — only if the `game_tree` entries name the file or the platform.

## Done when
1. `checks.sh --folder editor` prints `FINDINGS: 0`.
2. `grep -c EDITOR_IMPORTS build/debug/generated/editor/toolchain.h` prints 1 and the line
   defines `""`.
3. In `p=$(mktemp -d)` with `cp -r examples/capsule/. $p`:
   `build/debug/editor/voe_editor --capture $p/shot.png $p 2>$p/err` exits 0, `$p/err` is empty,
   `$p/Build/editor/loaded/project-1.so` exists, and
   `grep -c VOE_EDITOR_IMPORTS $p/Build/editor/CMakeCache.txt` prints 0.
