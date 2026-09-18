# 07 — `theme` reads a theme file into inputs
folder: theme
decisions: 0168, 0170, 0172, 0175, 0176

## Change

The folder does not exist; create it, standalone like every other (rule 1), with its `theme.md` and, per 0173,
`include/theme/theme.md`, `src/src.md` and `tests/tests.md` — copy the shape of `ui/`'s four pages.

Registration, in this commit (0175): in `cmake/voe.cmake`'s `voe_allowed_deps`, a `theme` branch after `ui`'s
with `set(deps ui text render assets math base)` and a comment citing ADR-0170 and ADR-0176 (render is for the
test's headless device only); in the root `CMakeLists.txt`, `add_subdirectory(theme)` on its own line after
`add_subdirectory(ui)`. `theme/CMakeLists.txt` is the four lines, ending
`voe_module(theme DEPENDS ui text render assets math base)`.

`include/theme/theme.h`:
- `voe_theme` — `const char *name` (the section's name, in the caller's arena), `voe_ui_theme_inputs inputs`,
  `voe_text_typeface typeface`.
- `[[nodiscard]] bool voe_theme_read(const char *text, size_t size, voe_base_arena *arena, voe_theme *out)` —
  parses with `voe_assets_sectioned_parse` and interprets the keys itself (ADR-0096 layer two). `accent` is
  `#RRGGBB`, kept as authored sRGB in 0..1; `contrast_strength` and `surface_separation` are numbers within
  `VOE_UI_THEME_SCALAR_MIN`..`MAX`; `mode` is `light` or `dark`; all four required. `font` (`oxanium`,
  `pixel_operator`) and `text_size` (a number above nought, millimetres per em) are optional and fall back to
  `voe_ui_theme_default_inputs()` and Oxanium. Exactly one section per file. Refused whole, `out` untouched, one
  `VOE_BASE_ERROR` report each naming the line (from the section's or key's `line`) and what is wrong: no
  section, a second section, a missing key (the section's line), an unknown key, a value that will not convert,
  a scalar out of range. It opens no file and never learns the file's name.
- The header comment says the file's shape with an example, every refusal, why the name a person reads is the
  section's and the identity is the file name (ADR-0172), and that `src/` names no `render/` header (ADR-0176).

`src/theme.c` carries it out.

`tests/theme.c`, one plain C program: a good file's every value; defaults when `font` and `text_size` are
absent; each refusal above, checking `voe_base_report_error_first()` names the expected line; and the
criterion-10 case — a light-mode theme's text read here, derived with `voe_ui_theme_derive` on a font made on a
headless device, set on a `ui` context, one frame with a SURFACE panel holding a label and a button, and the
emitted element records' colours equal to that palette's `surface`, text and control roles. Make the device, and
skip naming what went unchecked when there is no driver, exactly as `ui/tests/widgets.c` does.

## Done when

`cmake --preset debug && cmake --build --preset debug --target voe_theme && ctest --test-dir build/debug -R '^theme/'`
passes, and `cmake -P check.cmake` exits 0 (its steps 1b and 5 are the ones a new folder can fail).
