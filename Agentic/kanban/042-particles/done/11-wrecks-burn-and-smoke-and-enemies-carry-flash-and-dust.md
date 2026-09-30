# 11 — Wrecks burn and smoke, and enemies carry flash and dust
folder: examples
after: 06
decisions: 0168, 0298, 0299

## Change
0299 points 1, 2 and 4. Data only. Read `3d/include/3d/emitter_component.h`
for the row's name and fields, and a prefab's `voe_scene_light` or
`voe_3d_material` colour in `examples/` for how a COLOUR is written.
Never edit `main.scene` or `tank_body.prefab` (0272).

- `examples/tank_game/Assets/enemy_wreck.prefab` and `house_wreck.prefab`: a
  `voe_3d_emitter` row on the root, the fire, and one on the first child
  part, the smoke, with 0299 point 1's numbers. Also: fire speed 3, spread
  60, drag 2, size 0.8 to 0.2, glow; smoke speed 0.6, spread 25, rise 0.8,
  drag 0.5, size 0.6 to 2.5, alpha 0.7 to 0, grey to dark grey; both with an
  empty texture. The smoke's offset lifts it above the wreck's top.
- `examples/tank_game/Assets/enemy_tank.prefab`: on the turret part, the
  muzzle flash of 0299 point 2: playing false, offset the enemy's `muzzle`
  (read `examples/tank_game/Code/tank_enemy.h` for its space), direction
  along the barrel, speed 2, spread 30, size 0.9 to 0.3, orange-white to
  orange, alpha 1 to 0, glow. On the root, the dust of point 4: offset at
  the rear near the ground, speed 0.5, spread 50, rise 0.3, drag 1, size 0.4
  to 1.4, brown, alpha 0.5 to 0, lit.

## Done when
`grep -c voe_3d_emitter examples/tank_game/Assets/enemy_wreck.prefab` prints
2, the same for `house_wreck.prefab` and `enemy_tank.prefab`, and
`git diff --quiet HEAD -- examples/tank_game/main.scene
examples/tank_game/Assets/tank_body.prefab` exits 0.
