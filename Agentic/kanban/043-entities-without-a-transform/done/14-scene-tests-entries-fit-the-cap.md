# 14 — The scene tests index entries for parent.c and identity.c fit the cap
folder: scene/tests
after: none
decisions: 0168

## Change
In `scene/tests/tests.md`, the entries for `parent.c` (361
characters) and `identity.c` (367) are over the 300 cap. Cut each to one
sentence under 300 characters naming what the file proves. Any detail a
cut drops that the header of `scene/tests/parent.c` or
`scene/tests/identity.c` does not already say moves into that header.

Only the `.md` entries and the named files' header comments change; no
code, no test body.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder scene/tests` prints no
`entry ... cap 300` FINDING for `scene/tests/tests.md`.
