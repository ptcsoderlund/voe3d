# 08 — Each shot flashes the ground around the barrel
folder: examples
after: 06
decisions: 0168, 0272, 0299, 0320

## Change
The player's and the enemies' shots light the ground for a moment (feature How to test 4). Read
`scene/include/scene/point_light_component.h`, `scene/include/scene/point_light_system.h`, and the
files below.

- `examples/tank_game/Code/tank_gun_system.c`: beside `add_flash`, a gun with a transform and no
  point light gets one queued onto it the same way (a structural add, never saved): warm orange
  (about 1, 0.7, 0.35), intensity about 4, range about 6 m, flash about 0.12 s, not flashed when
  made. Each shot fired, beside `burst_flash`, submits a flash for it; a full queue loses only that
  flash. The header's flash paragraph says the light is the muzzle flash's, at the gun's own place.
- `examples/tank_game/Assets/enemy_tank.prefab`: entity 2, "Enemy turret", gains a
  `[2.voe_scene_point_light]` section with the same numbers (`flash_when_made = false`), placed before
  its `voe_scene_parent` section, every field written.
- `examples/tank_game/Code/tank_enemy_system.c`: beside the turret's emitter burst, each shot submits
  a flash for the turret's point light when it has one; the header's flash sentence says so.
- `examples/tank_game/Code/Code.md`: the gun system and enemy system entries name the light. Each at
  most 300 characters.

## Done when
`(for f in examples/tank_game/Code/*.c; do clang -std=c23 -fsyntax-only -DVOE_BASE_DESCRIPTIONS=1
$(for d in */include; do printf -- "-I%s " $d; done) "$f" || exit 1; done)` exits 0, and
`grep -q voe_scene_point_light_flash_submit examples/tank_game/Code/tank_gun_system.c && grep -q
voe_scene_point_light_flash_submit examples/tank_game/Code/tank_enemy_system.c && grep -q
'^\[2\.voe_scene_point_light\]' examples/tank_game/Assets/enemy_tank.prefab` exits 0.
