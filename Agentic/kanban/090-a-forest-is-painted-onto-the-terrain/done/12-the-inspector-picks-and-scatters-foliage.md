# 12 — The Inspector picks and scatters foliage
folder: editor
after: 11
decisions: 0168, 0379, 0442

## Change
0442 point 3: a Foliage section under Layers on a thing wearing a loaded landscape: Scatter and
Erase, a row per kind toggling it picked, an add row taking dropped `.glb` files, and the
four foliage sliders.

- `editor/src/inspector_foliage.h`, `editor/src/inspector_foliage.c` — new, shaped as
  `inspector_layers.*` (forget, draw, read; nodes in the sculpt state). Draw: a "Foliage"
  heading as Layers'; Scatter and Erase buttons, drawn chosen while that kind is; one row per
  kind of the grid by its model's file name, drawn chosen while picked; a last row "Drop a
  model to add a kind" while fewer than VOE_ASSETS_LANDSCAPE_KINDS; sliders Density, Size,
  Turn and Lean with figures as the Sculpt section's (Size and Lean in %, Turn in degrees);
  Radius and Softness stay Sculpt's. Read: a fired button chooses its kind, or none when it was
  chosen; a fired row flips its picked bit; sliders write the numbers clamped to sculpt.h's
  ranges. Header: what 0442 point 3 says of the section.
- `editor/src/sculpt.h` — the section's nodes, rectangles seen and figures, sized from
  VOE_ASSETS_LANDSCAPE_KINDS + 1 rows, two buttons and four sliders, beside the Layers ones.
- `editor/src/inspector_sculpt.c`, `editor/src/inspector_layers.c` — a sculpt button or a
  layer row fired while SCATTER or ERASE is chosen chooses its own kind; none of theirs is
  drawn chosen then.
- `editor/src/inspector.c`, `editor/src/inspector.h` — draw the Foliage section right after
  Layers, on the same condition, given what Layers is given.
- `editor/src/interface.c`, `editor/src/interface_read.c` — forget it when the frame opens and
  read it after the Layers read.
- `editor/src/assets_drag.h`, `editor/src/assets_drag.c` — a new outcome before the `.glb`
  ones: a `.glb` released over the Foliage section's kind rows of a thing wearing a loaded
  landscape (not a prefab's part): over a kind row, `voe_editor_models_kind_set` at that
  index; over the add row, at kind_count; the project marked unsaved as a layer drop does, no
  undo step. The ghost is refused over the rows when the add row is not drawn. The header's
  outcome list gains it. The kind's model loads at the next update (card 08).
- `editor/src/src.md` — entries for the new files and for `inspector.*`, `assets_drag.*`,
  `sculpt.*`, `inspector_sculpt.*`, `inspector_layers.*`.

## Done when
The editor builds and `grep -n "voe_editor_models_kind_set" editor/src/assets_drag.c` prints a
line. The human's steps are at card 14.
