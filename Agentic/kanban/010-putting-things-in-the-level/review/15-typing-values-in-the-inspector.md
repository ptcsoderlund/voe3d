# 15 — Typing values in the Inspector
folder: editor
decisions: 0168, 0192, 0177

## Change
`src/main.c`: read Tab's down edge and hand it with Escape's to `voe_ui_keyboard` (`tab`, `escape`, card 09).
While `voe_ui_typing(ui)` was true at the last frame's end, Escape goes to `ui` alone. It does not cancel the
browser or hide Preferences that frame.

`src/inspector.c`:
- A number box's result is read as today. A typed value (`changed`) goes through the same replace intent a drag
  does, so rotation angles, rounding and read-only fields behave as they do for a drag.
- A CHAR field of rank 1 on a type with a replace intent is a `voe_ui_field` (card 09), not a label. On
  `committed` with text that differs from the row, the text is copied into the row's bytes, truncated to leave
  its terminating zero, and submitted as the replace intent. This is how the name is edited.
- Tab order is call order: the name, then each number in the order drawn.

Update the header's paragraph on edits, `editor.md` and `src/src.md`.

## Done when
`checks.sh` for `editor` passes. With `C=$(mktemp -d)`, `./build/debug/editor/voe_editor --capture $C/o.png`
exits 0, and the PNG (ADR-0177) looks as before.
