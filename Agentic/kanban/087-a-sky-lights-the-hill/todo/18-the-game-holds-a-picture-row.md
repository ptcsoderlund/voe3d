# 18 — The game holds a picture row
folder: game
after: 17
decisions: 0168, 0406

## Change
A game world registers the picture row and drains its replaces; the sky row's new fields reach the game
through 3d's frame with no game code. The editor shares the world and the step, so Add component, the
Inspector, undo, saving, Play and Ship need no editor code. Read `scene/include/scene/picture_system.h`
(card 16), `scene/include/scene/sky_component.h` (card 15) and the files below.

- `game/include/game/world.h`: `VOE_GAME_WORLD_PICTURES` 1 with a comment (0406 point 4: one picture a
  world); `VOE_GAME_WORLD_TYPES` and the "twenty-five types" wording rise by one; the header's list of
  types names the picture.
- `game/src/world.c`: `voe_scene_picture_register(world, VOE_GAME_WORLD_PICTURES)` beside the sky's;
  the header's counts and `MAX_INTENT_TYPES` rise by one.
- `game/include/game/scene.h`: includes `scene/picture_component.h`, so the cook knows it.
- `game/src/frame.c`: `voe_scene_picture_system_run(world)` after the sky's in
  `voe_game_world_step`; `game/include/game/frame.h`'s order of a step names it, and its frame
  paragraph says the picture comes with the camera beside the sky.
- `game/tests/world.c`: the new key among the registered ones and the new counts.
- `game/tests/frame.c`: its sky case builds the row with 0406's defaults (no `light`); the exposure
  replace of −4 is submitted on a picture row added with the sky; a world with neither reads the
  clear colour in the corner as now; a world with a picture row and no sky reads the cube darker at
  exposure −4 than without the row.
- `game/game.md`, `game/include/game/game.md`, `game/src/src.md`, `game/tests/tests.md`: entries for
  world.h, world.c, frame.h, frame.c and the two tests changed where what they say changed. Each at
  most 300 characters.

## Done when
The tests `game/world` and `game/frame` pass after the folder's build.
