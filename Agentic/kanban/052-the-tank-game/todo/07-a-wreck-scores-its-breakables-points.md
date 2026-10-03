# 07 — A wreck the player makes carries its breakable's points
folder: examples/tank_game/Code
after: 06
decisions: 0168, 0296, 0334

## Change
0334 point 4. Read, in this folder, `tank_breakable.h`, `tank_breakable.c`, `tank_shell.h`,
`tank_shell_system.c` and `Code.md`.

- `examples/tank_game/Code/tank_breakable.h`: `F(int32_t, points, INT32)` after `wreck`.
  Header: what the player scores for wrecking it.
- `examples/tank_game/Code/tank_breakable.c`: the default row's `points` 100.
- `examples/tank_game/Code/tank_shell.h`: `int32_t points` last in `tank_shot`: the
  breakable's points when this shot swapped it, else 0; the header's shot paragraph says so.
- `examples/tank_game/Code/tank_shell_system.c`: at a swap that is not refused, the shot row
  (written whole, as now) carries the breakable's `points`; `tank_shell_fire` queues 0. Only
  the player's shots swap (0296), so only they carry points. The file header's swap paragraph.
- `examples/tank_game/Code/Code.md`: the breakable and shell entries say the points.

## Done when
`(for f in examples/tank_game/Code/*.c; do clang -std=c23 -fsyntax-only -DVOE_BASE_DESCRIPTIONS=1
$(for d in */include; do printf -- "-I%s " $d; done) "$f" || exit 1; done)` exits 0, and `grep -q points examples/tank_game/Code/tank_shell_system.c` exits 0.
