# 38 — The Errors panel's lines are selected by dragging, and Copy all is a button
folder: editor/src
after: 32
decisions: 0168, 0194, 0242, 0342

## Change
0342 points 1, 2 and 5, the panel's half. Nothing reaches the clipboard yet; card 39 does that.
Read `editor/src/errors.h`, `editor/src/errors.c`, `editor/src/interface.h` (its budget paragraphs
and the Errors panel paragraph), `editor/src/interface.c`, `editor/src/src.md`, and in
`ui/include/ui/widgets.h` the choice and `voe_ui_button_action`, and in `ui/include/ui/layout.h`
`voe_ui_node_rect`.

- `errors.h`. `voe_editor_errors` gains:
  - each line's node;
  - the selection: an anchor line, an end line, whether a drag is under way, and whether any line
    is selected;
  - the copy wanted: none, the selection, or all;
  - Copy all's node.

  `VOE_EDITOR_ERRORS_TEXT_BYTES` is the size of every line plus a `\n` after each.
  `voe_editor_errors_clicks_read` becomes
  `bool voe_editor_errors_read(const voe_ui_context *ui, voe_editor_errors *errors, voe_ui_pointer pointer)`:
  it answers whether Close fired and keeps what the frame did to the selection and to Copy all.
  New:
  - `void voe_editor_errors_copy_selection(voe_editor_errors *errors)` marks a copy of the
    selection; it does nothing when no line is selected.
  - `uint32_t voe_editor_errors_copy_take(voe_editor_errors *errors, char *out, uint32_t capacity)`
    writes the wanted lines into `out`, joined by `\n` with none after the last, clears the want,
    and returns the byte count (0 when nothing was wanted). It asserts that `capacity` is at least
    `VOE_EDITOR_ERRORS_TEXT_BYTES`.

  The header's example and its points cover: the lines are choice rows; press and drag selects the
  run of lines; the copy is exactly what is shown; the panel only marks a copy and the caller takes
  it after the draw, because the interface has no window.
- `errors.c`:
  - Each line is drawn as `voe_ui_choice_begin(ui, "errors_line", i, selected)` around its label.
  - Copy all is a button labelled "Copy all", before Close in the bottom row.
  - The read:
    - A line that is held while no drag is under way sets anchor and end to that line and starts
      the drag.
    - While `pointer.down`, the end is the line whose `voe_ui_node_rect` spans `pointer.at.y`.
      Above the first line it is the first; below the last it is the last.
    - Letting go ends the drag.
    - Copy all firing marks all.
  - `voe_editor_errors_show` clears the selection and the want.
- `interface.c`: where Close is read, call `voe_editor_errors_read` with that root's own pointer,
  the one handed to `voe_ui_pointer_set`. Close hides the panel as before.
- `interface.h`. The Errors panel's budget paragraph adds:
  - 48 line choices, plus Copy all's button and label, as nodes: 50 more;
  - each choice's border and fill, 96, and Copy all's border, fill and seven letters, 9, as
    elements: 105 more.

  `VOE_EDITOR_INTERFACE_NODES` becomes 2325 and `VOE_EDITOR_INTERFACE_ELEMENTS` becomes 47603. The
  panel paragraph says a drag selects lines and Copy all marks a copy.
- `src.md`: `errors.h` and `errors.c` name the line selection, Copy all and the copy taken.

## Done when
`grep -q voe_editor_errors_copy_take editor/src/errors.h && grep -q 'Copy all' editor/src/errors.c && grep -q voe_editor_errors_read editor/src/interface.c && ! grep -rq voe_editor_errors_clicks_read editor/src`
exits 0, and the folder builds.
