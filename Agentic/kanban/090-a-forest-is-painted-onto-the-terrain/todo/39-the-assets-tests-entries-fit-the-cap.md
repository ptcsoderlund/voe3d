# 39 — The assets/tests entries fit the cap
folder: assets/tests
after: none
decisions: 0168

## Change
The whole suite found index entries in `assets/tests/tests.md` over the 300-character cap. Comments and `.md` only; no code changes.

Shorten each of these entries to one sentence of at most 300 characters (the cap counts the whole `- `name` — ...` line): `png.c` (385), `model.c` (340), `jpeg.c` (391). Any point an entry drops that its file's own header comment does not already make goes into that header (`assets/tests/png.c`, `assets/tests/model.c`, `assets/tests/jpeg.c`), each kept under its 60-line cap (measure with `bash ~/.claude/skills/checks/scripts/checks.sh --header-lines <file>`).

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --structure 2>&1 | grep -F 'FINDING assets/tests/tests.md'` prints nothing.
