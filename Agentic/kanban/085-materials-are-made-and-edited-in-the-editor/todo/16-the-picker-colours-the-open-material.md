# 16 — The picker colours the open material
folder: editor
after: 15
decisions: 0168, 0192, 0399

## Change
0399 point 8: the material's Colour swatch opens the same colour picker a shape's does.

- `editor/src/scene.h` / `scene.c` — `voe_editor_picking` gains `bool material`: the picker edits
  `scene->material.colour` rather than an entity's row. `voe_editor_scene_picker_showing` hands back
  that colour while `material` and `material_open` is set; closed when `material_open` empties.
  Update the struct's and the header's paragraph on the picker's targets.
- `editor/src/inspector_material.c` — a fired swatch opens the picker with `material` set, anchored as
  the Inspector's swatch anchors it.
- `editor/src/interface_read.c` (or wherever card 09 left the picker's read) — a `changed` with
  `material` set writes `scene->material.colour`, so card 15's compare carries it to the table and
  store in the same frame; otherwise as before through `voe_editor_inspector_colour_submit`.

## Done when
`grep -n 'material' editor/src/scene.h` shows the picker's field, and the editor builds. Human: the
open material's swatch opens the picker and a cube wearing it changes colour while dragging.
