# 13 — The game tests index entry for project.c fits the cap
folder: game/tests
after: none
decisions: 0168

## Change
In `game/tests/tests.md`, the entry for `project.c` is 342 characters,
over the 300 cap. Cut it to one sentence under 300 characters naming what
the file proves. Any detail the cut drops that the header of
`game/tests/project.c` does not already say moves into that header.

Only the `.md` entries and the named files' header comments change; no
code, no test body.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder game/tests` prints no
`entry ... cap 300` FINDING for `game/tests/tests.md`.
