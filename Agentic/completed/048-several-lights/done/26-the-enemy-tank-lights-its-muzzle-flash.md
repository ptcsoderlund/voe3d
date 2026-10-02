# 26 — The enemy tank lights its muzzle flash
folder: examples/tank_game/Assets
after: 25
decisions: 0168, 0272, 0323

## Change
The enemy turret's light moves onto a child at its muzzle flash (0323 point 2). This card ends bug
02. Read the file below.

- `examples/tank_game/Assets/enemy_tank.prefab`: entity 2 loses its `[2.tank_light_fade]` and
  `[2.voe_scene_point_light]` sections. A new entity `[3]`, name "Enemy muzzle light", after entity
  2 and separated as entity 2 is from 1, carries in this order: those two sections with the same
  fields and numbers, renumbered to 3; `[3.voe_scene_parent]` with `parent = 2`;
  `[3.voe_scene_transform]` at entity 2's `voe_3d_emitter` `offset`, `[0, 0.5, 3]`, rotation
  `[0, 0, 0, 1]`, scale `[1, 1, 1]`. Every field written, numbers spelled as the neighbouring
  sections spell them. Nothing else changes.

## Done when
`(cd examples/tank_game/Assets && ! grep -q '^\[2\.\(tank_light_fade\|voe_scene_point_light\)\]$'
enemy_tank.prefab && [ "$(grep -c '^\[3\.\(tank_light_fade\|voe_scene_point_light\|voe_scene_parent\|voe_scene_transform\)\]$'
enemy_tank.prefab)" = 4 ])` exits 0.

The human's, in the editor on `examples/tank_game` (bug 02's reproduce steps 3 and 4):
1. Press Play and shoot: the light flashes at the muzzle explosion, at the end of the barrel. Turn
   the turret, drive, and shoot again: the light follows the barrel.
2. Let an enemy tank shoot: its light flashes at its own barrel. Wrecks and explosions still light
   up where they are.
