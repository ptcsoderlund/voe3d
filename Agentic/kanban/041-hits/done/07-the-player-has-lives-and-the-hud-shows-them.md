# 07 — The player has lives and the HUD shows them
folder: examples
after: 05
decisions: 0168, 0294

## Change
0294 point 4. Files under `examples/tank_game/`: new `Code/tank_lives.h`
and `Code/tank_lives_system.c`, `Code/project.c`, `Code/Code.md`,
`tank_game.md`. Read `Code/tank_control.h` and the top of
`Code/tank_control_system.c` (a runtime-only row added to the first hull),
`Code/tank_shell.h` (the shot row), `game/include/game/project.h` (the
frame), and the header and HUD part of
`examples/coin_game/Code/game_interface.c` for a HUD panel at the top left.
Never edit `main.scene` or `Assets/tank_body.prefab` (0272).

- `tank_lives.h`: `tank_lives`, runtime-only, capacity 1, no menu:
  `lives` (`int32_t`). Register, `tank_lives_run(const
  voe_game_project_step *step)` and `bool tank_lives_interface(const
  voe_game_project_frame *frame)`. Header points: one row, one writer;
  3 at the start; what costs one; never below 0; 0 does nothing until
  milestone 14; read by the enemies (08) for whom to fire at.
- `tank_lives_system.c`: on the first step with a `tank_hull` row and no
  `tank_lives` row, add one of 3 to that hull through the structural queue
  (headless too). Each step with the row: count the `tank_shot` rows with
  `hit` whose `target` is the row's entity or has it as an ancestor
  (`scene/include/scene/parent_component.h`); take that many off, not
  below 0; write the row whole. The interface: a small panel at the top
  left reading `Lives N`, then ends the ui frame and returns true; with no
  row it only ends the frame.
- `project.c`: register it; run it right after the shells, before the
  spawner; `voe_game_project_interface` returns `tank_lives_interface`;
  header's order and interface lines follow.
- `Code.md`, `tank_game.md`: the lives and the HUD, each entry under 300
  characters.

## Done when
, and `grep -q tank_lives_interface examples/tank_game/Code/project.c`
exits 0.
