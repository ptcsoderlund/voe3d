# 21 — The editor/src entries and the drag header fit the caps
folder: editor/src
after: none
decisions: 0168

## Change
The whole suite found these in `editor/src`. Comments and `.md` only; no code changes.

`editor/src/src.md`: shorten each of these entries to one sentence of at most 300
characters (the cap counts the whole `- `name` — ...` line): `assets_drag.h` (309),
`inspector.c` (301), `sculpt.h` (325), `models.h` (312). Any point an
entry drops that its file's header comment does not already make goes into that header
(`editor/src/assets_drag.h`, `editor/src/inspector.c`, `editor/src/sculpt.h`,
`editor/src/models.h`), each kept under its 60-line cap.

`editor/src/assets_drag.h`: its header comment is 64 lines, cap 60. Move the paragraph
"THE INSPECTOR DROP REPLACES THE PATH" (why a drop onto the Inspector swaps the path rather
than adding a component, what it leaves alone, the too-long-path case) out of the header
into a comment directly above `onto_inspector` in `editor/src/assets_drag.c`, and leave a
one-line pointer to it in the header's Inspector outcome bullet if needed. Reflow the
CONSTRAINTS paragraph, whose lines run past the others' width. Measure each header touched
with `bash ~/.claude/skills/checks/scripts/checks.sh --header-lines <file>`.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --structure 2>&1 | grep -F 'FINDING editor/src/'`
prints nothing.
