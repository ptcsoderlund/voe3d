# 42 — The 3d/tests shadows.c index entry fits the cap
folder: 3d/tests
after: none
decisions: 0168

## Change
`3d/tests/tests.md`: the `shadows.c` entry is 319 characters, cap 300. Cut
it to one sentence under 300 characters naming what the test proves: a cube
under a sun straight down shadows the floor beneath it, a fill lifts the
shadow, no light casts nothing, and the same holds 100 km out and for a
model; keep that it skips without a graphics card.

Every case the cut drops is already in the header comment of
`3d/tests/shadows.c`; do not change that file. Only this entry changes; no
code, no other entries.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder 3d/tests` prints no
finding naming `tests.md`.
