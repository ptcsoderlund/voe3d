# 12 — The Panels list is as wide as its rows
folder: editor
after: 11
decisions: 0168, 0363, 0364

## Change
Bug 01: the Panels list's background is only the tick column wide; the names
spill over the scene. Cause: in `editor/src/panels_menu.c` each row inside a
button is `VOE_UI_SIZE_GROW` along, and a grow child adds nothing to its
parent's natural size (`ui/include/ui/layout.h`), so each button, and the
panel FILLing to them, comes out the padding plus the tick column. A button
centres what it holds (`ui/include/ui/widgets.h`), so a row merely made
natural would centre each name differently.

`editor/src/panels_menu.h`:
- `voe_editor_panels_menu` gains `voe_ui_node names[VOE_EDITOR_CLOSABLE_COUNT]`
  (the row inside each button, as drawn) and `float rows_wide`: the widest
  of those rows' content as measured in the last frame the list was drawn,
  millimetres, nought until one has been.
- `voe_editor_panels_menu_read` takes a non-const `voe_editor_panels_menu *`
  and, besides what it does now, sets `rows_wide` to the largest
  `voe_ui_node_measured(ui, names[i]).x` of this frame. The call in
  `editor/src/interface.c` already passes `&bar->menu`, mutable; open
  interface.c only to confirm it compiles unchanged.
- Header: drop nothing true; add that every row is `rows_wide` wide so the
  names line up and the panel spans the longest, why not GROW (adds nothing to
  the panel's width), and that the first frame after the list first opens
  draws the rows natural (names centred, background whole). Zeroed is still
  closed.

`editor/src/panels_menu.c`:
- The row inside each button: `VOE_UI_SIZE_FIXED` at `menu->rows_wide` when
  it is above nought, else natural; recorded into `menu->names[i]`. Nothing
  else in the draw changes; the tick column and √ stay as they are (card 11
  put √ in the sheet).
- The read: the measuring above, skipping a `VOE_UI_NODE_NONE` row.
- The file's top comment: replace the "row that grows to the button's width"
  explanation with the fixed width from the measure.

`editor/src/src.md`: the `panels_menu.c` entry, if it names the grow row,
names the measured width instead; under the entry cap.

## Done when
- The folder builds.
- `! grep -q 'VOE_UI_SIZE_GROW' editor/src/panels_menu.c` exits 0.
- `d=$(mktemp -d) && XDG_CONFIG_HOME=$d build/debug/editor/voe_editor --capture $d/a.png && test -s $d/a.png`
  exits 0.
- The human, in tank_game, does bug 01's `## How to reproduce`
  (`Agentic/kanban/059-close-and-reopen-panels/bugs/01-the-panels-list-has-no-background.planned.md`):
  the whole list, tick column to the end of "Bottom view", is on one menu
  background, and every open panel shows √. Then steps 1 and 2 of
  `## How to test` in `feature.md`.
