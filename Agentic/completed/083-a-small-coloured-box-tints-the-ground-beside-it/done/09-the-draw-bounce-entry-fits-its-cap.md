# 09 — The draw_bounce.c entry fits its cap
folder: 3d/src
after: none
decisions: 0168

## Change
In `3d/src/src.md`, the entry for `draw_bounce.c` is over the 300-character cap. Cut it to
one sentence that says what the file is for: the frame's probe bounce run when a light bounces, fitted to the still casters, ending in the relight. Before cutting, open the
header comment of `3d/src/draw_bounce.c` (header only, not the body) and add to it any point
the old entry made that the header does not already make; the header is where
the detail lives. No code changes; no other entry changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder 3d/src` reports no
finding naming `draw_bounce.c`.
