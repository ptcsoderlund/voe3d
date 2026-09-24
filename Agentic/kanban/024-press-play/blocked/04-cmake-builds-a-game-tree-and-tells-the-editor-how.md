# 04 — cmake builds a game tree, and tells the editor the tools it was built with
folder: cmake
decisions: 0168, 0145, 0235, 0237

## Change
Only `cmake/` (0237 points 3 and 4). Nothing in the editor uses it yet (cards 05, 06).

- `cmake/voe.cmake`
  - `option(VOE_FOLDER_DATABASES … ON)` beside `VOE_BASE_DESCRIPTIONS`, with a comment: a game tree
    built from this source must not write databases into the engine's folders.
    `voe_export_compile_commands` does nothing when it is OFF (the root's own call included).
  - The `editor` row gains `game`, and its comment says why (the one world type list).
  - `voe_executable`: when the folder is `editor`, a new function `voe_editor_toolchain(<target>)`
    writes `${CMAKE_BINARY_DIR}/generated/editor/toolchain.h` (at configure time, rewritten only
    when its bytes change) and adds that folder as a PRIVATE include of the target. The header
    defines one C string literal each: the engine source root (the folder above `cmake/`),
    `CMAKE_COMMAND`, `CMAKE_C_COMPILER`, `CMAKE_MAKE_PROGRAM`, `PKG_CONFIG_EXECUTABLE`,
    `VOE_SLANGC`, `VOE_WAYLAND_SCANNER`; `\` and `"` escaped; an unset value is `""`. Name them
    `VOE_TOOLCHAIN_*`. The function's comment: the editor passes these to a game tree's configure,
    so the game is built with the tools this build was (0237).
- `cmake/game.cmake` — new; included by a game tree's `CMakeLists.txt` after it sets `VOE_ENGINE` to
  the engine source root. It turns `VOE_BASE_DESCRIPTIONS` and `VOE_FOLDER_DATABASES` off (as
  normal variables before `include(${VOE_ENGINE}/cmake/voe.cmake)`, which CMP0077 lets win over the
  options), makes `CMAKE_BUILD_TYPE` `Debug` when it is empty, adds `${VOE_ENGINE}/game` with the
  binary folder `${CMAKE_BINARY_DIR}/voe_game` (not `game`, which is the program's file), and
  builds the executable `game` from the tree's own `main.c` and `scene.c`, linking `voe::game`,
  through `voe_target_settings`. The file's comment: what a game tree is (0235), why descriptions
  and databases are off, and that milestone 3 adds the project's own C here.

## Done when
1. `cmake --preset debug` exits 0 and `build/debug/generated/editor/toolchain.h` holds a non-empty
   engine root, CMake, compiler and make program.
2. In `t=$(mktemp -d)`: a `$t/src/CMakeLists.txt` (minimum 3.28, `project(scratch_game C)`,
   `VOE_ENGINE` set to this repository, `include(${VOE_ENGINE}/cmake/game.cmake)`), a `main.c`
   returning `voe_game_run("scratch")` from `<game/run.h>`, and a `scene.c` including
   `<game/scene.h>` whose `voe_game_scene_build` returns true.
   `cmake -S $t/src -B $t/b -G Ninja` with `-DCMAKE_C_COMPILER`, `-DCMAKE_MAKE_PROGRAM`,
   `-DPKG_CONFIG_EXECUTABLE`, `-DVOE_SLANGC` and `-DVOE_WAYLAND_SCANNER` from `toolchain.h`, then
   `cmake --build $t/b --target game`, exits 0 and `$t/b/game` exists.
3. After step 2, `git status --porcelain` lists no `compile_commands.json`.
4. `cmake -P check.cmake` exits 0.

## Blocked
The change is complete and all four `## Done when` steps pass (the scratch game tree builds `game`, no
engine folder's `compile_commands.json` is touched, `check.cmake` exits 0). `checks.sh --folder cmake`
leaves one finding: CLAUDE.md's per-folder check builds `voe_$top`, and there is no `voe_cmake` target.
Unblock by having the per-folder check skip (or special-case) `cmake/`, or by accepting this finding.
