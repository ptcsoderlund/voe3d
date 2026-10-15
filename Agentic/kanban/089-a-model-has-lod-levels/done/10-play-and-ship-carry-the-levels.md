# 10 — Play and Ship carry the levels files
folder: cmake
after: none
decisions: 0168, 0426

## Change
0426 point 4: the game reads the editor's levels, so a game tree's build copies and installs
`Cache/levels/**/*.levels` at the same relative path, as it does the project's models.

- `cmake/game.cmake`, the project-files block (the `voe_top` glob, around its line 175): the
  top-level `Cache` stays out of the general walk; after that walk, a `file(GLOB_RECURSE ...
  CONFIGURE_DEPENDS "${voe_project_root}/Cache/levels/*.levels")` appended to `voe_files`, so the
  existing copy and install loop carries them unchanged. A comment line above it names 0426 and
  says a missing file is made by the game at load.
- `cmake/cmake.md`: the `game.cmake` entry names the levels files among what is copied.

## Done when
`grep -n 'Cache/levels/\*\.levels' cmake/game.cmake` prints one line, and
`grep -n 'levels' cmake/cmake.md` prints the entry.
