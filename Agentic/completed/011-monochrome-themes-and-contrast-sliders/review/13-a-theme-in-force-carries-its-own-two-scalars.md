# 13 — A theme in force carries its own two scalars
folder: editor
decisions: 0168, 0172, 0178, 0197

## Change
`editor/src/themes.h`:
- `voe_editor_theme` gains `const char *identity` (the file name, or `near_black`/`near_white` for entries
  0 and 1, ADR-0197), and `float contrast_strength` and `float surface_separation` — the two in force,
  which are the file's own until a remembered line or a slider replaces them. The theme's own two stay
  where they are, in `theme.inputs`.
- `voe_editor_themes` gains `voe_editor_theme_scalars scalars` (card 12) and `bool scalars_unwritten`.
- Declare, with a comment each:

	void voe_editor_themes_adjust(voe_editor_themes *themes, uint32_t index,
				      float contrast, float separation);
	void voe_editor_themes_reset(voe_editor_themes *themes, uint32_t index);
	[[nodiscard]] bool voe_editor_themes_scalars_write(voe_editor_themes *themes);

  `adjust` clamps both into `VOE_UI_THEME_SCALAR_MIN..MAX`, stores them, derives that entry's palette again
  in place — the address `ui` keeps does not move, exactly as `_check` does — and remembers the pair in the
  list; `reset` puts the theme's own two back, derives again and drops its line; `scalars_write` writes the
  file when something is unwritten and answers false only when the write failed (nothing to write is true).
- The header gains a paragraph: what a person set is remembered per theme in `theme_scalars.h`'s file, the
  theme file is never written, and a remembered pair replaces the file's own two at load and at every live
  re-read.

`editor/src/themes.c`: `#include "theme_scalars.h"`; at load, fill `identity` for every entry, read the
scalars file once into the list (its arena is the list's own), and derive each entry's palette from its
inputs with the remembered pair when there is one and its own pair otherwise. `_check`'s re-read derives the
new inputs with the entry's in-force pair. `_choose` leaves both alone. Write the three new calls.

`editor/src/src.md`: extend the `themes.h` and `themes.c` lines with the two scalars and where they are
remembered.

## Done when
The folder's check passes (`checks.sh` for `editor`), and with
`printf '0.400 2.500 near_black\n' > ~/.config/voe3d/theme_scalars`, `voe_editor --capture /tmp/flat.png
--size 1280x720` (ADR-0177) draws an editor whose text is visibly closer to its background and whose panels
are further apart than the same capture with that file removed.
