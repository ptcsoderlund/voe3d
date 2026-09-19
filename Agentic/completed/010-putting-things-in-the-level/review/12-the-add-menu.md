# 12 — The Add menu
folder: editor
decisions: 0168, 0190, 0193, 0177

## Change
New `src/entities.h` / `src/entities.c`: what the editor does to which entities exist and which rows they hold.
All of it goes through the structural queue (0193). No function here draws or selects.
- `typedef enum { VOE_EDITOR_ADD_ENTITY, VOE_EDITOR_ADD_CUBE, VOE_EDITOR_ADD_CAPSULE, VOE_EDITOR_ADD_CYLINDER }
  voe_editor_add;`
- `[[nodiscard]] bool voe_editor_entities_add(voe_ecs_world *world, voe_editor_add what, voe_ecs_entity *out)`:
  `voe_ecs_entity_create`, then `voe_ecs_structure_add` an identity (0193's id and name). For the three shapes it
  also adds the transform's default row (`voe_ecs_component_default`) and the shape's default row with `kind` set
  to `VOE_3D_SHAPE_CUBE`, `_CAPSULE` or `_CYLINDER`. False when the world or the queue is full. The entity is then
  destroyed through the queue if it was made.
- `[[nodiscard]] bool voe_editor_entities_component_add(voe_ecs_world *, voe_ecs_entity, voe_ecs_type)`: that
  type's default row. `[[nodiscard]] bool voe_editor_entities_component_remove(...)` with the same arguments.
  `[[nodiscard]] bool voe_editor_entities_delete(voe_ecs_world *, voe_ecs_entity)`.
  `[[nodiscard]] bool voe_editor_entities_duplicate(voe_ecs_world *, voe_ecs_entity source, voe_ecs_entity *out)`:
  every described row the source has, the identity's id and name replaced (0193). Cards 13 and 14 call these
  three.
- The name and id rules are internal helpers, and the header says them.

`src/project.c`: the project world's limits gain `.structure_requests = 256` and `.structure_bytes = 32768`.
`src/main.c`: `voe_ecs_structure_apply(session.project->world)` once a frame, before the first system runs.
The header's frame-order paragraph says so.

The Scene panel (`src/dock.c`, state in `src/scene.h` / `scene.c`): an **Add** button above the list. Clicking it
shows four buttons under it: Entity, Cube, Capsule, Cylinder. A second click on Add hides them, and so does a
press anywhere else. Choosing one calls `voe_editor_entities_add`, selects the new entity, hides the list and
counts one structural change in a new `uint32_t structural` on `voe_editor_scene`, zeroed each frame.
`src/main.c` calls `voe_editor_session_edited` when it is non-zero, where it already does for
`scene.inspector.replaced`. A false return sets the session notice "The scene is full." and counts nothing.
Raise `src/interface.h`'s budgets by what the menu adds, itemized as the others are.

Update `editor.md` and `src/src.md`.

## Done when
`checks.sh` for `editor` passes (the build). With `C=$(mktemp -d)`, `./build/debug/editor/voe_editor --capture
$C/o.png` exits 0, and reading it (ADR-0177) shows the Add button at the top of the Scene list.
