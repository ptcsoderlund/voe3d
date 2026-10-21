# 62 — The 3d/models.h entry fits the cap
folder: 3d/include/3d
after: none
decisions: 0168

## Change
The whole suite found index entries in `3d/include/3d/3d.md` over the 300-character cap.
Comments and `.md` only; no code changes.

Shorten each of these entries to one sentence of at most 300 characters (the
cap counts the whole line, bullet and name included): `models.h` (335). Any point an entry
drops that its file's own header comment does not already make goes into that
header (`3d/include/3d/models.h`), each kept under its 60-line cap (measure with
`bash ~/.claude/skills/checks/scripts/checks.sh --header-lines <file>`).

`models.h` is a public header whose comment is at 59 lines: comments only,
no declaration changes. A dropped point that would push it past 60 goes in a
comment directly above the function it is about, in this same header.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --structure 2>&1 | grep -F 'FINDING 3d/include/3d/3d.md'` prints nothing.
