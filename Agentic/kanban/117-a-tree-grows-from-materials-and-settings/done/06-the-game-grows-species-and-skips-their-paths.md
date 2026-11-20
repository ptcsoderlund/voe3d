# 06 — The game grows species, and its loader skips their paths
folder: game
after: 01, 05
decisions: 0168, 0515, 0513, 0485, 0504, 0386, 0370

## Change
New `game/include/game/trees.h` and `game/src/trees.c`. Read the header of
`game/include/game/materials.h` and mirror its table and cooked symbol.
- `voe_game_tree` is `{ path, voe_assets_tree values }`; `voe_game_trees` is `{ rows, count }`.
- `extern const voe_game_trees voe_game_trees_cooked;`, defined by the game tree's cooked `materials.c`.
- `voe_game_trees_grow(models, device, const voe_game_trees *, scratch, voe_game_progress *)`, in two
  phases:
  - Growing, on one burst (`platform/burst.h`): each variation of every row is a task with its own arena.
    The task runs `voe_3d_tree_grow`, then `voe_3d_tree_mesh`, then `voe_3d_model_prepare_grown`. The
    worker count is `voe_platform_burst_workers(progress != NULL)`.
  - Upload, on the caller: `voe_3d_models_put_grown` with the row's bark and leaf, then
    `voe_3d_models_species_set`.
  - Progress is "Growing trees", done over total. A stop that is asked returns before the next species.
  - It returns `voe_game_models_failures`.
- An opaque `voe_game_trees_job` for growing between frames:
  - `_new`, `_destroy`;
  - `_start(job, const voe_game_tree *)`: false while busy; it copies the row and starts its burst;
  - `_done`: polls and never waits;
  - `_put(job, models, device, error)`: uploads one variation per call; true when the last one is in and
    the species is set.

`game/src/models.c`:
- the update's paths and a landscape's kind gather skip every path `voe_assets_tree_path_is` accepts;
- the watch and reload pass over grown entries.

Headers:
- `game/include/game/models.h`: in its UPDATE and WATCH points, tree paths are grown by `trees.h` and
  never read as files.
- `trees.h`: the two ways to grow, and threads per 0370.
- Index entries in `game/game.md`, `game/src/src.md` and `game/tests/tests.md`.

New test `game/tests/trees.c`. Make a device as `game/tests/models.c` does; read its header. It checks:
- a table with one species of 3 variations, grown, gives a species entry with 3 loaded variations, each
  with 4 levels;
- a job started on the same row is done after polls and puts in 3 calls;
- an update over a world whose landscape kind is `a.tree` asks for no file.

## Done when
`ctest --test-dir build/debug -R '^game/(trees|models)$'` exits 0.
