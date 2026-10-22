# 17 — The 3d/tests entry fits the cap
folder: 3d/tests
after: none
decisions: 0168

## Change
The whole suite found index entries in `3d/tests/tests.md` over the 300-character cap. Shorten these entries:
- `draw_foliage.c` (385 characters), header `3d/tests/draw_foliage.c`

Each entry becomes one sentence of at most 300 characters (the cap counts the whole `- `name` — ...` line). Any point the entry drops that the file's own header comment does not already make goes into that header, kept under its 60-line cap (measure with `checks.sh --header-lines <file>`). Comments and `.md` only; no code changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --structure 2>&1 | grep -F 'FINDING 3d/tests/'` prints nothing.
