# 25 — The game tests index entry fits the cap
folder: game/tests
decisions: 0168

## Change
`game/tests/tests.md`: the entry for `project.c` is 309 characters, cap 300.
Cut it to one sentence under 300 characters naming what the test file proves.
Any detail the cut drops that is not already in the header comment of
`game/tests/project.c` moves there. No code changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder game/tests` prints no
finding naming `tests.md`.
