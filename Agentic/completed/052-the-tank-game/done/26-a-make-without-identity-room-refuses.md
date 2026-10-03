# 26 — Add entity, a drop and Duplicate refuse rather than make an entity with no identity
folder: editor/src
after: 22, 25
decisions: 0168, 0193, 0300, 0335, 0337

## Change
0337 point 4, the editor's half. The fault: with the identity table full, `entity_make` and
`voe_editor_entities_duplicate` create the entity and queue its identity, the queue's apply drops
the identity silently (ecs/structure.h), and the entity lives on with none. Read
`editor/src/entities.h`, `editor/src/entities.c` and `editor/src/src.md`.

- `entities.c`: one static check, used by `entity_make` (Add entity, model drop, prefab drop) and by
  `voe_editor_entities_duplicate`, before `voe_ecs_entity_create`: false when the identity type's
  `voe_ecs_component_count` has reached its `voe_ecs_component_capacity` (ecs/component.h, card 22).
  Refused, nothing is created or queued, and the callers' existing false path ("The scene is full.")
  is what the person sees; no caller changes.
- `entities.h`: the "AN ADD MAKES AN IDENTITY ALONE" paragraph says every entity this file makes has
  an identity, always (0335), and so a make or duplicate refuses when the identity table is full,
  because the queue would drop the identity and leave a bare entity no panel can name. Each make's
  and the duplicate's "False when" names the full identity table. CONSTRAINTS: the count read is the
  table as it stands, so the one-make-a-frame rule already stated covers this check too.
- `src.md`: `entities.c`'s entry names the identity room check.

## Done when
`grep -c voe_ecs_component_capacity editor/src/entities.c` prints 1 or more, and the folder builds.

Human: open `examples/tank_game` in the editor, click Add entity, select the new entity: the
Inspector shows its Identity section, named "Entity" (or "Entity N"), with no Remove.
