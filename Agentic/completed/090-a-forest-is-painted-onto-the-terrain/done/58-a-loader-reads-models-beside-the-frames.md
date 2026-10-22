# 58 — A loader reads models beside the frames
folder: game
after: 57
decisions: 0168, 0370, 0386, 0447, 0448

## Change
0448 point 5: a load between frames (a model placed, a re-export) runs its reading to baking on
a thread while frames go on, and only its upload between two frames, one file a frame.

- `game/include/game/models_loader.h` — new. `typedef struct voe_game_models_loader
  voe_game_models_loader;`, `_new(void)`, `_destroy(loader)` (joins its thread, drops what it
  held, no store touched) and `voe_game_models_failures voe_game_models_loader_step(loader,
  const voe_ecs_world *world, voe_3d_models *models, voe_render_device *device, const char
  *folder, bool watch, voe_base_arena *scratch)`, called between frames each frame:
  1. a batch whose prepare is done: its thread joined; a batch for another folder destroyed
     whole; else its next file put (card 56's put), unless the store now holds that path at
     the file's stamp; after the last, the line, then the batch destroyed.
  2. no batch: the paths update would read (world rows, emitters, loaded landscapes' kinds)
     and, with `watch`, the entries the watch would reload, up to a chunk, gathered into a
     batch whose prepare starts on a `thrd_t` of the loader's over the store's pool.
  3. the dot and the water as update loads them.
  It returns the failures of what it put this call. Header: why (0386, 0448 point 5), what
  runs where, the drop rules, that a splash wait still calls `voe_game_models_update`, and
  that a file's upload is what remains between frames.
- `game/src/models_loader.c` — new: the above; the done flag atomic, the thread's work card 56's
  prepare alone.
- `game/src/models.c`, `game/src/models_batch.h` — update's and watch's gathering into a batch
  become internal calls both the update and the loader use; no change in what update or watch
  read.
- `game/include/game/game.md` — an entry for `models_loader.h`. `game/src/src.md` — an entry for
  `models_loader.c`; `models.c` entry if it changes.
- `game/tests/models_loader.c` — new, skipping without a graphics card as `models.c` does: a
  folder with a `.glb` (`glb_build.h`) a world row names; the first step leaves the store
  without it; steps (`thrd_yield` between, at most 30 s by `voe_platform_clock_now`) until it is
  loaded, the failures none; a re-written file with `watch` true is loaded again at its new
  stamp; a step with an empty second folder while a batch is out drops that batch: the entry ends
  failed, as the second folder lacks the file, never loaded from the first;
  destroy while a prepare runs returns.
- `game/tests/tests.md` — an entry for `models_loader.c`.

## Done when
`ctest --test-dir build/debug -R '^game/(models_loader|models|models_levels)$'` passes.
