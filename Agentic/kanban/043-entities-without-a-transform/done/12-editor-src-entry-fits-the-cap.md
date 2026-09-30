# 12 — The editor src index entry for scene.c fits the cap
folder: editor/src
after: none
decisions: 0168

## Change
In `editor/src/src.md`, the entry for `scene.c` is 366 characters,
over the 300 cap. Cut it to one sentence under 300 characters naming what
the file owns. Any detail the cut drops that the header of
`editor/src/scene.c` does not already say moves into that header.

Only the `.md` entries and the named files' header comments change; no
code, no test body.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder editor/src` prints no
`entry ... cap 300` FINDING for `editor/src/src.md`.
