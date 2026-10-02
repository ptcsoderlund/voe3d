# 13 — The game tests index entry for frame.c fits its cap
folder: game/tests
after: none
decisions: 0168

## Change
In `game/tests/tests.md`, the entry for `frame.c` is over the 300-character cap. Cut it to one
sentence that says what the file tests. Open the header comment of
`game/tests/frame.c` and move into it whatever the entry loses that the header does not
already say. Change no code and no other entry.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder game/tests` reports no
finding for `game/tests/tests.md`.
