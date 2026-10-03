# 01 — A theme's spacing scales ui's widget pads and gaps
folder: ui
after: none
decisions: 0168, 0306, 0344

## Change
- `ui/include/ui/theme.h`: add `float spacing` to `voe_ui_theme`, beside `text_size`. Its comment
  says: a multiplier on every millimetre a widget puts round its content; 1 from
  `voe_ui_theme_derive`; not authored, so not in `voe_ui_theme_inputs`; a caller wanting a tighter
  interface sets it on the derived palette (ADR-0344).
- `ui/src/theme.c`: `voe_ui_theme_derive` sets `spacing` to 1.
- `ui/src/context.h`: `BUTTON_PAD` becomes a static inline function of the theme in force,
  `voe_ui_pad button_pad(const voe_ui_theme *theme)`, 2.5 mm times `theme->spacing` on all four
  sides; its comment gains that the theme's spacing scales it.
- `ui/src/button.c`, `ui/src/field.c`: every use of `BUTTON_PAD` calls `button_pad` with the theme
  each already takes from `voe_ui_theme_current`. `NUMBER_REFUSED_GAP` in `button.c` is multiplied
  by that theme's spacing where used.
- `ui/src/colour.c`: `PICKER_PAD` and `PICKER_GAP` are multiplied by the spacing of the theme in
  force where used. The square, strip, markers and cells are not.
- Scroll bars, the slider's track and thumb, and hairlines are not scaled.
- `ui/ui.md`: the `theme.h` entry names the spacing.
- `ui/tests/button.c`: add a case `button_pad_follows_spacing`: the same labelled button measured
  under a derived theme and under a copy with `spacing` 0.5 is 2.5 mm narrower and 2.5 mm shorter.
  `ui/tests/tests.md`'s `button.c` entry names it.

## Done when
`ctest --test-dir build/debug -R "^ui/"` passes with `button_pad_follows_spacing` in
`ui/tests/button.c`, and `grep -c "BUTTON_PAD" ui/src/button.c ui/src/field.c` prints 0 for both.
