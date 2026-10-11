# 41 — A list of strings is a field per string
folder: editor
after: 40
decisions: 0168, 0399

## Change
A model row's `materials` (card 23) is a CHAR field of rank 2, eight strings; the Inspector shows a
rank-1 CHAR as a text field and nothing editable for rank 2. Make each string its own row.

- `editor/src/inspector.c` — a CHAR field of rank 2 is one text field row per string, labelled with
  the field's heading and its 1-based number ("Materials 1" … "Materials 8"), each control recording
  the string's byte offset (outer index × inner size) and its own size, so a commit writes that string
  only. Its record names the field and index so card 42 can find the row under the pointer. A part
  shown read-only shows the same rows as labels. Keep `inspector.c` under 800 lines: if the rows push
  it over, move the field-row code into a new `editor/src/inspector_rows.c` with its header first.
- `editor/src/inspector_edit.c` — the text commit takes the recorded offset and size rather than the
  field's whole size, truncating to leave the NUL, as it does for rank 1.
- `editor/src/inspector_value.c` — if it formats a field as one string for a label, a rank-2 CHAR's
  string at an index.

Update `inspector.h`'s and `inspector_edit.h`'s headers where they say a CHAR array is a text field,
and `editor/src/src.md`.

## Done when
`grep -n 'rank == 2' editor/src/inspector*.c` finds the case, and the editor builds. Human: a model
thing's Inspector shows Materials 1–8; typing `Assets/Dirt.material` into Materials 1 and Enter makes
the model's first part wear it, and Ctrl+Z takes it off.
