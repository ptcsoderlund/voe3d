# 03 — A cooked picture keeps its blocks
folder: assets
after: 02
decisions: 0168, 0470, 0477

## Change
0477 point 8 for the cooked picture: a copy in `Cache/` may hold BC blocks, and the copies made
before this card are stale.

- `assets/include/assets/image.h` — the cooked form's head gains the format.
  `VOE_ASSETS_IMAGE_COOKED_HEAD` may grow if there is no room; it stays a multiple of 16, and the
  callers outside this folder use the macro, never the number.
  - `voe_assets_image_cooked_read` hands back the picture with its format, and checks the size by
    the format. It is refused when the format is unknown.
  - The comment says a copy may hold blocks, and that the version rose for them.
- `assets/src/image_levels.c` — the write and the read; `COOKED_VERSION` rises.
- `assets/tests/image_levels.c`:
  - a BC7 and a BC5 picture (from `voe_assets_image_compress`) round-trip through the cooked form,
    pixels pointing into the bytes;
  - a head with an unknown format is refused;
  - a head of the old version is refused by the read and by `voe_assets_image_cooked_key`.
- `assets/tests/tests.md` — the `image_levels.c` entry.

## Done when
The `assets/image_levels` test passes.
