# 07 — The game tree carries the project's `.wav` files
folder: cmake
decisions: 0168, 0235, 0237, 0264, 0266

## Change
The game reads a sound beside its program (0266 point 3): `Build/debug/` in Play,
`Build/ship/<name>/` shipped. The game tree copies and installs them.

- `cmake/game.cmake`, in the executable branch only (not `VOE_GAME_LIBRARY`):
  - The project folder is `${CMAKE_CURRENT_SOURCE_DIR}/../..`, normalised (the tree is always
    `<project>/Build/game/`, 0235).
  - `file(GLOB_RECURSE … CONFIGURE_DEPENDS "<project>/*.wav")`, dropping every path under
    `<project>/Build/` or `<project>/Cache/`.
  - Each kept file: an `add_custom_command` whose output is the same relative path under
    `CMAKE_BINARY_DIR` (the program's folder), `copy_if_different`, depending on the source;
    all of them one `game_sounds` target that `game` depends on, so a changed file is copied
    again by the next build and a new one is found by it.
  - Each kept file: `install(FILES …)` into its relative folder under the install root.
  - The top comment gains: the sounds copied beside the program and installed with it, why
    by relative path, and that `Build/` and `Cache/` are skipped.
- `cmake/cmake.md` — the `game.cmake` entry mentions the sounds.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder cmake` prints `FINDINGS: 0`.
2. `cmake --build --preset debug --target voe_editor`, then in `p=$(mktemp -d)` with
   `cp -r examples/coin_game/. $p`, `rm -rf $p/Build`, `mkdir -p $p/Sounds` and
   `printf RIFF > $p/Sounds/a.wav`:
   `build/debug/editor/voe_editor --capture $p/shot.png $p 2>$p/err` exits 0 (writes
   `$p/Build/game/`); `mkdir -p $p/Build/x && printf RIFF > $p/Build/x/b.wav`; then
   `cmake -S $p/Build/game -B $p/Build/release -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build $p/Build/release --target game && cmake --install $p/Build/release --prefix $p/out`
   exits 0, `$p/Build/release/Sounds/a.wav` and `$p/out/Sounds/a.wav` exist, and
   `find $p/out $p/Build/release -name b.wav` prints nothing.
