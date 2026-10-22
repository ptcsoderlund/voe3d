# 50 — The 3d/tests shape_geometry entry fits the cap
folder: 3d/tests
after: none
decisions: 0168

## Change
The whole suite found the `shape_geometry.c` entry in `3d/tests/tests.md` at 325 characters, over the 300-character cap (the cap counts the whole `- `name` — ...` line). Comments and `.md` only; no code changes.

Shorten that entry to one sentence of at most 300 characters. The header comment of `3d/tests/shape_geometry.c` (30 lines now) already makes most of the points; the one it may lack is that the cube's arrays built alone give its edge count. Any point the entry drops that the header does not make goes into that header, kept under its 60-line cap (measure with `bash ~/.claude/skills/checks/scripts/checks.sh --header-lines 3d/tests/shape_geometry.c`).

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --structure 2>&1 | grep -F 'FINDING 3d/tests/tests.md'` prints nothing.
