# 12 — A game holds drifting clouds
folder: game
after: 01, 11
decisions: 0168, 0237, 0480, 0492

## Change
A game world registers the Clouds row and its drift and steps them each fixed step, where water's
clock steps (0492 point 2); the clouds reach the pass through 3d's frame (card 11) with no other
game code. The editor shares the world, so Add component, the Inspector, undo, saving, Play and
Ship need no editor code for the row. Read `scene/include/scene/clouds_system.h` (card 01), the
headers of the files below and, in each, the lines naming the Haze row or the water, which this
card follows. Card 60 of 087
(`Agentic/completed/087-a-sky-lights-the-hill/done/60-the-game-holds-a-haze-row.md`) is the shape.

- `game/include/game/world.h`: `VOE_GAME_WORLD_CLOUDS` 1 with a comment (one a world, the draw
  reads the first); `VOE_GAME_WORLD_TYPES` rises by two, the row and its drift, and the
  "twenty-seven types" wording with it; the list of types names them.
- `game/src/world.c`: `voe_scene_clouds_register(world, VOE_GAME_WORLD_CLOUDS)` beside the haze's;
  the header's counts and `MAX_INTENT_TYPES` rise by one.
- `game/include/game/scene.h`: includes `scene/clouds_component.h`, so the cook knows it.
- `game/src/steps.c`: `voe_scene_clouds_system_run(world, (float)VOE_GAME_STEP_SECONDS)` right
  after water's; `game/include/game/steps.h`'s order of a step names it.
- `game/tests/world.c`: the new keys among the registered ones and the new counts.
- `game/tests/steps.c`: a case: a world with a Clouds row blowing towards +X (90) at 6 m/s, 60
  steps run, its drift reads x 6 and z 0 within 1e-4.
- `game/tests/frame.c`: a case: a world with a Volumetric Clouds row at height 1500,
  `voe_3d_draw_system_clouds(world, origin)` reads `VOE_RENDER_CLOUDS_VOLUMETRIC` and bottom 1500.
- `game/game.md`, `game/include/game/game.md`, `game/src/src.md`, `game/tests/tests.md`: follow where
  what they say changed (the type count, steps, world), each at most 300 characters.

## Done when
The tests `game/world`, `game/steps` and `game/frame` pass after the folder's build.
