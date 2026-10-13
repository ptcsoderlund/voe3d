# 39 — The Inspector edits emission colour and strength
folder: editor
after: 38
decisions: 0168, 0399, 0420

## Change
The open material's emission colour as a second swatch, and its strength. Read the headers of
`editor/src/inspector_material.h` and `editor/src/scene.h` (the picker's target) first.

- `editor/src/scene.h`, `editor/src/scene.c`: `voe_editor_picking` with `material` set also uses
  `offset`, the bytes from the start of `voe_assets_material_file` to the three floats it edits
  (`colour` or `emission`); the comment says so.
- `editor/src/inspector_material.h`, `editor/src/inspector_material.c`: under Colour, an Emission
  swatch opening the picker on `emission`, and Strength, a number box clamped 0–1000; the read takes
  them into the shown copy. Header: the list of controls.
- `editor/src/interface_read.c` and the picker's submit: a picked colour lands in the field the offset
  names; emission and strength are live sets, as Colour is.
- `editor/src/src.md`: entries follow (each at most 300 characters).

## Done when
`voe_editor` builds in the folder's check with no warning. The human: open a material worn by a
model, pick an orange Emission and Strength 3; the part glows; Colour's swatch still edits colour.
