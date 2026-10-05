# 21 — The game src index entry fits its cap
folder: game/src
after: none
decisions: 0168

## Change
`game/src/src.md`: the entry `frame.c` is 341 characters, cap 300. Make it one sentence under
300; detail dropped and not already in `game/src/frame.c`'s header comment goes there, comments
only.

## Done when
`checks.sh --folder game/src` prints no FINDING naming `src.md`.
