# 09 — The player's hull is solid, so enemy shells hit it
folder: examples
after: none
decisions: 0168, 0294, 0295

## Change
Bug 01: enemy shells pass through the player and cost no life, because
the hull in `Assets/tank_body.prefab` has no collider, so the sweep in
`tank_shell_system.c` has nothing to hit. The fix is in the lives system,
which owns the player's hull (0295). Files under `examples/tank_game/`:
`Code/tank_lives.h`, `Code/tank_lives_system.c`, `Code/Code.md`. Read
`physics/include/physics/collider_component.h` (the row, the box kind,
`voe_physics_collider_get`, `voe_physics_collider_key`). Never edit
`main.scene` or `Assets/tank_body.prefab` (0272).

- `tank_lives_system.c`, `tank_lives_run`: each step, when there is a
  first hull (the same one the lives row goes on) and it has no collider,
  queue onto it through the structural queue, as the lives row is queued,
  a collider row: box kind, size (4.64, 4, 2.62), not a trigger. A
  refused add tries again next step. A hull that already has a collider
  keeps it untouched. This happens whether or not the lives row exists
  yet. Header: the collider, why it is added here and not authored, a full
  queue retries.
- `tank_lives.h`: header points: the system also makes the hull solid
  when it is not, the size and that it is the enemy's; a sponsor's collider
  wins; never saved (the game is a separate program).
- `Code.md`: the `tank_lives.h` and `tank_lives_system.c` entries say the
  hull is made solid; each under 300 characters.

## Done when
`(for f in examples/tank_game/Code/*.c; do clang -std=c23 -fsyntax-only -DVOE_BASE_DESCRIPTIONS=1
$(for d in */include; do printf -- "-I%s " $d; done) "$f" || exit 1; done)` exits 0, and
`grep -q voe_physics_collider_key examples/tank_game/Code/tank_lives_system.c` exits 0.

For the human, in the editor on `examples/tank_game`: Play, drive into an
enemy's view and stand still. Its shell stops on your tank and `Lives N`
goes down by one (feature step 4).
