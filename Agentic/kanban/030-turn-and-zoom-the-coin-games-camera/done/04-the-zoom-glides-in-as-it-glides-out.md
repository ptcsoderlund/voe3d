# 04 — The zoom glides in as it glides out
folder: examples/coin_game
decisions: 0168, 0261, 0262

## Change
Read `examples/coin_game/Code/player_camera.h`, `Code/player_camera_system.c`,
`Code/Code.md`.

- `Code/player_camera_system.c` — the spring arm in `player_camera_run` follows 0262: the
  length tested is max(arm, distance), not distance; clear is that whole length or the
  bisected longest clear length over [0, it]; clear shorter than `arm` sets `arm` to clear at
  once; otherwise `arm` moves towards min(distance, clear) by at most
  `PLAYER_CAMERA_ARM_SPEED` × seconds in either direction (today it only limits growth, so a
  shorter wheel distance snaps). The existing overlap helper and bisection stay as they are.
  Header: snaps only when something is in the way of where the camera now is; glides both
  ways otherwise; why the test reaches the current arm (no wall inside the glide in).
- `Code/player_camera.h` — header's spring-arm paragraph: the zoom glides nearer and farther
  at 10 m/s; in at once only for something solid in the way (0262).
- `Code/Code.md` — player_camera_system.c's entry: the arm glides to the wheel's distance
  both ways and snaps in only when blocked.

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
4. The human's: `bugs/01-zoom-in-jumps.md` `## How to reproduce` steps 1 to 4 (scrolling in
   glides like scrolling out; the wall still snaps the camera in at once), then `## How to
   test` steps 4 and 6 of `feature.md` in the editor on `examples/coin_game/`.
