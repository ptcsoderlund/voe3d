# 09 — A game tree builds from the engine source, proven under the game folder
folder: game
decisions: 0168, 0145, 0235, 0237

## Change
Card 04's change to `cmake/` is in HEAD (commit edaaa63) and its steps passed; it was blocked only
because `checks.sh --folder cmake` runs CLAUDE.md's per-folder build of `voe_cmake`, which no
folder defines. This card proves that change from `game`, the folder `cmake/game.cmake` builds,
so `checks.sh --folder game` is the gate. Cards 05–08 already have what they need from it.

Read the headers of `cmake/game.cmake` and of `voe_editor_toolchain` in `cmake/voe.cmake`, and
`game/game.md`. Change nothing unless a step below fails; a fix then goes in `cmake/voe.cmake` or
`cmake/game.cmake` (the build files `--folder` allows beside a card naming a decision), never in
`cmake/cmake.md`, which `--folder game` counts as outside the folder.

- `game/game.md` — its opening paragraph gains that a game tree builds this folder through
  `cmake/game.cmake` (0235). No entry changes.

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
4. `checks.sh --folder game` prints `FINDINGS: 0`.
