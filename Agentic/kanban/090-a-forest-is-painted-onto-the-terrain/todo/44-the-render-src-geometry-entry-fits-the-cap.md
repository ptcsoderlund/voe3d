# 44 — The render/src geometry entry fits the cap
folder: render/src
after: none
decisions: 0168

## Change
The whole suite found index entries in `render/src/src.md` over the 300-character cap. Comments and `.md` only; no code changes.

Shorten each of these entries to one sentence of at most 300 characters (the cap counts the whole `- `name` — ...` line): `geometry.c` (345). Any point an entry drops that its file's own header comment does not already make goes into that header (`render/src/geometry.c`), each kept under its 60-line cap (measure with `bash ~/.claude/skills/checks/scripts/checks.sh --header-lines <file>`).

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --structure 2>&1 | grep -F 'FINDING render/src/src.md'` prints nothing.
