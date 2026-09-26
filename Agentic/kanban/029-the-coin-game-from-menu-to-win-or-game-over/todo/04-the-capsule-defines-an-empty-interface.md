# 04 — The capsule defines an empty interface
folder: examples/capsule
decisions: 0168, 0259

## Change
Read `game/include/game/project.h` for the fourth entry point. Never touch `main.scene`.

- `examples/capsule/Code/project.c` — defines `voe_game_project_interface`: asserts its frame,
  ends the ui frame, returns true (the showcase draws no interface). Header: four entry points,
  the interface once a frame after the steps.
- `examples/capsule/Code/Code.md` — `project.c`'s entry says four entry points.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder examples/capsule` prints
   `FINDINGS: 0`.
2. `cmake --build --preset debug --target voe_editor`, then in `p=$(mktemp -d)` with
   `cp -r examples/capsule/. $p` and `rm -rf $p/Build`:
   `build/debug/editor/voe_editor --capture $p/shot.png $p 2>$p/err` exits 0, `$p/err` is
   empty, and `cmake -S $p/Build/game -B $p/Build/debug -G Ninja && cmake --build
   $p/Build/debug --target game` exits 0.
