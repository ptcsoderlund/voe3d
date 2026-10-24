# 02 — A picture is compressed to BC7 or BC5 by its kind
folder: assets
after: 01
decisions: 0168, 0477

## Change
0477 points 1–3 in `assets`: BC5 beside card 01's BC7, and a picture with its whole chain compressed,
whole or a run of blocks at a time.

- `assets/src/image_blocks.h`, new `assets/src/image_bc5.c` — `voe_assets_bc5_encode(rgba[64],
  block[16])` takes red then green, each a BC4 block. Both of BC4's modes are tried, endpoints from the
  channel's extent, and the lesser error is kept. `voe_assets_bc5_decode` gives red and green, blue 0
  and alpha 255, as a card gives them.
- `assets/include/assets/image.h`:
  - `voe_assets_image_format`: `VOE_ASSETS_IMAGE_RGBA8 = 0`, `_BC7`, `_BC5`.
  - `voe_assets_image` gains a last field, `format`. Zero is RGBA8, so every picture made today is
    unchanged. The struct comment says a BC picture's `pixels` are 16-byte blocks: each level's
    ⌈w/4⌉ × ⌈h/4⌉, row by row, the levels back to back.
  - `voe_assets_image_size` counts by format.
  - `voe_assets_image_format voe_assets_image_blocks_format(voe_assets_image_kind kind)`: BC5 for
    NORMAL, else BC7.
  - `uint32_t voe_assets_image_block_count(const voe_assets_image *image)`: the blocks over every
    level it holds.
  - `voe_assets_image voe_assets_image_blocks_new(voe_base_arena *arena, const voe_assets_image
    *source, voe_assets_image_kind kind)`: the compressed picture's shape and room. Same sides and
    levels, format by kind, blocks zeroed.
  - `void voe_assets_image_blocks_encode(const voe_assets_image *source, voe_assets_image_kind kind,
    voe_assets_image *target, uint32_t first, uint32_t count)`: blocks [first, first + count) in
    chain order, so a caller can spread the work (0477 point 3). An edge block's missing texels
    repeat the nearest edge texel.
  - `voe_assets_image voe_assets_image_compress(voe_base_arena *arena, const voe_assets_image *source,
    voe_assets_image_kind kind)`: the two above, every block.
  - Header points: why BC7 and BC5; a NORMAL picture keeps x and y, and whoever reads it makes z
    again. The source is RGBA8 with any number of levels. A BC source, or a range past the count,
    asserts.
- `assets/src/image_blocks.c` (new) — the calls above: a block's 64 bytes gathered, handed to the
  per-block encoder of the format.
- `assets/src/image_levels.c` — `voe_assets_image_size` by format.
- `assets/tests/image_blocks.c` (new):
  - sizes and block counts of 1×1, 5×3 and 256×128 chains;
  - a smooth 64×64 colour chain compressed, then decoded block by block with card 01's decoder,
    within 4 per channel;
  - a gentle bump's normal map (x and y change at most 4 per texel) as BC5: every decoded x and y
    within 3;
  - runs of 7 blocks give the same bytes as one call;
  - a 5×3 picture's edge blocks.
- `assets/include/assets/assets.md`, `assets/assets.md`, `assets/src/src.md`,
  `assets/tests/tests.md` — entries.

## Done when
The `assets/image_blocks` and `assets/image_levels` tests pass.
