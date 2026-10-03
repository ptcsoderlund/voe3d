# 03 — A shipped game carries the pictures its emitters read
folder: cmake
after: none
decisions: 0168, 0264

## Change
An emitter's texture (`.png` or `.jpg`, 0298 point 5) is read beside the program as sounds and
models are, but `game_files` copies and installs only `.wav` and `.glb`, so the shipped tank
game loses its particle picture. Read `cmake/game.cmake` and `cmake/cmake.md`.

- `cmake/game.cmake`: the per-folder glob and the top-level match take `.png` and `.jpg` beside
  `.wav` and `.glb`; copied and installed the same way. The header's "Sounds and models"
  paragraph and its "only the sounds and models are read" line say pictures too.
- `cmake/cmake.md`: the `game.cmake` entry says pictures.

## Done when
`grep -c 'png' cmake/game.cmake` and `grep -c 'jpg' cmake/game.cmake` each print 2 or more.
The shipped picture itself is checked by the human's Ship on card 11.
