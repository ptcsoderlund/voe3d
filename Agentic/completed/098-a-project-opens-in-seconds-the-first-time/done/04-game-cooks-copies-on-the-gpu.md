# 04 — game cooks copies on the GPU
folder: game
after: 01, 02, 03
decisions: 0168, 0470, 0477, 0484, 0486

## Change
Every copy `game` writes is compressed by render (0486 points 1, 4–6). The queue
(`game/src/compressing.c`) is card 07's; leave it. Public surface: `editor` calls prepare and
cook, and card 09 moves it; touch no other folder.

- `game/src/models_cooked.h`, `game/src/models_cooked.c`:
  - `voe_game_models_compress(voe_render_device *device, const voe_assets_image *chain,
    voe_assets_image_kind kind, voe_base_arena *arena)` — new: `voe_assets_image_blocks_new`,
    then `voe_render_compress_start` and `_finish` into it; returns the blocks. A start that
    fails is one line and the RGBA8 chain returned as it is (a copy may hold RGBA8, 0477 point 8).
  - `voe_game_models_cooked_save(folder, path, prepared, key, device, arena)` — each image mipped
    by `voe_3d_model_cooked_kind`, compressed by the above, then `voe_3d_model_cooked_write` with
    those pictures.
  - `voe_game_models_picture_save(folder, path, image, kind, stamp, device, arena)` — compresses
    by the above.
  - The header's COPIES ARE COMPRESSED says on the GPU, on the caller's device and thread.
- `game/src/models_batch.h`, `game/src/models_batch.c` — `voe_game_models_batch_new` takes the
  device after `cooking`; cooking passes it to the save. The header's phases paragraph: cooking
  compresses on that device, on the calling thread.
- `game/src/models.c` — every `voe_game_models_batch_new` passes its call's device.
- `game/include/game/models.h`:
  - `voe_game_models_prepare(folder, path, device, scratch, remake, made, error)`;
  - `voe_game_models_cook(folder, paths, path_count, table, device, scratch, progress)`;
  - their comments and the COOKED COPIES paragraph: compressed on `device`, from the owner's
    thread between frames or the one other thread (0370).
- `game/src/models_cooked.c` (cook's loop), `game/src/models_materials.c` (its `picture_save`
  call) — pass the device.
- `game/tests/models_cooked.c`, `game/tests/models_levels.c` — prepare and cook take the
  headless device each already makes (move its make before the first prepare where it is not).
- `game/src/src.md` — `models_cooked.c`'s entry: compressed on the GPU, a cook on the caller's device.

## Done when
The `game/models_cooked`, `game/models_levels` and `game/models` tests pass, and
`grep -n "voe_assets_image_compress" game/src/*.c` names only `compressing.c`.
