# 11 — The authoring tests index entry for scene_read.c fits the cap
folder: authoring/tests
after: none
decisions: 0168

## Change
In `authoring/tests/tests.md`, the entry for `scene_read.c` is 308
characters, over the 300 cap. Cut it to one sentence under 300 characters
naming what the file proves. Any detail the cut drops that the header of
`authoring/tests/scene_read.c` does not already say moves into that header.

Only the `.md` entries and the named files' header comments change; no
code, no test body.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder authoring/tests` prints no
`entry ... cap 300` FINDING for `authoring/tests/tests.md`.
