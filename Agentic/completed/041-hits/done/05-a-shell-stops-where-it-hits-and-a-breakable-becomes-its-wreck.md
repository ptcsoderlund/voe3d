# 05 — A shell stops where it hits and a breakable becomes its wreck
folder: examples
after: 04
decisions: 0168, 0293, 0294

## Change
0294 points 2 and 3. Files under `examples/tank_game/`: `Code/tank_shell.h`,
`Code/tank_shell_system.c`, new `Code/tank_breakable.h` and
`Code/tank_breakable.c`, `Code/project.c`, `Code/Code.md`, `tank_game.md`.
Read `physics/include/physics/sweep.h`, `game/include/game/project.h`,
`Code/tank_spawner.h` for an authored component with a prefab name, and
`scene/include/scene/transform_component.h` for a world place. Never edit
`main.scene` or `Assets/tank_body.prefab` (0272).

- `tank_breakable.h` / `.c`: `tank_breakable`, "Tank / Breakable",
  `VOE_GAME_WORLD_AUTHORED` rows, one field `wreck` (a prefab name, 64
  bytes, default empty); its key and register. No system: the shell system
  swaps. Header: what a hit does to it; a wreck carries none.
- `tank_shell.h`: `tank_shell` gains `radius`, metres, default 0.1.
  Header: the sweep, the stop, the swap.
- `tank_shell_system.c`: each step, gather the obstacles once into a local
  array of `VOE_GAME_WORLD_MAX_DRAWN`. Per shell with life left: `to` is
  its position plus its -Z times speed times seconds; sweep from its shot
  row's `from` (its position with no row) to `to` with its radius,
  ignoring the row's `owner` (itself with no row). On a hit: the row gets
  `hit` and `target`, the shell is queued for removal and not moved; if
  the target has a breakable not yet swapped this step (a local list),
  spawn its `wreck` at the target's world place and, only when that is
  not refused, remove the target. With no hit: move as today, and the row's
  `from` becomes `to`. Rows written whole through
  `voe_ecs_component_set`.
- `project.c`: register the breakable; header lists it.
- `Code.md`, `tank_game.md`: the breakable and the stopping shell, each
  entry under 300 characters.

## Done when
`(for f in examples/tank_game/Code/*.c; do clang -std=c23 -fsyntax-only -DVOE_BASE_DESCRIPTIONS=1
$(for d in */include; do printf -- "-I%s " $d; done) "$f" || exit 1; done)` exits 0, and `grep -q voe_physics_sweep examples/tank_game/Code/tank_shell_system.c`
exits 0.
