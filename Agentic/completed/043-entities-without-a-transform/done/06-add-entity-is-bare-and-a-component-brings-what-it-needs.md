# 06 — Add entity is bare, and a component brings what it needs
folder: editor
after: 01, 02, 03, 04
decisions: 0168, 0300, 0302

## Change
0300 and 0302 point 3. Read `editor/src/entities.h`, `editor/src/entities.c`,
`editor/src/scene.h` (the paragraph near line 44 on Add entity),
`ecs/include/ecs/component.h` (`voe_ecs_component_needs`,
`voe_ecs_component_default`) and `editor/src/src.md`.

- `editor/src/entities.c`: `voe_editor_entities_add` queues the identity
  row and nothing else. The model and prefab adds keep their transform at
  the drop point; a duplicate still copies whatever rows the source has.
- `editor/src/entities.c`: `voe_editor_entities_component_add` follows the
  needs chain first: while the type being added (then its needed type, and
  so on) needs a type the entity has no row of, that needed type's default
  row is queued before the row that needs it, deepest first, so the
  structural queue applies them in an order where each row's need is there.
  A needed collider on an entity with a shape gets the fitting row, as a
  collider added by hand does. The walk stops at a type the entity has, at a
  type with no need, and after as many links as the world has types (a loop
  is a registration bug; assert). A needed type with no default row is the
  program's bug and asserts. False when the queue is full, as now.
- `editor/src/entities.h`: the "AN ADD MAKES AN IDENTITY AND A TRANSFORM"
  paragraph becomes: an add makes an identity alone (0300), an entity
  without a transform is data, listed but in no view; a new paragraph on
  the needs chain (0302 point 3), with the transform brought at the origin
  as the everyday case. The function comments of `_add` and
  `_component_add` say the same in a line each.
- `editor/src/scene.h`: the Add entity sentence says only an identity.
- `editor/src/src.md`: the `entities.h` entry if it says what an add makes.

## Done when
`grep -c voe_ecs_component_needs editor/src/entities.c` prints at least 1,
and the editor builds in the folder's checks.
