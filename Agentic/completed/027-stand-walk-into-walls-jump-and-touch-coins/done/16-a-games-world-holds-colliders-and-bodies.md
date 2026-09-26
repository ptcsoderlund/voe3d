# 16 — A game's world holds colliders and bodies, and its frame drains them
folder: game
decisions: 0168, 0175, 0253, 0254, 0250

## Change
0253 point 1: `physics` added to the `game` row in `cmake/voe.cmake` (comment names 0253) and to
`game/CMakeLists.txt`'s DEPENDS. Mends what cards 03, 08, 09 broke here.

- `game/include/game/world.h`, `game/src/world.c` — registered after the shape: collider and
  body at `VOE_GAME_WORLD_AUTHORED` each, and the transforms' previous table
  (`voe_scene_transform_previous_register`, card 04) at the transforms' room;
  `VOE_GAME_WORLD_TYPES` 11 and the intent room counting the collider's and body's two intents.
  Header: the list names them.
- `game/include/game/scene.h` — includes `physics/collider_component.h` and
  `physics/body_component.h`, so a cooked scene names their keys.
- `game/include/game/frame.h`, `game/src/frame.c` — `voe_game_frame(app, world, shapes, scratch,
  size, float lag)`: after the shape system, `voe_physics_collider_system_run` and
  `voe_physics_body_system_run`; the draw through `voe_3d_draw_system_frame(world, size, lag)`.
  Also a new `void voe_game_world_step(voe_ecs_world *world, const voe_3d_shapes *shapes)`: the
  structural queue, the project's replaces, then every owning system in the frame's order, which
  `voe_game_frame` calls first and card 17's steps call too. Header: the order with the two new
  systems; what `lag` is (0254).
- `game/src/run.c` — passes lag 0 (card 17 steps).
- `game/tests/world.c` — eleven keys, eleven types; `game/tests/frame.c` — lag 0; a collider's
  replace intent is drained by one frame. `game/tests/tests.md` where counts are named.
- `game/include/game/game.md`, `game/src/src.md` — "eight" becomes the new count; frame's entry.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder game` prints `FINDINGS: 0`.
2. `ctest --test-dir build/debug -R '^game/'` passes.
