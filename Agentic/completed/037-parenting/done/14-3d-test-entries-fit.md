# 14 — The 3d tests' index entries fit the cap
folder: 3d/tests
decisions: 0168

## Change
`3d/tests/tests.md`: the entries `pick.c` (348 characters) and `outline.c` (337) are over the
300-character entry cap. Shorten each to one sentence under 300 characters that says what the
file proves. Open the header comment of `3d/tests/pick.c` and `3d/tests/outline.c`; any point
the entry drops that the header does not already make (the child hit at its world place, the
child's quads about its world place, the half that skips without a device) goes into that header.
No code changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder 3d/tests` prints `FINDINGS: 0`.
