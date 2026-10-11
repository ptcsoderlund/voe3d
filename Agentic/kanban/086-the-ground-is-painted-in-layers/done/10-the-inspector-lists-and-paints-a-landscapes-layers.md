# 10 — The Inspector lists and paints a landscape's layers
folder: editor
after: 08, 09
decisions: 0168, 0379, 0402

## Change
0402 point 7: a Layers section under Sculpt on a thing wearing a loaded landscape, choosing the
paint brush's layer and taking dropped `.material` files.

- `editor/src/inspector_layers.h`, `editor/src/inspector_layers.c` — new, shaped as
  `inspector_sculpt.h/.c` (forget, draw, read). Draw: a "Layers" heading as Sculpt's; one row per
  layer of the landscape's grid (`voe_3d_models_find` on the row's path, `entry->landscape`),
  showing its material's file name, drawn chosen while `sculpt.chosen`, kind PAINT and `layer`
  is it; then a last row "Drop a material to add a layer" while fewer than eight. Read: a fired
  layer row chooses PAINT with that layer, or none when it was the chosen one. The nodes and
  last frame's row rectangles (the drop needs them, as the material map rows' are kept, see
  `inspector_material.h`) live in `voe_editor_sculpt` (`sculpt.h` gains them, sized
  `VOE_ASSETS_LANDSCAPE_LAYERS` + 1). Header: what 0402 point 7 says of the section.
- `editor/src/inspector_sculpt.c` — a fired sculpt button while PAINT is chosen chooses its kind
  (no sculpt button is drawn chosen while PAINT is).
- `editor/src/inspector.c`, `editor/src/inspector.h` — draw the Layers section right after the
  Sculpt section, on the same condition; the draw needs the model store, passed in as the call
  already takes what it needs (follow how `interface.c` reaches the store).
- `editor/src/interface.c`, `editor/src/interface_read.c` — forget it when the frame opens, read
  it with the Sculpt section's read.
- `editor/src/assets_drag.h`, `editor/src/assets_drag.c` — a new outcome before the `.material`
  one: a `.material` released over the Layers section of a thing wearing a loaded landscape
  (not a prefab's part): over a layer row, `voe_editor_models_layer_set` at that index; over the
  add row, at index layer_count; the project marked unsaved as a stroke does, no undo step. The
  ghost is refused over the section when the landscape has eight layers and the add row is not
  drawn. The header's outcome list gains it.
- `editor/src/src.md` — entries for the new files and for `inspector.*`, `assets_drag.*`,
  `sculpt.*`, `inspector_sculpt.*`.

## Done when
The editor builds and `grep -n "voe_editor_models_layer_set" editor/src/assets_drag.c` prints a
line. The human: select the hill, drop Dirt on the add row (the hill turns dirt), add Grass,
press Grass and paint: grass shows where painted (How to test 1–2).
