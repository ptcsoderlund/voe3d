# 24 — The authoring tests index entry fits the cap
folder: authoring/tests
decisions: 0168

## Change
`authoring/tests/tests.md`: the entry for `prefab.c` is 309 characters, cap 300.
Cut it to one sentence under 300 characters naming what the test file proves.
Any detail the cut drops that is not already in the header comment of
`authoring/tests/prefab.c` moves there. No code changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder authoring/tests` prints
no finding naming `tests.md`.
