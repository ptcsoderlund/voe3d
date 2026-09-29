# 08 — Enemies fire at the player
folder: examples
after: 06, 07
decisions: 0168, 0294
read: feature.md

## Change
0294 point 5, and the last card of 041. Files under `examples/tank_game/`:
`Code/tank_enemy.h`, `Code/tank_enemy_system.c`, `Code/project.c`,
`Code/Code.md`, `tank_game.md`. Read `Code/tank_gun.h` (fields of a gun),
`Code/tank_shell.h` (`tank_shell_fire`) and `Code/tank_lives.h` (07).
Never edit `main.scene` or `Assets/tank_body.prefab` (0272).

- `tank_enemy.h`: `tank_enemy` gains `prefab` (64 bytes, default
  `shell`), `rate` (shots a second, default 0.5), `range` (metres,
  default 30), `muzzle` (float3, default (0, 0.5, −3)) and read-only
  `wait`. Header points: whom it fires at, the aim along the level, the
  muzzle's frame, the turret not turning yet (0294 point 5).
- `tank_enemy_system.c`: each step, the target is the `tank_lives` row's
  entity and its world position; none, nothing fires. Per enemy with life
  left: `wait` counts down; when spent, the target within `range` and the
  way to it flattened onto the ground not nought: the fire rotation is the
  turn about +Y whose −Z points that way, the muzzle is turned by it and
  added to the enemy's position, and `tank_shell_fire` spawns at the muzzle
  with `owner` the enemy and `from` its position; `wait` becomes 1 /
  `rate` only when the fire is not refused. A rate at or below 0 never
  fires. The row stays written whole, once a step.
- `project.c`, `Code.md`, `tank_game.md`: the enemy fires; entries under
  300 characters.

## Done when
, and `grep -q tank_shell_fire examples/tank_game/Code/tank_enemy_system.c`
exits 0.

For the human, before `## How to test` in `feature.md`, in the editor on
`examples/tank_game`: open `Assets/tank_body.prefab`, give the hull
(`tank_body`) a box Collider of size (4.64, 4, 2.62), save; in
`main.scene` place a `house` from Assets at y 1.5, a wall (cube shape and
box collider, about 6 × 3 × 0.3 m) and a thin post (about 0.1 × 3 × 0.1 m),
both standing on the ground; save. Then steps 1–5 in Play.
