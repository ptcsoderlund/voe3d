# 01 — The project file holds the game window
folder: authoring
after: none
decisions: 0168, 0291, 0236

## Change
0291 point 1. Files: `authoring/include/authoring/project.h`, `authoring/src/project.c`,
`authoring/tests/project.c`, `authoring/authoring.md`.

- `project.h`: a `voe_authoring_project_window` struct — `int width`, `int height`,
  `bool fullscreen` — and a `window` field of that type on `voe_authoring_project`. Macros for
  the defaults (1280, 720) and the range (`..._WINDOW_MIN` 160, `..._WINDOW_MAX` 16384).
- `voe_authoring_project_read`: accepts an optional `[window]` section beside `[project]`, keys
  `width`, `height` (whole decimal digits only, in range) and `fullscreen` (`true` or `false`);
  each missing key or a missing section is its default. Refused and reported with its line: a
  value out of range or not in that form, an unknown key in `[window]`, a repeated section. An
  unknown key in `[project]` stays a warning. The sectioned reader hands back text; the digits
  are converted here.
- `voe_authoring_project_write`: always writes `[window]` after `[project]` with all three keys;
  asserts a width and height in range.
- Header points: the section, its keys, range and defaults, why old files still load, and why the
  game never reads it (it is handed the numbers, 0291 point 3). The example block shows both
  sections. The paragraph saying `scene` is the only key changes.
- Tests in `tests/project.c`: write-then-read keeps a 1600×900 fullscreen window; a file with no
  `[window]` reads the defaults; `[window]` with only `height = 900` keeps the other two
  defaults; refused: `width = 100`, `width = 17000`, `width = 12.5`, `fullscreen = yes`,
  `[window]` with `depth = 3`. Existing tests that build a `voe_authoring_project` set a window
  in range.
- `authoring.md`: the `project.h` entry says the window section too.

## Done when
`ctest --test-dir build/debug -R '^authoring/project$'` passes after the folder builds, with the
tests above in `authoring/tests/project.c`.
