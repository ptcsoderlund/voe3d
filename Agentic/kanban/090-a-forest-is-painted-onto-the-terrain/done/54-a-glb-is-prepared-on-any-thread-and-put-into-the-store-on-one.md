# 54 — A `.glb` is prepared on any thread and put into the store on one
folder: 3d
after: 52, 53
decisions: 0168, 0370, 0445, 0447, 0448

## Change
0448 point 3, baking and uploading: the store's `.glb` load splits into a prepare that touches
no device and no store, so files bake side by side, and a put that uploads on one thread.

- `3d/include/3d/model_prepare.h` — new. `typedef struct voe_3d_model_prepared
  voe_3d_model_prepared;` and `[[nodiscard]] const voe_3d_model_prepared *
  voe_3d_model_prepare(voe_base_arena *arena, const voe_assets_model *model, const uint8_t
  *glb, size_t glb_size, const uint8_t *levels, size_t levels_size, voe_base_error *error)`:
  level 0 baked, the file's own levels baked or the levels file read against `glb` and checked
  against the bake (the same stale rules as now), the CPU shape built, the sphere measured; all
  on `arena`, which must outlive the put. NULL with `error` set as the load fails today at those
  steps, one line at the cause (0445). `model` may still hold its pictures encoded (card 52)
  and is kept by pointer for the put. Header: the split, what is safe on any thread, that the
  put needs the pictures decoded by then.
- `3d/include/3d/models.h` — `[[nodiscard]] bool voe_3d_models_put(voe_3d_models *models,
  voe_render_device *device, const char *path, uint64_t stamp, const voe_3d_model_prepared
  *prepared, voe_base_error *error)`: the pictures and materials uploaded, the parts, twins and
  levels, the shape copied into the entry's arena; replaces and fails as `_load` does, REFUSED
  for no room. Asserts the model's pictures are decoded. `_load_levelled`'s comment: it is
  `voe_assets_model_read_glb`, the prepare and the put, unchanged in what it does.
- `3d/src/model_prepare.c` — new: the prepare, taking from `models.c`'s `load_glb` and
  `build_shape` and from `models_levels.c` the bakes and the levels read.
- `3d/src/models.c` — `load_glb` becomes the put; `voe_3d_models_load_levelled` reads, prepares
  on its scratch and puts. Header: A LOAD IS FOUR STEPS becomes the prepare and the put.
- `3d/src/models_levels.c`, `3d/src/models_store.h` — `voe_3d_models_levels_upload` uploads the
  levels the prepare baked or read and bakes or reads nothing; the prepared struct defined in
  `models_store.h` for the three files. Headers to match.
- `3d/include/3d/3d.md`, `3d/src/src.md` — entries for the new files; `models.c` and
  `models_levels.c` entries.
- `3d/tests/models_levels.c` — new checks: the sphere with its levels file and the own-levels
  file, read with `voe_assets_model_read_glb_encoded`, prepared on a second thread
  (`thrd_create`), their pictures decoded with `voe_assets_model_decode_picture`, and put on the
  test's thread: the entry matches what `_load_levelled` makes of the same bytes (parts,
  `level_count`, bounds, `radius`, shape's edge count); a model with one material more than
  `VOE_3D_MODEL_PARTS` (as `3d/tests/models.c` builds it) prepares to NULL, UNSUPPORTED.
- `3d/tests/tests.md` — the `models_levels.c` entry names the checks.

## Done when
`ctest --test-dir build/debug -R '^3d/(models|models_levels|draw_levels|pick|outline|import)$'`
passes with the new checks.
