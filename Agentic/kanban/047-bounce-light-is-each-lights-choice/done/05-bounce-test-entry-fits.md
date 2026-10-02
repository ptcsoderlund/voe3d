# 05 — The bounce test's index entry fits its cap
folder: 3d/tests
after: 03
decisions: 0168

## Change
`3d/tests/tests.md`, the entry for `bounce.c`: it is 324 characters, the cap
is 300. Cut it to one sentence naming what the test proves (the shadows call's
pass count with and without bounces, the stale spheres of a moved wall) and
that it skips without a graphics card. The detail it drops is already in the
header comment of `3d/tests/bounce.c`; read that header and add to it only if
a dropped point is missing there. No other entry and no code changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder 3d/tests` prints no
`FINDING` line naming `3d/tests/tests.md`.
