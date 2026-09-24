# 02 — The game registers a project's components and runs its systems
folder: game
decisions: 0168, 0239, 0242

## Change
Points 2 and 3 of 0242, in the engine's `game` folder. Nothing builds a project yet (card 03).

- `game/include/game/project.h` (new) — the seam a project's code is written against:
  - `voe_game_project_step` {world, window (`voe_platform_window *`), seconds (double)};
  - `voe_game_project_type` {key, size, capacity, description, default_row (NULL for zeros), menu};
  - `VOE_GAME_PROJECT_TYPES` 32 and `VOE_GAME_PROJECT_ROW` 240;
  - `VOE_GAME_PROJECT_DESCRIPTION(name)`: `name##_description()` when `VOE_BASE_DESCRIPTIONS` is
    1, else `&voe_ecs_description_compiled_out` (base/describe.h);
  - `bool voe_game_project_component(voe_ecs_world *, const voe_game_project_type *)`;
  - `void voe_game_project_replaces_apply(voe_ecs_world *)`;
  - the two entry points, declared here and defined by the project, never by the engine:
    `void voe_game_project_register(voe_ecs_world *)` and
    `void voe_game_project_systems_run(const voe_game_project_step *)`.
  The header says: key and struct share one name; a row over the cap, a 33rd type or a zero size
  are refused and reported (false), not asserted; the editor calls register and never the systems;
  who resolves the entry points (link in the game, one lookup in the editor, ADR-0008); a project
  system writes its own rows directly and other folders' through their intents.
- `game/src/project.c` (new) — 32 static `voe_ecs_key`s, `voe_game_project_replace_0` to `_31`.
  `voe_game_project_component` takes the first key with no intent in this world (ask ecs/intent.h
  whether one is registered; if it only asserts, keep the count by the number of types the world
  has past `VOE_GAME_WORLD_TYPES`), registers the table, an intent of size row offset + size with
  room for `VOE_GAME_WORLD_AUTHORED`, `voe_ecs_component_replace_set` with the entity at 0 and the
  row at the next `alignof(max_align_t)` boundary, the default and the menu. `_replaces_apply`
  drains each of those queues: a live entity that has the row gets it set whole; any other is
  dropped silently; each queue is cleared.
- `game/include/game/world.h`, `game/src/world.c` — `VOE_GAME_WORLD_TYPES` 8 names the engine's
  count; the limits get room for `VOE_GAME_PROJECT_TYPES` more types and as many more intents (and
  whatever row or queue byte budget the limits need for them; read ecs/world.h). Header: the room
  for a project's types.
- `game/src/frame.c`, `game/include/game/frame.h` — `voe_game_project_replaces_apply` right after
  the structural queue; THE ORDER paragraph says so.
- `game/src/run.c`, `game/include/game/run.h` — `voe_game_project_register(world)` after
  `voe_game_world_new` and before `voe_game_scene_build`; in `run_frames`, before each
  `voe_game_frame`, `voe_game_project_systems_run` with the world, `voe_app_window(app)` and
  `frame.tick.step`. The header says the project's code runs there.
- `game/tests/project.c` (new) — a test type (a struct with a described float) registered through
  `voe_game_project_component` on a `voe_game_world_new` world: its menu and default read back; a
  replace submitted through `voe_ecs_component_replace`'s layout and applied lands whole; one for a
  destroyed entity is dropped and the queue is empty after; a row of 241 bytes is refused.
- `game/game.md`, `game/include/game/game.md`, `game/src/src.md`, `game/tests/tests.md` — the
  entries for project.h, project.c and tests/project.c; world, frame and run entries if their
  sentence changes.

## Done when
1. `checks.sh --folder game` prints `FINDINGS: 0`; `ctest --test-dir build/debug -R '^game/project'`
   passes.
