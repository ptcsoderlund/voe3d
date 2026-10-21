# 52 — A `.glb` is read with its pictures left encoded
folder: assets
after: none
decisions: 0168, 0445, 0448

## Change
0448 point 3: parsing and decoding pictures become two steps, so pictures decode on a pool,
one task a picture, and a reader wanting only geometry (levels, Import) decodes none.

- `assets/include/assets/model.h` —
  - `typedef struct { const uint8_t *bytes; size_t size; } voe_assets_model_encoded;` and in
    `voe_assets_model` a field `encoded`, `image_count` long: each picture's bytes as the file
    holds them.
  - `voe_assets_model_read_glb_encoded(bytes, size, arena, model, error)` — as
    `voe_assets_model_read_glb` in everything but the pictures: `images` NULL, `encoded`
    pointing into the caller's `bytes`, which must outlive every decode of them. A picture
    that is neither PNG nor JPEG is still refused here, as now.
  - `[[nodiscard]] bool voe_assets_model_decode_picture(const voe_assets_model *model,
    uint32_t index, voe_base_arena *arena, voe_assets_image *out, voe_base_error *error)` —
    decodes `encoded[index]` into `arena` under the subject "the picture `name`", refusing as
    the PNG and JPEG readers do. Touches nothing shared: safe on any thread with an arena of
    its own.
  - `voe_assets_model_read_glb` — the same promise as now: the encoded read, every picture
    decoded into `arena`, `images` set and `encoded` NULL (the bytes are not kept).
  - Header: the two steps and why (0448); the BYTES paragraph says which read keeps them.
- `assets/src/model_gltf_materials.c` — `read_image` stops decoding: it checks the signature and
  records the span and name; the decode under its subject moves to
  `voe_assets_model_decode_picture`. Header: images are recorded, decoded on asking.
- `assets/src/model_glb.c` — `voe_assets_model_read_glb_encoded` is today's read; `_read_glb`
  is it plus a decode of each picture. Header: the two reads.
- `assets/src/model_gltf.c`, `assets/src/model_gltf_read.h`, `assets/src/model_gltf.h` — only
  as the above needs; comments that say pictures are decoded during the read.
- `assets/src/src.md`, `assets/include/assets/assets.md`, `assets/assets.md` — the
  `model.h`, `model_gltf_materials.c` and `model_glb.c` entries: pictures decoded or left
  encoded.
- `assets/tests/model.c` — new checks on the test's `.glb` with its picture: the encoded read
  gives the same counts, materials and nodes as `_read_glb`, `images` NULL and `encoded[0]`
  starting with the PNG signature; `voe_assets_model_decode_picture(…, 0, …)` gives the same
  width, height and pixels as `_read_glb`'s `images[0]`; with the picture's bytes broken after
  the signature, the encoded read succeeds, the decode refuses MALFORMED and the kept error
  (cleared first) names "the picture".
- `assets/tests/tests.md` — the `model.c` entry names the checks.

## Done when
`ctest --test-dir build/debug -R '^assets/(model|png|jpeg|model_material)$'` passes with the new
checks.
