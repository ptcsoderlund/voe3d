# 20 — The game tests index entry fits its cap
folder: game/tests
after: none
decisions: 0168

## Change
`game/tests/tests.md`: the `models.c` entry is 323 characters, cap 300. Cut it to
one sentence under 300 characters. If the cut drops a point that is not already in
the header comment of `game/tests/models.c`, add that point to the header comment.
No test code change.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder game/tests` prints `FINDINGS: 0`.
