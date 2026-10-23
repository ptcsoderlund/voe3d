# 05 — A prepared model is written and read as a cooked copy
folder: 3d
after: 04
decisions: 0168, 0470, 0426

## Change
0470 point 1: the bytes of `Cache/models/<path>.model`. This card is the format alone, with no file access
(`game` reads and writes the files).

- New public `3d/include/3d/model_cooked.h`:
  - `typedef struct { uint64_t glb, parts; } voe_3d_model_cooked_key;`
  - `#define VOE_3D_MODEL_COOKED_HEAD`: the head's size, ≤ 64.
  - `voe_assets_bytes voe_3d_model_cooked_write(voe_base_arena *arena, const voe_3d_model_prepared *prepared, voe_3d_model_cooked_key key)`.
    The prepared must have its pictures decoded. The write gives each picture its full chain with
    `voe_assets_image_mips`, of the kind its first use in the model's materials says: base colour and
    emissive are colour, normal is normal, metal-roughness and occlusion are data, and an unused one is
    colour. Each picture is written with `voe_assets_image_cooked_write`, key 0.
  - `bool voe_3d_model_cooked_key_of(const uint8_t *head, size_t size, voe_3d_model_cooked_key *key)`.
    False when it is too short or the magic or version differ.
  - `const voe_3d_model_prepared *voe_3d_model_cooked_read(voe_base_arena *arena, const uint8_t *bytes, size_t size, voe_3d_model_cooked_key *key)`.
    The prepared and its `voe_assets_model` are pushed on the arena. Their arrays point into `bytes`,
    which must outlive the put and be 16-aligned (assert). NULL with no line on a wrong magic, version
    or size: a broken copy is made again. The caller compares `*key`.
  - The header's paragraphs cover:
    - the layout (head, then sections aligned to 16, native-endian, this machine's cache only);
    - the version rising with any change to the bake, the levels or the layout;
    - which model fields a read fills: those the put reads, the rest empty;
    - why mips are by use.
- New `3d/src/model_cooked.c`. What it serializes is everything in `struct voe_3d_model_prepared`
  (`3d/src/models_store.h`): the bake, the shape, the sphere, the own levels, the levels read, and the
  flags. It also writes the model's materials and its pictures. Read these headers for the shapes:
  - `3d/src/model_bake.h`
  - `3d/include/3d/shape_geometry.h`
  - `3d/include/3d/model_levels.h`
  - `assets/include/assets/model.h`

  To learn which model fields the put reads, read `voe_3d_models_put` in `3d/src/models.c` and
  `3d/src/model_upload.c`. If one file would pass about 600 lines, split it into `model_cooked_write.c`
  and `model_cooked_read.c`.
- `3d/src/src.md` and `3d/include/3d/3d.md`: the new entries.
- New test `3d/tests/model_cooked.c`, on the sphere `.glb` of `3d/tests/model_glb.inc`. No fixture has a
  picture, so after the read the test gives the model one 8×8 picture and points its material's base
  colour at it, before the prepare. Its cases:
  - `round_trip_keeps_the_bake`: vertex and index bytes of every part and level, the sphere, the
    flags, the edges count.
  - `round_trip_mips_every_picture`: levels equal `voe_assets_image_level_count`.
  - `key_comes_back`.
  - `a_changed_version_is_refused`.

  And its entry in `3d/tests/tests.md`.

## Done when
`ctest --test-dir build/debug -R '^3d/model_cooked$'` passes.
