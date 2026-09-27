# 07 — A store loads models by path
folder: 3d
decisions: 0168, 0277, 0035

## Change
Needs cards 05 and 06. The store of 0277 point 2, first loads only; a second load of a path and
emptying are card 08. Read the headers of `3d/include/3d/import.h`, `3d/src/import.c` (its tree
walk: explicit stack, `VOE_3D_IMPORT_MAX_DEPTH`, rule 14) and `3d/src/model_upload.h`.

- New `3d/include/3d/models.h`:
  - Constants `VOE_3D_MODELS` 128, `VOE_3D_MODEL_PARTS` 16, and the device room
    `VOE_3D_MODELS_VERTICES` (1u << 21), `_INDICES` (3u << 21), `_GEOMETRIES` 512,
    `_SHADINGS` 512, which a program adds to its capacities.
  - `voe_3d_model_part { voe_render_geometry geometry; voe_3d_material material; }`.
  - `voe_3d_model_entry`: `const char *path`, `uint64_t stamp`, `bool loaded`, `parts`,
    `part_count`, `voe_3d_shape_geometry shape` (the whole model, model space, edges welded).
  - `voe_3d_models *voe_3d_models_new(void);` and `void voe_3d_models_destroy(voe_3d_models *);`
    (CPU only; rule 11's long-lived exception: the store and each entry own their memory).
  - `[[nodiscard]] bool voe_3d_models_load(voe_3d_models *, voe_render_device *, const char
    *path, uint64_t stamp, const uint8_t *bytes, size_t size, voe_base_error *error);` — parse
    with `voe_assets_model_read_glb`, bake each node's world matrix into positions and its normal
    matrix into normals, merge primitives by material into parts, upload through
    `model_upload.h` and `voe_render_geometry_create`, build `shape` with
    `voe_3d_shape_geometry_build`. On any failure the path is kept as a failed entry with that
    stamp and false comes back with the category (`MALFORMED`, `UNSUPPORTED` for more than 16
    parts, `REFUSED` for no room). A path already held asserts (card 08 lifts that).
  - `void voe_3d_models_fail(voe_3d_models *, const char *path, uint64_t stamp);` — keeps a
    failed entry for a file that could not be read at all.
  - `const voe_3d_model_entry *voe_3d_models_find(const voe_3d_models *, const char *path);`
    NULL when never asked; `uint32_t voe_3d_models_count(...)` and
    `const voe_3d_model_entry *voe_3d_models_at(const voe_3d_models *, uint32_t index);`.
  - Header points: why a store and not a component (one upload per file however many wear it);
    why nodes are baked and parts merged; why a failed entry is kept (not re-read each frame).
- New `3d/src/models.c`, and an internal `3d/src/model_bake.h`/`.c` for the walk and merge if
  `models.c` would pass ~400 lines.
- New `3d/tests/models.c`, headless, skipping without a card: `TWO_PRIMITIVES_GLB` from
  `3d/tests/model_data.inc` loads, `find` gives a loaded entry whose part count is its material
  count and whose `shape` positions carry the node transform `3d/tests/import.c` checks; bytes
  that are not a `.glb` are false, `MALFORMED`, with a failed entry; `fail` then `find` gives a
  failed entry; an unknown path is NULL. List it in `3d/tests/tests.md`.
- `3d/3d.md` and `3d/src/src.md`: entries for the new files.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_3d $(ninja -C build/debug -t
targets all | grep -oE "^voe_test_3d_[A-Za-z0-9_]+") && ctest --test-dir build/debug -R "^3d/"`
exits 0, `voe_test_3d_models` among them.
