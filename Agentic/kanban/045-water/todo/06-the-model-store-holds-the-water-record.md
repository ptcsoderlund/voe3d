# 06 — The model store holds the water record
folder: 3d
after: 05
decisions: 0168, 0305

## Change
0305 point 6. Read `3d/include/3d/models.h`, `3d/src/models.c`,
`3d/src/model_picture.h` (the quad), `render/include/render/device.h`
(`voe_render_shading_create`, the `water` flag) and `3d/tests/models.c`.

- `models.h`:
  - `[[nodiscard]] bool voe_3d_models_load_water(voe_3d_models *models,
    voe_render_device *device, voe_base_error *error)`: makes the quad if
    not yet made and one shading record, BLENDED, `water` set, white,
    metallic 0, roughness 0.05; true at once when already there; false,
    `error` set, when the device has no room;
  - `const voe_3d_model_part *voe_3d_models_water(const voe_3d_models
    *models)`: the quad and that record, NULL until loaded;
  - `VOE_3D_MODELS_SHADINGS` grows by one;
  - header points: the water record is held apart as the dot is, freed by
    `_clear`, made in code and never re-read.
- `models.c`: the record kept beside the dot, freed at clear.
- `3d/tests/models.c`: `_water` NULL before; loaded twice is one record;
  the part is on the quad pictures use; `_clear` makes it NULL again.
- `3d/src/src.md` entry for models.c if its line changes.

## Done when
The test `3d/models` passes after the folder's build.
