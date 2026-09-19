# 10 — Type a number into a number box
folder: ui
decisions: 0168, 0192, 0177

## Change
`include/ui/widgets.h` / `src/widgets.c`: 0192's typing for the number box, on card 09's focus.
- A press and release inside `VOE_UI_NUMBER_DEAD_ZONE` opens the box for typing: it takes the focus, and from the
  next frame it draws as a field would (the field's colours, the focused role, caret, selection). The label the
  caller composed is not drawn while it is open. It opens with `value` formatted `%.6g`, all selected. A drag
  never opens it.
- Enter, Tab or a press elsewhere commits. If the text is unchanged from what it opened with, nothing changes.
  Otherwise the text is parsed with `strtod`, blanks allowed around it, the whole text consumed, and the result
  finite. A good parse gives `changed` true and `value` the typed number, and the box closes. A refused parse on
  Enter or Tab keeps the box open, adds a label "not a number" after the text in `text_secondary`, and Tab does
  not move. On a press elsewhere it closes and changes nothing. Escape closes and changes nothing.
- Tab moves between fields and number boxes alike, in call order.
- `voe_ui_number_result` gains `bool typing` (the box is open for typing this frame) and `bool refused` (this
  frame's commit was refused).
- Rewrite the header's "A PRESS AND A RELEASE WITHOUT MOVEMENT DOES NOTHING" paragraph and the "WHAT IS NOT HERE"
  sentence on it. Point to 0192.

Update `ui.md` and `tests/tests.md`.

## Done when
The folder's check passes (`checks.sh` for `ui`) with `tests/widgets.c` extended:
- a click without movement gives `typing` next frame, and a drag of 5 mm never does.
- typing `0.1` then Enter gives `changed` with value 0.1.
- opening and pressing Enter with nothing typed gives no `changed`.
- `abc` then Enter gives `refused`, no `changed`, and still `typing`; then Escape closes it with no `changed`.
- `3` then Tab commits 3 and opens the next number box; Escape there leaves its value unchanged.
- ` 2.5 ` is accepted, and `1e999`, `nan` and `2x` are refused.
