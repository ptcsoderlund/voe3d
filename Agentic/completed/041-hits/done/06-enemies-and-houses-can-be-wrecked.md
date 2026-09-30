# 06 — Enemies and houses can be wrecked
folder: examples
after: 05
decisions: 0168, 0294, 0253

## Change
0294 point 6, the prefabs agents make. Files under
`examples/tank_game/Assets/`: `enemy_tank.prefab`, new `enemy_wreck.prefab`,
`house.prefab` and `house_wreck.prefab`; and `tank_game.md`. Read
`Assets/shell.prefab` and `Assets/enemy_tank.prefab` for a prefab's text,
`main.scene`'s Ground for a collider row (read only), and
`Code/tank_breakable.h` (05). A collider is centred on its own entity and
scaled by its transform (0253 point 2); a hit swaps only an entity that has
both the collider and the breakable. Never edit `main.scene` or
`Assets/tank_body.prefab` (0272).

- `enemy_tank.prefab`: the root gains a box collider of size
  (4.64, 4, 2.62), the hull and turret models' extent about the root, and
  `tank_breakable` with `wreck` `enemy_wreck`.
- `enemy_wreck.prefab`: a root with the same box collider and no
  breakable; a child with `Assets/tank_body.glb` tilted a few degrees and
  sunk 0.3 m; a child with `Assets/tank_head.glb` knocked off: turned well
  away from the hull's forward, tilted, beside the hull and lower. It must
  read as the same tank, broken, from the game's camera.
- `house.prefab`: a root with a box collider of size (4, 3, 4) and
  `tank_breakable` with `wreck` `house_wreck`; a child cube shape of scale
  (4, 3, 4) in a pale wall colour; a child cube roof of scale
  (4.4, 0.4, 4.4) at y 1.7 in a dark red. Its root is its centre.
- `house_wreck.prefab`: a root with no collider; a child at y −1.1 with a
  cube shape of scale (4.2, 0.8, 4.2) in a dark grey and a box collider of
  size 1, so its collider is the rubble; two or three smaller tilted cubes
  on it in the wall and roof colours, no collider.
- `tank_game.md`: the new prefabs in the `Assets` line, under 300
  characters.

## Done when
`grep -q tank_breakable examples/tank_game/Assets/enemy_tank.prefab && grep -q
tank_breakable examples/tank_game/Assets/house.prefab && grep -q
voe_physics_collider examples/tank_game/Assets/enemy_wreck.prefab && grep -q
voe_physics_collider examples/tank_game/Assets/house_wreck.prefab` exits 0.
