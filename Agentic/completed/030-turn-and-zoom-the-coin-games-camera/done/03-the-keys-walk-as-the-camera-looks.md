# 03 — The keys walk as the camera looks
folder: examples/coin_game
decisions: 0168, 0261, 0260
read: feature.md

## Change
Read `examples/coin_game/Code/player.h`, `Code/player_system.c`, `Code/player_camera.h`,
`Code/Code.md`.

- `Code/player_system.c` — the walk reads the `player_camera_state` row (read only; 0261
  point 6): forward is (−sin yaw, 0, −cos yaw), W along it, S against it, A and D to its left
  and right; no row yet, yaw 0, so W is −Z as before. Header: the walk follows the camera.
- `Code/player.h` — header: the keys walk as the camera looks.
- `Code/Code.md` — player_system.c's entry says so.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder examples/coin_game` prints
   `FINDINGS: 0`.
2. `cmake --build --preset debug --target voe_editor`, then in `p=$(mktemp -d)` with
   `cp -r examples/coin_game/. $p` and `rm -rf $p/Build`:
   `build/debug/editor/voe_editor --capture $p/shot.png $p 2>$p/err` exits 0,
   `grep -v -e camera_turn_speed -e camera_distance_min -e camera_distance_max $p/err` prints
   nothing, `$p/Build/editor/loaded/project-1.so` exists, and `cmake -S $p/Build/game -B
   $p/Build/debug -G Ninja && cmake --build $p/Build/debug --target game` exits 0.
3. `bash ~/.claude/skills/checks/scripts/checks.sh --all` prints `FINDINGS: 0`.
4. The human's: every step of `## How to test` in `feature.md`, 1 to 8, in the editor on
   `examples/coin_game/` (step 6 needs a wall the sponsor places).
