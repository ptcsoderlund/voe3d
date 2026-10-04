# 09 — the starting.c entry in tests.md fits its cap
folder: game/tests
after: none
decisions: 0168

## Change
Prose only; no code changes.

- `game/tests/tests.md`: the entry for `starting.c` (line ~10) is 334
  characters, cap 300. Cut it to one sentence under 300: the starting frame
  plain and with a splash in wide and tall windows, and the prepare loop
  answering prepared; skips without a graphics card.
- `game/tests/starting.c`: open its header comment only; if a detail the
  cut drops (theme ground near a corner, the line across the middle row,
  edge colour in the margin, picture centred, ground near the bottom) is not
  there, add it as a phrase to that header.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder game/tests` prints
`FINDINGS: 0`.
