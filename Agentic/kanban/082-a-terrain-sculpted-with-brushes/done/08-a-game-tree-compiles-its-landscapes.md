# 08 — A game tree compiles its landscapes
folder: cmake
after: 07
decisions: 0168, 0379

## Change
A game tree's `landscapes.c`, the cooked table of `game/landscapes.h` (card 07), is compiled into the
game, beside `prefabs.c` (0379 point 7).

- `cmake/game.cmake` — the executable `game` (OFF mode) adds `${CMAKE_CURRENT_SOURCE_DIR}/landscapes.c`
  to its sources; the library `project` (ON mode) does not, as it has no `prefabs.c`. The file's head
  comment names `landscapes.c` and what it defines beside `prefabs.c`, and the OFF mode's source list.
- `cmake/cmake.md` — the `game.cmake` entry names the cooked landscapes in a phrase.

The editor writes the file in card 09; until then a Play's configure fails, which card 09 ends.

## Done when
`grep -c 'landscapes.c' cmake/game.cmake` prints 2 or more.
