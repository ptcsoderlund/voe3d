# 06 — The store uploads a compressed picture as blocks
folder: 3d
after: 03, 05
decisions: 0168, 0470, 0477

## Change
A model's or a material's picture may now come as BC blocks (0477 point 1). The store uploads it as
it is.

- `3d/src/picture_upload.h`, `3d/src/picture_upload.c` (new, internal) —
  `[[nodiscard]] bool voe_3d_picture_upload(voe_render_device *device, const voe_assets_image *image,
  voe_render_texture_kind kind, voe_render_texture *out, voe_base_error *error)`:
  - a BC7 or BC5 picture through `voe_render_texture_create_blocks`, its format naming the blocks;
  - RGBA8 with levels through `_create_levels`;
  - one level through `_create`, SMOOTH, mipped on the GPU.
- `3d/src/model_upload.c` (`upload_picture`) and `3d/src/models_material.c` (`upload_map`) — each
  calls it in place of its own two-way branch.
- `3d/include/3d/models.h` — the comments of `_put` and `_put_material` say a picture may be blocks,
  uploaded as they are.
- `3d/tests/models_material.c` — a new case: a material is put with a BC7 colour map and a BC5
  normal map, made with `voe_assets_image_mips` then `voe_assets_image_compress`. It loads, and
  `voe_render_texture_memory_smooth`'s `compressed` rises by two. It skips without a card, as the
  file does.
- `3d/src/src.md`, `3d/tests/tests.md` — entries.

## Done when
The `3d/models_material` test passes.
