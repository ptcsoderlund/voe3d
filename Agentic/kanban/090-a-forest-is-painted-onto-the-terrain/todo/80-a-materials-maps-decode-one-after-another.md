# 80 — A material's maps decode one after another
folder: game
after: 79
decisions: 0168, 0445, 0457, 0459

## Change
Bug 09, 0459 point 1 for materials: the three steps and their line stay, the pool goes. After
this card nothing in `game` names a pool.

- `game/src/models_materials.c` — `chunk_prepare` loses `workers`: reading runs its materials,
  then decoding its maps, index by index on the caller in order, each under the subjects it has
  now. A chunk is one material (the gathered list taken one at a time, read, decoded, put, its
  arenas given back) in `voe_game_models_materials` and in the one-material load; neither calls
  `voe_3d_models_workers` nor `voe_base_workers_threads`. Header: THE PHASES paragraph says in
  order on the calling thread, one material at a time (0459); "over the store's pool in chunks
  of its threads" goes. The `game: materials:` line unchanged.
- `game/include/game/models.h` — the MATERIALS paragraph and the material load's comment: maps
  decoded one after another on the caller, uploaded in order, the line; "side by side on the
  store's pool" and "the store's pool" go. `#include <base/workers.h>` goes.
- `game/src/src.md` — the `models_materials.c` entry: one material at a time, no pool.
- `game/tests/models.c` — `material_pool_load`, `check_material_pool`, their call in `main` and
  `#include <base/workers.h>` go, and `struct entry_seen` with its comment if nothing else
  uses it. `broken_load` stays as a check of its own, on a fresh store
  with nothing set (no `workers` parameter): a broken map keeps its material failed, its path in
  the kept error; called from `main`. Header: THE MATERIAL POOL paragraph becomes that broken
  map check.
- `game/tests/tests.md` — the `models.c` entry: "a pool's materials the same as none's" goes,
  the broken map's path kept stays.

## Done when
`ctest --test-dir build/debug -R '^game/(models|frame)$'` passes, and
`! grep -rnE "workers|voe_platform_cores|pool's|the pool" game/src game/include game/tests`
exits 0.
