# 64 — The 3d/tests entries fit the cap
folder: 3d/tests
after: none
decisions: 0168

## Change
The whole suite found index entries in `3d/tests/tests.md` over the 300-character cap.
Comments and `.md` only; no code changes.

Shorten each of these entries to one sentence of at most 300 characters (the
cap counts the whole line, bullet and name included): `shape_geometry.c` (368), `models.c` (335), `models_levels.c` (334), `models_material.c` (310). Any point an entry
drops that its file's own header comment does not already make goes into that
header (`3d/tests/shape_geometry.c`, `3d/tests/models.c`, `3d/tests/models_levels.c`, `3d/tests/models_material.c`), each kept under its 60-line cap (measure with
`bash ~/.claude/skills/checks/scripts/checks.sh --header-lines <file>`).

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --structure 2>&1 | grep -F 'FINDING 3d/tests/tests.md'` prints nothing.
