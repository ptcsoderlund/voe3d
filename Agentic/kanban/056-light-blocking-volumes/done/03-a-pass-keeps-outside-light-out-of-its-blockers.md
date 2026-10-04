# 03 — A pass keeps outside light out of its blockers
folder: render
after: 02
decisions: 0168, 0347

## Change
A pass carries its blockers and draw.slang gates the sun, the fill and the point lights by them
(0347 point 3); the bounce is card 07's. Read the headers of `render/src/pass.c`,
`render/src/descriptors.c`, `render/src/device_parts.h`, `render/src/light_blockers.h`,
`render/shaders/bindings.slangh`, `render/shaders/lighting.slangh` and `render/shaders/draw.slang`,
and `voe_render_pass_camera` in `render/include/render/device.h`.

- `render/include/render/device.h`: `voe_render_pass_camera` gains `voe_render_light_blockers
  blockers` last; its comment: copied at pass begin, zero is none, the old picture; more than
  VOE_RENDER_LIGHT_BLOCKERS, NULL with a count or a row not finite asserts.
- `render/src/device_parts.h`: the frame block's reserved word becomes `uint32_t blockers` (this
  pass's count), offsets unchanged; the per-slot struct gains one mapped storage buffer of
  `capacities.passes` regions, each VOE_RENDER_LIGHT_BLOCKERS records then VOE_RENDER_POINT_LIGHTS
  `uint32_t` light masks. Header: what the word and the region hold.
- `render/src/descriptors.c`: binding 11, that buffer, fragment stage, built, written and freed
  beside binding 7's; the pool counts it; the asserts cover the word. Header: "twelve bindings".
- `render/src/pass.c`: pass begin copies the blockers into this pass's region and writes each
  point light's mask, `voe_render_light_blockers_mask` at its position, beside them; `blockers` in
  the block; none writes 0. Header point on it.
- `render/shaders/bindings.slangh`: the word, the record matching the C one, binding 11, the
  constants matching device.h.
- `render/shaders/lighting.slangh`: `uint voe_render_blocker_mask(float3 p)` over this pass's
  region, the same test as the C call; the lit path takes the surface's mask at world + n × PUSH
  once; the sun's direct term (its shadow included) and the fill are nought unless that mask is 0;
  a point light's term is nought unless its mask from the region equals the surface's. Header: a
  paragraph on blockers (0347).
- `render/shaders/draw.slang`: only what the lit exit needs to hand the normal on; the header's
  lighting paragraph names the blockers.
- `render/tests/blocked_light.c` (new), modelled on `render/tests/point_lights.c`: a grey ground
  quad, sun of some strength and a fill; a blocker over its left half (box from y −1 to 1): the left
  reads black, the right as with no blocker; the same with blockers zeroed and with one box far off
  reads the no-blocker picture exactly; a red point light outside the box with range reaching both
  halves lights only the right; one inside lights only the left; an unshaded pass is unchanged.
- `render/src/src.md`, `render/shaders/shaders.md`, `render/tests/tests.md`,
  `render/include/render/render.md`: entries changed or added for what moved. Each at most 300
  characters.

## Done when
The test `render/blocked_light` passes, and `render/point_lights`, `render/unshaded`,
`render/shadow`, `render/bounce_read` and `render/passes` still pass, after the folder's build.
