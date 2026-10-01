# 06 — Render updates a probe grid from the bounce map
folder: render
after: 05
decisions: 0168, 0307, 0308

## Change
0308 point 4, and point 6's "only when updated". Read
`render/include/render/device.h`, `render/src/device_parts.h`,
`render/src/device_internal.h`, `render/src/bounce_map.c`,
`render/src/bounce_schedule.h`, `render/src/pipeline.c` (how a module is
embedded and a pipeline made), `render/src/pass.c`, `render/src/src.md`,
`render/shaders/shaders.md` and `render/tests/bounce_grid.c`.

- `device.h`: `typedef struct voe_render_bounce_update` {int32_t cell[3];
  float3 corner; const float4 *stale; uint32_t stale_count;} and
  `[[nodiscard]] bool voe_render_bounce_update(voe_render_device *device,
  voe_render_target target, const voe_render_bounce_update *update)`.
  Comment points: `cell` the grid's lowest world cell at
  `VOE_RENDER_BOUNCE_SPACING`, `corner` its lowest corner about the eye,
  `stale` spheres about the eye (xyz, w radius); reads this frame's last
  bounce pass; false outside a frame, inside a pass, with no bounce pass this
  frame, or for a target that is not live; at most once a target a frame
  (false again); a camera pass on that target later this frame reads the
  grid, one without an update this frame reads no bounce.
- `render/shaders/bounce.slang`, new, header comment, two compute entries:
  - `reduce`: one VPL per 8×8 block of the 512² map, 64×64 into a storage
    buffer: position about the eye from depth through the inverse light
    matrix, mean normal, flux summed × the block's area in m²;
  - `gather`: one probe a thread from the listed indices: each VPL's
    radiance toward the probe is flux × max(0, n·−ω) / (π × max(d², 1)),
    projected into L1 SH RGB; blend into the grid's three images with the
    listed blend.
- `render/src/bounce_grid.c`, new, header comment: the two compute
  pipelines and their own set layout (the map's images, the VPL buffer, the
  per-slot probe list buffer with room for (targets + 1) ×
  (`VOE_RENDER_BOUNCE_PROBES`³) indices and blends, the grid's images as
  storage, a push block with light matrices, corner and offset); the update
  call: run the schedule kept per grid, write the list, dispatch reduce once
  a frame and gather per call, barriers before and after, and mark the grid
  updated in the frame slot. `device_internal.h` declares what the device
  build calls.
- `pass.c`: a camera pass fills the block's bounce record from its target's
  grid when the frame slot marks it updated, else ~0u.
- `render/tests/bounce_grid.c`, cases added:
  - with no bounce pass this frame the update is false;
  - after a bounce pass the update is true, a second on that target is
    false, and one on the other target is true;
  - inside a camera pass it is false;
  - a destroyed target is false.
- `render/include/render/render.md`, `render/src/src.md`,
  `render/shaders/shaders.md`, `render/tests/tests.md`: entries changed or
  added.

## Done when
The test `render/bounce_grid` passes, and `render/bounce_map` and
`render/bounce_schedule` still pass, after the folder's build.
