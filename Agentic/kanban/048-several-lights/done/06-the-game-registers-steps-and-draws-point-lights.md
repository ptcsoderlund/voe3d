# 06 — The game registers, steps and draws point lights
folder: game
after: 01, 05
decisions: 0168, 0320

## Change
A game world holds point lights, steps their flashes and lights its window pass by them (0320 points
6 and 9). Read `scene/include/scene/point_light_system.h`, the header of
`3d/include/3d/draw_system.h` for `voe_3d_draw_system_point_lights`, and the files below.

- `game/include/game/world.h`: `VOE_GAME_WORLD_POINT_LIGHTS` 256, with a static_assert that it is at
  most VOE_RENDER_POINT_LIGHTS and a comment why (a pass carries no more); `VOE_GAME_WORLD_TYPES` and
  the "twenty-one types" wording rise by the two tables point light registration makes (the light and
  its runtime-only glow).
- `game/src/world.c`: `voe_scene_point_light_register(world, VOE_GAME_WORLD_POINT_LIGHTS)` after the
  transforms, beside the sun's registration.
- `game/include/game/scene.h`: includes `scene/point_light_component.h`, so the cook knows it.
- `game/src/steps.c`: `voe_scene_point_light_system_run(world, (float)VOE_GAME_STEP_SECONDS)` beside
  the emitter system's run; its header's order of systems names it.
- `game/src/frame.c`: after `voe_3d_draw_system_shadows`, `voe_3d_draw_system_point_lights(world,
  &frame, scratch)` (a false leaves the frame unlit by points and is not a failed frame); the
  positional pass camera at line 101 becomes designated and passes `.points = frame.points`.
  `game/include/game/frame.h`'s order of what a frame does names the point lights.
- `game/tests/world.c`: names the two new keys among the registered ones and the new count.
- `game/tests/steps.c`: a point light with a flash, flashed through
  `voe_scene_point_light_flash_submit`, has its full strength after one step and none after enough
  steps to outlast its flash.
- `game/game.md`, `game/src/src.md`, `game/tests/tests.md`: the entries for world.h, world.c, steps.c,
  frame.c and the two tests changed where what they say changed. Each at most 300 characters.

## Done when
The tests `game/world`, `game/steps` and `game/frame` pass after the folder's build.
