# 60 — The 3d tests pass and fit their caps
folder: 3d/tests
after: none
decisions: 0168, 0392, 0393

## Change
- `3d/tests/bounce_scene.c`, the BLOCKED case, `a_blocker_keeps_the_bounce_out`: remove the last check,
  after the blocker is destroyed and settled, that the patch is within 1/255 of the tint before (0393).
  Keep everything before it, including the blocker-standing checks. Drop any local the removal leaves unused.
  Keep the blocker being destroyed and settled only if a later case needs that world. Read the function's
  comment and the cases called after it in `main` to decide.
- `3d/tests/bounce_scene.c` header: 60 lines or fewer, down from 62. The BLOCKED paragraph and the
  function's comment no longer claim the tint comes back. They say the check is set aside until work
  order 083 (0393). Tighten other paragraphs to fit. Lose no claim the remaining cases make.
- `3d/tests/tests.md`: these entries are each one sentence of 300 characters or fewer:
  `bounce_grid.c` (544), `shadow_lights.c` (379), `bounce.c` (321), `bounce_scene.c` (367), `shape.c` (450).
  A point an entry drops goes into that file's header only if the header does not already make it. Keep
  every header at 60 lines or fewer (`bounce.c` is at 58 now).

## Done when
`cmake --build --preset debug --target voe_test_3d_bounce_scene && ctest --test-dir build/debug -R '^3d/bounce_scene$'`
passes, and `checks.sh --folder 3d/tests` reports no finding in `3d/tests`.
