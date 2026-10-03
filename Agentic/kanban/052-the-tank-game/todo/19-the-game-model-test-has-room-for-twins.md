# 19 — The game's model test has room for the blended twins
folder: game/tests
after: 17
decisions: 0168, 0336

## Change
0336 point 2 doubles a `.glb` part's shading records; this test's device is sized by hand.
Read `game/tests/models.c` (its header and `CAPACITIES`) and the header of
`3d/include/3d/models.h`.

- `game/tests/models.c`: `CAPACITIES.shadings` gains a record per `.glb` part held at once,
  the reload's included (a picture's and the dot's parts need no twin); the comment above it
  counts the twins. Nothing else changes.

## Done when
`ctest --test-dir build/debug -R '^game/models$'` passes; on a machine without a graphics card
it skips, and building `voe_test_game_models` is the proof.
