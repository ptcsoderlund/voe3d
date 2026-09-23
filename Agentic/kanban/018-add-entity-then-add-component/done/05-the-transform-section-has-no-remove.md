# 05 — The transform's section has no Remove
folder: editor
decisions: 0168, 0193, 0217, 0221

## Change
The Inspector is handed the types whose section has no Remove button as a list, instead of the one identity
type, and `dock.c` hands it the identity and the transform.

- `editor/src/inspector.h` — `voe_editor_inspector_draw`'s `voe_ecs_type identity` becomes
  `const voe_ecs_type *kept, uint32_t kept_count`: types whose section is drawn with no Remove. The header's
  "WHICH ROWS AN ENTITY HOLDS" paragraph says the kept types are handed in, the identity because the Scene list
  is built from it and the transform because every entity has one (0217), and that the panel still names
  neither. The function's comment follows.
- `editor/src/inspector.c` — where the identity type decides whether a section gets Remove, test membership of
  `kept` instead.
- `editor/src/dock.c` — the call (near the comment "handed the world, the selection and the identity's type")
  passes a two-entry array: the identity's type and the transform's, the transform found with
  `voe_ecs_component_type(world, &voe_scene_transform_key)` (`scene/transform_component.h`). That comment
  follows.
- `editor/src/src.md` — the `inspector.c` entry's "Remove" phrase if it names the identity alone.

## Done when
The Checks line of `CLAUDE.md` with `{folder}` = `editor` exits 0. The human, in 09's walk, sees no Remove on
a transform.
