# 17 — The game steps at a fixed rate and draws a lag behind
folder: game
decisions: 0168, 0254, 0065, 0249

## Change
0254 points 1, 2 and 4 in `game`, where the accumulator lives (ADR-0065 point 3).

- `game/include/game/steps.h` (new) — `VOE_GAME_STEP_SECONDS` (1/60), `VOE_GAME_STEPS_MAX` (4);
  `voe_game_steps { double banked; }`; `float voe_game_steps_run(voe_game_steps *steps,
  voe_ecs_world *world, voe_platform_window *window, const voe_3d_shapes *shapes, double elapsed,
  void (*systems)(const voe_game_project_step *))`: banks `elapsed`, runs whole steps up to the
  max, drops what is left past it to under one step, and returns the lag
  (1 − banked / step). One step, in 0254 point 2's order: `voe_scene_transform_remember`,
  `systems` with `seconds` the step, `voe_game_world_step` (card 16),
  `voe_physics_body_system_move`, `voe_scene_transform_system_run`. Header points: why fixed
  (the jump is as high at 30 and 240 frames a second), why a maximum, why the systems are handed
  in (the game's archive is exported by the editor on Windows and must not name a project's entry
  point outside `run.c`, 0245).
- `game/src/steps.c` (new); `game/src/src.md`.
- `game/src/run.c`, `game/include/game/run.h` — each frame: the clock's elapsed seconds into
  `voe_game_steps_run` with `voe_game_project_systems_run`, then `voe_game_frame` with the lag.
  The project's systems no longer run once a frame. Header: the new order.
- `game/tests/steps.c` (new) — a world from `voe_game_world_new`, a stub systems function that
  counts calls: exactly one step's time runs one and returns lag 1 (nothing banked);
  2.5 steps' time runs two and returns lag 0.5; one second runs four and leaves under
  one step banked; a body over a box floor with the stub pulling it down lands and reads on_floor
  after 60 steps. `game/tests/tests.md`.
- `game/include/game/game.md` — the entry for `steps.h`.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder game` prints `FINDINGS: 0`.
2. `ctest --test-dir build/debug -R '^game/steps$'` passes.
