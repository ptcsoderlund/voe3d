# 006 Themes — tasks

- [x] 1. `assets/` — a section and a key carry the line they came from
  - Change: add `uint32_t line` to `voe_assets_sectioned_section` and to
    `voe_assets_sectioned_key`, the 1-based physical line of the `[Section]` header and of the
    `key=value` line, filled by the parser as it walks (it already counts lines for its refusals).
    Say in the header that a consumer naming a line in its own refusal takes it from here and does
    not walk the text again. Add the cases to `tests/sectioned.c`: lines across comments, blank
    lines, `\r\n` endings and a file whose first section is not on line 1. Nothing else about the
    parser changes.
  - Covers: 6 (the reader's half — a refusal names a line)
  - Depends on: -
  - Done when: `cmake --build --preset debug --target voe_assets && ctest --test-dir build/debug -R '^assets/'` — all tests pass.

- [x] 2. `text/` — Pixel Operator beside Oxanium, and a font asked for by typeface
  - Change: build ADR-0167. Add `PixelOperator.ttf` (Regular) and its CC0 licence text to
    `text/fonts/`, unrenamed and unmodified, beside Oxanium and its OFL. Add
    `voe_text_typeface` — `VOE_TEXT_TYPEFACE_OXANIUM`, `VOE_TEXT_TYPEFACE_PIXEL_OPERATOR` — and make
    it `voe_text_font_new`'s first argument; both faces are `#embed`ded and one font stays one face,
    one atlas, one weight, no fallback. Check Pixel Operator against `ATLAS_EM` as ADR-0167 requires
    — its design grid is 0.0625 em — and, if the sampling has to move for that face alone, move it
    for that face alone and put the reason in `src/font.c` beside the existing one. Update the call
    sites the signature breaks: `dev/src/main.c`, `editor/src/main.c`, `ui/tests/widgets.c`, and
    `text`'s own tests, each naming the face it draws in. Extend `tests/truetype.c` to read the new
    face as it reads Oxanium, and check that both faces' fonts can exist at once.
  - Covers: 8 (a theme can name a face that exists)
  - Depends on: -
  - Done when: `cmake --build --preset debug && ctest --test-dir build/debug -R '^(text|ui)/'` — all tests pass, and `voe_dev` and `voe_editor` still link.

- [x] 3. `ui/` — the theme: authored inputs in, a derived palette out
  - Change: new `include/ui/theme.h` and `src/theme.c`, plus internal `src/oklab.h` / `src/oklab.c`.
    `voe_ui_theme_inputs` is the authored set of ADR-0097 — `accent` as authored sRGB,
    `contrast_strength`, `surface_separation`, `mode` (light or dark) — plus `text_size` in
    millimetres per em. `voe_ui_theme` is the derived palette: ground, surface, raised surface,
    hairline border, control, control hovered, three text lightnesses, accent, ink on accent, each
    linear RGBA, beside the `const voe_text_font *` the caller chose and the text size. Derivation is
    `voe_ui_theme_derive(const voe_ui_theme_inputs *, const voe_text_font *)`, all of it in OKLab per
    ADR-0169: sRGB in, lightness steps sized by the two scalars, hue never rotated, every text role
    stepped until it clears its surface, the accent's chroma clamped harder in dark than in light.
    `voe_ui_theme_default_inputs()` is the built-in near-black theme with a blue accent. No widget
    and no existing function changes in this task. Tests in `tests/theme.c`: a round trip through
    OKLab, that both modes stay legible at both ends of both scalars, that only the accent moves when
    only the accent moves, that each scalar moves what it names and nothing else, and the dark/light
    chroma asymmetry. Needs no graphics card — a NULL font is allowed here and asserted on only where
    a label is measured.
  - Covers: 4, 10 (the derivation's half)
  - Depends on: -
  - Done when: `cmake --build --preset debug --target voe_ui && ctest --test-dir build/debug -R '^ui/'` — all tests pass.

- [ ] 4. `ui/` — every widget draws from the nearest theme
  - Change: the context carries a theme — `voe_ui_theme_set(ui, const voe_ui_theme *)`, the caller's
    memory, outliving the context as the font does — and a subtree carries its own through
    `voe_ui_theme_push` / `voe_ui_theme_pop`; every node records the theme in force when it was made
    and the widget pass reads the node's own, so the nearest wins (ADR-0168). Replace
    `voe_ui_panel_begin`'s `voe_math_float4 colour` with a `voe_ui_surface` role — NONE (emits
    nothing, as an alpha of nought does today), GROUND, SURFACE, RAISED. Every remaining colour
    constant in `src/widgets.c` becomes a role: the button's three states with the pressed one on the
    accent, the number box on the accent while it is dragged, the field, the caret, the scrollbar's
    track and thumb, and a label's text. A panel and a button draw a hairline border in the border
    role — the border rectangle with the fill inset, two element records, the hairline width a
    constant of this folder. Add `voe_ui_text_role` (NORMAL, ACCENT) and
    `voe_ui_label_role(ui, text, role)`; `voe_ui_label` stays and means NORMAL. Remove
    `voe_ui_text_scale_set`: the theme's `text_size` is the one place a size is said. Move the
    capacities that a second record per panel and button costs. Update the call sites this breaks:
    `dev/src/interface.c` (its HUD plate becomes a RAISED panel and its text-size knob sets the size
    on a theme of dev's own) and `editor/src/{dock,topbar,browser,inspector,interface}.c`. Tests:
    nearest wins over the theme above, a pop restoring it, an unbalanced push refused the way this
    folder already refuses a frame, a panel and a button emitting border and fill in the right order,
    and a label in ACCENT coming out in the accent.
  - Covers: 1, 4, 7 (the mechanism), 8
  - Depends on: 2, 3
  - Done when: `cmake --build --preset debug && ctest --test-dir build/debug -R '^ui/'` — all tests pass, and `voe_dev` and `voe_editor` still link.

- [ ] 5. `theme/` — the folder that turns a theme file into inputs
  - Change: new folder `theme`, four-line `CMakeLists.txt`, `voe_module(theme DEPENDS ui text assets
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
  - Covers: 6, 8, 10
  - Depends on: 1, 2, 3, 4
  - Done when: `cmake --build --preset debug --target voe_theme && ctest --test-dir build/debug -R '^theme/'` — all tests pass, and `cmake -P check.cmake` gets past its standalone-configure step for the new folder.

- [ ] 6. `editor/` — the themes it has, the one it is in, and the accent
  - Change: add `theme` to `editor/CMakeLists.txt`'s `DEPENDS`. New `src/theme.h` / `src/theme.c`: the
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
  - Covers: 1, 3 (found, chosen, remembered), 9
  - Depends on: 4, 5
  - Done when: `cmake --build --preset debug --target voe_editor`, then with a `*.theme` file in `~/.config/voe3d/themes/` and its name in `~/.config/voe3d/theme`, `build/debug/editor/voe_editor --capture /tmp/themes.png --size 1280x800` exits zero and writes a picture drawn in that theme.

- [ ] 7. `editor/` — Preferences
  - Change: new `src/preferences.h` / `src/preferences.c`: an anchored panel over the dock, below the
    bar, the same shape as `src/browser.c` — one row per theme, the built-in first, each row a label
    and a Choose button, the one in use marked, and a Close button; what fired is recorded and read
    after the frame as the top bar's buttons are. Each row is drawn inside
    `voe_ui_theme_push`/`voe_ui_theme_pop` of that row's own theme, so a row shows what its theme
    looks like while the rest of the editor stays in the chosen one. A Preferences button in
    `src/topbar.c`, and the panel drawn and its clicks read in `src/interface.c` where the browser's
    are. Choosing a theme sets it on the context and writes the remembered file name. Escape closes
    it when the browser is not showing; it suppresses nothing else. `editor.md` updated.
  - Covers: 2, 3 (chosen from Preferences), 7
  - Depends on: 6
  - Done when: `cmake --build --preset debug --target voe_editor` builds and `cmake -P check.cmake` exits zero; the sponsor opens Preferences, sees each row in its own theme, chooses one and sees every panel change.

- [ ] 8. `editor/` — live editing, and a broken theme keeps the last good one
  - Change: `src/theme.c` re-reads the chosen theme's file every second by `platform/clock.h` — and
    only that file — comparing the bytes with the last good ones; a change is read and derived, and
    the new palette is in the next frame. A file that refuses leaves the palette that is drawing
    untouched and puts a notice through `src/notice.h` naming the file, the line and what is wrong,
    built from `base/report.h`'s first kept error the way an open failure already is; the next good
    save clears it. The built-in theme watches nothing. Its header says why the comparison is the
    bytes and not a timestamp, and what a refusal leaves behind.
  - Covers: 5, 6
  - Depends on: 6
  - Done when: `cmake --build --preset debug --target voe_editor` builds and `cmake -P check.cmake` exits zero; with the editor running the sponsor saves an edit and sees it within about a second, then saves a mistake and sees the last good theme still drawn with a notice naming the line.

- [ ] 9. `authoring/` — the lines come from the parser
  - Change: now that task 1 puts a line on every section and key, `src/line_index.h` / `.c` keeps
    only the key spans a kept section is written back from; `scene_read.c` and `project.c` take their
    line numbers from `voe_assets_sectioned` directly. The two output arrays and the walk that filled
    them go, and the header says that a line is the parser's and a span is this file's. No refusal
    changes its wording and no test's expected line changes.
  - Covers: - (one place a line number comes from, after task 1)
  - Depends on: 1
  - Done when: `cmake --build --preset debug --target voe_authoring && ctest --test-dir build/debug -R '^authoring/'` — all tests pass, unchanged.
