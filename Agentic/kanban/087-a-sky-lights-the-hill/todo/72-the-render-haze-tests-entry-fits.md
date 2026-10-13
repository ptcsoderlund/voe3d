# 72 — The render haze test's index entry fits its cap
folder: render/tests
after: none
decisions: 0168

## Change
- `render/tests/tests.md`: the `haze.c` entry is 308 characters, cap 300. Shorten it to one
  sentence of at most 300 characters: keep that far surfaces fade into haze of the sky's hue at
  the lights' brightness and that it is headless; summarise the list of cases (sun position,
  horizons, light sums, no light, no sky, depth clear, veiled sky, haze start) rather than
  list each.
- `render/tests/haze.c`: read its header comment only. Any case dropped from the entry that
  the header does not already name is added there as a phrase. Comments only; no code changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder render/tests` prints `FINDINGS: 0`.
