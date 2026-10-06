# 01 — Material textures get a mip chain, read trilinear, NEAREST up close
folder: render
after: none
decisions: 0168, 0359

## Change
A texture created with `VOE_RENDER_SAMPLING_SMOOTH` (every model and material texture: `3d`'s model upload and
model picture, `dev`'s cubes) gets a full mip chain built on the GPU at upload, and its sampler minifies
trilinear while magnifying NEAREST. SHARP (sprite sheets, app pictures, targets) and FIELD (glyph atlas) keep
one level and their samplers exactly as today. No public signature changes; `draw.slang` already samples with
an implicit level, so no shader changes.

- `render/src/texture_levels.c` (new): `voe_render_texture_levels_record(voe_render_device *device,
  VkCommandBuffer commands, VkImage image, uint32_t width, uint32_t height, uint32_t level_count)` — with
  level 0 in TRANSFER_DST holding the upload, blits each level from the one above with `VK_FILTER_LINEAR`
  (halving each side, never below 1), and leaves every level in SHADER_READ_ONLY. Header: why a GPU blit
  (the sRGB format makes the average linear-correct; R8G8B8A8 SRGB/UNORM always support linear blit), and
  that it runs inside the startup upload's one submission.
- `render/src/device_internal.h`: declare it, beside the texture slot declarations.
- `render/src/texture.c`: for SMOOTH, the image's `mipLevels` is ⌊log2(max(w, h))⌋ + 1 and its usage adds
  TRANSFER_SRC; its view covers every level; after the staging copy it calls the new function instead of the
  single SHADER_READ_ONLY transition (other modes unchanged). The SMOOTH sampler: mag NEAREST, min LINEAR,
  mipmap mode LINEAR, `maxLod` `VK_LOD_CLAMP_NONE`, anisotropy still off (card 03 turns it on). Rewrite the
  file header and the sampler comments: the old "no mipmaps, no linear filtering, engine's rule" passage is
  gone, replaced by 0359's rule (chain for material textures, hard texels up close, sheets and glyphs one
  level and why). Slot 0's white default stays one level.
- `render/include/render/device.h`: comments only. `voe_render_sampling`'s block ("NO PICTURE IN THIS ENGINE
  IS FILTERED") and SMOOTH's entry now say SMOOTH is mipped, trilinear when minified, NEAREST when magnified,
  REPEAT; SHARP and FIELD unchanged. `voe_render_texture_create`'s comment drops "one level sampled NEAREST
  whichever is passed" and says a SMOOTH texture's chain is built here.
- `render/src/src.md`: a `texture_levels.c` entry; `texture.c`'s entry mentions the chain for SMOOTH.
- `render/tests/mips.c` (new, headless, skips with its reason when there is no Vulkan, as
  `render/tests/offscreen.c` does — read that file's header and its device, camera, quad and readback setup
  and reuse its approach, unlit or fully rough so shading does not colour the result). Two checks, each a
  SMOOTH COLOUR texture on a quad facing the camera:
  - up close: a 4×4 black/white one-texel checker filling most of the picture; every read-back pixel is
    within 2 of pure black or pure white on each channel (hard texels).
  - far: a 256×256 one-texel black/white checker on a quad about 16 pixels across; every pixel of its
    inside is between 64 and 220 on each channel (a blend, where one level would give 0 or 255).
- `render/tests/tests.md`: a `mips.c` entry.

`offscreen.c`, `textures.c`, `pools.c` and `surface_maps.c` create SMOOTH textures and must still pass
unchanged.

## Done when
`ctest --test-dir build/debug -R "^render/(mips|offscreen|best_practices)$"` exits 0 after a debug build of
`voe_render` and its tests.
