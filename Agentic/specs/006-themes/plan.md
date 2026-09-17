# 006 Themes — plan

A theme is one colour, two scalars, a mode, a typeface and a text size, authored in the sectioned
format. A new folder `theme` turns those bytes into `voe_ui_theme_inputs`; `ui` derives a palette of
roles from them in OKLab and every widget draws from the nearest theme in force, pushed and popped
around a subtree. The editor lists the built-in theme plus every `*.theme` file in
`<settings>/voe3d/themes/`, remembers the chosen one by file name, re-reads it on a timer, and keeps
the last good palette when a save is broken. `text` gains Pixel Operator beside Oxanium first, so a
theme can name its face — 005's own work (the editor's default and its font override) is not built.

## Decisions

- A new folder `theme` between the parser and the widgets, `ui` keeping the derivation, nearest
  wins as a push and a pop — project-wide: `Agentic/decisions/0168-a-theme-is-read-in-theme-derived-in-ui-and-nearest-wins.md`.
- OKLab, lightness steps, contrast as a constraint, chroma clamped harder in dark (answers D-167),
  hairline borders drawn as two records — project-wide:
  `Agentic/decisions/0169-the-palette-is-derived-in-oklab-and-dark-mode-clamps-chroma.md`.
- One theme per file, remembered by file name, the built-in one compiled in, live editing by
  re-reading on a timer (answers D-160) — project-wide:
  `Agentic/decisions/0170-a-theme-is-one-file-in-a-themes-folder-and-a-program-remembers-it-by-file-name.md`.
- ADR-0167 is built as far as 006 needs it: both faces embedded, `voe_text_font_new` taking a
  typeface. The editor creates both fonts at startup — two atlases — because a theme may name
  either; lazily is an optimisation nothing has measured.
- `voe_assets_sectioned` hands back each section's and key's line number, because the theme reader
  has to name a line in a refusal and `authoring`'s private second walk is the only other way to
  get one.
- `voe_ui_text_scale_set` goes: the theme's `text_size` is the one place a text size is said. `dev`'s
  text-size knob sets the size on a theme of its own.
- Feature-local: a theme's `accent` is authored sRGB and stays sRGB in `voe_ui_theme_inputs`; the
  conversion to linear is the derivation's, so the inputs are exactly what the file said.
- Feature-local: Preferences suppresses nothing. It is an anchored panel with a Close button, and
  Escape closes it when the file browser is not showing.
- Feature-local: the built-in theme is near-black with a blue accent and Oxanium at the editor's
  present text size, so criterion 1 is a look the sponsor can compare against today's.

## Folders

- `assets/` — changed — `voe_assets_sectioned_section` and `voe_assets_sectioned_key` gain `line`.
- `authoring/` — changed — internal only; `line_index` keeps the key spans and drops the two line
  arrays.
- `text/` — changed — `voe_text_font_new` takes a `voe_text_typeface`; `VOE_TEXT_TYPEFACE_OXANIUM`
  and `VOE_TEXT_TYPEFACE_PIXEL_OPERATOR` added, and the second face and its licence ship in
  `text/fonts/`.
- `ui/` — changed — `voe_ui_theme_inputs`, `voe_ui_theme`, `voe_ui_theme_default_inputs`,
  `voe_ui_theme_derive`, `voe_ui_theme_set`, `voe_ui_theme_push`, `voe_ui_theme_pop`,
  `voe_ui_label_role` and `voe_ui_text_role`; `voe_ui_panel_begin` takes a `voe_ui_surface` in place
  of a colour; `voe_ui_text_scale_set` removed.
- `theme/` — new — `voe_theme` (display name, `voe_ui_theme_inputs`, `voe_text_typeface`),
  `voe_theme_read`, and the file's shape documented in its header.
- `editor/` — changed — internal only; the themes it found, the chosen one, Preferences and the
  re-read timer.
- `dev/` — changed — internal only; call sites moved with `text` and `ui`.
- `cmake/`, root `CLAUDE.md` — changed — `theme`'s row, `editor`'s row gaining it, the folder table
  and the diagram.

## Verification

- `cmake -P check.cmake` — exits zero on Linux: every folder configures standalone, `theme`
  included, and every test passes (criterion 11).
- `ctest --test-dir build/debug -R '^theme/'` — a theme's own text decides the colours of a panel,
  a label and a button, with no editor anywhere in it (criterion 10).
- `ctest --test-dir build/debug -R '^ui/'` — the derivation's claims: the mode flips and text stays
  legible, the accent recolours every role that uses it, both scalars move what they name, dark mode
  clamps chroma harder, and the nearest theme wins over the one above it (criterion 4, 7).
- `build/debug/editor/voe_editor --capture /tmp/themes.png --size 1280x800` with a theme chosen —
  writes a picture of the panels in that theme (criterion 9).
- The sponsor's own run for criteria 1, 2, 3, 5, 6 and 8: start the editor, open Preferences, drop a
  theme file in `~/.config/voe3d/themes/`, choose it, restart, edit it while it runs, and break it.
