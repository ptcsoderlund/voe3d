# 07 — The game's loader reads the pictures emitters name
folder: game
after: 06
decisions: 0168, 0298

## Change
0298 point 5. Read `game/include/game/models.h`, `game/src/models.c` and
`3d/include/3d/models.h` (card 04).

- `game/src/models.c`, `voe_game_models_update`: besides every model row's
  path, every emitter row's non-empty texture is read and loaded the same
  way (the store picks picture or model by the extension); when the world
  has any emitter row and the store finds nothing at "", it loads the dot
  with `voe_3d_models_load_dot`, a failure counted and said as a load's.
  `voe_game_models_watch` needs nothing: picture entries are entries.
- `game/include/game/models.h`: the header says update reads emitter
  textures too and loads the dot, and that watch re-reads pictures as it
  does models.
- `game/tests/models.c`: a world with an emitter naming a PNG written to the
  test's folder loads a picture entry at that path and the dot at ""; an
  emitter naming a missing file is one failure; a world with no emitter
  loads no dot. Write the PNG with `assets/include/assets/image.h`'s encoder
  if the game's test links `assets`, else inline its bytes.
- `game/tests/tests.md`: the entry.

## Done when
`ctest --test-dir build/debug -R '^game/models$'` passes, after the folder's
build.
