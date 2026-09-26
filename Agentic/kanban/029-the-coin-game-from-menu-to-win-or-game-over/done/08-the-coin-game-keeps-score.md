# 08 — The coin game keeps score
folder: examples/coin_game
decisions: 0168, 0260, 0254

## Change
Read `examples/coin_game/Code/player.h`, `Code/coin.h`, `Code/project.c`,
`ecs/include/ecs/structure.h`, `ecs/include/ecs/component.h`.

- `Code/game_state.h`, `Code/game_system.c` (new) — the runtime-only `game_state` { phase
  (menu, playing, won, lost as named constants), `dropped` INT32, `banked` seconds,
  `restarts`, `enter_down`, `escape_down` }, menu NULL; `game_state_register`.
  - `const game_state *game_state_get(const voe_ecs_world *)` — the one row, NULL before it is
    made.
  - `int32_t game_score(const voe_ecs_world *)` — first player's `start_score` − dropped +
    points of coins whose `coin_taken` has the current restarts, at least 0.
  - `uint32_t game_coins_left(const voe_ecs_world *)` — coins without such a row.
  - `game_system_run(world, seconds)` — no row yet: adds one (phase menu) to the first player
    entity through the structural queue. Playing: banks seconds, each whole second adds
    `score_drop` to dropped; then score 0 is lost, no coins left (of at least one) is won.
  Header: one owner, this module, writes the row, here and in the interface (card 11); the
  score is derived, so a take needs no message.
- `Code/project.c` — registers game_state; runs game first before the move.
- `Code/Code.md` — the two files.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder examples/coin_game` prints
   `FINDINGS: 0`.
2. `cmake --build --preset debug --target voe_editor`, then in `p=$(mktemp -d)` with
   `cp -r examples/coin_game/. $p` and `rm -rf $p/Build`:
   `build/debug/editor/voe_editor --capture $p/shot.png $p 2>$p/err` exits 0, `$p/err` is
   empty, `$p/Build/editor/loaded/project-1.so` exists, and `cmake -S $p/Build/game -B
   $p/Build/debug -G Ninja && cmake --build $p/Build/debug --target game` exits 0.
