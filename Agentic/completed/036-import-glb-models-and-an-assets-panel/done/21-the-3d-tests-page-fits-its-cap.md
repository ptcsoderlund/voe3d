# 21 — The 3d tests page fits its cap
folder: 3d/tests
decisions: 0168

## Change
In `3d/tests/tests.md` three entries are over the 300-character cap an entry may have:
`shadows.c`, `pick.c` and `models.c`. Shorten each to under 300 characters, keeping the claims
a reader looks up (for `shadows.c`, that a model casts as the cube does; for `pick.c`, that a
model is hit; for `models.c`, the store's reload, failure and draw claims) and the line saying
whether it needs or skips without a graphics card. Merge claims into shorter phrases rather than
dropping the model ones this feature added. Change no other entry and no code.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder 3d/tests` prints `FINDINGS: 0`.
