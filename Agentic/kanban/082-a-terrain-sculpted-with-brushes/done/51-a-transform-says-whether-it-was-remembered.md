# 51 — A transform says whether it was remembered
folder: scene
after: none
decisions: 0168, 0389

## Change
The bounce marks a new caster as stale (0389 point 7). To know it is new, 3d asks whether the world
remembered it.

- `scene/include/scene/transform_system.h`:
  - New `bool voe_scene_transform_remembered(const voe_ecs_world *world, voe_ecs_entity entity)`. True
    when the world has a previous table and it holds a row for the entity. False otherwise, never an
    assert.
  - The opt-in paragraph is wrong today: "The editor steps nothing and never registers it". It should say
    that a world from `voe_game_world_new` registers the table, that a game remembers each step, and that
    the editor remembers once a frame before its step (0389). The editor draws at lag 0, so its drawing is
    unchanged.
- `scene/src/transform_previous.c`: the function, finding the table the way `between` does. Its header
  gets one phrase.
- `scene/include/scene/scene.md`: the `transform_system.h` entry mentions the new query if it lists the
  calls.
- `scene/tests/transform.c`, new cases:
  - `a_world_without_a_previous_table_remembers_nothing`
  - `a_transform_added_after_the_remember_is_not_remembered`
  - `a_remembered_transform_is_remembered`
- `scene/tests/tests.md`: the `transform.c` entry names them.

## Done when
`ctest --test-dir build/debug -R '^scene/transform$'` passes with the three new cases.
