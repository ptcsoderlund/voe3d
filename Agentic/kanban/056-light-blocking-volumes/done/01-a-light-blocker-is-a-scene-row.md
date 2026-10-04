# 01 — A light blocker is a scene row
folder: scene
after: none
decisions: 0168, 0347

## Change
The row of 0347 point 1, built as the sun's. Read `scene/include/scene/light_component.h`,
`scene/include/scene/light_system.h`, `scene/src/light_component.c`, `scene/src/light_system.c` and
`scene/tests/light.c` as the pattern. Nothing else changes.

- `scene/include/scene/light_blocker_component.h` (new): `voe_scene_light_blocker` as a described
  field list, one field `size` FLOAT3; its key; the `_get`, `_count`, `_rows`, `_entities` the sun's
  header has. Header points: a box that keeps light from outside out and light from inside in (0347
  point 3, in a sentence); placed, turned and scaled by its transform, so a parent carries it; size
  is metres along its local axes before scale; it draws nothing, casts nothing, collides with
  nothing; how many a world holds is the caller's capacity.
- `scene/include/scene/light_blocker_system.h` (new):
  - `void voe_scene_light_blocker_register(voe_ecs_world *world, uint32_t capacity)`: the table,
    default row size (1, 1, 1), needs a transform, menu "Rendering / Light blocker", the whole-row
    intent as its replace.
  - `[[nodiscard]] bool voe_scene_light_blocker_add(voe_ecs_world *world, voe_ecs_entity entity,
    voe_scene_light_blocker blocker)`: direct; asserts on a row the drain would refuse; false when
    full or the entity is dead.
  - `voe_scene_light_blocker_intent { entity; blocker; }` and `[[nodiscard]] bool
    voe_scene_light_blocker_submit(world, voe_scene_light_blocker_intent)`.
  - `void voe_scene_light_blocker_system_run(voe_ecs_world *world)`: applies the replaces in order,
    refusing a size with a component not finite or below nought (the row is kept).
  - Header points: what is refused; why the replace is queued (rule 4).
- `scene/src/light_blocker_component.c`, `scene/src/light_blocker_system.c` (new): the above, one
  file each, each with its header comment.
- `scene/tests/light_blocker.c` (new): registration (menu path, transform need, default size); add
  and read back; a replace changes the size after a run; a negative, an infinite and a NaN size are
  each refused and keep the row; add on a dead entity is false.
- `scene/scene.md`: entries for the two headers; `scene/src/src.md`, `scene/tests/tests.md`: one
  each for the new files. Each at most 300 characters.

## Done when
The test `scene/light_blocker` passes, and `scene/light` still passes, after the folder's build.
