# 09 — The round's state: phase, score, a win at the goal, a loss at no lives
folder: examples/tank_game/Code
after: 08
decisions: 0168, 0334

## Change
0334 point 6, the step half; the screens are card 10. Read, in this folder, `tank_lives.h`,
`tank_lives_system.c` (a runtime-only row on the first hull: the pattern), `tank_shell.h`,
`tank_goal.h`, `tank_hull.h`, `project.c`, `Code.md`, and the header of
`scene/include/scene/transform_component.h` for `voe_scene_transform_world`.

- `examples/tank_game/Code/tank_state.h`, new: phases `TANK_PHASE_MENU`, `_PLAYING`,
  `_PAUSED`, `_WON`, `_LOST`; `typedef struct { uint32_t phase; int32_t score; } tank_state;`;
  its key; `tank_state_register` (runtime-only, capacity 1, no menu), `const tank_state
  *tank_state_get(const voe_ecs_world *world)` (NULL before it is made),
  `void tank_state_run(const voe_game_project_step *step)`. Header: a fresh world starts at the
  menu; one row, this module its one writer (the step here, the screens in card 10's file).
- `examples/tank_game/Code/tank_state_system.c`, new: the first step with a hull queues the
  row (menu, 0) onto the first hull. While playing, each step: adds the `points` of every shot
  row with `hit`; at the lives row's 0, lost; when the hull's z is at or below the first
  goal's world z, the goal's points added and won. Written whole.
- `examples/tank_game/Code/project.c`: registers the state. `systems_run` reads the phase
  first: playing, it runs control through lives as now, then the state, then spawner and enemy;
  else only the state. The light fade and camera always run. `systems_after_move` runs the
  scroll only while playing. The header's order to match.
- `examples/tank_game/Code/Code.md`: entries for both files; the project entry.

## Done when
`(for f in examples/tank_game/Code/*.c; do clang -std=c23 -fsyntax-only -DVOE_BASE_DESCRIPTIONS=1
$(for d in */include; do printf -- "-I%s " $d; done) "$f" || exit 1; done)` exits 0, and `grep -q tank_state_run examples/tank_game/Code/project.c` exits 0.
