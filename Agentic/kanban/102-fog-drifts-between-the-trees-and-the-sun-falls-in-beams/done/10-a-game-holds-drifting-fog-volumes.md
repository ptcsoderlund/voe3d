# 10 — A game holds drifting fog volumes
folder: game
after: 01, 07, 08, 09
decisions: 0168, 0237, 0480, 0493

## Change
A game world registers the Fog volume row and its drift and steps them each fixed step right
after the clouds (0493 point 2); the fog reaches the pass through 3d's frame (card 08) with no
other game code. The editor shares the world, so Add component, the Inspector, undo, saving, Play
and Ship need no editor code for the row. Read `scene/include/scene/fog_volume_system.h` (card
01), the headers of the files below and, in each, the lines naming the Clouds row, which this card
follows. Card 12 of 101
(`Agentic/completed/101-clouds-in-the-sky-cast-shadows-on-the-world/done/12-a-game-holds-drifting-clouds.md`)
is the shape.

- `game/include/game/world.h`: `VOE_GAME_WORLD_FOG_VOLUMES` 8 with a comment (as many as a pass
  carries, VOE_RENDER_FOG_VOLUMES); `VOE_GAME_WORLD_TYPES` rises by two, the row and its drift,
  and the "twenty-nine types" wording with it; the list of types names them.
- `game/src/world.c`: `voe_scene_fog_volume_register(world, VOE_GAME_WORLD_FOG_VOLUMES)` beside
  the clouds'; the header's counts and `MAX_INTENT_TYPES` rise by one.
- `game/include/game/scene.h`: includes `scene/fog_volume_component.h`, so the cook knows it.
- `game/src/steps.c`: `voe_scene_fog_volume_system_run(world, (float)VOE_GAME_STEP_SECONDS)` right
  after the clouds'; `game/include/game/steps.h`'s order of a step names it.
- `game/tests/world.c`: the new keys among the registered ones and the new counts.
- `game/tests/steps.c`: a case: a world with a Fog volume blowing towards +X (90) at 3 m/s, 60
  steps run, its drift reads x 3 and z 0 within 1e-4.
- `game/tests/frame.c`: a case: a world with a fog volume of thickness 1 on a placed entity,
  `voe_3d_fog_volumes(world, origin, 0)` reads count 1 and density 0.1, and off it reads count 0.
- `game/game.md`, `game/include/game/game.md`, `game/src/src.md`, `game/tests/tests.md`: follow
  where what they say changed (the type count, steps, world), each at most 300 characters.

## Done when
The tests `game/world`, `game/steps` and `game/frame` pass after the folder's build.
