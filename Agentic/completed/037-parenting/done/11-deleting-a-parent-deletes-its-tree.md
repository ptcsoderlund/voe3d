# 11 — Deleting a parent deletes its tree, and Remove unparents in place
folder: editor
decisions: 0168, 0281, 0271, 0218, 0204

## Change
Needs card 06. 0281 point 5; the camera is never deleted (0218), so neither is a tree holding it.

- `editor/src/entities.h`, `editor/src/entities.c`:
  - `voe_editor_entities_delete` queues the destruction of the entity and everything under it:
    `voe_scene_parent_tree` into a local `voe_ecs_entity[VOE_EDITOR_SCENE_ROWS]` (authored
    entities are all an editor world has), one `voe_ecs_structure_destroy` each. False when the
    queue is full. Its comment says the tree goes.
  - `voe_editor_entities_component_remove` on the parent type (`voe_scene_parent_key`) calls
    `voe_scene_parent_set(world, entity, (voe_ecs_entity){ 0 })` instead of a plain remove, so the
    Inspector's Remove on "Parent" keeps the thing where it is. One line in the header says so.
- `editor/src/scene.h`, `editor/src/scene.c`: `voe_editor_scene_delete` refuses when the camera's
  entity is the selection or anywhere under it (walk `voe_scene_parent_tree` of the selection);
  its comment says so.
- `editor/src/src.md`: the `entities` and `scene.c` entries mention the tree.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_editor` exits 0. The human's:
Delete on a hull with a turret and barrel under it removes all three; Ctrl+Z brings all three
back, still nested; Remove on the barrel's Parent section leaves it where it was, at the top.
