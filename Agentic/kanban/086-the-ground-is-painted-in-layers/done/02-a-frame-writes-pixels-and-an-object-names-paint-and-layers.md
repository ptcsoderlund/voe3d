# 02 — A frame writes pixels, and an object names paint and layers
folder: render
after: none
decisions: 0168, 0402

## Change
0402 point 4, the render half: an in-frame write into an RGBA8 texture, and the object record's
room for a landscape's paint and layers. No shader reads the new fields yet (card 03).

- `render/include/render/device.h` (a public header: read only the parts you change; grep for
  `voe_render_object`, `heights_texels`, `voe_render_texture_write_heights`) —
  - `voe_render_object`: `uint32_t reserved[2]` becomes `uint32_t paint[2]`, each a paint
    texture's index plus one, nought none; after `morph`, `uint32_t layers[8]`, shading record
    indices of a landscape's layers 0–7. Both read only when `heights` and `paint[0]` are not
    nought. The record grows from 224 to 256 bytes; the comment above it says so and what each
    holds, citing 0402.
  - `voe_render_texture_write_pixels(device, texture, x, y, width, height, const uint8_t *rgba,
    error)`, `[[nodiscard]]`: as write_heights, but for a one-level RGBA8 texture
    (`VOE_RENDER_SAMPLING_SHARP` or `_FIELD`), 4 bytes a texel from the same per-slot staging
    and the same `heights_texels` room, barriers round the copy against fragment reads. Refused
    over the room; asserts outside a frame, after a pass began, past the texture, on a SMOOTH or
    heights texture. The `heights_texels` comment says pixel writes share it.
- `render/src/texture_heights.c` — the write, sharing write_heights' staging bookkeeping; header
  says both writes share the slot's staging. `render/src/texture.c` only if a texture's sampling
  is not yet kept where the assert can ask it.
- `render/shaders/bindings.slangh` — `voe_render_object`: `reserved1`, `reserved2` become
  `paint_a`, `paint_b`; after `morph`, `uint4 layers_low; uint4 layers_high;`. The comment's byte
  tally ends at 256.
- `render/src/records_layout.c` — size 256; offsets of `paint` (136), `layers` (224) asserted.
- `render/src/descriptors.c` — whatever names the object record's size by number.
- `render/tests/pixels_write.c` — new: a headless device with `heights_texels` 64; a 4 × 4 black
  RGBA8 FIELD texture; inside a frame a 4 × 4 red write is taken, then a 8 × 8 write refused with
  the frame still drawing; the frame draws an unlit quad filling the target wearing the texture
  as base colour, and the picture's centre reads red. Mirror an existing test's quad and read
  (`render/tests/heights.c`, `render/tests/unshaded.c`).
- `render/src/src.md`, `render/tests/tests.md`, `render/shaders/shaders.md` — entries follow.

## Done when
Tests `render/pixels_write`, `render/heights` and `render/terrain_vertex` pass.
