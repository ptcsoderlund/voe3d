# 06 — The game's world holds emitters and steps them
folder: game
after: 05
decisions: 0168, 0298

## Change
0298 points 4 and 8. Read `3d/include/3d/emitter_component.h` and
`3d/include/3d/emitter_system.h`.

- `game/include/game/world.h`: `VOE_GAME_WORLD_EMITTERS`, defined as
  `VOE_GAME_WORLD_MAX_DRAWN`, with its reason (an effect sits on a drawn
  thing and wrecks keep theirs, 0298 point 8); `VOE_GAME_WORLD_TYPES` 17; the header's list names the emitter and its particles.
- `game/src/world.c`: `voe_3d_emitter_register(world,
  VOE_GAME_WORLD_EMITTERS)` after the model's.
- `game/include/game/scene.h`: includes `3d/emitter_component.h` (the test
  checks every registered type's header is there).
- `game/src/steps.c`: each step runs `voe_3d_emitter_system_run(world,
  (float)VOE_GAME_STEP_SECONDS)` after its second world step, so bursts that
  `systems` and `after_move` sent this step spawn in it.
- `game/include/game/steps.h`: the step's order in the header names it.
- `game/include/game/frame.h`: `VOE_GAME_CAPACITIES.objects` adds
  `VOE_GAME_WORLD_EMITTERS * VOE_3D_EMITTER_PARTICLES` for the window pass
  (none per cascade); the comment above says so.
- `game/tests/world.c`: the type count and any list it checks.
- `game/tests/steps.c`: a new case: an emitter of rate 60 on a thing with a
  transform, 60 steps run through `voe_game_steps_run`, has a particles row
  holding live particles.
- `game/src/src.md`, `game/tests/tests.md`: the entries that name the
  types or the step order.

## Done when
`ctest --test-dir build/debug -R '^game/(world|steps|frame)$'` passes, after
the folder's build.
