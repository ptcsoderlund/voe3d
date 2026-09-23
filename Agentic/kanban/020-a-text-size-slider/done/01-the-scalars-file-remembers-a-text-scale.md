# 01 — The scalars file remembers a text scale
folder: editor
decisions: 0168, 0197, 0219, 0224

## Change
`<settings>/voe3d/theme_scalars` gains a third number per line, as 0224 says.

- `editor/src/theme_scalars.h`
  - `VOE_EDITOR_TEXT_SCALE_MIN` 0.5f and `VOE_EDITOR_TEXT_SCALE_MAX` 2.0f: the range of a theme's text
    scale, a multiplier on the theme's own `text_size`.
  - `voe_editor_theme_scalars_line` gains `float text_scale`.
  - `voe_editor_theme_scalars_set` gains `float text_scale` after `separation`.
  - Header: the line shape is now `<contrast> <separation> <text_scale> <identity>`; an older two-number
    line is still read, at a scale of 1.0, and written back in the new shape; a scale outside the range
    skips the line as an out-of-range scalar does; the example's `_set` call shows the third number.
- `editor/src/theme_scalars.c` — the read takes two numbers, then a third when the token after the blank
  run is a number followed by a blank, else 1.0 with the identity starting at that token; range check on
  all three; the write prints three `%.3f` numbers then the identity. File header: "two numbers" becomes
  three, the older shape read too.
- `editor/src/themes.c` — its one `voe_editor_theme_scalars_set` call passes `1.0f` for now; card 02
  replaces it with the entry's own scale. Touch nothing else in this file.
- `editor/src/src.md` — the `theme_scalars.h` and `theme_scalars.c` entries say text scale beside contrast
  and separation, and three numbers.

## Done when
The Checks line of `CLAUDE.md` with `{folder}` = `editor` exits 0. Then, with `XDG_CONFIG_HOME` pointed at
a scratch folder holding `voe3d/theme_scalars` with the two lines `1.200 0.800 near_white` and
`1.000 1.000 1.500 near_black`, `voe_editor <scratch>/p --capture <scratch>/f.png --size 640x360` writes the
picture with no line about the file on stderr (a capture writes no scalars; this proves the read accepts
both shapes).
