# 05 — A target's probe volume, built on first use, and the bounce begin
folder: render
after: 04
decisions: 0168, 0316, 0326

## Change
0326 points 2 and 8 (the begin). 046's grids in `target.c` stay until card 14.
- `render/include/render/device.h`: `VOE_RENDER_BOUNCE_FACE` 8 and `VOE_RENDER_BOUNCE_REACH`
  24.0f; `struct voe_render_bounce_frame` (tag, no typedef, as `voe_render_bounce_update`):
  `cell[3]`, `corner`, `stale` and `stale_count`, `sun` (`voe_render_light`), `sun_bounces`,
  `sun_strength`, `shadow` (`voe_render_shadow`), `points` (`voe_render_point_lights`);
  `void voe_render_bounce_begin(voe_render_device *device, voe_render_target target, const struct
  voe_render_bounce_frame *frame)`. Comment: records this target's bounce for the frame, between
  passes; the first begin onto a target builds its volume at the top of the next frame (one GPU
  idle) and this frame bounces nothing; a volume with no begin for 300 frames is freed the same
  way (0316); on a card without `shaderOutputLayer` nothing bounces; asserts outside a frame, with
  a pass open, on a target not live, a second begin for one target in a frame, or sun bounces past
  `VOE_RENDER_BOUNCES_MAX`. The memory a volume costs, in the capacities comment's `targets`
  paragraph.
- `render/src/device_parts.h`: `struct voe_render_bounce_volume`: the albedo atlas (RGBA8 sRGB),
  the normal-and-distance atlas (RGBA16F), the moments atlas (RG16F), each 1152 × 2304; the
  validity 3D image (R16F) and seven SH grids of three RGBA16F 3D images (six levels, the sum),
  each 24 × 12 × 24; built, wanted and idle frames; card 04's `voe_render_bounce_probes`; per
  frame slot whether that slot's frame began it, with its lowest cell and corner. The window and
  each target slot own one beside the old grid. The device keeps the begun target and a copy of
  its frame record, the bouncing lamps in a fixed array of `VOE_RENDER_BOUNCE_LAMPS`.
- `render/src/bounce_volume.c`, new: build (every image storage, sampled and transfer-dst,
  cleared to nought, resting in GENERAL) and teardown; the top-of-frame apply over the window and
  every live target, building the wanted and freeing those idle 300 frames, waiting for the GPU
  only when one does; `voe_render_bounce_begin`, which on a built volume runs card 04's place
  call. Header: what it owns, the lifetime, why the top of a frame.
- `render/src/device_internal.h`: the apply, build and teardown declared; the begun state.
- `render/src/frame.c`: the apply beside `voe_render_targets_apply_resizes`; the begun state
  reset as each frame opens.
- `render/src/device.c`: volumes torn down at close-down.
- `render/tests/bounce_volume.c`, new, headless: after a begin the window's volume is wanted and
  not built; built after the next frame begins; a target's and the window's are apart; after 300
  frames with no begin it is freed. It reads the volume through `../src/device_internal.h`.
- `render/src/src.md`, `render/tests/tests.md`, `render/include/render/render.md` if needed:
  entries.

## Done when
`ctest --test-dir build/debug -R "^render/bounce_volume$"` passes.
