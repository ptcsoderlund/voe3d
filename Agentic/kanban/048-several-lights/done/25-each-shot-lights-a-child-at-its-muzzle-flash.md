# 25 — Each shot lights a child at its muzzle flash
folder: examples/tank_game/Code
after: none
decisions: 0168, 0272, 0322, 0323

## Change
The shot's light shines from the gun's or turret's pivot, under the tank; it moves onto a child at
the muzzle flash (0323). Read the headers of `scene/include/scene/parent_component.h`,
`ecs/include/ecs/structure.h` and `ecs/include/ecs/world.h` (`voe_ecs_entity_create`), and the files
below.

- `examples/tank_game/Code/tank_light_fade.h`: add
  `bool tank_light_fade_under(const voe_ecs_world *world, voe_ecs_entity parent, voe_ecs_entity *out)`:
  the first tank_light_fade row whose `voe_scene_parent` row names `parent` into `out`, false with
  none. Header points: a shot's light is a child at its muzzle flash, since a light shines from its
  own transform; the finder scans the fade rows.
- `examples/tank_game/Code/tank_light_fade_system.c`: defines it, walking the fade table as the run
  does.
- `examples/tank_game/Code/tank_gun_system.c`: `add_flash_light` no longer adds a light or fade to
  the gun. A gun with a transform and no `tank_light_fade_under` child gets one: create an entity
  (none this step when the world is full), then structural adds in this order: a transform at
  `turned_by(aim, gun->muzzle)` (the same offset `add_flash` gives the emitter; share the aim
  computation rather than repeat it), identity rotation, scale 1; a `voe_scene_parent` row naming
  the gun; the point light it queues today (colour, intensity 0, range, falloff 1); the fade (peak
  4, seconds 0.12, left 0). A refused add queues the new entity's destroy and stops. Each shot calls
  `tank_light_fade_start` on the found child, not on the gun. The header's flash paragraph says the
  light is a child at the flash, carried by the barrel.
- `examples/tank_game/Code/tank_enemy_system.c`: each shot restarts the fade of the turret's
  `tank_light_fade_under` child, not of the turret; a turret with none sends nothing. The header's
  flash sentence says the light is the turret's child at its flash.
- `examples/tank_game/Code/Code.md`: the light fade header entry names the finder; the gun and enemy
  system entries say the light is a child at the flash. Each at most 300 characters.

## Done when
`(for f in examples/tank_game/Code/*.c; do clang -std=c23 -fsyntax-only -DVOE_BASE_DESCRIPTIONS=1
$(for d in */include; do printf -- "-I%s " $d; done) "$f" || exit 1; done)` exits 0, and
`grep -q tank_light_fade_under examples/tank_game/Code/tank_gun_system.c && grep -q
tank_light_fade_under examples/tank_game/Code/tank_enemy_system.c` exits 0.
