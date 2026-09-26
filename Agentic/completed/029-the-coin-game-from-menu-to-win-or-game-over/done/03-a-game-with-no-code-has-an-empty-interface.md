# 03 — A game with no code has an empty interface
folder: cmake
decisions: 0168, 0259, 0245, 0257

## Change
Read `game/include/game/project.h` for the fourth entry point.

- `cmake/game.cmake` — the no-code stub defines `voe_game_project_interface`: ends the frame
  (`voe_ui_frame_end`, its answer discarded with a cast) and returns true; includes
  `<ui/layout.h>`. The file's top comment says four entry points.
- `cmake/exports.cmake` — the skip pattern and the header's list gain
  `voe_game_project_interface`.
- `cmake/cmake.md` — only if an entry's words go stale.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder cmake` prints `FINDINGS: 0`.
2. `cmake --build --preset debug --target voe_editor`, then in `p=$(mktemp -d)` with
   `cp -r examples/capsule/. $p`, `rm -rf $p/Build $p/Code`:
   `build/debug/editor/voe_editor --capture $p/shot.png $p 2>$p/err` exits 0 and then
   `cmake -S $p/Build/game -B $p/Build/debug -G Ninja && cmake --build $p/Build/debug --target
   game` exits 0 (the stub compiles).
