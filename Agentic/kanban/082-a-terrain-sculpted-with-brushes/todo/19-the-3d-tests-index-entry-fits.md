# 19 — The 3d tests index entry fits its cap
folder: 3d/tests
after: none
decisions: 0168

## Change
`3d/tests/tests.md`: the `draw_markers.c` entry is 367 characters, cap 300. Cut it
to one sentence under 300 characters. If the cut drops a point that is not already
in the header comment of `3d/tests/draw_markers.c`, add that point to the header
comment. No test code change.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder 3d/tests` prints `FINDINGS: 0`.
