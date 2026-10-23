# 02 — Render takes a texture's mips as given
folder: render
after: 01
decisions: 0168, 0470

## Change
Cooked pictures carry their own mip chain (0470 point 3). Render must upload that chain as it is, instead
of building one on the GPU.

- `render/include/render/device.h`: beside `voe_render_texture_create`, add
  `[[nodiscard]] bool voe_render_texture_create_levels(voe_render_device *device, voe_render_texture_kind kind, uint32_t width, uint32_t height, uint32_t level_count, const uint8_t *rgba, voe_render_texture *out, voe_base_error *error)`.
  It is always SMOOTH. `rgba` holds `level_count` levels one after another, level i being
  `max(1, width >> i)` × `max(1, height >> i)` RGBA8 texels. The image has exactly those levels and none
  is built. A level_count of 0, or more than the full chain, asserts. It fails, is copied, and waits for
  the GPU as `_create` does. Its comment says that a level_count of 1 makes an image of one level,
  not a GPU-built chain.
- `render/src/texture.c` (755 lines): put no new code here. Make the helpers the new function shares
  with `_create` (image creation, the staging copy into an image, the slot bookkeeping) non-static, and
  declare them in `render/src/device_internal.h`. Read the header of `texture.c` and the declarations
  in `device_internal.h` to find them.
- New `render/src/texture_given.c`: `voe_render_texture_create_levels`, with a header comment on why
  the levels come from the caller (0470) and how one staging buffer holds every level, copied with one
  region per level.
- `render/src/src.md`: an entry for `texture_given.c`.
- `render/tests/mips.c`: add a case `given_levels_are_drawn`. A 256×256 black-and-white checker is
  level 0, and every lower level is solid red. On the far quad of the existing far case, every pixel
  inside its edge must read red within 8. That proves the given levels are sampled and no chain was
  built over them.
- `render/tests/tests.md`: the `mips.c` entry names the case.

## Done when
`ctest --test-dir build/debug -R '^render/mips'` passes with `given_levels_are_drawn` in it. (It skips on a
machine with no usable Vulkan.)
