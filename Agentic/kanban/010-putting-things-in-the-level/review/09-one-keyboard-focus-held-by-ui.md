# 09 — One keyboard focus, held by `ui`
folder: ui
decisions: 0168, 0192, 0177

## Change
`include/ui/widgets.h` / `src/widgets.c` (and `src/context.h`): 0192's focus for the field. The number box
joins it in card 10.
- `voe_ui_keyboard` gains `bool escape` and `bool tab`, this frame's edges, like `enter`.
- While a field is focused, the context holds its text (`VOE_UI_FIELD_CAPACITY`), seeded from the caller's
  `text` when the focus arrives. The caller's `text` is not read again until the focus leaves. The result's
  `text` is that buffer while focused.
- The focus arrives (a press inside, `voe_ui_field_focus`, or Tab) with the whole text selected, drawn with the
  accent behind the text. The first typed text replaces it; Backspace empties it; after that editing is as now.
- Typed bytes below 0x20, and 0x7F, are ignored.
- `voe_ui_field_result` gains `bool committed` and `bool cancelled`. `committed` is true on the frame focus left
  by Enter, Tab or a press elsewhere, and `text` is then the final text. `cancelled` is true on the frame Escape
  arrived while focused: the focus is dropped, and `text` is the caller's own. `entered` keeps its meaning.
- Tab moves the focus to the next typeable widget made after this one in the frame, wrapping to the first. From
  this card that means fields; card 10 adds number boxes. The new one opens the next frame.
- `bool voe_ui_typing(const voe_ui_context *ui)`: true when a widget held the focus at the end of the last frame.
- The header's "WHAT IS NOT HERE" paragraph and the field's paragraphs say all this. Note that the text lives in
  the context now, so a caller keeps no copy of an edit.

Downstream (ADR-0113): `editor/src/browser.c` writes the field's `text` back each frame. Check that its name
field still types, confirms on Enter and survives the selection-on-focus. Adjust only if it no longer builds
or behaves.

Update `tests/tests.md`.

## Done when
The folder's check passes (`checks.sh` for `ui`) with `tests/widgets.c` extended: typing into a newly focused
field replaces its text; Backspace on a newly focused field empties it; Escape gives `cancelled`, drops the
focus, and the next frame shows the caller's text; Enter and a press elsewhere each give `committed` with the
typed text; Tab from the first of two fields commits it and focuses the second, and Tab from the second
focuses the first; a 0x09 or 0x7F byte in `text` is not appended; `voe_ui_typing` follows the focus.
