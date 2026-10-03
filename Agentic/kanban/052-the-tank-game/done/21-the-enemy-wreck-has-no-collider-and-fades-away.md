# 21 — The enemy's wreck has no collider and fades away
folder: examples/tank_game/Assets
after: 18, 20
decisions: 0168, 0294, 0336

## Change
0336 point 4, the data. Agents make the wreck prefabs (0294); `main.scene` is the sponsor's
and stays untouched (0272). Read `examples/tank_game/Assets/enemy_wreck.prefab`,
`examples/tank_game/Assets/Assets.md` and `examples/tank_game/Code/tank_fade_away.h`.

- `enemy_wreck.prefab`: entity 1's `[1.voe_physics_collider]` section goes. Entity 1 gains a
  `[1.tank_fade_away]` section, its sections in the file's order, with `wait = 2`,
  `seconds = 1`, `age = 0`. Nothing else changes; `house_wreck.prefab` is untouched.
- `Assets.md`: says the enemy's wreck is not solid and fades away; the house's stays.

## Done when
`! grep -q voe_physics_collider examples/tank_game/Assets/enemy_wreck.prefab`,
`grep -q tank_fade_away examples/tank_game/Assets/enemy_wreck.prefab`,
`grep -q voe_physics_collider examples/tank_game/Assets/house_wreck.prefab` and
`git diff --quiet HEAD -- examples/tank_game/main.scene` each exit 0.

The human's, in the editor on `examples/tank_game` (Play), then in the shipped game:
1. Destroy an enemy tank. Drive the player's tank into its wreck and shoot at it: the tank
   drives through and the shell flies through.
2. The wreck lies still for about 2 s, turns see-through over about 1 s, and is gone.
3. Destroy a house or other breakable: its wreck stays, and the tank stops against it.
4. Ship, run the shipped game from the menu, and see 1–3 the same.
