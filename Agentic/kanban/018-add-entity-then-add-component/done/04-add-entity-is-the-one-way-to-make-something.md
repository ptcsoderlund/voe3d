# 04 — Add entity is the one way to make something
folder: editor
decisions: 0168, 0189, 0193, 0217

## Change
The Scene panel's Add menu (Entity, Cube, Capsule, Cylinder) becomes one button, **Add entity**, which makes
an entity with an identity and a transform at its default row (the origin, 0189) and nothing else.

- `editor/src/entities.h` / `entities.c` — drop `voe_editor_add` and the three shape bases. The call becomes
  `voe_editor_entities_add(voe_ecs_world *world, voe_ecs_entity *out)`: a new id and the name "Entity" by the
  existing rules, the identity row and the transform's default row (`voe_ecs_component_default` of the type
  found with `voe_ecs_component_type(world, &voe_scene_transform_key)`, from
  `scene/transform_component.h`), both queued, the failure rule unchanged. The shape include goes if nothing
  else uses it. Header: the usage example, the name paragraph ("Entity" for an add) and the enum's comment
  follow; add a point that there is no shortcut that adds components with the entity (0217).
- `editor/src/scene.h` / `scene.c` — drop `VOE_EDITOR_SCENE_ADD_CHOICES`, `add_choices` and `adding`, and
  `pointer_was_down` if only the menu read it. `voe_editor_scene_add_menu_record` becomes
  `voe_editor_scene_add_record(scene, voe_ui_node add)`. In `voe_editor_scene_clicks_read`, a fired Add entity
  adds, selects the new entity and counts one in `structural`; refusal returns false as today. The header's
  Add menu paragraph and the two functions' comments say this.
- `editor/src/dock.c` — the Scene panel's Add block (the comment beginning "SCENE SAYS `adding`" and the code
  under it) draws the one button labelled "Add entity" and records it.
- `editor/src/src.md` — the `entities.h`, `entities.c`, `scene.h` entries where they name the Add menu.

Nothing else calls `voe_editor_entities_add` (checked while planning); `project.c`'s untitled scene is
untouched.

## Done when
`grep -rnE "VOE_EDITOR_ADD_|add_choices|ADD_CHOICES" editor/src` prints nothing, and the Checks line of
`CLAUDE.md` with `{folder}` = `editor` exits 0.
