# 04 — A shell knows who fired it and from where
folder: examples
after: 02, 03
decisions: 0168, 0294

## Change
0294 point 1. Files under `examples/tank_game/`: `Code/tank_shell.h`,
`Code/tank_shell_system.c`, `Code/tank_gun_system.c`, `Code/Code.md`. Read
`Code/tank_control.h` and the top of `Code/tank_control_system.c` for a
runtime-only row registered and added through the structural queue, and
`game/include/game/project.h` for the spawn. Never edit `main.scene` or
`Assets/tank_body.prefab` (0272).

- `tank_shell.h`: `tank_shot`, runtime-only, `TANK_SHELL_ROWS` rows:
  `owner` (`voe_ecs_entity`), `from` (`voe_math_double3`), `hit` (bool),
  `target` (`voe_ecs_entity`); its `tank_shot_key`. `[[nodiscard]] bool
  tank_shell_fire(const voe_game_project_step *step, const char *prefab,
  voe_math_double3 position, voe_math_quat rotation, voe_ecs_entity owner,
  voe_math_double3 from)`: spawns the prefab and queues a `tank_shot` row
  onto its root; false when either is refused. Header points: the shot row,
  who adds it (the one firing, at the spawn) and who writes it after (this
  system); why `from` is not the muzzle (0294 point 1).
- `tank_shell_system.c`: `tank_shell_register` registers `tank_shot` too,
  runtime-only, no menu; `tank_shell_fire` as above. The flight is
  unchanged (05 sweeps it).
- `tank_gun_system.c`: fires through `tank_shell_fire` in place of the
  bare spawn: `owner` the gun's topmost ancestor by
  `scene/include/scene/parent_component.h` (itself with no parent),
  `from` the gun's world position
  (`scene/include/scene/transform_component.h`). Header: the shot row.
- `Code.md`: the shell and gun entries, each under 300 characters.

## Done when
`(for f in examples/tank_game/Code/*.c; do clang -std=c23 -fsyntax-only -DVOE_BASE_DESCRIPTIONS=1
$(for d in */include; do printf -- "-I%s " $d; done) "$f" || exit 1; done)` exits 0, and `grep -q tank_shell_fire examples/tank_game/Code/tank_gun_system.c`
exits 0.
