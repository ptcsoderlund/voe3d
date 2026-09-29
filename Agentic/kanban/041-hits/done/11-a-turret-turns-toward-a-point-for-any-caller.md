# 11 — A turret turns toward a point for any caller
folder: examples
after: 10
decisions: 0168, 0297

## Change
Bug 03, first half: enemies will turn their own turrets (0297), so the
turret system turns only the player's and exports its turn. Files under
`examples/tank_game/`: `Code/tank_turret.h`, `Code/tank_turret_system.c`,
`Code/Code.md`. Read `Code/tank_control.h` (the row is on the player's
hull, `tank_control_key`) and `scene/include/scene/parent_component.h`
(`voe_scene_parent_within`). Never edit `main.scene` or any prefab.

- `tank_turret.h`: new
  `[[nodiscard]] bool tank_turret_turn_toward(voe_ecs_world *world,
  voe_ecs_entity turret, voe_math_double3 at, double seconds,
  voe_math_quat *barrel, float *left)`: turns the turret, as the run does
  today, about +Y toward facing `at` (flattened to the turret's height)
  with its barrel, by at most its row's `turn` for `seconds`, through the
  same transform intent and `voe_scene_transform_local`. Writes the
  barrel's world rotation after that turn (the turret's world rotation
  turned about its own +Y by `aim`) and `left`, the degrees still between
  the barrel and the way to `at`, at or above 0. False, nothing queued or
  written, with no turret row or transform, a flat way nought, or a full
  queue. Header points: turns the player's turrets only (those within the
  control row's hull); any other turret is its owner's to turn with this
  call (0297); the table has the drawn room because spawned enemies carry
  turrets.
- `tank_turret_system.c`: register the table with
  `VOE_GAME_WORLD_MAX_DRAWN` rows. The per-turret turn moves into
  `tank_turret_turn_toward`; the run finds the pointer or pad aim point as
  today and calls it, only for turrets within the control row's entity.
  Header: which turrets it turns and the exported call.
- `Code.md`: the `tank_turret.h` and `tank_turret_system.c` entries say the
  player's turrets and the call; each under 300 characters.

## Done when
`(for f in examples/tank_game/Code/*.c; do clang -std=c23 -fsyntax-only -DVOE_BASE_DESCRIPTIONS=1
$(for d in */include; do printf -- "-I%s " $d; done) "$f" || exit 1; done)` exits 0, and
`grep -q tank_turret_turn_toward examples/tank_game/Code/tank_turret_system.c && grep -q
voe_scene_parent_within examples/tank_game/Code/tank_turret_system.c` exits 0.
