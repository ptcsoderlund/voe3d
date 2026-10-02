# 09 — Probes relit a level at a time, each light to its own count
folder: render
after: 08
decisions: 0168, 0312, 0317, 0326

## Change
0326 point 6, the levels and the sum, in the relight call card 08 made.
- `render/shaders/bounce_relight.slang`: `relight`, one workgroup of 64 per valid probe over its
  384 texels, each weighted by its cube texel's solid angle: no hit (distance at the reach) or a
  back face is nought; else the surface point (probe + direction × distance) and normal, and
  radiance = albedo × at level 1 the sum over the chain's lights of colour × intensity × bounce
  strength × max(0, n·l) × shadow — the sun with the frame's cascades (the first whose box holds
  the point, lit outside them), a lamp with `lighting.slangh`'s falloff and, when slotted and the
  point shadows are ready, its compare through `point_shadow.slangh` — and at level k > 1
  `bounce_read.slangh`'s E of level (chain, k − 1) / π; projected to L1 SH and reduced in group
  memory into level (chain, k). An invalid probe writes nought. `sum`: per probe, the sum's SH
  is the sum of every level in use. Push block: chain, level, the sun and lamps' chain numbers.
- `render/src/bounce_relight.c`: the set grows the levels and sum as storage and sampled, the
  slot's shadow and point shadow maps with their comparison sampler, the shadow record and the
  bouncing lamps in a per-slot buffer. When card 04 says a relight is needed: for k 1 to 3, for
  each chain n ≥ k holding a light, `relight` over every probe, a barrier between levels; then
  `sum`; then the relit call. A chain with no light is skipped and its levels zeroed once.
- `render/src/device_parts.h`: whatever the set needs per volume.
- `render/tests/bounce_probes_scene.c`, new, headless: a sun from above at an angle casting
  through cascades the test draws (as `render/tests/shadow.c` does), grey ground, a red wall;
  frames of begin, capture passes and relight until no capture opens, then a camera pass read
  back:
  - the ground on the wall's lit side within 2 m reads redder than ground 10 m off; ground in the
    wall's shadow at its foot within 4/255 of bounces 0 (0312);
  - open ground: five points 2 m apart along x within 2/255 of each other (no blotches);
  - a closed box room (floor, four walls, roof): a floor point inside within 2/255 of bounces 0;
    a doorway cut in one wall: the floor before it brighter than at 0, and the far corner the
    doorway does not face brighter at sun bounces 2 than at 1;
  - strength 0 equals bounces 0; strength 2 brightens the ground by the wall more than 1, and
    open ground with nothing to bounce from reads the same at both (direct light unchanged);
  - under a dim sun, a lamp of bounces 1 over the ground beside a white wall: the wall above
    the lamp's reach brighter than at bounces 0;
  - a further frame with nothing changed opens no capture pass and dispatches nothing.
  Fewer probes than the whole grid is fine: put the eye near the scene and run as many frames as
  fill the probes about it.
- `render/shaders/shaders.md`, `render/src/src.md`, `render/tests/tests.md`: entries.

## Done when
`ctest --test-dir build/debug -R "^render/bounce_probes_scene$"` passes.
