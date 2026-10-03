# 07 — Lit surfaces read the probe volume, with visibility and no gain
folder: render
after: 06
decisions: 0168, 0307, 0312, 0326

## Change
0326 point 7. 046's grids stop being read here; 046's update still runs unread until card 14.
- `render/shaders/bounce_read.slangh`, new: the one read, E(n) at a surface, taking the three SH
  images, the validity image, the moments atlas and the placement (lowest corner about the eye,
  lowest cell wrapped per axis, spacing) as parameters, so the lit pass and card 09's relight
  call it on different images. The eight probes about the surface pushed 0.25 × spacing along n,
  probe centres at cell centres, each loaded at its toroidal index; weight trilinear × validity ×
  (((n · to-probe + 1) / 2)² + 0.2) × Chebyshev visibility (mean, mean² sampled bilinear inside
  the probe's face toward the surface, the uv held half a texel inside the face; full when nearer
  than the mean); E from L1 SH with A0 π, A1 2π/3 as 046's; normalised by the weights' sum,
  nought when it is nought or not finite; faded over the grid's outer cell. Header: 0326 point 7,
  why each weight.
- `render/shaders/bindings.slangh`: binding 6 is four entries a volume (the sum's three SH, the
  validity), binding 10 the moments atlases, one a volume; the frame block's bounce record keeps
  its shape, `cell` the lowest cell wrapped per axis.
- `render/shaders/lighting.slangh`: the bounce through `bounce_read.slangh` on the pass's
  volume; `VOE_BOUNCE_GAIN` and its paragraph go; lit = direct + base × max(E/π, fill floor)
  (0307 point 5). Header phrases: no gain (0317 point 5), visibility (0312).
- `render/src/descriptors.c`: eleven bindings; 6 counted (targets + 1) × 4; 10 (targets + 1)
  2D images through a linear, clamping sampler of its own; a write naming a volume's five images,
  called by `bounce_volume.c` as it builds one (all sets: the GPU is idle there). The old grid
  write and its calls in `target.c` and `target_own.c` go; `bounce_grid.c`'s own set is untouched.
- `render/src/device_internal.h`, `device.c` (the binding 6 note): to match.
- `render/src/pass.c`: a camera pass names its target's volume (descriptor, cell, corner of
  this slot's begin) only when this slot's frame began it and it is built; otherwise
  `VOE_RENDER_NO_BOUNCE`.
- `render/tests/bounce.c` and `render/tests/bounce_scene.c`: deleted, 046's look; their entries
  go from `render/tests/tests.md`.
- `render/tests/bounce_read.c`, new, headless: a sunlit grey ground with fill 0.1 under a
  begun, built window volume with nothing captured reads the same pixels as with no begin.
- `render/shaders/shaders.md`, `render/src/src.md`, `render/tests/tests.md`: entries.

## Done when
`ctest --test-dir build/debug -R "^render/bounce_read$"` passes and
`! grep -rn "VOE_BOUNCE_GAIN" render/shaders` exits 0.
