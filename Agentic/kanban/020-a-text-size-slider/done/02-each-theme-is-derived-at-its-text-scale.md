# 02 — Each theme is derived at its own text scale
folder: editor
decisions: 0168, 0197, 0219, 0224

## Change
Every theme entry carries a text scale and its palette is derived with it.

- `editor/src/themes.h`
  - `voe_editor_theme` gains `float text_scale` beside the two scalars: 1.0 until a remembered line or an
    adjust replaces it.
  - `voe_editor_themes_adjust` gains `float text_scale` after `separation`, clamped into
    `VOE_EDITOR_TEXT_SCALE_MIN..MAX` and remembered with the other two.
  - Header: "two scalars" becomes the two scalars and the text scale throughout; the palette's text size is
    the theme's own `text_size` times the scale; the theme's own `text_size` is what Reset gives back
    (scale 1.0).
- `editor/src/themes.c` — wherever a palette is derived (load, live re-read, adjust, reset) the inputs
  handed to `voe_ui_theme_derive` have `text_size` multiplied by the entry's `text_scale`; a remembered
  line's `text_scale` is taken at load and at every re-read with the two others; adjust stores it and
  passes it to `voe_editor_theme_scalars_set` (replacing card 01's `1.0f`); reset sets it to 1.0. File
  header: "the two scalars" includes the text scale.
- `editor/src/interface.c` — the one `voe_editor_themes_adjust` call passes the chosen entry's current
  `text_scale` for now; card 03 replaces it with the slider's value.
- `editor/src/src.md` — the `themes.h` and `themes.c` entries say each theme carries its text scale.

## Done when
The Checks line of `CLAUDE.md` with `{folder}` = `editor` exits 0. With `XDG_CONFIG_HOME` at a scratch
folder whose `voe3d/theme_scalars` holds `1.000 1.000 2.000 near_black`,
`voe_editor <scratch>/p --capture <scratch>/big.png --size 1280x720` draws the top bar's and panels'
text twice as tall as the same capture without that line (compare the two PNGs; the bar's buttons may be
cut until card 04).
