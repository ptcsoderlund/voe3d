# 07 — The queue compresses on the GPU without waiting
folder: game
after: 06
decisions: 0168, 0477, 0484, 0487, 0489, 0386

## Change
0489. Public surface: `editor/src/models.c` calls the queue, and card 09 moves it; touch no
other folder.

- `game/include/game/compressing.h` — the API becomes:
  - `voe_game_compressing_add(queue, folder, path, kind, stamp)` — no chain; a waiting job of the
    same path and kind is replaced and starts over;
  - `const char *voe_game_compressing_step(voe_game_compressing *queue,
    voe_render_device *device, voe_base_arena *scratch)` — moves the oldest job one stage on and
    never waits; its path when its copy is written, else NULL;
  - `voe_game_compressing_clear(queue, device)` and `voe_game_compressing_destroy(queue, device)`
    — a running burst finished, a GPU job dropped (`voe_render_compress_finish` with NULL);
  - `_waiting` and `VOE_GAME_COMPRESSING` stay.
  - The header: the stages and who runs each (0489 point 2), one job on the GPU, that a step's
    longest part is a start's copy into its buffer, a job's memory its own arena; the ONE THREAD
    paragraph and the 2 ms pacing go.
- `game/src/compressing.c` — the jobs as above: a one-worker `voe_platform_burst` reads the map,
  decodes and fits it as `models_materials.c` does and mips it by kind; then start, ready, finish
  into `voe_assets_image_blocks_new`; then a one-worker burst writes it by
  `voe_game_models_picture_write`. A map that will not read or decode is one line under its path
  and the job dropped; a failed start leaves the job to try at the next step.
- `game/src/models_materials.c` — given a queue, a map with no fresh copy is not read: it is
  added, and the material loads as if that map were empty. With no queue, as now.
- `game/include/game/models.h` — MATERIALS, `voe_game_models_material_load` and
  `voe_game_models_materials` say so.
- `game/src/src.md` — the `compressing.c` and `models_materials.c` entries.
- `game/tests/compressing.c` — rewritten on the headless device against PNGs it writes
  (`voe_assets_png_encode`) into its folder, stepping with a short sleep up to a bound:
  `a_job_ends_in_its_copy` (the path returned once, the copy reads BC7 at the picture's sides
  with its whole chain), `a_second_add_replaces`, `clear_drops_every_job`,
  `a_missing_map_writes_nothing`, `a_normal_map_is_bc5`.
- `game/tests/models_cooked.c` — `a_queued_map_draws_uncompressed` becomes
  `a_queued_map_loads_empty` (the call with a queue: that map's texture absent, one job, no
  copy); `the_stepped_queue_writes_the_copy` steps with the device until the path comes back.
  The two entries at the file's top and in `game/tests/tests.md` follow.

## Done when
The `game/compressing`, `game/models_cooked` and `game/models` tests pass, and
`grep -rn "voe_assets_image_compress\|voe_assets_image_blocks_encode" game` prints nothing.
