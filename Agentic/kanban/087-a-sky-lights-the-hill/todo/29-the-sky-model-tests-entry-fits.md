# 29 — The sky model tests' index entry fits its cap
folder: render/tests
after: none
decisions: 0168

## Change
`render/tests/tests.md`: the `sky_model.c` entry is 313 characters, cap 300. Shorten it to one
sentence of at most 300 characters naming what the file proves (the gradient, glow and
irradiance, with no card). The header comment of `render/tests/sky_model.c` already lists every
case; detail it does not carry moves there (read the header comment only; it stays at most 60
lines). No code changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder render/tests` prints `FINDINGS: 0`.
