# 05 — The batch and a material's maps run in bursts
folder: game
after: 02, 04
decisions: 0168, 0485, 0487, 0457, 0386

## Change
0487 points 3 and 4. Nothing public changes. Every task touches only its own file's or picture's
record and arena, pushes its own report subject (its file's path) on its worker, and keeps its
failure in its record as today; the progress and the times are written by the calling thread
alone, around each burst.

- `game/src/models_batch.c`:
  - reading, parsing, making LODs, decoding and baking each run as one `voe_platform_burst` over
    the phase's tasks, workers `voe_platform_burst_workers(progress != NULL)`, finished before the
    next phase; each phase's seconds are its burst's wall time;
  - a decoding task of a file that will be cooked also mips its picture by
    `voe_3d_model_cooked_kind`, so the cooking phase only compresses and writes;
  - cooking and the put stay on the calling thread.
- `game/src/models_cooked.h`, `game/src/models_cooked.c`:
  - `voe_game_models_cooked_save` takes pictures already mipped (the chains, one a model image)
    and compresses them;
  - `voe_game_models_folders_make` counts a folder another thread made between its look and its
    make as made;
  - `voe_game_models_cook` makes its `.glb`s through batches of up to the room below, as
    prepare's STALE row, not one prepare each.
- `game/src/models.c` — an update given a progress, and the cook, hold up to
  `min(4, voe_platform_burst_workers(true))` files a batch; every other call one, as now.
- `game/src/models_materials.c` — a material's maps that need reading are read, decoded and
  mipped as one burst, a task a map, then compressed, written and uploaded in order on the caller.
- `game/src/models_batch.h` — THE PHASES: each phase a burst on the workers 0487 gives, files and
  pictures at once, cooking and put on the caller; Constraints: a splash batch's room and its
  memory. `game/include/game/models.h` — PHASES and the materials paragraph say the same in a
  phrase each.
- `game/src/src.md` — the entries of `models_batch.h`, `models_batch.c`, `models.c`,
  `models_materials.c`.

## Done when
The `game/models`, `game/models_levels` and `game/models_cooked` tests pass, and
`game/tests/models_batch.c` (new; `game/tests/models_cooked.c` is near 800 lines, so the batch's
checks start their own file; entry in `game/tests/tests.md`) passes
`a_splash_cook_matches_one_by_one`: three `.glb`s of two pictures each (`glb_build.h`), cooked
once with a progress and once by `voe_game_models_prepare` one at a time with none, on the
headless device, give copies equal byte for byte.

## Blocked
A failure now happens on a burst's worker, so `voe_base_report_error_first()` (thread_local) on the calling thread stays NULL: `game/models` `broken_load` (models.c:502) and `game/models_levels` (models_levels.c:373) fail, and the editor's notice would lose its reason. Everything else is done: `game/models_cooked` and the new `game/models_batch` pass. To unblock, `base/report.h` needs a call that keeps a given text as the calling thread's first error without printing it (e.g. `voe_base_report_error_keep`). Then each task clears its own first error, copies it into its record when it fails, and the caller keeps the first failing record's text in order at the put and fold.
