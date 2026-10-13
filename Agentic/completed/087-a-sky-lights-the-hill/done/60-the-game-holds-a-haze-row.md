# 60 — The game holds a Haze row
folder: game
after: 56, 57, 59
decisions: 0168, 0416, 0417

## Change
A game world registers the Haze row and drains its replaces; the haze reaches the pass through
3d's frame (card 59) with no other game code. The editor shares the world and the step, so Add
component, the Inspector, undo, saving, Play and Ship need no editor code. Read
`scene/include/scene/haze_system.h` (card 56), the headers of the files below, and in each the
lines naming the picture row, which this card follows.

- `game/include/game/world.h`: `VOE_GAME_WORLD_HAZES` 1 with a comment (one haze a world, 0416);
  `VOE_GAME_WORLD_TYPES` 27 and the "twenty-six types" wording rises by one; the list of types
  names the haze.
- `game/src/world.c`: `voe_scene_haze_register(world, VOE_GAME_WORLD_HAZES)` beside the sky's;
  the header's counts and `MAX_INTENT_TYPES` rise by one.
- `game/include/game/scene.h`: includes `scene/haze_component.h`, so the cook knows it.
- `game/src/frame.c`: `voe_scene_haze_system_run(world)` after the sky's in `voe_game_world_step`;
  `game/include/game/frame.h`'s order of a step names it.
- `game/tests/world.c`: the new key among the registered ones and the new counts.
- `game/tests/frame.c`: its sky row drops `.haze`; a new case: a world with a Haze row of 1000 and
  no sky, a replace of 500 submitted, one step, and `voe_3d_draw_system_sky(world).haze` reads 500
  with `drawn` 0. The header's list follows.
- `game/game.md`, `game/include/game/game.md`, `game/src/src.md`, `game/tests/tests.md`: entries
  for the files changed where what they say changed. Each at most 300 characters.

## Done when
The tests `game/world` and `game/frame` pass after the folder's build.

The human's, in the editor (feature.md's How to test step 7):
1. Opening `lumberhill_project` now warns that the sky's `haze` is no field and the hill has no
   haze. In `main.scene`, move the Sky entity's `haze = 50` line under a new
   `[6.voe_scene_haze]` section (or select the Sky entity, Add component "Rendering / Haze", set
   Haze 50); save and reopen: the haze is back (0417 point 4).
2. Fly to the top of the hill: far ridges fade into haze of the sky's horizon hue, nearer ground
   less, ground within the start not at all.
3. Lower the sky's strength to near 0: the haze stays as bright and the same hue. Lower the
   directional light's strength: the haze darkens with it. Turn the light orange: the haze keeps
   the sky's hue.
4. Remove the Sky: the haze stays and takes the light's colour. Remove the Haze: nothing fades.
5. Open `examples/tank_game`: exactly as before. Save, close and reopen; Play; Ship: the same
   picture in all three.
