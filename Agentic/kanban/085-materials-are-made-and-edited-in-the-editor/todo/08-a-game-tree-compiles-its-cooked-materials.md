# 08 — A game tree compiles its cooked materials
folder: cmake
after: none
decisions: 0168, 0399

## Change
0399 point 7: a game tree's `materials.c`, written by the editor (card 13), is compiled into the game.

- `cmake/game.cmake` — `${CMAKE_CURRENT_SOURCE_DIR}/materials.c` joins `landscapes.c` in the game
  executable's sources, and the file's header comment lists `materials.c` beside `landscapes.c` (the
  lines that say what the tree holds and what the game is built from), defining
  `voe_game_materials_cooked` of `game/materials.h`. Nothing else changes: the maps are `.png` and
  `.jpg` files already copied beside the program.

Update `cmake/cmake.md` only if it lists the tree's files.

## Done when
`grep -n 'materials.c' cmake/game.cmake` shows it in the executable's source list beside
`landscapes.c` and in the header comment. Play proves the build once card 13 writes the file.
