# 14 — The picture test names its image fields
folder: app
after: none
decisions: 0168, 0470

## Change
The whole-tree Debug build stops in `app/tests/picture.c`: card 03 gave
`voe_assets_image` (`assets/include/assets/image.h`) a fourth field, `levels`,
and the positional initializer in `reads_a_png` (line 45,
`{ WIDTH, HEIGHT, pixels }`) now trips `-Werror,-Wmissing-field-initializers`.

- `app/tests/picture.c`, `reads_a_png`: initialize the image by field name
  (width, height, pixels), the way `3d/tests/models.c` does; `levels` is left
  zero, which the header defines as level 0 alone.

No other file changes. Nothing else in the tree failed to compile; every other
`voe_assets_image` initializer is already `{ 0 }` or designated.

## Done when
The test `app/picture` builds with no warning and passes.
