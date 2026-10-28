# 03 — A model's copy takes its pictures compressed
folder: 3d
after: 01
decisions: 0168, 0470, 0484, 0486

## Change
The write stops compressing (0484 point 2); the caller hands it the blocks. Public surface: `game`
calls the write, and card 04 moves it; this card touches no other folder.

- `3d/include/3d/model_cooked.h`:
  - `voe_3d_model_cooked_write(voe_base_arena *arena, const voe_3d_model_prepared *prepared,
    const voe_assets_image *pictures, voe_3d_model_cooked_key key)` — `pictures` holds one
    picture a model image, in the model's order, each BC7 or BC5 as its kind's format says
    (`voe_assets_image_blocks_format`), that image's sides and full chain, all asserted; written
    as given with key 0.
  - `voe_assets_image_kind voe_3d_model_cooked_kind(const voe_assets_model *model,
    uint32_t image)` — new: the kind the image's first use says, what MIPS ARE BY USE describes.
  - Header points: MIPS ARE BY USE and PICTURES ARE COOKED AS BLOCKS now say the caller mips by
    this kind and compresses (on the GPU, render), the write only lays them out; CONSTRAINTS no
    longer push a chain or blocks.
- `3d/src/model_cooked.c` — the write takes `pictures`; its own first-use function becomes
  `voe_3d_model_cooked_kind`; no `voe_assets_image_mips` or `_compress` left. The layout and
  VERSION are unchanged.
- `3d/tests/model_cooked.c` — the writes (lines near 99, 206, 226) pass pictures made by
  `voe_assets_image_blocks_new` for each image's kind with known bytes; the round trip checks
  those bytes come back; a new check that `voe_3d_model_cooked_kind` says COLOUR, NORMAL and DATA
  for a base-colour, normal and metal-roughness picture and COLOUR for one no material names.
- `3d/tests/models_material.c` — the two `voe_assets_image_compress` calls (near lines 226, 228)
  become `voe_assets_image_blocks_new` with known bytes, unless the test reads drawn colours back,
  then `voe_render_compress_start` and `_finish` on its device.
- `3d/tests/tests.md` — the `model_cooked.c` entry.

## Done when
The `3d/model_cooked` and `3d/models_material` tests pass, and
`grep -n "voe_assets_image_compress\|voe_assets_image_blocks_encode" -r 3d` prints nothing.
