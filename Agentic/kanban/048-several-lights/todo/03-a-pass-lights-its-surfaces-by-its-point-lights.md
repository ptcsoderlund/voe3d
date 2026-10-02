# 03 — A pass lights its surfaces by its point lights
folder: render
after: 02
decisions: 0168, 0320

## Change
A pass carries its point lights and draw.slang lights by them (0320 points 3–5). This changes
`voe_render_pass_camera`; the callers' positional initializers in 3d, game and editor are fixed by
their own cards (04, 06, 07), not here. Read the headers of `render/src/pass.c`,
`render/src/descriptors.c`, `render/src/device_parts.h`, `render/shaders/bindings.slangh`,
`render/shaders/lighting.slangh`, `render/shaders/draw.slang`, and `render/src/light_bins.h`.

- `render/include/render/device.h`: `voe_render_pass_camera` gains `voe_render_point_lights points`
  last; its comment says what a pass does with them, that they are copied at pass begin, that zero is
  none, and that more than VOE_RENDER_POINT_LIGHTS or NULL with a count asserts.
- `render/src/device_parts.h`: the frame block's `reserved[3]` becomes `uint32_t lights` (this pass's
  count, 0 for none) and `uint32_t region` (this pass's region index) and one reserved word, offsets
  unchanged; the per-slot struct gains two mapped storage buffers, the lights (`capacities.passes` ×
  VOE_RENDER_POINT_LIGHTS records) and the bins (`capacities.passes` × one `struct
  voe_render_light_bins`). The block's header no longer says it is a copy of the pass camera; it says
  what the two words are.
- `render/src/descriptors.c`: bindings 7 (lights) and 8 (bins), storage buffers, fragment stage, built,
  written, freed beside binding 2's per-slot buffer; the pool counts them; the size and offset asserts
  cover the two words. The header's binding list gains both and says "nine bindings".
- `render/src/pass.c`: pass begin copies the lights into the slot's lights region of this pass, bins
  them through `voe_render_light_bins_fill` with the pass's view into its bins region, and writes
  `lights` and `region` in its block; a pass with none writes 0 and bins nothing. Header point on it.
- `render/shaders/bindings.slangh`: the block's two words; `voe_render_point_light` matching the C
  record; bindings 7 and 8; the tile, slice and word constants matching light_bins.h.
- `render/shaders/lighting.slangh`: `float3 voe_render_points(float3 world, float3 n, float3 v,
  float3 diffuse_colour, float3 f0, float alpha)`: the fragment's tile from NDC of `world` through the
  block's view and projection, its slice from its view distance (the same exponential as
  `voe_render_light_slice`), then for each set bit of tile mask AND slice mask per word the BRDF
  (the functions already here) × N·L × colour × saturate(1 − (d/range)²)²; nought with no lights.
  Header: a paragraph on point lights (no shadow, no fill fade, no bounce, not on water).
- `render/shaders/draw.slang`: the lit exit adds `voe_render_points(...)` to the sun's direct term;
  the unshaded exit does not. The header's lighting paragraph names it.
- `render/tests/point_lights.c` (new), modelled on `render/tests/unshaded.c`: a grey ground quad, a sun
  of intensity 0 and no fill, one red point light 1 m above its middle with range 3: the pixel under
  it is red above 0.2 and green and blue near 0; a pixel 5 m off is black; the same with `points`
  zeroed is black under it; an unshaded pass with the light is the unshaded picture; two lights, one
  each side, light both pools.
- `render/src/src.md`, `render/shaders/shaders.md`, `render/tests/tests.md`,
  `render/include/render/render.md`: entries changed or added for what moved. Each at most 300
  characters.

## Done when
The test `render/point_lights` passes, and `render/unshaded`, `render/light_bins`, `render/offscreen`
and `render/passes` still pass, after the folder's build.
