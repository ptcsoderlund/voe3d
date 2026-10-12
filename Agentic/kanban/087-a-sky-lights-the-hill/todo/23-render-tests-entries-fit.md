# 23 — The render tests' index entries fit their cap
folder: render/tests
after: none
decisions: 0168

## Change
`render/tests/tests.md`: the `exposure.c` entry is 308 characters and the `sky_light.c` entry 320,
cap 300. Shorten each to one sentence of at most 300 characters. Detail dropped that the header
comment of `render/tests/exposure.c` or `render/tests/sky_light.c` does not already carry moves
there (read the header comments only; each stays at most 60 lines). No code changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder render/tests` prints `FINDINGS: 0`.
