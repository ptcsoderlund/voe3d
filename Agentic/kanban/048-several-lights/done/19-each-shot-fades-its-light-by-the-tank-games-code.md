# 19 — Each shot fades its light by the tank game's code
folder: examples/tank_game/Code
after: 13, 18
decisions: 0168, 0272, 0321, 0322

## Change
The player's and the enemies' shots light the ground through the tank game's fade, not the
engine's flash (0322 point 6). Read `examples/tank_game/Code/tank_light_fade.h`, the header of
`scene/include/scene/point_light_component.h`, and the files below.

- `examples/tank_game/Code/tank_gun_system.c`: `add_flash_light` queues a light with no flash
  fields, intensity 0 and falloff 1 (same colour and range), and beside it, the same way, a
  `tank_light_fade` row of peak 4, seconds 0.12, left 0 when the gun has none, queued after the
  light so the fade's need of a point light is met. Each shot calls
  `tank_light_fade_start` on the gun in place of the flash intent. The header's flash paragraph
  says the light rests dark and the fade lights it per shot.
- `examples/tank_game/Code/tank_enemy_system.c`: each shot calls `tank_light_fade_start` on the
  turret in place of `voe_scene_point_light_flash_submit`; a turret with no fade row sends nothing.
  The header's flash sentence to match.
- `examples/tank_game/Code/tank_gun.h`: header only, if it names the light's flash.
- `examples/tank_game/Code/Code.md`: the gun system and enemy system entries say the fade, if they
  say flash for the light. Each at most 300 characters.

## Done when
`(for f in examples/tank_game/Code/*.c; do clang -std=c23 -fsyntax-only -DVOE_BASE_DESCRIPTIONS=1
$(for d in */include; do printf -- "-I%s " $d; done) "$f" || exit 1; done)` exits 0, and
`! grep -rn "flash_submit\|flash_when_made" examples/tank_game/Code` exits 0.
