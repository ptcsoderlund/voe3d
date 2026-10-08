# 26 — The Assets panel scrolls its rows
folder: editor/src
after: none
decisions: 0168, 0378

## Change
Bug 01: the Assets panel never scrolls. The fault is in the panel, not in
`ui` or the dock: `dock_walk.c` hands every leaf a scroll area that scrolls
when its children's natural size exceeds it, but `voe_editor_assets_draw`
(`editor/src/assets_panel.c`) puts everything inside one `body` column sized
`VOE_UI_SIZE_GROW` along, and a grow child adds nothing to its container's
natural size (`ui/include/ui/layout.h`, GROW). So the area measures nothing to
scroll and the rows past its edge are cut off. Fix it once, in the panel.

Files: `editor/src/assets_panel.h` and `editor/src/assets_panel.c` only.

In `assets_panel.c`, `voe_editor_assets_draw`:
- `body` becomes natural along (still stretched across by the area), holding
  the Up/Import row, the path, the naming field and the rows as now.
- `empty`, still inside `body` after the rows, becomes a fixed strip (a
  `#define` near the top, about 6 mm) so a right press for Create still finds
  an empty part when the rows overflow and the list is scrolled to its end.
- After `body` closes, a new sibling node `fill`, `VOE_UI_SIZE_GROW` along,
  takes what is left of the area when the rows fit, and nothing when they
  overflow. The "no `Assets/`" line path keeps working; `fill` is
  `VOE_UI_NODE_NONE` wherever `empty` and `body` are reset today.

In the read after the frame (where `body_seen` and `empty_seen` are taken):
also keep `fill_seen`.

In `voe_editor_assets_row_at`: `at` is OUTSIDE only when it is in neither
`body_seen` nor `fill_seen`; in `empty_seen` or `fill_seen` it is EMPTY. Every
other use of `body_seen` in the file is checked the same way, so the panel's
"whole visible rectangle" is `body` plus `fill`.

In `assets_panel.h`:
- struct `voe_editor_assets`: add `voe_ui_node fill` and `voe_ui_rect
  fill_seen` beside `empty`/`empty_seen`; their comments say what each is.
- The header's "THE ROWS SCROLL IN THE LEAF'S OWN SCROLL AREA" paragraph
  makes these points: the body is natural so the area measures the rows and
  scrolls them, wheel and scrollbar both being the area's; why it must not
  grow; the empty part is a fixed strip under the rows plus the filler after
  the body; the right button and the drag read the two as the empty part.
- The draw's comment names the filler.

`editor/src/src.md`'s entries for the two files stay true; change them only if
they stop being so.

## Done when
- `grep -c VOE_UI_SIZE_GROW editor/src/assets_panel.c` prints `1` (the filler).
- The human's, in the editor: open a project whose `Assets/` has more entries
  than fit, or drag the Scene/Assets seam down until the rows overflow. The
  wheel over the Assets panel scrolls it and a scrollbar shows at its right;
  the last row can be selected, renamed with F2 and dragged into a view. A
  right press under the last row, scrolled to the end, opens Create. With few
  rows, a right press anywhere under them opens Create and no scrollbar shows.
