# 33 — The render tests index entry fits its cap
folder: render/tests
after: none
decisions: 0168

## Change
`render/tests/tests.md`: the entry `directional_lights.c` is 302 characters, cap 300. Make it one
sentence under 300; detail dropped and not already in `render/tests/directional_lights.c`'s
header comment goes there, comments only.

## Done when
`checks.sh --folder render/tests` prints no FINDING naming `tests.md`.
