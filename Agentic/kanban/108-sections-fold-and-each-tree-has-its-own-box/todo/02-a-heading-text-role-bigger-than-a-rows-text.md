# 02 — A heading text role, bigger than a row's text
folder: ui
after: 01
decisions: 0168, 0496, 0503

## Change
A section's title needs a heading style that stands out from the rows under it (0503 point 3).

- `ui/include/ui/widgets.h`:
  - `voe_ui_text_role` gains `VOE_UI_TEXT_ROLE_HEADING`.
  - Add `#define VOE_UI_HEADING_SCALE 1.25f` beside it.
  - The enum's comment says: a role names a lightness, and for a heading a size as well. A heading
    is `text_primary` at the theme's `text_size` times the scale. No palette role is added, so
    `theme.h` is unchanged. An inverted control's ink still wins over the role.
  - `voe_ui_label_role`'s comment says a heading's natural size is measured at the heading size.
- `ui/src/label.c`: the size comes from the role, the scale for HEADING and 1 otherwise. It is used
  for the measure at the call and for every glyph box and advance in `push_label`. The ink is
  `text_primary`, the same as NORMAL.
- `ui/tests/widgets.c`: one new case. The same string as a HEADING label measures
  `VOE_UI_HEADING_SCALE` times the NORMAL label's width and height. Its glyph records are in
  `text_primary`. Use the label case already in the file for how it gets its font.
- `ui/tests/tests.md`, `ui/ui.md`, `ui/include/ui/ui.md`: the label's entries name the heading.

## Done when
The test `ui/widgets` passes, with the new heading case in it.
