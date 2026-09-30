# 16 — The tank game's placed types call for their need
folder: examples/tank_game
after: 15
decisions: 0168, 0303

## Change
Bug 01, after card 15: `voe_game_project_type` is six members again, and a
need is `voe_game_project_component_needs`. Read `game/include/game/project.h`
(the function's comment), then the register function in each of
`examples/tank_game/Code/tank_hull_system.c`, `tank_turret_system.c`,
`tank_gun_system.c`, `tank_shell_system.c`, `tank_spawner_system.c` and
`tank_enemy_system.c`.

- In each of those six files, the registration that passes
  `&voe_scene_transform_key` as a seventh initialiser drops it, and after a
  successful registration the file calls
  `voe_game_project_component_needs(world, &<its key>,
  &voe_scene_transform_key)`, the register function returning false when
  either fails. In `tank_shell_system.c` only the shell type needs it, as
  now; the shot type stays as it is.
- `tank_breakable.c`, `tank_camera_system.c`, `tank_control_system.c` and
  `tank_lives_system.c` are not touched: they build unchanged once card 15
  has run.
- `examples/tank_game/Code/Code.md`: nothing, unless an entry names the
  seventh member.

## Done when
From the repo root,
`for f in examples/tank_game/Code/*.c; do clang -std=c23 -fsyntax-only -Wall -Wextra -Wpedantic -Werror $(printf -- '-I%s ' */include) -I"${f%/*}" "$f" || exit 1; done`
exits 0, and
`grep -l voe_game_project_component_needs examples/tank_game/Code/*.c | wc -l`
prints 6.

The human's, feature step 7: open `examples/tank_game` in the editor; the
project's code builds; give a bare entity Breakable, press Play, and the
game plays as before.
