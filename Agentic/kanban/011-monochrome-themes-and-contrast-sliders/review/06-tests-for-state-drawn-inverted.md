# 06 — Tests for state drawn inverted
folder: ui
decisions: 0168, 0196

## Change
Add cases, each called from its file's `main`, using the file's existing tree helpers and `TEST_THEME`:

`ui/tests/button.c`:
- `a_held_button_is_inverted`: press on a button holding a label and keep it down one frame; the button's
  fill is `TEST_THEME.inverse` and the label's glyph records are `TEST_THEME.inverse_ink`.
- `a_dragged_number_box_is_inverted`: drag a number box past `VOE_UI_NUMBER_DEAD_ZONE`; its fill is
  `inverse` and its label's glyphs `inverse_ink`.
- `a_selected_choice_is_inverted`: `voe_ui_choice_begin(ui, "c", 0, true)` with a label: fill `inverse`,
  glyphs `inverse_ink`; the same with `false` and the pointer elsewhere: fill `control`, glyphs
  `text_primary`; with `false` and the pointer over it: fill `control_hovered`.
- Change any check that compares a held or dragged fill with `TEST_THEME.accent` to `TEST_THEME.inverse`.

`ui/tests/field.c`: `a_selected_text_is_inverted`: a field just focused (its whole text selected) draws a
record of `inverse` behind the text and its glyphs in `inverse_ink`.

`ui/tests/scroll.c`: `a_held_thumb_is_inverted`: press on a scroll area's thumb and hold; the thumb's record
is `inverse`.

Add a line for each new case to `ui/tests/tests.md`.

## Done when
The folder's check passes (`checks.sh` for `ui`) and `grep -c "accent" ui/tests/button.c ui/tests/field.c
ui/tests/scroll.c` prints 0 for each.
