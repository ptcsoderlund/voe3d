# 14 — Add and remove components
folder: editor
decisions: 0168, 0190, 0193, 0177

## Change
`src/inspector.h` / `inspector.c` (still naming no component, 0193):
- Types registered runtime-only are not shown.
- A section's heading is the key name's last `_` word, capitalised (`voe_3d_shape` → "Shape").
- Every section has a **Remove** button, except the section of the type the Scene list is built from. That type
  is identity: `voe_editor_inspector_draw` is handed it as a `voe_ecs_type`, from
  `voe_ecs_component_type(world, &voe_scene_identity_key)` in `dock.c`. After the frame, a fired Remove calls `voe_editor_entities_component_remove`.
- A section whose type has a `voe_ecs_component_needs` the entity lacks shows the line "Needs <Heading>" under
  its heading.
- At the bottom, an **Add component** button shows one button per described type the entity does not have. The
  list is hidden again by a second click, a choice, or a press elsewhere. A choice calls
  `voe_editor_entities_component_add`.
- Each success adds one to `scene->structural`. A false return sets the notice "The scene is full.", as card 12
  does.

Raise `src/interface.h`'s budgets. Update the header's opening paragraphs, `editor.md` and `src/src.md`.

## Done when
`checks.sh` for `editor` passes. With `C=$(mktemp -d)`, `./build/debug/editor/voe_editor --capture $C/o.png`
exits 0, and reading it (ADR-0177) shows the Inspector unchanged with nothing selected.
