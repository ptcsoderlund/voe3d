# 11 — A burst's first error reaches its caller
folder: game
after: 10
decisions: 0168, 0487, 0490, 0445, 0160

## Change
Replaces blocked card 05, whose bursts are already in the tree (commit "098 card 05:
blocked"): the model batch's phases and a material's maps run on burst workers. What is
missing is 0490 points 2 and 3: a failure on a worker leaves `voe_base_report_error_first()` on
the calling thread NULL, so `game/models` `broken_load` and `game/models_levels` fail and the
editor's notice loses its reason. Nothing public changes.

- `game/src/models_batch.c`:
  - every burst task (reading, parsing, making LODs, decoding a picture, baking) clears its
    thread's first error with `voe_base_report_error_clear` before its work, and when it fails
    copies `voe_base_report_error_first()` (when not NULL) into its own record: the file's
    record for a file's phase, the picture's for a picture, in that record's arena;
  - the put, on the calling thread, before it counts a failed file: `voe_base_report_error_keep`
    of that file's carried text, else of its first failed picture's in picture order. Puts run
    in file order, so the first failing file in the batch's order is the one kept.
- `game/src/models_materials.c`:
  - each map task clears before its work and copies its first error into its map's record on
    failure, in the map's arena;
  - the calling thread, where it folds the maps' failures into the material's, keeps the first
    failing map's text in map order.
- `game/src/models_batch.h` — THE PUT: a failure's text met on a worker is carried in its
  record and kept on the calling thread in order (0490), so a reader of the first error sees
  what one thread would have.
- `game/src/models_materials.c` header — THE PHASES: the same, at the fold, in a phrase.
- `game/tests/models_batch.c` — add `a_batchs_first_error_is_its_first_failing_file`: two
  `.glb`s (`glb_build.h`) that fail to parse, loaded by one update given a progress, so one
  batch on workers; after a clear, the first error read once it has failed names the file
  the batch holds first. Its entry in `game/tests/tests.md` names it.
- `game/src/src.md` — the `models_batch.h` and `models_materials.c` entries name the carried
  error, if they list what the header says.

## Done when
The `game/models` (`broken_load`), `game/models_levels`, `game/models_cooked` and
`game/models_batch` tests pass, `a_batchs_first_error_is_its_first_failing_file` among them.
