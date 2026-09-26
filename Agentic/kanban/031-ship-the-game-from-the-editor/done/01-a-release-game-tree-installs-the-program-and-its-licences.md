# 01 — A release game tree installs the program and its licences
folder: cmake
decisions: 0168, 0235, 0237, 0264

## Change
- `cmake/game.cmake`, in the executable branch only (not `VOE_GAME_LIBRARY`):
  - `VOE_GAME_NAME` defaults to `game` when unset (the tree's `CMakeLists.txt` sets it from
    card 02 on).
  - When `CMAKE_BUILD_TYPE` is `Release` and not `WIN32`: `target_link_options(game PRIVATE -s)`.
  - Install rules: the program `$<TARGET_FILE:game>` as `${VOE_GAME_NAME}${CMAKE_EXECUTABLE_SUFFIX}`
    into the install root (`install(PROGRAMS … RENAME …)`, destination `.`);
    `${VOE_ENGINE}/LICENSE` as `voe3d-LICENSE.txt` and `${VOE_ENGINE}/text/fonts/OFL.txt` as
    `Oxanium-OFL.txt`, same destination.
  - The top comment gains: Release is what Ship builds (0264), stripped off Windows; what
    `cmake --install` puts in the shipped folder and why nothing else is needed at run time
    (shaders and font embedded); `VOE_GAME_NAME`.
- `cmake/cmake.md` — the `game.cmake` entry mentions the release install.

If a folder fails to compile under Release (`NDEBUG`, `-O3`, `-Werror`), do not fix it here:
block the card naming the folder and the warning.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder cmake` prints `FINDINGS: 0`.
2. `cmake --build --preset debug --target voe_editor`, then in `p=$(mktemp -d)` with
   `cp -r examples/coin_game/. $p` and `rm -rf $p/Build`:
   `build/debug/editor/voe_editor --capture $p/shot.png $p 2>$p/err` exits 0 (writes
   `$p/Build/game/`), then
   `cmake -S $p/Build/game -B $p/Build/release -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build $p/Build/release --target game && cmake --install $p/Build/release --prefix $p/out`
   exits 0, `ls $p/out` shows `game`, `voe3d-LICENSE.txt`, `Oxanium-OFL.txt`, and
   `file $p/out/game | grep -c debug_info` prints 0.
