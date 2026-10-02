# 08 — Captured probes settle: their validity and distance moments
folder: render
after: 07
decisions: 0168, 0326

## Change
0326 point 5, and the relight call whose first step it is; the levels are card 09.
- `render/include/render/device.h`: `void voe_render_bounce_relight(voe_render_device *device)`,
  beside the capture pass. Comment: after the begun target's capture passes, between passes;
  settles what was captured or emptied this frame and, from card 09, relights when card 04 says
  it is needed; nothing at all on a settled frame, when only the read runs (0317 point 4);
  nothing on a volume not built; asserts outside a frame, with a pass open, or with no begin.
- `render/shaders/bounce_relight.slang`, new, compute: `settle`, one workgroup per listed probe:
  validity 1, or 0 when the probe holds no picture or more than a quarter of its 384 texels see a
  back face (normal · direction from the probe > 0), into the validity image; each texel's
  (mean, mean²) of the distances over its 3×3 within its own face, clamped at the face's edge,
  into the moments atlas. Header: 0326 point 5, the push block matching its C struct.
- `render/src/bounce_relight.c`, new: the compute set layout, pool and pipelines (as
  `bounce_grid.c` builds its own; read its header), a per-slot list buffer of probe indices with
  a "holds a picture" flag, sets per slot and volume; `voe_render_bounce_relight`: lists card
  04's changed probes, dispatches `settle`, barriers against the capture copy before and the
  fragment reads after, then card 04's relit call (card 09 moves it after the levels). Header.
- `render/src/device.c`, `device_internal.h`: startup and shutdown of the above.
- `render/src/loader.h`: only if a Vulkan entry it needs is not in the table yet.
- `render/tests/bounce_settle.c`, new, headless: a cube 1.5 m beside the eye, frames of begin,
  capture passes and relight until none opens: the probe nearest the cube reads validity 1 and
  a moments mean toward the cube near 1.5 m − half its size; a probe inside the cube reads
  validity 0; a frame with nothing changed records no dispatch (count dispatches in the device
  through the internal header).
- `render/src/src.md`, `render/shaders/shaders.md`, `render/tests/tests.md`: entries.

## Done when
`ctest --test-dir build/debug -R "^render/bounce_settle$"` passes.
