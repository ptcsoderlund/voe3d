# 16 — A game world has no point light glow
folder: game
after: 13, 15
decisions: 0168, 0321, 0322

## Change
The game world registers no glow table and steps the point lights with no seconds (0321 point 2,
0322 point 5). Read the header of `scene/include/scene/point_light_system.h` and the files below.

- `game/include/game/world.h`: `VOE_GAME_WORLD_TYPES` 23 becomes 22 and the "twenty-three"
  wording "twenty-two"; the type list and the point light room's comment no longer name a glow.
- `game/src/world.c`: the header's count of types and of second queues loses the glow and the
  point light's flash queue (fifteen queues become fourteen).
- `game/src/steps.c`: `voe_scene_point_light_system_run(world)`, no seconds.
- `game/include/game/steps.h`: the order's last sentence no longer names a flash; an edit still
  takes in the same step.
- `game/tests/world.c`: `voe_scene_point_light_glow_key` goes from the keys, `TYPES` 22 becomes
  21, and the header's list no longer names the glow.
- `game/tests/steps.c`: `a_point_light_flashes` becomes a case that a replace takes in one step: a
  lamp of intensity 2 and falloff 1, a replace submitted through `voe_scene_point_light_submit`
  with intensity 0.5 and falloff 3, and after one `voe_game_steps_run` its row reads 0.5 and 3 —
  how game code fades a light. The header's sentence on it to match.
- `game/include/game/game.md`, `game/src/src.md`, `game/tests/tests.md`: the world.h, world.c,
  world.c test and steps.c test entries lose the glow and the flash, and give the new counts. Each
  at most 300 characters.

## Done when
The tests `game/world` and `game/steps` pass after the folder's build, and
`! grep -rn "point_light_glow\|point_light_strength\|flash_submit" game` exits 0.
