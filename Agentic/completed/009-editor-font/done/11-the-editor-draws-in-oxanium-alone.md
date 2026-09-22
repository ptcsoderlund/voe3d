# 11 — The editor draws in Oxanium alone, with no font choice
folder: editor
decisions: 0168, 0185, 0178, 0177

## Change
Cards 01 and 02 (commits `cdd7a0a`, `74e3009`) are the only changes to `editor/` since this feature began,
and ADR-0185 undoes both. Start with `git checkout cdd7a0a^ -- editor`: the built-ins are back in
Oxanium, and the font override, `voe_editor_font_choice`, `in_force`, `_palette`, `_font_choose`, the
`<settings>/voe3d/font` line, the Preferences font group, `VOE_EDITOR_PREFERENCES_FONT` and the budget
raise are gone. Nothing reads `<settings>/voe3d/font` any more, so a leftover one is ignored.

That older tree still makes a second face for theme files. Remove it:
- `src/themes.h` / `src/themes.c`: `voe_editor_themes_load(themes, oxanium, pixel_operator)` becomes
  `voe_editor_themes_load(themes, font)`, which takes one `const voe_text_font *font`. The field
  `pixel_operator` goes, `oxanium` becomes `font`, and `font_for` is deleted. Every entry, built-in or
  read from a file, is derived with `themes->font`, whatever `theme.typeface` says (ADR-0185: any font
  a theme names draws in Oxanium). Update the header's example, and change the comment on the entry's
  palette ("Derived with whichever of the two fonts…") to say it is always the one font.
- `src/main.c`: make only the Oxanium font, drop the `pixel_operator` variable and its create and
  destroy, and pass the one font to `_load`. Rewrite the header paragraph "Both faces are created…" to
  say the editor makes one font, Oxanium (ADR-0185).
- `src/src.md` and `editor.md`: correct any line that names two fonts or a second face.

## Done when
`git grep -n -i -e "pixel.\?operator" -e font_choice -- editor` prints nothing, and
`cmake --preset debug && cmake --build --preset debug --target voe_editor` succeeds. With `C=$(mktemp -d)`,
each run below is `XDG_CONFIG_HOME=$C ./build/debug/editor/voe_editor --capture $C/<name>.png --size 1280x800`
and exits 0:
1. `default.png`, with nothing in `$C`.
2. `leftover.png`, with `$C/voe3d/font` holding `pixel_operator`. `cmp -s $C/default.png $C/leftover.png`
   exits 0.
3. `white.png`, with `$C/voe3d/theme` holding `near_white` (and the leftover font file still there).
   `cmp -s $C/default.png $C/white.png` exits 1.

Read `default.png` and `white.png` (ADR-0177): both show the panels in Oxanium.
