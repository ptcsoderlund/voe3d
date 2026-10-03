# 02 — Every editor theme is derived at the PC monitor's base
folder: editor
after: 01
decisions: 0168, 0224, 0306, 0344

## Change
- `editor/src/themes.h`: add `VOE_EDITOR_TEXT_BASE` (0.8f) and `VOE_EDITOR_SPACING` (0.65f), with a
  comment saying: the editor's 100 % text is 80 % of a theme's own `text_size` and its spacing 65 %
  of `ui`'s, for a PC monitor (ADR-0306, ADR-0344); a game's interface keeps both at 1. The header's
  paragraph on how a palette is derived gains the base: text size is the theme's own `text_size`
  times the base times the scale, and the palette's spacing is `VOE_EDITOR_SPACING`.
- `editor/src/themes.c`: every place a palette is derived (`derive_one`, `derive_in_force`, and any
  other call of `voe_ui_theme_derive` in the file) multiplies the inputs' `text_size` by
  `VOE_EDITOR_TEXT_BASE` as well as the scale, and sets the result's `spacing` to
  `VOE_EDITOR_SPACING`. The theme file's `text_size`, `text_scale`'s range, the remembered line and
  Reset are unchanged, so the slider still reads 100 %.
- `editor/src/src.md`: the `themes.h` entry names the base text size and spacing.

## Done when
`grep -c "VOE_EDITOR_TEXT_BASE" editor/src/themes.c` and
`grep -c "VOE_EDITOR_SPACING" editor/src/themes.c` each print 1 or more, and the folder builds.
