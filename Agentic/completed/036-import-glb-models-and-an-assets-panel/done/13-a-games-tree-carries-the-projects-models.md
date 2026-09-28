# 13 — A game's tree carries the project's models
folder: cmake
decisions: 0168, 0277, 0266

## Change
0277 point 4: the game reads a model at its project-relative path beside its program, in Play
and shipped, exactly as a sound.

- `cmake/game.cmake`: the `.wav` glob becomes `*.wav` and `*.glb` (still skipping `Build/` and
  `Cache/`); the target `game_sounds` becomes `game_files`, the loop variables named for files;
  every copy and install is unchanged otherwise. The header's "Sounds (0266)" paragraph speaks
  of sounds and models, and "the sounds are the only files read" names models too.
- `cmake/cmake.md`: the `game.cmake` entry says `.wav` sounds and `.glb` models.

## Done when
`grep -c 'glb' cmake/game.cmake` prints at least 2, `grep -c game_sounds cmake/game.cmake` prints
0, and `cmake -P check.cmake` exits 0. Play and Ship carrying a model is the human's step 8 on
card 19.
