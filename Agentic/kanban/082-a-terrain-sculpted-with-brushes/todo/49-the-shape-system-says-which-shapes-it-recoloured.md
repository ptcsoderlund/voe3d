# 49 — The shape system says which shapes it recoloured
folder: 3d
after: 48
decisions: 0168, 0389

## Change
0389 point 8: an opt-in, runtime-only record of which shapes the last shape run changed. The bounce
recaptures around them in card 51.

- `3d/include/3d/shape_system.h`:
  - New `void voe_3d_shape_changes_register(voe_ecs_world *world, uint32_t capacity)`. It registers a
    runtime-only table under its own key, once per world, after shapes. `capacity` is the shape table's.
  - New `bool voe_3d_shape_changed(const voe_ecs_world *world, voe_ecs_entity entity)`. True when the last
    `voe_3d_shape_system_run` changed that entity's colour or kind. False when the table is not registered,
    or the entity has no row.
  - Header points: it is opt-in like the previous transforms (`scene/transform_system.h`); only the run
    writes it; a world without it pays nothing; why the bounce needs it (0389).
  - `voe_3d_shape_system_run`'s comment adds that the run clears it first, then marks each drained intent
    whose colour or kind differs from the row.
- `3d/src/shape_system.c`:
  - The run finds the table by walking the world's types, as `scene/src/transform_previous.c` does (read
    its header only).
  - It sets every row false, then sets or adds a true row for each entity whose drained intent changed its
    colour or kind.
  - A row outlives its entity, as a previous row does.
  - Registration as `ecs/include/ecs/component.h` describes for a runtime-only type.
- `3d/src/src.md` and `3d/include/3d/3d.md`: the `shape_system` entries mention the changes table.
- `3d/tests/shape.c`, new cases:
  - `a_recolour_is_a_change_for_one_run`
  - `a_kind_change_is_a_change`
  - `the_same_colour_again_is_no_change`
  - `a_world_without_the_table_reports_no_change`
- `3d/tests/tests.md`: the `shape.c` entry names them.

## Done when
`ctest --test-dir build/debug -R '^3d/shape$'` passes with the four new cases.
