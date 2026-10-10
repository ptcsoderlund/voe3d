# 21 — The model store holds materials
folder: 3d
after: 01, 03, 20
decisions: 0168, 0399

## Change
0399 points 4 and 5: a `.material` is an entry of the model store, live-editable. Card 21 wrote and
committed this work but could not build it: `voe_assets_material` named two types. Card 20 renamed the
file's values `voe_assets_material_file` (`assets/include/assets/material.h`).

- `3d/include/3d/models.h`, `3d/src/models_material.c`, `3d/tests/models_material.c` — every use of
  the file's values becomes `voe_assets_material_file`. The glTF `voe_assets_material` in
  `3d/src/model_upload.c` and `models.c` stays.
- Then build and finish what card 21 left. Read the headers of the files below; each must carry out:
  - `3d/include/3d/material_component.h`, `3d/src/material_component.c` — `voe_3d_material` has
    `float uv_repeat` (0 reads as 1) carried into the record's `uv_repeat`;
    `voe_3d_material_values` is shared by the upload and the store's in-frame write.
  - `3d/include/3d/models.h` — `voe_3d_material_map` / `voe_3d_material_maps`;
    `voe_3d_models_load_material` (maps decoded as `models.c` decodes a picture, uploaded mipped,
    colour as COLOUR, normal and roughness as DATA, roughness into the metal-roughness slot, 0399
    point 2; one part with no geometry and its BLENDED twin; replaces and fails as `_load` does;
    MALFORMED when a map will not decode); `voe_3d_models_material_set` (factors, repeat and shader
    taken, both records marked, maps ignored, nothing for a path that is no loaded material);
    `voe_3d_models_material_frame` (inside a frame, every marked record and twin written with
    `voe_render_shading_write`, marks cleared); `voe_3d_model_entry.material`. Header points: a
    material is an entry with no shape; why it shares the store; a map shared by two materials is
    uploaded twice (0399 point 5).
  - `3d/src/models_store.h` — the entry's write mark; its held texture ids freed with it.
  - `3d/src/models_material.c` — the load, set and frame.
  - `3d/3d.md`, `3d/include/3d/3d.md`, `3d/src/src.md`, `3d/tests/tests.md` — lines for the above.
- `3d/tests/models_material.c`: a load with three 2×2 PNG maps is loaded, `material`, one part, a
  live shading; `_material_set` then `_material_frame` inside a frame passes; a map that is not a
  picture fails MALFORMED and the entry is failed; `voe_3d_models_rename` finds it at the new path.

## Done when
`ctest --test-dir build/debug -R '^3d/(models_material|models|material)$'` passes.
