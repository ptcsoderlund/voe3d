# 05 — The tank game's placed types need a transform
folder: examples/tank_game
after: 04
decisions: 0168, 0302

## Change
0302 point 5: with card 04 a project type can name what it needs. Read
`game/include/game/project.h` (the `needs` member), then the registration
call in each of `examples/tank_game/Code/tank_hull_system.c`,
`tank_turret_system.c`, `tank_gun_system.c`, `tank_shell_system.c`,
`tank_spawner_system.c`, `tank_enemy_system.c` and `tank_breakable.c`, and
`examples/tank_game/Code/Code.md`.

- Hull, Turret, Gun, Shell, Spawner and Enemy each register with `needs` =
  `&voe_scene_transform_key` (include `scene/transform_component.h` where a
  file lacks it): each is placed in the world and its header already says it
  needs a transform.
- Breakable stays without `needs`: it is data a shell's hit reads, so a bare
  entity may carry it (the feature's step 7). Its header says so in a line.
- `examples/tank_game/Code/Code.md`: nothing unless an entry says what a
  type needs.

## Done when
`grep -l voe_scene_transform_key examples/tank_game/Code/tank_{hull,turret,gun,shell,spawner,enemy}_system.c | wc -l`
prints 6, and `grep -c voe_scene_transform_key examples/tank_game/Code/tank_breakable.c`
prints 0.
