# 53 — A slider can be shown and not moved
folder: ui
after: none
decisions: 0168, 0402

## Change
0402: a new call beside the slider that draws its track and thumb with no number box round them, so
it answers no pointer and no key.

- `ui/include/ui/slider.h` — new
  `voe_ui_node voe_ui_slider_shown(voe_ui_context *ui, const char *name, uint32_t index,
  double value, double min, double max, float width_mm);` The header gains a paragraph: the same
  size and thumb place as `voe_ui_slider`, the value clamped into the range for the thumb only;
  no drag, no click into typing, no Tab stop, no action to read; the node handed back is the track's,
  for layout only; it takes the theme in force, so a caller greys it by pushing a theme with
  `text_disabled` text and fills that do not light; what it costs (its nodes and element records).
- `ui/src/slider.c` — `voe_ui_slider_shown`: the track as a panel of the slider's size with the
  number box's resting fill (`control`), the thumb as the same raised panel `voe_ui_slider` puts at
  the same place. Share the thumb's place arithmetic with `voe_ui_slider` rather than repeating it;
  read `slider.c`'s header and fix it if it no longer says what the file holds.
- `ui/src/src.md`, `ui/include/ui/ui.md`, `ui/ui.md` — the slider lines name the shown slider.

Test: `ui/tests/slider.c` (read its header and its `build` / `thumb` helpers) gains cases:
- the shown slider's rectangle is the slider's, and its thumb is where `voe_ui_slider`'s thumb is
  at the same value, at `min`, halfway and `max`;
- a press and a drag across it over two frames change nothing: no widget is held or focused
  (whatever `widgets.h` offers to ask that), and the thumb stays put;
- a value outside the range draws the thumb at the range's end.
The header's case list and `ui/tests/tests.md`'s `slider.c` line follow.

## Done when
`ctest --test-dir build/debug -R '^ui/slider$'` passes with the new cases.
