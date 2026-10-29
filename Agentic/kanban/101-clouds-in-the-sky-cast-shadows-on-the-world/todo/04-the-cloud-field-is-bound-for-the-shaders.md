# 04 — The cloud field is bound for the shaders
folder: render
after: 03
decisions: 0168, 0492

## Change
0492 point 4: card 03's bytes become two sampled images every pass can read. Read the headers of
`render/src/descriptors.c`, `render/src/device.c`, `render/src/startup.h`,
`render/src/bounce_volume.c` (a 3D image made) and `render/src/clouds_noise.h`; in
`render/src/device_calls.h` the entries for buffer.c, descriptors.c and target.c's image build;
the device struct in `render/src/device_internal.h`; set 0's bindings in
`render/shaders/bindings.slangh`.

- `render/src/clouds_textures.c` (new): `_build` makes, at open, a 64³ `R8_UNORM` 3D image (the
  detail) and a 256² `R8_UNORM` 2D image (the weather), sampled and transfer-dst, filled from
  clouds_noise.c through buffer.c's staging, resting in SHADER_READ_ONLY_OPTIMAL, and one linear
  sampler repeating on every axis, all named for a capture; `_teardown` frees them, safe on a
  device that never built them. False with a message. Header: what, why at open, the cost in
  bytes.
- `render/src/device_internal.h`: the device holds the two images and the sampler.
- `render/src/device_calls.h`: the two calls, under a `clouds_textures.c` entry.
- `render/src/device.c` and `render/src/startup.h`: open builds them before the descriptors and
  close tears them down; the headless device too; the order's comment says why there.
- `render/src/descriptors.c`: binding 13, the detail, and 14, the weather, each a combined image
  sampler for the fragment stage, written into every slot's set at startup; the header's binding
  list grows to fifteen and says them.
- `render/shaders/bindings.slangh`: `Sampler3D` at binding 13 and `Sampler2D` at 14, with a
  comment each.
- `render/src/src.md` (new file, descriptors.c, device.c) and `render/shaders/shaders.md`
  (`bindings.slangh`): follow, each at most 300 characters.

A Best Practices warning from the new images fails the checks: fix it, never allowlist it.

## Done when
The tests `render/passes`, `render/sky` and `render/best_practices` pass after the folder's build.
