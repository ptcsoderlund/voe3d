# 04 — A project type may name the type it needs
folder: game
after: 02
decisions: 0168, 0302

## Change
0302 point 5. Read `game/include/game/project.h`, `game/src/project.c`,
`game/include/game/world.h` (the engine's types are registered before a
project's), `game/tests/project.c` and `game/tests/tests.md`.

- `game/include/game/project.h`: `voe_game_project_type` gains a last
  member `const struct voe_ecs_key *needs;`, NULL for none, so every
  existing positional initialiser still compiles. Its comment: the key of a
  type a row of this one does nothing without, such as
  `voe_scene_transform_key`, told to `ecs` so Add component brings it and
  the Inspector keeps it (0302); it must be an engine type or a project type
  registered before this one.
- `game/src/project.c`: `voe_game_project_component` calls
  `voe_ecs_component_needs_set(world, table,
  voe_ecs_component_type(world, type->needs))` when `needs` is not NULL,
  beside the menu path.
- `game/tests/project.c`: a check that a project type registered with
  `needs` = `&voe_scene_transform_key` answers `voe_ecs_component_needs`
  with the transform's type, and one registered with NULL answers false.
  `game/tests/tests.md`: the entry.
- `game/game.md`: only if it describes the type's members.

## Done when
The new check in `game/tests/project.c` passes in the folder's checks.
