# 04 — The pipeline cache kept in the settings folder
folder: app
after: 03
decisions: 0168, 0362, 0370

## Change
- `app/include/app/pipeline_cache.h` (new), its header saying why the settings folder and that no
  failure here fails a start (0370 point 6):
  - `const char *voe_app_pipeline_cache_path(voe_base_arena *arena, const char *name)`:
    `<settings>/voe3d/<name>`, NULL with no settings folder (`platform/folder.h`, `platform/path.h`).
  - `void voe_app_pipeline_cache_load(voe_render_device *device, const char *path,
    voe_base_arena *scratch)`: the file read and handed to `voe_render_device_cache_seed`; a
    missing file is silent, a refused one a stderr line saying it is built again.
  - `void voe_app_pipeline_cache_save(voe_render_device *device, const char *path,
    voe_base_arena *scratch)`: `voe_render_device_cache_bytes` written with
    `voe_platform_file_write` (atomic), the path's parent and its parent made as needed; a failure
    is one stderr line.
- `app/src/pipeline_cache.c` (new): the three.
- `app/app.md`, `app/src/src.md`, `app/tests/tests.md`: entries.
- `app/tests/pipeline_cache.c` (new): a headless device prepares and saves to a path under the
  test's working folder; a second loads it and prepares; a garbage file loads without harm; a
  missing path is nothing; the path call ends in `voe3d/<name>` when settings exist.

## Done when
`ctest --test-dir build/debug -R '^app/pipeline_cache$'` passes.
