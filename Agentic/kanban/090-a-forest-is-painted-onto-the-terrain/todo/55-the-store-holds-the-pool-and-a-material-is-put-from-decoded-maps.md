# 55 — The store holds the pool, and a material is put from decoded maps
folder: 3d
after: 50, 54
decisions: 0168, 0399, 0448

## Change
0448 points 2 and 3: the game's loader reads its pool from the store, and decodes a material's
maps on it before the one-thread upload.

- `3d/include/3d/models.h` —
  - `void voe_3d_models_workers_set(voe_3d_models *models, voe_base_workers *workers)` and
    `voe_base_workers *voe_3d_models_workers(const voe_3d_models *models)`: the program's pool,
    NULL until set; the store never runs it itself, nor destroys it, and `_clear` keeps it.
    The header's paragraph: the store carries the pool its loaders run on (0448 point 2).
  - `typedef struct { voe_assets_image colour, normal, orm, emission; } voe_3d_material_pictures;`
    a map of width 0 being none, and `[[nodiscard]] bool voe_3d_models_put_material(models,
    device, path, stamp, const voe_assets_material_file *material, voe_3d_material_pictures
    pictures, voe_base_error *error)`: `_load_material` with the maps already decoded, the same
    uploads, records, twin, replace and failures but no decode. `_load_material`'s comment: it
    decodes each map (below) and puts.
- `3d/include/3d/model_prepare.h` — `voe_3d_model_picture_is` and `voe_3d_model_picture_decode`
  declared here, public, moved from `3d/src/model_picture.h`: a `.png`, `.jpg` or `.jpeg`
  path's bytes decoded by its extension; safe on any thread with an arena of its own.
- `3d/src/model_picture.h`, `3d/src/model_picture.c` — the two declarations go from the internal
  header, which includes the public one; the definitions stay. Header to match.
- `3d/src/models_material.c` — `voe_3d_models_put_material` holds today's upload;
  `voe_3d_models_load_material` decodes each map under its subject as now, then puts.
- `3d/src/models.c`, `3d/src/models_store.h` — the store's `workers` field and the two calls.
- `3d/src/src.md`, `3d/include/3d/3d.md` — the entries these change.
- `3d/tests/models_material.c` — a new check: the test's four maps decoded with
  `voe_3d_model_picture_decode` and put with `voe_3d_models_put_material` give an entry like
  `_load_material`'s of the same bytes (one part, no shape, its record's maps set).
- `3d/tests/models.c` — a new check: `voe_3d_models_workers` is NULL on a new store, the pool
  set is the pool read back, and it survives `_clear`.
- `3d/tests/tests.md` — both entries name the checks.

## Done when
`ctest --test-dir build/debug -R '^3d/(models|models_material|draw_material|draw_particles)$'`
passes with the new checks.
