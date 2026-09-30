# 12 — An enemy aims before it fires
folder: examples
after: 11
decisions: 0168, 0294, 0297

## Change
Bug 03: enemy shells leave sideways or backwards while the barrel faces
forward, because `tank_enemy_system.c` fires along a frame turned toward
the player and never turns the turret. The fix is in the enemy system,
which fires (0297). Files under `examples/tank_game/`:
`Code/tank_enemy.h`, `Code/tank_enemy_system.c`,
`Assets/enemy_tank.prefab`, `Code/Code.md`, `tank_game.md`. Read
`Code/tank_turret.h` (11: `tank_turret_key`, `tank_turret_turn_toward`),
`scene/include/scene/parent_component.h` (`voe_scene_parent_get`) and
`scene/include/scene/transform_component.h` (a world place). Never edit
`main.scene` or `Assets/tank_body.prefab`.

- `enemy_tank.prefab`: the child `Enemy turret` (2) gains a `tank_turret`
  row, `turn` 60, `aim` 180.
- `tank_enemy_system.c`: per enemy with life left, its turret is the first
  `tank_turret` row whose parent row names the enemy; none, it never fires.
  With a target within `range`: `tank_turret_turn_toward` the target's
  position for the step's seconds, giving the barrel and what is left.
  `wait` counts down as today; it fires only when spent, the call returned
  true, and `left` is at most 2 degrees (a named constant): the muzzle is
  turned by the barrel and added to the turret's world position, the shell
  spawns there turned as the barrel, `owner` the enemy, `from` the enemy's
  position. The old fire frame goes. Header: the aim, the fire rule, 0297.
- `tank_enemy.h`: `muzzle` is in the barrel's frame about the turret; the
  turret turns toward the player within range at its row's turn and the
  enemy fires only when on target; replace "The turret does not turn yet".
- `Code.md`, `tank_game.md`: the enemy entries say it turns its turret and
  fires along the barrel; each under 300 characters.

## Done when
`(for f in examples/tank_game/Code/*.c; do clang -std=c23 -fsyntax-only -DVOE_BASE_DESCRIPTIONS=1
$(for d in */include; do printf -- "-I%s " $d; done) "$f" || exit 1; done)` exits 0, and
`grep -q tank_turret_turn_toward examples/tank_game/Code/tank_enemy_system.c && grep -q
tank_turret examples/tank_game/Assets/enemy_tank.prefab` exits 0.

For the human, in the editor on `examples/tank_game`: Play, drive beside
and behind an enemy. Its turret swings toward you, readably, and it fires
only once the barrel points at you; every shell leaves the muzzle along
the barrel. Your own turret still follows the mouse; enemies' do not.
