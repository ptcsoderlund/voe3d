# 07 — The game's window remembers its casters
folder: game
after: 05, 06
decisions: 0168, 0388, 0394
read: feature.md

## Change
0394 point 4: the game's window is a target, so its frame carries a `voe_3d_bounce_casters`
(header of `3d/include/3d/bounce_casters.h`), kept by the run loop.

- `game/include/game/frame.h`: `voe_game_frame` takes `voe_3d_bounce_casters *casters` after `lag`, NULL
  for none (a removed caster then marks nothing). Its comment says so and that the caller keeps one per
  window. The usage block at the top of the header passes one.
- `game/src/frame.c`: the `voe_3d_frame` handed to the shadows call carries `casters`.
- `game/src/run.c`: the run keeps one memory, zeroed, for as long as the window lives, beside the other
  per-run state, and passes it at the `voe_game_frame` call. 20 KB; not on a small stack.
- `game/include/game/steps.h`: the usage line calling `voe_game_frame` passes the memory.
- `game/tests/frame.c`: every call passes NULL.
- Indexes: `game/include/game/game.md`'s `frame.h` entry and `game/src/src.md`'s `run.c` entry name the
  remembered casters.

## Done when
After `cmake --build --preset debug --target voe_game voe_test_game_frame`,
`ctest --test-dir build/debug -R '^game/frame$'` passes.

Then the human, in the editor, walks `feature.md`'s How to test; the coder does not:
1. A flat floor, the sun low, `bounces` 1, strength 2.
2. A 1 m purple cube: the floor beside its lit face goes pink, fading within 1–2 m; none on the shaded
   side.
3. Strength 5: the pink is stronger. A red slab in front of the lit face reddens the cube's face.
4. Away from the cube the floor is even: no blotch, streak, ring or curved edge.
5. Move the cube: the pink follows. Delete it: the floor is plain. Undo: the pink is back.
6. Strength 2 → 5 → 2 ends exactly as it began.
7. Steps 2 and 5 again on the Hill landscape, the cube on a slope.
8. Flying about the cube: no pop or crawl, and a steady 60 fps at 2560×1440 (0388).
9. Play: the same tint.
