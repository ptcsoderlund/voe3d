# 29 — A step runs the project's after-the-move systems
folder: game
decisions: 0168, 0256, 0257, 0254, 0245

## Change
0257 points 1 to 3 in `game`. The game executable does not link until cards 30 and 31 define
the new entry point; `game`'s own target and tests do.

- `game/include/game/project.h` — declare `void voe_game_project_systems_after_move(const
  voe_game_project_step *step)` beside the other two, defined by the project. Header points: the
  two slots (0256): `systems_run` is before the move, `systems_after_move` after the bodies' move
  in the same step (a camera following a body belongs there); within a slot the project calls
  its systems in list order and that is the only ordering; a project with none defines it empty;
  "the two entry points" becomes three.
- `game/include/game/steps.h`, `game/src/steps.c` — `voe_game_steps_run` gains a second function
  pointer `after_move`, same type as `systems`, after it. `step_once` appends: `after_move` with
  the same step value, then `voe_game_world_step(world, shapes)` again. Header: the ONE STEP
  paragraph gets the two new calls and why the second world step (0257 point 2); the example
  call passes `voe_game_project_systems_after_move`.
- `game/src/run.c`, `game/include/game/run.h` — pass `voe_game_project_systems_after_move`; the
  run.h ORDER paragraph and constraint name the third entry point.
- `game/tests/steps.c` — every existing call passes a second stub (empty, or counting). New
  test: a body entity and a follower entity with transforms, the before stub pulling the body
  down, the after stub submitting the follower's transform with the body's current position;
  after one step's time the follower's current transform position equals the body's, the body
  has moved from where it started, and both stubs ran once. Its header line names it.
- `game/tests/tests.md` — `steps.c` entry mentions the after-the-move slot.
- `game/include/game/game.md` — `project.h` entry: three entry points; `steps.h` entry: the two
  slots.
- `game/src/src.md` — `steps.c` entry: the step's calls including the second world step.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder game` prints `FINDINGS: 0`.
2. `ctest --test-dir build/debug -R '^game/steps$'` passes.
