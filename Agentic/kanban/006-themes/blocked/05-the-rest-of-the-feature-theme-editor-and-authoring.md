# 05 — The rest of the feature: theme, editor and authoring
folder: theme
decisions: 0170, 0171, 0172, 0167, 0168
read: feature.md

## Change

Everything of 006 that cards 01 to 04 did not do, exactly as the old plan (2026-09-17) had it. The planner
re-cuts it into cards under the Agentic workflow; nothing here is ready for a coder as it stands.

The plan's summary, with ADR numbers as they are now (0168 → 0170, 0169 → 0171, 0170 → 0172):

A theme is one colour, two scalars, a mode, a typeface and a text size, authored in the sectioned
format. A new folder `theme` turns those bytes into `voe_ui_theme_inputs`; `ui` derives a palette of
roles from them in OKLab and every widget draws from the nearest theme in force, pushed and popped
around a subtree. The editor lists the built-in theme plus every `*.theme` file in
`<settings>/voe3d/themes/`, remembers the chosen one by file name, re-reads it on a timer, and keeps
the last good palette when a save is broken. `text` gains Pixel Operator beside Oxanium first, so a
theme can name its face — 005's own work (the editor's default and its font override) is not built.

The decisions the plan made, project-wide ones now records 0170–0172 and the rest feature-local and recorded
nowhere but here:

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

The folders the plan named, and what changes in each:

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

The old tasks still to do:

### Old task 5 — `theme/` — the folder that turns a theme file into inputs

New folder `theme`, four-line `CMakeLists.txt`, `voe_module(theme DEPENDS ui text assets
math base)`. Add its row to `cmake/voe.cmake`'s dependency map, add `theme` to `editor`'s row
there, add `add_subdirectory(theme)` to the root `CMakeLists.txt` — check.cmake step 1b fails
without it — and update the root `CLAUDE.md` folder table and diagram — `dev`'s line reads *every
folder but `editor`, `authoring` and `theme`*. Public surface: `voe_theme` — display name,
`voe_ui_theme_inputs`, `voe_text_typeface` — and
`voe_theme_read(const char *text, size_t size, voe_base_arena *arena, voe_theme *out)`, false on
failure. It parses through `voe_assets_sectioned_parse` and interprets the keys itself (layer two,
ADR-0096): `accent` as `#RRGGBB`, `contrast_strength` and `surface_separation` as numbers,
`mode` as `light` or `dark`, all required; `font` (`oxanium`, `pixel_operator`) and `text_size`
optional, falling back to `voe_ui_theme_default_inputs()`. Exactly one section per file, its name
the theme's; a second section, a missing key, an unknown key, a value that will not convert and a
scalar outside the range the derivation clamps to are each refused whole through `base/report.h`
with the line from task 1 and what is wrong — the file's name is not this folder's to know. It
opens no file. Its header says the file's shape, every refusal, and why the name a person reads
is the section's. `theme.md` written. Tests: a good file's every value, every refusal with the
line it names, and — the criterion-10 case — a theme's text read here, derived through
`voe_ui_theme_derive`, set on a `ui` context, and a panel, a label and a button coming out in
that theme's colours, with a headless device for the label and no editor in sight.

Done when (as the old plan had it): `cmake --build --preset debug --target voe_theme && ctest --test-dir build/debug -R '^theme/'` — all tests pass, and `cmake -P check.cmake` gets past its standalone-configure step for the new folder.

### Old task 6 — `editor/` — the themes it has, the one it is in, and the accent

Add `theme` to `editor/CMakeLists.txt`'s `DEPENDS`. New `src/theme.h` / `src/theme.c`: the
built-in theme plus every `*.theme` file in `<settings>/voe3d/themes/` (the folder made when it is
not there), each read with `platform/file.h` bytes through `voe_theme_read` into an arena of its
own, derived with the font its typeface names, and held as a `voe_ui_theme` with its display name
and file name. The chosen one is remembered in `<settings>/voe3d/theme` — one line, the file name,
empty for the built-in — read at startup and written when it changes, in the shape
`src/last_project.c` already uses; a chosen file that is gone or refused falls back to the
built-in with a notice. `src/main.c` creates both fonts (ADR-0167) and sets the chosen theme on
the context; `src/scene.c` draws the selected entity's row with `voe_ui_label_role` in ACCENT and
drops the `> ` marker. Its header says why a theme's arena is its own and what a failure to read
one leaves behind. `editor.md` updated.

