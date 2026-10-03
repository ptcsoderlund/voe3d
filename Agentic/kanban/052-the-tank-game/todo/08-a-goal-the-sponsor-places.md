# 08 — A goal the sponsor places at the level's end
folder: examples/tank_game/Code
after: 07
decisions: 0168, 0303, 0334

## Change
0334 point 5. Read, in this folder, `tank_breakable.h` and `tank_breakable.c` (a component
with no system: the pattern), `tank_spawner_system.c`'s register (a need of a transform,
0303), `project.c` and `Code.md`.

- `examples/tank_game/Code/tank_goal.h`, new: `F(int32_t, points, INT32)` described as
  `tank_goal`, its key, `tank_goal_register(world)`. Header: the player wins when its hull's z
  reaches the goal's world z, the level running along −Z; `points` is added then; the state
  system reads it; no system; needs a transform.
- `examples/tank_game/Code/tank_goal.c`, new: registers it under "Tank / Goal", default
  `points` 1000, capacity `VOE_GAME_WORLD_AUTHORED`, needing the transform.
- `examples/tank_game/Code/project.c`: registers the goal; the header's register list.
- `examples/tank_game/Code/Code.md`: entries for both new files; the project entry's count.

## Done when
`(for f in examples/tank_game/Code/*.c; do clang -std=c23 -fsyntax-only -DVOE_BASE_DESCRIPTIONS=1
$(for d in */include; do printf -- "-I%s " $d; done) "$f" || exit 1; done)` exits 0, and `grep -q tank_goal_register examples/tank_game/Code/project.c` exits 0.
