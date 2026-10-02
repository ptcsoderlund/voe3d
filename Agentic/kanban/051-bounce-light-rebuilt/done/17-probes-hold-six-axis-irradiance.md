# 17 — Probes hold irradiance along the six axes, not L1 SH
folder: render
after: 13, 14
decisions: 0168, 0312, 0326, 0327

## Change
0327: the level grids and the sum change what they hold; bindings, image counts and calls stay.
- `render/src/device_parts.h`: in `struct voe_render_bounce_volume`, `sh[7][3]` is renamed
  `irradiance[7][3]`; its comment says six-axis irradiance, image a axis a, + and − side by side,
  48 × 12 × 24.
- `render/src/bounce_volume.c`: the 21 grid images are built 2 × `VOE_RENDER_BOUNCE_PROBES_XZ`
  wide; the rename; header and the image comments say what they hold.
- `render/src/bounce_relight.c`, `render/src/descriptors.c`: the rename; the clear of a chain's
  levels and every copy or barrier over the grids cover the new extent; the binding 6 comment in
  `descriptors.c` names the three axis images.
- `render/shaders/bounce_relight.slang`: `relight` sums, per texel, radiance × solid angle ×
  max(0, ±direction_a) into six RGB values in place of the SH projection, reduced in group memory
  as now, and stores them at (2i, j, k) and (2i + 1, j, k) of image a; `store_grid` takes the six.
  `sum` adds every texel of the 48-wide images (one thread per texel). Header: the six-axis store,
  why (0327), the texel layout.
- `render/shaders/bounce_read.slangh`: each probe's E(n) = Σ_a n_a² × E(sign(n_a), axis a), six
  loads at its toroidal index; weights, edge fade and normalising as now. Header: the "E FROM L1
  SH" paragraph becomes the six-axis read and why it gives nothing from behind a surface's plane;
  the CONSTRAINTS load count.
- `render/shaders/bindings.slangh`: the binding 6 comment names the three axis images.
- `render/tests/bounce_volume.c`: the rename, and a check that a sum image is 48 wide.
- `render/tests/bounce_probes_scene.c`: a new scene beside `a_red_wall`: a 1 × 1 × 1 m red box
  (0.6) on grey ground, sun 2 at the wall scene's angle, fill 0 (the wall scene's fill 0.1 hid
  the leak); claim: the ground in its shadow 0.25 m from its foot within 4/255 of bounces 0 in
  every channel (0312). Run it before the change: it must fail on the L1 SH build; note the
  before and after values in the commit message. Header line for the scene.
- `render/shaders/shaders.md`, `render/src/src.md`, `render/tests/tests.md`: the entries that
  say L1 SH say six-axis irradiance.

## Done when
`ctest --test-dir build/debug -R "^render/bounce_(volume|settle|read|probes_scene)$"` passes and
`! grep -n "L1 SH" render/shaders/bounce_read.slangh render/shaders/bounce_relight.slang` exits 0.
