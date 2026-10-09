# 11 — The frame.h entry fits its cap
folder: game/include/game
after: none
decisions: 0168

## Change
In `game/include/game/game.md`, the entry for `frame.h` is over the 300-character cap. Cut it to
one sentence that says what the file is for: the world step and one frame of window pass and interface, with the device capacities it needs. Before cutting, open the
header comment of `game/include/game/frame.h` (header only, not the body) and add to it any point
the old entry made that the header does not already make; the header is where
the detail lives. No code changes; no other entry changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder game/include/game` reports no
finding naming `frame.h`.
