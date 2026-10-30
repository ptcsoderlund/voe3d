# 06 — Each foliage kind is its own folding box
folder: editor
after: 05
decisions: 0168, 0496, 0502, 0503

## Change
- `editor/src/inspector_foliage.c` and `.h`. Inside the Foliage section's body, each kind is a
  section begun with `voe_editor_folds_begin`:
  - It is named `voe_editor_folds_kind_name(path, k)`, where `path` is the landscape's
    project-relative path as the entity's model row names it, and keyed `kind` with index k.
  - Its title is the kind's choice row (`kind_rows[k]`), selected while the kind is picked. It holds
    the model's file name as a HEADING label. The title still picks and lights the kind. Only the
    fold button folds.
  - Its body holds Base size, Cast shadows, Blend with ground, Blend height and the three spreads,
    drawn only while it is open. An undrawn control stays `VOE_UI_NODE_NONE`; check that the read
    skips it.
  - Scatter and Erase stay above the boxes. The add row and Density stay below them.
  - `kinds_seen` keeps the title's rectangle, so a drop on a title still replaces that kind's
    model.
  - The header says: the boxes, the title picks, the fold button folds, and names follow 0502.
- `editor/src/assets_drag.c`, and the add-row outcome in `assets_drag.h`'s comment: once a kind is
  added on the add row, call `voe_editor_folds_clear(&scene->folds, name)`. `name` is
  `voe_editor_folds_kind_name` of the same landscape path and the new kind's index. That way the
  kind starts open (0502 point 2). A replaced model keeps its fold, so nothing is needed there.
- `editor/src/src.md`: the `inspector_foliage` and `assets_drag` entries.

## Done when
`grep -q voe_editor_folds_kind_name editor/src/inspector_foliage.c && grep -q voe_editor_folds_clear editor/src/assets_drag.c`
exits 0.

The human, in the editor, walks the feature's How to test:
1. On the hill, select the landscape. Each Foliage kind is its own bordered box, titled with its
   model's name as a heading.
2. A kind's `-` folds it to its title line, and its `+` opens it as it was.
3. Clicking a kind's name picks and lights it, and it does not fold. Painting grows that kind.
4. With all kinds but one folded, the open kind's settings show without scrolling. Changing one of
   them moves the forest.
5. Drop a `.glb` on the add row: the new kind's box is open.
6. A light, a model and the clouds row: every section has a heading and folds the same way.
7. Restart the editor: what was folded is still folded.
8. Ctrl+Z steps back through setting changes and never through folds.
