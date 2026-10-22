# 77 — The loader beside the frames comes out of game
folder: game
after: 76
decisions: 0168, 0456, 0457, 0459

## Change
Bug 09, 0459 point 4: no loading thread beside the frames. The editor still includes the
loader until card 82; it does not build between the two cards.

- `game/include/game/models_loader.h`, `game/src/models_loader.c`,
  `game/tests/models_loader.c` — deleted (`git rm`).
- `game/include/game/game.md` — the `models_loader.h` entry goes.
- `game/src/src.md` — the `models_loader.c` entry goes; the opening phrase naming what
  `models_loader.h` shares goes.
- `game/tests/tests.md` — the `models_loader.c` entry goes; the `glb_build.h` entry counts the
  model tests that include it as two.
- `game/tests/glb_build.h` — its header stops naming `tests/models_loader.c`.
- `game/src/models_batch.h` — header: the paragraph's sentence about the loader of
  `models_loader.c` (filling a batch, preparing it on a thread, a file a frame) goes; the
  gathering calls are now the update's and the watch's alone. Nothing else here; card 79
  rewrites the rest of the header.
- `game/src/models.c` — header: the last sentence's "the loader beside the frames
  (models_loader.c) shares" goes. No code changes.
- `game/include/game/models.h` — only if its header names the loader: that phrase goes.

## Done when
`test ! -e game/include/game/models_loader.h && ! grep -rn "models_loader" game` exits 0, and
`ctest --test-dir build/debug -R '^game/(models|models_levels)$'` passes.
