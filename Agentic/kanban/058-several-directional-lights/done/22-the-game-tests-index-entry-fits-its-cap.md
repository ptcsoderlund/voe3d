# 22 — The game tests index entry fits its cap
folder: game/tests
after: none
decisions: 0168

## Change
`game/tests/tests.md`: the entry `frame.c` is 312 characters, cap 300. Make it one sentence under
300; detail dropped and not already in `game/tests/frame.c`'s header comment goes there, comments
only.

## Done when
`checks.sh --folder game/tests` prints no FINDING naming `tests.md`.
