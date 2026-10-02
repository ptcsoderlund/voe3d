# 06 — Dev's sun casts
folder: dev
after: 02, 04
decisions: 0168, 0324

## Change
0324 point 2: a light built from a literal casts nothing, and dev shows what
the engine can do, shadows included. Read `dev/src/startup.c` (the sun,
around line 283) and `dev/dev.md`.

- `startup.c`: the sun's literal sets `cast_shadows = true`; the comment
  above it says why (dev shows shadows; a light's default casts none, 0316).
- `dev/dev.md`, only if its startup entry says what the sun is and no
  longer does.

## Done when
`grep -n "cast_shadows = true" dev/src/startup.c` prints the sun's line,
and `ctest --test-dir build/debug -R '^dev/'` passes.
