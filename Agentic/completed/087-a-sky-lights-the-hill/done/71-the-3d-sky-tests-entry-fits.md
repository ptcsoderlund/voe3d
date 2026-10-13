# 71 — The 3d sky test's index entry fits its cap
folder: 3d/tests
after: none
decisions: 0168

## Change
- `3d/tests/tests.md`: the `sky.c` entry is 321 characters, cap 300. Shorten it to one
  sentence of at most 300 characters: summarise the cases (zeroed sky and picture with no
  table or row, the Haze row's haze and top, each light's disc and glow, the lit-slab picture)
  rather than list each, keeping that the picture case skips without a graphics card.
- `3d/tests/sky.c`: read its header comment only. Any case dropped from the entry that the
  header does not already name is added there as a phrase. Comments only; no code changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder 3d/tests` prints `FINDINGS: 0`.
