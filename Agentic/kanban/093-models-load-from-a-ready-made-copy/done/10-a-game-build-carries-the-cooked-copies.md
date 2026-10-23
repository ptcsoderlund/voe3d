# 10 — A game build carries the cooked copies
folder: cmake
after: none
decisions: 0168, 0470, 0426

## Change
0470 point 8: Play's build tree and the shipped install carry the project's cooked copies. They are
copied and installed exactly as the levels files are.

- `cmake/game.cmake`, the block near lines 191–193 that globs `Cache/levels/*.levels` with
  CONFIGURE_DEPENDS: two more recursive globs append to `voe_files`, so the existing
  `copy_if_different` command and `install(FILES …)` take them unchanged.
  - `Cache/models/*.model`
  - `Cache/pictures/*.colour`, `*.normal` and `*.data`

  The comment above the block says the cooked copies ride along like the levels, and that the game
  trusts them and makes a missing one at load (0470 point 5).
- `cmake/cmake.md`: the `game.cmake` entry names the cooked copies.

## Done when
`grep -q 'Cache/models/\*\.model' cmake/game.cmake && grep -q 'Cache/pictures/\*\.data' cmake/game.cmake`
exits 0. The human's Play and Ship steps on card 12 prove that the globs configure.
