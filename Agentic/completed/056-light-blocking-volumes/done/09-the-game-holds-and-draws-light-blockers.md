# 09 — The game holds and draws light blockers
folder: game
after: 01, 05, 08
decisions: 0168, 0347

## Change
A game world holds blockers, drains their replaces and lights its window pass by them; nothing is
drawn for them (0347 points 1, 2, 5). Read `scene/include/scene/light_blocker_system.h`, the header
of `3d/include/3d/draw_system.h` for `voe_3d_draw_system_light_blockers`, and the files below.

- `game/include/game/world.h`: `VOE_GAME_WORLD_LIGHT_BLOCKERS` VOE_RENDER_LIGHT_BLOCKERS, with a
  static_assert that it is at most that and a comment why (0347 point 2), as the point lights'
  is; `VOE_GAME_WORLD_TYPES` and the "twenty-two types" wording rise by one.
- `game/src/world.c`: `voe_scene_light_blocker_register(world, VOE_GAME_WORLD_LIGHT_BLOCKERS)`
  after the transforms, beside the sun's; its header's counts rise by one type and one queue.
- `game/include/game/scene.h`: includes `scene/light_blocker_component.h`, so the cook knows it.
- `game/src/frame.c`: in `voe_game_world_step`, `voe_scene_light_blocker_system_run(world)` beside
  the sun's run; in the frame, `voe_3d_draw_system_light_blockers(world, &frame, scratch)` before
  the point lights (a false leaves the pass unblocked and is no failed frame), and the pass camera
  passes `.blockers = frame.blockers`. `game/include/game/frame.h`'s order of what a step and a
  frame do names the blockers.
- `game/tests/world.c`: names the new key among the registered ones and the new count.
- `game/tests/frame.c`: a world with a lit ground shape under a sun and a blocker over part of it,
  two frames as the file's cases, both true, then the window read by `voe_render_target_read`
  (`render/include/render/device.h`): black inside the box, lit outside, nothing drawn on the
  box's edge; a size replace submitted between the frames is drained by the second's step.
- `game/game.md`, `game/include/game/game.md`, `game/src/src.md`, `game/tests/tests.md`: the
  entries for world.h, world.c, frame.h, frame.c and the two tests changed where what they say
  changed. Each at most 300 characters.

## Done when
The tests `game/world` and `game/frame` pass after the folder's build.
