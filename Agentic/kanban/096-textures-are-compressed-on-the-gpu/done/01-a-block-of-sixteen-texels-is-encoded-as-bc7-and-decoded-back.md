# 01 — A block of sixteen texels is encoded as BC7 and decoded back
folder: assets
after: none
decisions: 0168, 0477

## Change
0477 points 1 and 2, the BC7 half. It stays internal to `assets` until card 02 makes it public.

- `assets/src/image_blocks.h` (new, internal) — one 4×4 block, both ways:
  `void voe_assets_bc7_encode(const uint8_t rgba[64], uint8_t block[16])` and
  `void voe_assets_bc7_decode(const uint8_t block[16], uint8_t rgba[64])`. Texels are RGBA8, row by
  row. The header says: mode 6 only and why (0477 point 2); the decode is what a card decodes; colour
  is encoded as the bytes are stored, so sRGB stays sRGB; card 02 adds BC5 here.
- `assets/src/image_bc7.c` (new) — mode 6 as the BC7 format defines it:
  - the mode bits;
  - two 7-bit RGBA endpoints, each with its own p-bit;
  - 4-bit indices, the anchor's top bit implied 0 (endpoints swapped and indices flipped to keep it so);
  - each texel `((64 − w)·e0 + w·e1 + 32) >> 6` with BC7's 4-bit weights.

  The encode:
  - endpoints from the block's extent along its principal axis in RGBA (power iteration on the
    covariance; a block of one colour is exact up to the p-bit);
  - each endpoint's p-bit tried both ways;
  - indices by nearest;
  - one least-squares refit of the endpoints from those indices;
  - the candidate with the lesser squared error kept.

  No allocation.
- `assets/tests/image_bc7.c` (new):
  - a hand-built block decoded to its known texels;
  - encode then decode: one colour within 1 per channel; a 4×4 of two colours within 3; a smooth
    gradient (at most 8 per texel per channel) within 4;
  - alpha 0 and 255 kept exact beside opaque texels;
  - a block encoded twice gives the same bytes.
- `assets/src/src.md`, `assets/tests/tests.md` — entries for the new files.

## Done when
The `assets/image_bc7` test passes.
