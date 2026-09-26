# 10 — A coin is taken and put back
folder: examples/coin_game
decisions: 0168, 0260, 0253, 0193

## Change
Read `examples/coin_game/Code/coin.h`, `Code/game_state.h`, `Code/player.h`,
`Code/project.c`, `examples/capsule/Code/coin_system.c` (the overlap),
`ecs/include/ecs/structure.h`, `3d/include/3d/shape_component.h`.

- `Code/coin.h`, `Code/coin_system.c` — `coin_system_run(world)`, for each coin with a
  transform:
  - a `coin_taken` under an older restart count: its shape added back (if it had one) and the
    row removed, both through the structural queue;
  - taken under the current count: skipped;
  - else, while playing, its trigger's overlap finds an entity with player: its Shape removed
    and `coin_taken` { that shape, current restarts } added.
  Header: why hidden and not destroyed; the score follows from the rows (game_state.h).
- `Code/project.c` — before the move: game, player, coin, rotator.
- `Code/Code.md` — the coin entries.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder examples/coin_game` prints
   `FINDINGS: 0`.
2. `cmake --build --preset debug --target voe_editor`, then in `p=$(mktemp -d)` with
   `cp -r examples/coin_game/. $p` and `rm -rf $p/Build`:
   `build/debug/editor/voe_editor --capture $p/shot.png $p 2>$p/err` exits 0, `$p/err` is
   empty, `$p/Build/editor/loaded/project-1.so` exists, and `cmake -S $p/Build/game -B
   $p/Build/debug -G Ninja && cmake --build $p/Build/debug --target game` exits 0.
