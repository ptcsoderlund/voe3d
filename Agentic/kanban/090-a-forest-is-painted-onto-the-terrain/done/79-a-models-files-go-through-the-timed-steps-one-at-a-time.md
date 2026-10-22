# 79 — A model's files go through the timed steps one at a time
folder: game
after: 77, 78
decisions: 0168, 0370, 0457, 0459

## Change
Bug 09, 0459 point 1 for update, watch, reload and prepare: the steps and their line stay, the
pool goes. The editor still passes a pool to the prepare until card 82; it does not build between.

- `game/src/models_batch.h`, `game/src/models_batch.c` —
  - `voe_game_models_batch_prepare(batch, times)`: no `workers`; each of the five steps runs its
    tasks index by index on the caller, in order, each still under its file's path as the
    subject. The batch's `workers` field and the asserts on it go; a prepare twice or a destroy
    mid-prepare need no guard on one thread.
  - `voe_game_models_levels_for` loses `workers`; it calls card 78's
    `voe_3d_model_levels_make` without one.
  - `#include <base/workers.h>` goes from both.
  - Header: the steps run in order on the calling thread (0457, 0459); a batch holds one file,
    so one file's arenas at a time; "each one voe_base_workers_run", "over the pool" and the
    chunk wording go; the usage example loses `workers`. The times line keeps all six steps.
- `game/src/models.c` — the two `room = voe_base_workers_threads(...)` become a batch of one
  file; progress set and stop asked before each file, as before each chunk now.
  `voe_game_models_prepare` loses `workers`. Header and function comments: "chunks of the
  store's pool's threads" and "prepared over the store's pool" become one file at a time.
- `game/include/game/models.h` — `voe_game_models_prepare(folder, path, scratch, remake, made,
  error)`; its comment's "made over `workers`" goes. The PHASES paragraph: one file at a time on
  the caller through the six steps, the same line; `voe_3d_models_workers` no longer named.
  `#include <base/workers.h>` stays until card 80 if the MATERIALS paragraph's code needs it;
  else it goes.
- `game/src/run.c` — the pool (`voe_base_workers_new`, `voe_3d_models_workers_set`, its
  destroy, the `workers` field) and `#include <base/workers.h>`, `<platform/cores.h>` go.
  Header: the sentence on the store's pool goes.
- `game/src/src.md` — the `run.c`, `models.c`, `models_batch.h` and `models_batch.c` entries:
  no pool, one file at a time.
- `game/tests/models.c` — `pool_load`, `check_pool`, their call in `main` and the header's
  THE POOL paragraph go. Keep `#include <base/workers.h>` for card 80's checks.
- `game/tests/models_levels.c` — every `voe_game_models_prepare` call loses its `NULL`.
- `game/tests/tests.md` — the `models.c` entry: "a pool's load the same as none's" goes.

## Done when
`ctest --test-dir build/debug -R '^game/(models|models_levels|frame)$'` passes, and
`! grep -nE "voe_base_workers|voe_platform_cores|voe_3d_models_workers" game/src/models.c
game/src/models_batch.c game/src/models_batch.h game/src/run.c` exits 0.
