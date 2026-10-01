# 05 — Every target keeps a probe grid
folder: render
after: 04
decisions: 0168, 0308

## Change
0308 point 3: the storage only; nothing writes or reads the grid yet. Read
`render/include/render/device.h` (capacities, the bounce constants),
`render/src/device_parts.h`, `render/src/device_internal.h`,
`render/src/target.c`, `render/src/target_own.c`,
`render/src/descriptors.c`, `render/src/pass.c` (the camera block),
`render/shaders/draw.slang` (the camera block and bindings),
`render/src/src.md` and `render/tests/targets.c` for a test's shape.

- `device_parts.h`: a grid record — three RGBA16F 3D images of
  `VOE_RENDER_BOUNCE_PROBES`³ (storage and sampled), their memory and views,
  and its descriptor index; the window and each caller target own one. The
  grid is not per frame slot (one copy, 0308 point 3); a frame slot records
  per grid whether it was updated this frame and its lowest cell and corner
  of that update.
- `target.c` / `target_own.c`: build the window's grid with the device and
  a target's with the target, cleared to zero in a one-off command with the
  layout left at general; free with them; resizing a target keeps its grid.
- `descriptors.c`: a binding of sampled 3D images, three per grid, (targets
  + 1) × 3 entries, plus a trilinear sampler with repeat; a grid's entries
  written when it is built; the binding count and asserts follow.
- `pass.c`: the camera block gains the bounce record — the grid's
  descriptor index (~0u: no bounce), the grid's lowest corner about the eye
  (float3), its lowest cell mod 32 (uint3) and the spacing; ~0u always for
  now.
- `draw.slang`: the bindings' and the block's mirrors; nothing reads them.
- `device.h`: the capacities comment says each target also costs a probe
  grid of 3 × 32³ × 8 bytes (about 786 kB).
- `render/tests/bounce_grid.c`, new, headless, cases:
  - a device with `targets` 2 makes, resizes and frees both targets twice
    and draws a red cube into each, reading back red;
  - the window draws a red cube too.
- `render/include/render/render.md`, `render/src/src.md`,
  `render/shaders/shaders.md`, `render/tests/tests.md`: entries changed or
  added.

## Done when
The test `render/bounce_grid` passes, and `render/targets`,
`render/depth_copy` and `render/textures` still pass, after the folder's
build.
