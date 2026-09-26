# 07 — The coin game offers Player and Coin
folder: examples/coin_game
decisions: 0168, 0260, 0239

## Change
Read `examples/coin_game/Code/project.c`, `examples/capsule/Code/player.h`,
`examples/capsule/Code/coin.h`, `game/include/game/project.h`,
`ecs/include/ecs/component.h` (runtime-only), `3d/include/3d/shape_component.h`.

- `Code/player.h`, `Code/player_system.c` — `player` { `speed` 2, `jump_height` 1.2,
  `gravity` 9.81, `camera_distance` 6 (FLOAT32), `start_score` 1000, `score_drop` 10
  (INT32, points a second) }, menu "Player"; `player_register`. Header: the numbers the
  sponsor sets and why they sit here (0260). The system comes in card 09.
- `Code/coin.h`, `Code/coin_system.c` — `coin` { `points` INT32, default 100 }, menu
  "Coin"; and the runtime-only `coin_taken` { `voe_3d_shape shape`, `uint32_t restarts` }
  (plain struct, `&voe_ecs_runtime_only` taken at run time, menu NULL); `coin_register`
  registers both. Header: a taken coin is hidden and kept, not destroyed (0260 point 3). The
  system comes in card 10.
- `Code/project.c` — registers player and coin too.
- `Code/Code.md` — the four files.

If game refuses a runtime-only project type, stop: that is `game`'s card, not this one.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder examples/coin_game` prints
   `FINDINGS: 0`.
2. `cmake --build --preset debug --target voe_editor`, then in `p=$(mktemp -d)` with
   `cp -r examples/coin_game/. $p` and `rm -rf $p/Build`:
   `build/debug/editor/voe_editor --capture $p/shot.png $p 2>$p/err` exits 0, `$p/err` is
   empty, `$p/Build/editor/loaded/project-1.so` exists, and `cmake -S $p/Build/game -B
   $p/Build/debug -G Ninja && cmake --build $p/Build/debug --target game` exits 0.
