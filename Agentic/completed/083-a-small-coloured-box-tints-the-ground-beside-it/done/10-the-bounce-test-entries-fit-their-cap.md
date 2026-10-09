# 10 — The bounce test entries fit their cap
folder: 3d/tests
after: none
decisions: 0168

## Change
In `3d/tests/tests.md`, the entries for `bounce_scene.c` and `bounce_tint.c`
are over the 300-character cap. Cut each to one sentence that says what the
file is for:
- `bounce_scene.c`: the probe bounce as the editor draws it tints, settles
  and stays put under turns, moves and a blocker; skips without a card.
- `bounce_tint.c`: a small box tints the ground beside it through strength,
  colour, moves, a whole relight and removal; skips without a card.

Before cutting, open the header comment of `3d/tests/bounce_scene.c` and of
`3d/tests/bounce_tint.c` (header only, not the body) and add to each any point
its old entry made that the header does not already make; the header is where
the detail lives. No code changes; no other entry changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder 3d/tests` reports no
finding naming `bounce_scene.c` or `bounce_tint.c`.
