# 08 — The game's world holds water and steps it
folder: game
after: 07
decisions: 0168, 0305

## Change
0305 points 5–6. Read `3d/include/3d/water_component.h`,
`3d/include/3d/water_system.h`, `3d/include/3d/models.h` (`_load_water`,
`_water`), and in `game`: `include/game/world.h`, `src/world.c`,
`include/game/scene.h`, `include/game/steps.h`, `src/steps.c`,
`include/game/frame.h` (the
capacities), `src/models.c`, and the tests `tests/world.c`,
`tests/models.c`, `tests/steps.c`.

- `include/game/world.h`: `VOE_GAME_WORLD_WATERS` 16, with the emitters'
  comment's reasoning.
- `src/world.c`: `voe_3d_water_register(world, VOE_GAME_WORLD_WATERS)`;
  the count of registrations in its header and in `src/src.md` goes up.
- `include/game/scene.h`: includes `3d/water_component.h`, so the cook
  sees the type.
- `src/steps.c`: `voe_3d_water_system_run(world, step seconds)` beside the
  emitters' run, in the same place of the step's order; the order list in
  `include/game/steps.h`'s header and `src/src.md` names it.
- `include/game/frame.h`: `VOE_GAME_CAPACITIES` gains
  `VOE_GAME_WORLD_WATERS` objects in the window pass (water casts no
  shadow), and its comment says so.
- `src/models.c`: when the world has water and the store has no water
  record, `voe_3d_models_load_water`; a failure counted as the dot's is.
- Tests: `tests/world.c` — the water type is registered; `tests/models.c`
  — a world with one water loads the record, one without does not;
  `tests/steps.c` — one fixed step advances a water's clock by the step.
- `game/game.md`, `src/src.md`, `tests/tests.md`: entries changed.

## Done when
The tests `game/world`, `game/models` and `game/steps` pass, and
`game/frame` still passes, after the folder's build.
