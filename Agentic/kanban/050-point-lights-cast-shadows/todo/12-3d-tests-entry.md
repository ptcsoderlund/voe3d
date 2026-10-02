# 12 — The 3d tests index entry for point_lights.c fits its cap
folder: 3d/tests
after: none
decisions: 0168

## Change
In `3d/tests/tests.md`, the entry for `point_lights.c` is over the 300-character cap. Cut it to one
sentence that says what the file tests. Open the header comment of
`3d/tests/point_lights.c` and move into it whatever the entry loses that the header does not
already say. Change no code and no other entry.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder 3d/tests` reports no
finding for `3d/tests/tests.md`.
