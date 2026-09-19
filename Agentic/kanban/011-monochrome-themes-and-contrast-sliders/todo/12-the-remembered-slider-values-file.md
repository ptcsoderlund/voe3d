# 12 — The remembered slider values file
folder: editor
decisions: 0168, 0172, 0197

## Change
Two new files, modelled on `editor/src/last_project.h` and `.c` (read that pair's header first).

`editor/src/theme_scalars.h` (new): `<settings>/voe3d/theme_scalars`, one line per adjusted theme,
`<contrast> <separation> <identity>` (ADR-0197). Its header says the line's shape, why the identity is the
tail of the line, what the identities are (`near_black`, `near_white`, a file name), that an unparsable or
out-of-range line is skipped without a report, and that lines for themes not listed right now are kept and
written back.

	#define VOE_EDITOR_THEME_SCALARS_LINES 64
	typedef struct { const char *identity; float contrast_strength; float surface_separation; }
		voe_editor_theme_scalars_line;
	typedef struct { voe_editor_theme_scalars_line lines[VOE_EDITOR_THEME_SCALARS_LINES];
			 uint32_t count; } voe_editor_theme_scalars;
	void voe_editor_theme_scalars_read(voe_editor_theme_scalars *scalars, voe_base_arena *arena);
	const voe_editor_theme_scalars_line *
	voe_editor_theme_scalars_find(const voe_editor_theme_scalars *scalars, const char *identity);
	void voe_editor_theme_scalars_set(voe_editor_theme_scalars *scalars, voe_base_arena *arena,
					  const char *identity, float contrast, float separation);
	void voe_editor_theme_scalars_forget(voe_editor_theme_scalars *scalars, const char *identity);
	[[nodiscard]] bool voe_editor_theme_scalars_write(const voe_editor_theme_scalars *scalars);

`editor/src/theme_scalars.c` (new): the read (no file, or no settings folder, is an empty list and no
report), each line parsed as two `strtof`s and the rest after the second blank run as the identity, pushed
on `arena`, skipping a line whose numbers are missing or outside `VOE_UI_THEME_SCALAR_MIN..MAX`; `set`
replacing a line with that identity or appending one (past the ceiling it replaces nothing and appends
nothing); `forget` removing it; `write` making `<settings>` and `<settings>/voe3d` as
`voe_editor_last_project_write` does and writing `%.3f %.3f %s\n` per line, reporting a failure at the site
through `base/report.h`.

Card 13 is what calls these.

`editor/src/src.md`: a line for each new file, in the order the list already uses.

## Done when
The folder's check passes (`checks.sh` for `editor`).