Done when (as the old plan had it): `cmake --build --preset debug --target voe_editor`, then with a `*.theme` file in `~/.config/voe3d/themes/` and its name in `~/.config/voe3d/theme`, `build/debug/editor/voe_editor --capture /tmp/themes.png --size 1280x800` exits zero and writes a picture drawn in that theme.

### Old task 7 — `editor/` — Preferences

New `src/preferences.h` / `src/preferences.c`: an anchored panel over the dock, below the
bar, the same shape as `src/browser.c` — one row per theme, the built-in first, each row a label
and a Choose button, the one in use marked, and a Close button; what fired is recorded and read
after the frame as the top bar's buttons are. Each row is drawn inside
`voe_ui_theme_push`/`voe_ui_theme_pop` of that row's own theme, so a row shows what its theme
looks like while the rest of the editor stays in the chosen one. A Preferences button in
`src/topbar.c`, and the panel drawn and its clicks read in `src/interface.c` where the browser's
are. Choosing a theme sets it on the context and writes the remembered file name. Escape closes
it when the browser is not showing; it suppresses nothing else. `editor.md` updated.

Done when (as the old plan had it): `cmake --build --preset debug --target voe_editor` builds and `cmake -P check.cmake` exits zero; the sponsor opens Preferences, sees each row in its own theme, chooses one and sees every panel change.

### Old task 8 — `editor/` — live editing, and a broken theme keeps the last good one

`src/theme.c` re-reads the chosen theme's file every second by `platform/clock.h` — and
only that file — comparing the bytes with the last good ones; a change is read and derived, and
the new palette is in the next frame. A file that refuses leaves the palette that is drawing
untouched and puts a notice through `src/notice.h` naming the file, the line and what is wrong,
built from `base/report.h`'s first kept error the way an open failure already is; the next good
save clears it. The built-in theme watches nothing. Its header says why the comparison is the
bytes and not a timestamp, and what a refusal leaves behind.

Done when (as the old plan had it): `cmake --build --preset debug --target voe_editor` builds and `cmake -P check.cmake` exits zero; with the editor running the sponsor saves an edit and sees it within about a second, then saves a mistake and sees the last good theme still drawn with a notice naming the line.

### Old task 9 — `authoring/` — the lines come from the parser

Now that task 1 puts a line on every section and key, `src/line_index.h` / `.c` keeps
only the key spans a kept section is written back from; `scene_read.c` and `project.c` take their
line numbers from `voe_assets_sectioned` directly. The two output arrays and the walk that filled
them go, and the header says that a line is the parser's and a span is this file's. No refusal
changes its wording and no test's expected line changes.

Done when (as the old plan had it): `cmake --build --preset debug --target voe_authoring && ctest --test-dir build/debug -R '^authoring/'` — all tests pass, unchanged.

## Done when

Every step of `## How to test` in `feature.md` can be followed as written, and `cmake -P check.cmake` exits zero
on Linux. Old task 5's "gets past its standalone-configure step" is the same condition said smaller.

## Blocked

Written before the Agentic workflow, as five tasks across four folders, so it breaks the framework's rules and
has to be re-cut rather than coded: a card names one folder, and old task 5 alone touches `theme/` (new),
`cmake/voe.cmake` (its row, and `theme` on `editor`'s row), the root `CMakeLists.txt` (`add_subdirectory(theme)`)
and the old `CLAUDE.md`'s folder table, which no longer exists — the folder map is now
`Agentic/tech-lead/system.md`, which the secretary redraws at acceptance and no card edits. A new folder and a
new edge in `cmake/voe.cmake` are rule 2's "reported, not made" (ADR-0168): record 0170 already decides the
folder and its row, so the planner writes the `cmake/` and root edits as their own card or asks for a decision
on how a root-level file is carded. Old tasks 6 to 8 are three cards in `editor/` in order; old task 9 is one in
`authoring/`. The planner should read `feature.md`, `Agentic/tech-lead/system.md`, records 0170–0172 and 0167,
and `ui/ui.md`, `text/text.md`, `assets/assets.md` and `editor/editor.md` as they stand on this branch, since
cards 01–04 changed them.
