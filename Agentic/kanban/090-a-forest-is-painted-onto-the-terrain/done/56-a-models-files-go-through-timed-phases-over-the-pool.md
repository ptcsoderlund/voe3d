# 56 — A model's files go through timed phases over the pool
folder: game
after: 51, 53, 54, 55
decisions: 0168, 0370, 0445, 0447, 0448

## Change
0448 points 2 to 4 for update, watch and reload: one file at a time on one thread becomes
chunks of files through phases on the store's pool, and each phase's time is printed.

- `game/src/models_batch.h`, `game/src/models_batch.c` — new, internal. A batch is up to a
  chunk of files, each with a copy of its path, the stamp it is known at, whether it loads
  whatever its stamp, and arenas of its own (`voe_base_arena_new`: one for the file, one per
  picture), destroyed after its put. Calls, named for what they do:
  - new for a folder and a room, add a path, count, destroy;
  - prepare, given the pool: reading (stamp; bytes; skipped when the stamp is the known one
    and not forced), parsing (`voe_assets_model_read_glb_encoded`), making LODs (`levels_for`,
    the make over the pool), decoding pictures (one task per picture of the chunk,
    `voe_assets_model_decode_picture`), baking (`voe_3d_model_prepare`); each phase one
    `voe_base_workers_run`, each task under its file's path as the subject, a `.landscape` or
    picture path only read; no store, no device touched;
  - put, one file by its index, under its path as the subject: a failed read kept failed at 0, a refused levels make or bake
    at its stamp (`voe_3d_models_fail`), a `.glb` through `voe_3d_models_put`, any other path
    through `voe_3d_models_load`, the failure counted as today; false when the store is full;
  - times: a record of the six phases' wall seconds and the files, added to by prepare and
    put (`voe_platform_clock_now`), and its line, 0448 point 4, written to stderr when a file
    was read.
  `levels_for` and the helpers it uses move here from `models.c` (`is_glb`, `report_unsaved`,
  `make_absent`, `make_levels_folders`, `levels_asked`, `levels_file`, `swaps_patched`), taking
  the pool for the make; those `models.c` still calls declared in `models_batch.h`.
- `game/src/models.c` — `load_changed` goes. Update gathers its paths as now, then for each
  chunk of `voe_base_workers_threads` paths: stop asked, prepare, put each in order with
  "Loading models" done/total set before it; the dot and water as now; the line once. Watch
  adds every entry whose stamp it would reload, known at its stamp; reload adds its one path,
  forced. `voe_game_models_prepare` takes the pool. The pool is `voe_3d_models_workers(models)`.
  Header to match.
- `game/include/game/models.h` — `voe_game_models_prepare` gains `voe_base_workers *workers`
  after `scratch` (NULL in order). Header: the phases and chunks (0448 point 3), the line,
  the pool read from the store; the Constraints' scratch sentence becomes each file's own
  arenas, scratch holding only the call's lists.
- `game/src/run.c` — after the store is made, a pool of `voe_platform_cores() - 1` threads set on
  it; destroyed after the store on every path out. Header to match.
- `game/src/src.md` — entries for `models_batch.h` and `.c`; `models.c` and `run.c` entries.
- `game/tests/models_levels.c` — prepare calls pass NULL.
- `game/tests/models.c` — new check: the test's folder loaded into a store with a pool of 3 set
  gives the same entries (loaded, parts, levels) as a store without, and the progress ends
  with done equal to total; the stop check still reads nothing.
- `game/tests/tests.md` — the `models.c` entry names the check.

## Done when
`ctest --test-dir build/debug -R '^game/(models|models_levels|frame)$'` passes with the new check.
