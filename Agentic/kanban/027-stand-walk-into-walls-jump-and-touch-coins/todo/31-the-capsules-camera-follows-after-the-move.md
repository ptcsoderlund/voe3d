# 31 — The capsule's camera follows after the move
folder: examples/capsule
decisions: 0168, 0256, 0257, 0254
read: feature.md

## Change
Closes bug 01: the camera trailed the capsule by one step because the follow ran before the
body's move. Read `game/include/game/project.h` for the third entry point.

- `examples/capsule/Code/project.c` — `voe_game_project_systems_run` runs keyboard, player, coin;
  the new `voe_game_project_systems_after_move` runs follow (same step assert). Header: two
  slots, what goes in each and why the follow is after the move; the constraint's order per
  slot.
- `examples/capsule/Code/follow_camera.h` — the usage line says after the move; the constraint
  that the follower trails by a step becomes: it reads the target where this step's move left it.
- `examples/capsule/Code/Code.md` — `project.c` entry: three entry points, follow after the move.
- No change to `follow_camera_system.c`: it already reads the target's current transform.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder examples/capsule` prints
   `FINDINGS: 0`.
2. `cmake --build --preset debug --target voe_editor`, then in `p=$(mktemp -d)` with
   `cp -r examples/capsule/. $p` and `rm -rf $p/Build`:
   `build/debug/editor/voe_editor --capture $p/shot.png $p 2>$p/err` exits 0, `$p/err` is
   empty, and then `cmake -S $p/Build/game -B $p/Build/debug -G Ninja && cmake --build
   $p/Build/debug --target game` exits 0 (the game Play builds links all three entry points).
3. `bash ~/.claude/skills/checks/scripts/checks.sh --all` prints `FINDINGS: 0`.
4. The human's: open `examples/capsule/` in the editor, Play; start, walk, stop, jump and land:
   the capsule holds still against the camera throughout (bug 01, feature step 15).
