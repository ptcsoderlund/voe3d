# 08 — The game holds and draws a sky
folder: game
after: 01, 06, 07
decisions: 0168, 0405
read: feature.md

## Change
A game world registers the sky row, drains its replaces and draws it through 3d's frame; the editor shares
the world and the step (`editor/src/world_step.h`), so Add component, the Inspector, undo, saving, Play
and Ship need no editor code. Read `scene/include/scene/sky_system.h` (card 01), the `voe_3d_frame` sky
comment in `3d/include/3d/draw_system.h` (card 07), and the files below.

- `game/include/game/world.h`: `VOE_GAME_WORLD_SKIES` 1 with a comment (0405 point 1: one sky a
  world); `VOE_GAME_WORLD_TYPES` and the "twenty-four types" wording rise by one.
- `game/src/world.c`: `voe_scene_sky_register(world, VOE_GAME_WORLD_SKIES)` beside the light's; the
  header's counts rise by one type and one intent queue, as does `MAX_INTENT_TYPES`.
- `game/include/game/scene.h`: includes `scene/sky_component.h`, so the cook knows it.
- `game/src/frame.c`: in `voe_game_world_step`, `voe_scene_sky_system_run(world)` after the light's
  run; `game/include/game/frame.h`'s order of a step names it, and its frame paragraph says the sky
  comes with the camera.
- `game/tests/world.c`: names the new key among the registered ones and the new counts.
- `game/tests/frame.c`: a world with a lit shape under an overhead sun and a sky row, two frames as
  the file's cases, then the window read by `voe_render_target_read`: a corner reads blue above red,
  not the clear colour; an exposure replace of −4 submitted between the frames is drained by the
  second's step and darkens the shape's pixel; a world with no sky row reads the clear colour there.
- `game/game.md`, `game/include/game/game.md`, `game/src/src.md`, `game/tests/tests.md`: the entries
  for world.h, world.c, frame.h, frame.c and the two tests changed where what they say changed. Each at
  most 300 characters.

## Done when
The tests `game/world` and `game/frame` pass after the folder's build.

The human's, in the editor (feature.md's How to test, all nine steps):
1. Open the hill, select an entity, Add component "Rendering / Sky": the sky shows behind the hill.
2. Turn the sun low with the rotate gizmo: evening sky, shaded sides warmer and darker.
3. Turn it back up: shaded slopes softly blue-ish, not black.
4. Fly to the top: far ridges in haze, nearer ground less (Haze in the Inspector changes how far).
5. Place a sphere wearing a metal material of low roughness: it reflects the sky.
6. Raise and lower Exposure: the picture brightens and darkens; with Tone Soft the bright sky rolls
   off, with Clip it clips.
7. Open `examples/tank_game`: it looks exactly as before.
8. Open the frame breakdown: `view …: sky` is a small share of the frame.
9. Save, close and reopen; Play; Ship: the same picture in all three.
