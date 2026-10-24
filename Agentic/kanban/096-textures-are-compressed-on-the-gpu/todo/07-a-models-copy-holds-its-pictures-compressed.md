# 07 — A model's copy holds its pictures compressed
folder: 3d
after: 06
decisions: 0168, 0470, 0477

## Change
0477 points 1, 3 and 8 for `Cache/models/<path>.model`.

- `3d/src/model_cooked.c` — the write compresses each mipped picture by the kind it was mipped as
  (`voe_assets_image_compress`) before `voe_assets_image_cooked_write`. `VERSION` rises. The read is
  unchanged: the cooked picture brings its format.
- `3d/include/3d/model_cooked.h` — the header says the pictures are cooked as blocks by their use
  (0477). The write's arena now holds the mipped picture, its blocks and the copy.
- `3d/tests/model_cooked.c`:
  - in the round trip, a colour picture and a metal-roughness one come back BC7 and a normal one
    BC5;
  - a copy carrying the old version number is refused.
- `3d/tests/tests.md` — the `model_cooked.c` entry.

## Done when
The `3d/model_cooked` test passes.
