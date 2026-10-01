# 12 — The game tests' index entries fit the cap
folder: game/tests
after: none
decisions: 0168

## Change
In `game/tests/tests.md`, the entries `world.c` and `steps.c` are each 306
characters; the cap is 300. Shorten each to one sentence under the cap, by
dropping enumerated detail (the list of keys in `world.c`, the list of steps
checked in `steps.c`). If a dropped detail is not already in the header
comment of `game/tests/world.c` or `game/tests/steps.c`, add it there as a
point; touch only the header comments, no test code.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder game/tests` prints
`FINDINGS: 0`.
