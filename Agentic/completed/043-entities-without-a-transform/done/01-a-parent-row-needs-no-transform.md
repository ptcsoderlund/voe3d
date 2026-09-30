# 01 — A parent row needs no transform
folder: scene
after: none
decisions: 0168, 0300, 0302

## Change
0302 point 1. Read `scene/include/scene/parent_system.h`,
`scene/include/scene/parent_component.h`, `scene/src/parent_system.c`,
`scene/tests/parent.c` and `scene/tests/tests.md`.

- `scene/src/parent_system.c`: `voe_scene_parent_register` no longer calls
  `voe_ecs_component_needs_set` (it still asserts the transform table is
  registered, since `voe_scene_parent_set` reads it). `voe_scene_parent_set`
  drops the two asserts that the child and a non-zeroed parent have a
  transform. A child without a transform gets the remove and the add and no
  transform intent. A child with a transform under a parent without one gets
  a transform intent whose row is the child's world place as it is now (the
  chain ends at a parent without a transform, as `parent_component.h` says).
  Unparenting is unchanged.
- `scene/include/scene/parent_system.h`: the register comment no longer says
  "and the transform it needs"; the `_set` comment's asserts and what a bare
  child or bare parent gets, per 0300.
- `scene/include/scene/parent_component.h`: one point that an entity without
  a transform may be a parent or a child (0300), a group such as "Lamps".
- `scene/scene.md`: the `parent_system.h` entry if it mentions the need.
- `scene/tests/parent.c`: new checks: a world's parent type names no needed
  type (`voe_ecs_component_needs` is false); a bare entity parented under a
  bare one lands its row and a later unparent removes it; three children with
  transforms parented under a bare entity keep their world place exactly and
  their rows equal what they were; a child with a transform under a bare
  parent under a moved parent with a transform is placed as its own row.
  `scene/tests/tests.md`: the `parent.c` entry.

## Done when
The new checks in `scene/tests/parent.c` pass in the folder's checks, and
`grep -c needs_set scene/src/parent_system.c` prints 0.
