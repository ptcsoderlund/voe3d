# 38 — The Inspector edits cut-out, sides and the emission map
folder: editor
after: 37
decisions: 0168, 0399, 0420

## Change
The open material's section shows card 04's keys but emission's colour and strength (card 39). Read
the header of `editor/src/inspector_material.h` first.

- `editor/src/inspector_material.h`, `editor/src/inspector_material.c`: under Repeat, a Cutout slider
  0–1, and Double-sided as two buttons,
  One side and Both sides, the chosen one lit as Lit and Unlit are; under ORMmap, an Emissionmap row as
  the other map rows, with ×. `VOE_EDITOR_MATERIAL_SLIDERS` and `VOE_EDITOR_MATERIAL_MAPS` grow; the
  read takes each into the shown copy only, as now. Header: the list of controls.
- `editor/src/assets_drag.h`, `editor/src/assets_drag.c`: a picture dropped on the Emissionmap row
  fills `emissionmap`, as the other three rows.
- `editor/src/interface_read.c`: a changed cutout or sides is a live set like Roughness (no reload); a
  changed Emissionmap is a reload with maps, as the other maps are.
- `editor/src/src.md`: entries follow (each at most 300 characters).

## Done when
`voe_editor` builds in the folder's check with no warning. The human: open a material, drag Cutout and
press Both sides; a model wearing it changes at once; drop a PNG on Emissionmap; Ctrl+Z puts each back.
