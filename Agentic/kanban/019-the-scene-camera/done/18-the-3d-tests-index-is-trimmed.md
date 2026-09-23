# 18 — The 3d tests' index entries fit the cap
folder: 3d/tests
decisions: 0168

## Change
In `3d/tests/tests.md` the `draw_system.c` entry (396 characters) and the `pick.c` entry
(354) are over the 300-character cap. Cut each to one sentence under 300 characters naming
what the tests prove, keeping the graphics-card note (skips without one / arithmetic half
needs none). Detail cut that the test file's header does not already carry goes into that
header (`3d/tests/draw_system.c`, `3d/tests/pick.c`, header comment only). No code changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder 3d/tests` prints `FINDINGS: 0`.
