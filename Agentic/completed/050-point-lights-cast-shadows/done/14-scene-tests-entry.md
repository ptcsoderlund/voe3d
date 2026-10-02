# 14 — The scene tests index entry for point_light.c fits its cap
folder: scene/tests
after: none
decisions: 0168

## Change
In `scene/tests/tests.md`, the entry for `point_light.c` is over the 300-character cap. Cut it to one
sentence that says what the file tests. Open the header comment of
`scene/tests/point_light.c` and move into it whatever the entry loses that the header does not
already say. Change no code and no other entry.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder scene/tests` reports no
finding for `scene/tests/tests.md`.
