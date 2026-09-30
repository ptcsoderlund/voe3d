# 04 — The model store holds pictures and the soft dot
folder: 3d
after: 03
decisions: 0168, 0298

## Change
0298 point 5. Read `3d/include/3d/models.h`, `3d/src/models.c`,
`3d/src/model_upload.h`, `3d/include/3d/material_component.h` (for
`voe_3d_material_upload`) and `assets/include/assets/image.h` (PNG and JPEG
decode).

- `3d/include/3d/models.h`:
  - `voe_3d_model_entry` gains `bool picture`. A picture entry has two
    parts, both on the store's one quad and both BLENDED with the picture as
    their base colour texture: part 0 lit, part 1 unlit (glow). Its shape is
    empty.
  - `voe_3d_models_load` takes a path ending `.png`, `.jpg` or `.jpeg`
    (any case) as a picture; every other path as a `.glb`, as now. A picture
    that will not decode is MALFORMED or UNSUPPORTED as the decoder says.
  - `[[nodiscard]] bool voe_3d_models_load_dot(voe_3d_models *models,
    voe_render_device *device, voe_base_error *error)`: uploads the built-in
    soft dot, once, as a picture entry held apart from the entries: not
    counted by `_count`/`_at`, found by `_find` at the empty path, freed by
    `_clear`. True at once when it is already there.
  - `VOE_3D_MODELS_GEOMETRIES` grows by one for the quad.
  - The header's new points: pictures share the store so the loaders and
    the watch serve them unchanged; the two parts and why (0298 point 5);
    the dot and why it is apart (never re-read from a file).
- `3d/src/model_picture.h` and `3d/src/model_picture.c`, new, internal:
  - the quad: four vertices in the XY plane, one metre a side, centred, UVs
    0..1, facing +Z, each corner's normal leaning outward as a sphere's
    (normalised x, y, 1); two triangles; uploaded once by the store on its
    first picture or dot and freed at clear;
  - decode by extension into the caller's scratch arena, then one COLOUR
    texture and the two materials;
  - the dot's pixels: 32 × 32, white, alpha falling from 1 at the centre to 0
    at the edge, made in code.
- `3d/src/models.c`: dispatches on the extension, keeps the quad and the dot,
  frees both at clear; a picture loaded again replaces its texture and parts
  as a model's does.
- `3d/tests/models.c`: a picture case (a small PNG built in the test through
  `assets/include/assets/image.h`'s encoder, or bytes inlined) loads as a
  picture entry with two BLENDED parts, the second unlit; garbage `.png`
  bytes fail and keep a failed entry; `voe_3d_models_load_dot` twice is one
  entry found at "" and not counted; clear empties both.
- `3d/include/3d/3d.md`, `3d/src/src.md`, `3d/tests/tests.md`: the entries.

## Done when
`ctest --test-dir build/debug -R '^3d/models$'` passes, after the folder's
build.
