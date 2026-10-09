# 04 — The model store holds materials
folder: 3d
after: 01, 03
decisions: 0168, 0399

## Change
0399 points 4 and 5: a `.material` is an entry of the model store, live-editable.

- `3d/include/3d/material_component.h`, `3d/src/material_component.c` — `voe_3d_material` gains
  `float uv_repeat` (0 reads as 1), carried into the record's `uv_repeat` (card 02). Split out of
  `voe_3d_material_upload` a `voe_render_shading_values voe_3d_material_values(const voe_3d_material
  *material)` that the upload and the store's in-frame write both use.
- `3d/include/3d/models.h` — new, each with its comment:
  - `voe_3d_material_map { const uint8_t *bytes; size_t size; }` (size 0: no map) and
    `voe_3d_material_maps { colour, normal, roughness; }`.
  - `[[nodiscard]] bool voe_3d_models_load_material(models, device, path, stamp, const
    voe_assets_material *material, voe_3d_material_maps maps, voe_base_error *error)` — the maps
    decoded as `models.c` decodes a picture and uploaded mipped, colour as COLOUR, normal and roughness
    as DATA (roughness into the metal-roughness slot, 0399 point 2); factors, repeat and unlit from
    the values; one part with no geometry and its BLENDED twin. Replaces and fails as `_load` does;
    MALFORMED when a map will not decode.
  - `void voe_3d_models_material_set(models, path, const voe_assets_material *material)` — the
    entry's factors, repeat and shader taken, its two records marked to write; maps ignored (a new map
    is a load). Nothing for a path that is no loaded material.
  - `void voe_3d_models_material_frame(models, device)` — inside a frame before any pass: every
    marked entry's record and twin written with `voe_render_shading_write` (card 03), marks cleared.
  - `voe_3d_model_entry` gains `bool material`.
  Header points: a material is an entry with no shape; why it shares the store; a map shared by two
  materials is uploaded twice (0399 point 5).
- `3d/src/models_store.h` — the entry's write mark and its held texture ids freed with it.
- New `3d/src/models_material.c` — the load, set and frame, beside `models_landscape.c`; read
  `3d/src/models.c`'s header and `models_store.h` for the helpers lent to it, and add any you need
  there as `models_landscape.c` does.

Update `3d/3d.md`'s `models.h` and `material_component.h` lines and `3d/src/src.md`.

New test `3d/tests/models_material.c` (follow `3d/tests/models.c` for the headless device; encode a
2×2 PNG with `assets/image.h`): a load with three maps is loaded, `material`, one part, a live
shading; `_material_set` then `_material_frame` inside a frame passes; a map that is not a picture
fails MALFORMED and the entry is failed; `voe_3d_models_rename` finds it at the new path.

## Done when
`ctest --test-dir build/debug -R '^3d/(models_material|models|material)$'` passes.
