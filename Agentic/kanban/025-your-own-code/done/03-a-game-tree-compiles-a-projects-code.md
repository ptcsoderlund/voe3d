# 03 — A game tree compiles a project's code, into the game or into a library
folder: game
decisions: 0168, 0175, 0235, 0242

## Change
Points 2 and 4 of 0242 in `cmake/game.cmake`, the build file `--folder` allows beside a card naming
a decision. Read its header, `voe_target_settings` in `cmake/voe.cmake`, and
`game/include/game/project.h` (card 02).

- `cmake/game.cmake`:
  - `VOE_PROJECT_CODE`, set by the tree's CMakeLists.txt before the include, is a folder; its `*.c`
    are globbed with `CONFIGURE_DEPENDS` (a file added later is picked up by the next build) and
    the folder goes on the include path. Unset, missing or holding no `.c`: a `no_code.c` defining
    the two entry points empty is written into `CMAKE_BINARY_DIR` with `file(CONFIGURE ...)`, so its
    bytes are rewritten only when they change, and compiled instead.
  - `VOE_GAME_LIBRARY` OFF (the default): as today, the executable `game` from main.c, scene.c and
    the code, descriptions off.
  - `VOE_GAME_LIBRARY` ON: on `WIN32`, `FATAL_ERROR` saying library mode is Linux-only for now.
    Else descriptions on (set before voe.cmake is included, as the OFF is now), no `game`, and a
    SHARED `project` from the code alone, `$<COMPILE_ONLY:voe::game>`, `voe_target_settings`,
    position-independent, built into `CMAKE_BINARY_DIR` under CMake's own name for it
    (`libproject.so`). No engine code is linked into it; its engine symbols stay undefined (if
    `voe_target_settings` adds a no-undefined link flag, drop it for this target only).
  - The header: both modes, the variables, what no_code.c is, that the editor builds the target
    `project` and that nothing else of the engine need be built in library mode.
- `game/game.md` — the opening paragraph gains that a tree compiles a project's `Code/` too
  (0242). No entry changes.

## Done when
1. `cmake --preset debug` exits 0; the tool values below come from
   `build/debug/generated/editor/toolchain.h`.
2. In `t=$(mktemp -d)`: `$t/src/` as 024 card 09 step 2 made it (CMakeLists.txt including
   `game.cmake`, main.c, scene.c), plus `set(VOE_PROJECT_CODE ${CMAKE_CURRENT_SOURCE_DIR}/Code)`
   before the include, and `$t/src/Code/spin.c` defining both entry points, the register calling
   `voe_game_project_component` for a one-float described struct `spin`.
   `cmake -S $t/src -B $t/g -G Ninja` with `-DCMAKE_C_COMPILER`, `-DCMAKE_MAKE_PROGRAM`,
   `-DPKG_CONFIG_EXECUTABLE`, `-DVOE_SLANGC` and `-DVOE_WAYLAND_SCANNER`, then
   `cmake --build $t/g --target game`, exits 0.
3. The same source, `-B $t/l -DVOE_GAME_LIBRARY=ON`, `cmake --build $t/l --target project`, exits 0;
   `nm -D --defined-only $t/l/libproject.so` lists `voe_game_project_register` and
   `nm -D --undefined-only` lists `voe_game_project_component`.
4. `rm -r $t/src/Code`, reconfigure `$t/g`, `cmake --build $t/g --target game` exits 0.
5. `git status --porcelain` lists no `compile_commands.json`.
6. `checks.sh --folder game` prints `FINDINGS: 0`.
