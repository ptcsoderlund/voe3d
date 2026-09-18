# 01 — The built-in themes in Pixel Operator, and a remembered font override
folder: editor
decisions: 0168, 0178, 0179, 0177

## Change
In `src/themes.h` / `src/themes.c`:
- Entries 0 and 1 (Near black, Near white) get `typeface = VOE_TEXT_TYPEFACE_PIXEL_OPERATOR` and are derived
  with `pixel_operator`. The header says "Pixel Operator" where it says "Oxanium" for them.
- Add `typedef enum { VOE_EDITOR_FONT_THEME, VOE_EDITOR_FONT_PIXEL_OPERATOR, VOE_EDITOR_FONT_OXANIUM }
  voe_editor_font_choice;`. Add two fields to `voe_editor_themes`: `font_choice` and `voe_ui_theme in_force`.
  `in_force` is the chosen entry's palette with `.font` replaced by the override's face (the theme's own face for
  `VOE_EDITOR_FONT_THEME`). A private helper refreshes it. Call the helper at the end of `_load`, `_choose`, on
  a `CHANGED` result from `_check`, and in the new `_font_choose`.
- `voe_editor_themes_load` also reads `<settings>/voe3d/font` as one line, the same way it reads `theme`. The
  line is empty or absent for THEME, `pixel_operator`, or `oxanium`. Any other line, or no file, means THEME,
  is not reported, and does not change the return value.
- New `const voe_ui_theme *voe_editor_themes_palette(const voe_editor_themes *themes)` returns `&in_force`. This
  is the palette the interface is set with.
- New `[[nodiscard]] bool voe_editor_themes_font_choose(voe_editor_themes *themes, voe_editor_font_choice
  choice)` puts the choice in force, refreshes `in_force`, and writes the line (empty, `pixel_operator` or
  `oxanium`) the way `_choose` writes `theme`. It returns false only when the write fails, and the choice is
  in force either way.
- The header gains a paragraph on the override: why the entries are not changed (each Preferences row shows its
  theme's own font), and that `in_force` is kept by pointer by `ui`, so `themes` must not move.

Call sites: every `voe_ui_font_set` / `voe_ui_theme_set` in `src/main.c` (around the load and the check) and in
`src/interface.c` (initial set and after Choose) is given `voe_editor_themes_palette(themes)` and its `->font`,
not `&voe_editor_themes_chosen(...)->palette`. Leave `preferences.c`'s per-row push on each entry's own palette.
Update the themes paragraph in `main.c`'s header, the `themes.h` entry in `src/src.md`, and `editor.md` (the
built-ins are in Pixel Operator, and the override is remembered in `<settings>/voe3d/font`).

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_editor` succeeds. With `C=$(mktemp -d)`,
each of these runs `XDG_CONFIG_HOME=$C ./build/debug/editor/voe_editor --capture $C/<name>.png --size 1280x800`,
and each run exits 0:
1. `default.png`, with nothing in `$C`.
2. `pixel.png`, with `$C/voe3d/font` holding `pixel_operator`. `cmp -s $C/default.png $C/pixel.png` exits 0.
3. `oxanium.png`, with the line `oxanium`. `cmp -s $C/default.png $C/oxanium.png` exits 1.
4. `junk.png`, with the line `junk`. `cmp -s $C/default.png $C/junk.png` exits 0.
5. `white.png`, with `$C/voe3d/theme` holding `near_white` and `$C/voe3d/font` holding `oxanium`.
   `cmp -s $C/oxanium.png $C/white.png` exits 1.

Read `default.png` and `oxanium.png` (ADR-0177). The first must show the panels in the pixel face, and the second
in Oxanium.
