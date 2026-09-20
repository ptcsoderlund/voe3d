# 07 — A named field is drawn as a dropdown
folder: editor
decisions: 0168, 0194, 0195, 0196, 0198

## Change
`editor/src/inspector.h`: `voe_editor_inspector_control` gains

	// The value names of a named field (base/describe.h), NULL for every
	// other control. Set, this control is the dropdown's closed button and
	// writes nothing itself: a fired one opens the list
	// (inspector_edit.h), and `writes` is the UINT32 the chosen value is.
	const voe_base_field_names *names;

with `<base/describe.h>` already among its includes. Its prose gains, beside the paragraph about a colour being a
swatch: A NAMED FIELD IS A DROPDOWN (0195, 0198) — a field whose description carries names is shown by the name
of the value it holds and not by its number, inside a button when the type has a replace intent and the field is
not read-only and as a plain label otherwise, for the same reason a read-only number is a label; a value no entry
names is shown as the number it is, which is what an older build seeing a newer file's kind shows; and the button
only opens the list, the choice arriving later through `voe_editor_inspector_named_submit`.

`editor/src/inspector.c`, in `field_row` and nowhere else: before the number-box path, a field of kind
`VOE_BASE_FIELD_UINT32` with rank 0 whose name `voe_base_names_find` finds in the component description the walk
already has is drawn this way and not as a number.

- The text is `names->values[value]` when `value` is inside `value_count` and that entry is not NULL, and
  otherwise whatever `value_text` already formats for this field, in the frame's arena either way.
- Editable — the same `shown_only` test the file already makes, so a replace intent and not read-only — it is a
  `voe_ui_button_begin` with that text as a plain `voe_ui_label` inside it, so a held button draws its label in
  `inverse_ink` (ADR-0196), recorded through the file's own `record` with `writes = VOE_BASE_FIELD_UINT32`,
  `offset` the field's, `size` nought, and `names` set. Shown only, it is the label the file already draws.
- The `room` check comes first, as it does for every other control.

## Done when
`checks.sh` for `editor` exits 0 and `cmake --build --preset debug` builds the whole tree. The button does
nothing yet — card 08 is what a fired one reaches — and what it looks like on the screen is card 10's walk.
