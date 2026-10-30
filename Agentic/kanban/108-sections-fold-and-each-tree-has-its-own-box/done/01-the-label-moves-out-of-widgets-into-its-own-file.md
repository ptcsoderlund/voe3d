# 01 — The label moves out of widgets.c into label.c
folder: ui
after: none
decisions: 0168

## Change
`ui/src/widgets.c` is 881 lines and 02 changes its label. Split the label out by function first.
Nothing changes in behaviour.

- New `ui/src/label.c`, with a header comment: the label's build and its records. Move these out of
  `widgets.c`: `voe_ui_label_role`, `voe_ui_label`, `push_glyph`, `label_ink` and `push_label`.
  Also move the part of widgets.c's header comment that is about the label: the one sign inside a
  label, baseline against corner.
- `ui/src/widgets.c`: those functions are gone. `emit` calls the label's push through a shared
  name, `void voe_ui_push_label(voe_ui_context *ui, uint32_t node)`. Any static helper both files
  then need (`control_above`, perhaps) gets a shared `voe_ui_` name in the same way. The header
  comment's list of "widgets that answer nothing" sends the reader to `label.c` for the label.
- `ui/src/context.h`: declare the newly shared names, each saying which file owns it, the way the
  button's and the field's shared names are declared.
- `ui/src/src.md`: add a `label.c` entry. `widgets.c`'s entry stops claiming the label.

## Done when
`test "$(wc -l < ui/src/widgets.c)" -lt 800` exits 0, and the test `ui/widgets` passes unchanged.
