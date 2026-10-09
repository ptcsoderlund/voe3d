# 12 — The run.c entry fits its cap
folder: game/src
after: none
decisions: 0168

## Change
In `game/src/src.md`, the entry for `run.c` is over the 300-character cap. Cut it to
one sentence that says what the file is for: the run's steps in the handed window, its threads, and the only file naming the cooked scene, prefabs and landscapes. Before cutting, open the
header comment of `game/src/run.c` (header only, not the body) and add to it any point
the old entry made that the header does not already make; the header is where
the detail lives. No code changes; no other entry changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder game/src` reports no
finding naming `run.c`.
