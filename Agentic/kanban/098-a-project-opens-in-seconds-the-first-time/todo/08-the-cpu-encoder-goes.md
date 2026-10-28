# 08 — The CPU encoder goes
folder: assets
after: 03, 07
decisions: 0168, 0477, 0484, 0486

## Change
0484 point 2: one encoder, render's. Every caller has moved (cards 03, 04, 07); this card
touches no other folder.

- Delete `assets/src/image_blocks.h`, `assets/src/image_bc7.c`, `assets/src/image_bc5.c`.
- `assets/src/image_blocks.c` — keep `voe_assets_image_blocks_format`,
  `voe_assets_image_block_count` and `voe_assets_image_blocks_new`; remove
  `voe_assets_image_blocks_encode` and `voe_assets_image_compress`.
- `assets/include/assets/image.h` — those two declarations go; GPU-COMPRESSED PICTURES says the
  blocks are made by render's encoder on the GPU (0484, 0486) and that `_blocks_new` is the room
  for them; the file's opening line no longer says it compresses.
- `assets/tests/image_bc7.c` — deleted: it tests the encoder alone.
- `assets/tests/image_blocks.c` — keeps its checks of sides, block counts and formats; the
  encode-and-decode checks go, with its include of `../src/image_blocks.h`.
- `assets/tests/image_levels.c` — the cooked round trip (near line 177) makes its blocks with
  `voe_assets_image_blocks_new` and known bytes instead of `voe_assets_image_compress`, and checks
  they come back.
- `assets/src/src.md` — the three deleted files' entries go, `image_blocks.c`'s says what is
  left; `assets/tests/tests.md` — `image_bc7.c`'s entry goes, `image_blocks.c`'s follows.

## Done when
The `assets/image_blocks` and `assets/image_levels` tests pass, and
`grep -rn "voe_assets_image_compress\|voe_assets_image_blocks_encode\|voe_assets_bc7_\|voe_assets_bc5_" --include=*.c --include=*.h assets 3d game editor app`
prints nothing.
