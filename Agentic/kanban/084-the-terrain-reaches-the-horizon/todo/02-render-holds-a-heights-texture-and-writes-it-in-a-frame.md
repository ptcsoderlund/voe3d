# 02 — render holds a heights texture and writes part of it inside a frame
folder: render
after: none
decisions: 0168, 0386, 0396

## Change
0396 points 3 and 5: a landscape's heights are an R32F texture the vertex stage reads, and a
sculpt writes a rectangle of it each frame without waiting for the card.

- `render/include/render/device.h`:
  - `voe_render_capacities` gains `heights_texels`: texels a frame slot may write with the call
    below; nought for none. Comment as the transient ones are.
  - `[[nodiscard]] bool voe_render_texture_create_heights(voe_render_device *device, uint32_t width,
    uint32_t height, const float *heights, voe_render_texture *out, voe_base_error *error)`:
    R32F, one level, no sampler use, metres as given, row-major. Startup operation like
    `voe_render_texture_create` (waits idle); destroyed by `voe_render_texture_destroy`. Fails
    REFUSED with no slot or a side above 4096.
  - `[[nodiscard]] bool voe_render_texture_write_heights(voe_render_device *device,
    voe_render_texture texture, uint32_t x, uint32_t y, uint32_t width, uint32_t height,
    const float *values, voe_base_error *error)`: inside a frame, before its first pass; copies
    `values` (width × height, row-major) into the frame slot's staging and records the copy into
    the texture at (x, y) before any pass reads it. REFUSED when the slot's remaining
    `heights_texels` are fewer; asserts outside a frame, after a pass began, on a rectangle past
    the texture or on a texture that is not a heights one.
  - Header points: the order is staging copy then a barrier from vertex reads of earlier frames
    to the transfer and back; a frame in flight reading the texture is ordered by the queue.
- New `render/src/texture_heights.c` (header comment, its entry in `render/src/src.md`): both
  calls. Keep `texture.c` (732 lines) from growing past what it needs: only the slot sharing and
  kind bookkeeping it must expose, through `device_internal.h`.
- `render/src/device.c` and `render/src/frame.c`: the per-slot staging buffer sized from
  `heights_texels`, its used count reset at frame begin, the recorded copies before the first pass.
- `render/src/descriptors.c`: the texture array is visible to the vertex stage too.
- New headless test `render/tests/heights.c` and its entry in `render/tests/tests.md`: a 2049 ×
  2049 heights texture is created; inside a frame a 4 × 4 write succeeds and a write over the
  remaining `heights_texels` is REFUSED and leaves the frame drawing; the frame ends true; the
  next frame's budget is whole again. A machine with no usable Vulkan skips, as the others do.

## Done when
`render/tests/heights.c` passes.
