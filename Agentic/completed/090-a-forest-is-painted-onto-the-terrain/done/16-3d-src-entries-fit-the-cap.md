# 16 — The 3d/src entries fit the cap
folder: 3d/src
after: none
decisions: 0168

## Change
The whole suite found index entries in `3d/src/src.md` over the 300-character cap. Shorten these entries:
- `draw_shadows.c` (330 characters), header `3d/src/draw_shadows.c`
- `models_landscape.c` (330 characters), header `3d/src/models_landscape.c`

Each entry becomes one sentence of at most 300 characters (the cap counts the whole `- `name` — ...` line). Any point the entry drops that the file's own header comment does not already make goes into that header, kept under its 60-line cap (measure with `checks.sh --header-lines <file>`). Comments and `.md` only; no code changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --structure 2>&1 | grep -F 'FINDING 3d/src/'` prints nothing.
