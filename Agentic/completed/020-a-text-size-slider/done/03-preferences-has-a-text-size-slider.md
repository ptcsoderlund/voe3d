# 03 — Preferences has a text size slider
folder: editor
decisions: 0168, 0196, 0197, 0219, 0224

## Change
A third slider, Text size, under the two in Preferences, for the theme in force.

- `editor/src/preferences.h`
  - `voe_editor_preferences` gains `voe_ui_node text_size_slider` and `char text_size_text[16]`.
  - `voe_editor_preferences_result` gains `float text_scale` (inside `VOE_EDITOR_TEXT_SCALE_MIN..MAX`, nought
    when the frame was refused); `ADJUST` fires when any of the three moved; `sliding` is true while any of
    the three is held.
  - Header: three sliders; the text size's range is `VOE_EDITOR_TEXT_SCALE_MIN..MAX` shown as a whole
    percentage (`%.0f%%` of scale × 100); Reset puts back all three.
- `editor/src/preferences.c` — after the separation row, a row with the label "Text size", the slider over
  the chosen entry's `text_scale` at the same width as the others, and the percentage label; the read takes
  it with `voe_ui_slider_action` over the same range. The sliders' rows must stay inside the panel at 200%:
  if they do not already wrap, make them fill across and wrap as `inspector.c`'s rows do (ui/layout.h, A
  RUN THAT WRAPS — read that part of the header only).
- `editor/src/interface.c` — the adjust call passes `result.text_scale` (replacing card 02's stand-in).
- `editor/src/interface.h` — the paragraph counting PREFERENCES' TWO SLIDERS AND RESET counts three
  sliders: add the third row's nodes (row, name label, three for the slider, value label) and elements
  ("Text size" drawn letters, the slider's three, "200%"'s four) to `VOE_EDITOR_INTERFACE_NODES` and
  `_ELEMENTS`, and to every running total in that header after it.
- `editor/src/src.md` — the `preferences.h` entry says three sliders.

## Done when
The Checks line of `CLAUDE.md` with `{folder}` = `editor` exits 0, and
`voe_editor <scratch>/p --capture <scratch>/f.png --size 1280x720` writes the picture with no capacity line
on stderr.
