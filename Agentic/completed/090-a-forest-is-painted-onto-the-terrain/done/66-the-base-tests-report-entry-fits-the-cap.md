# 66 — The base/tests report.c entry fits the cap
folder: base/tests
after: none
decisions: 0168

## Change
The whole suite found index entries in `base/tests/tests.md` over the 300-character cap.
Comments and `.md` only; no code changes.

Shorten each of these entries to one sentence of at most 300 characters (the
cap counts the whole line, bullet and name included): `report.c` (336). Any point an entry
drops that its file's own header comment does not already make goes into that
header (`base/tests/report.c`), each kept under its 60-line cap (measure with
`bash ~/.claude/skills/checks/scripts/checks.sh --header-lines <file>`).

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --structure 2>&1 | grep -F 'FINDING base/tests/tests.md'` prints nothing.
