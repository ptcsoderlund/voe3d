# 04 — One layered point-shadow pass, one instanced draw per caster
folder: render
after: 03
decisions: 0168, 0325

## Change
0325 points 2 and 3, the drawing; the lookup is card 05.
- `render/include/render/device.h`: `[[nodiscard]] bool voe_render_point_shadow_pass_begin(
  voe_render_device *device, const voe_render_point_lights *lights)`, beside
  `voe_render_shadow_pass_begin`, its comment on the same terms (a pass, counts against `passes`,
  false with a line when spent; asserts outside a frame, with a pass open, on a device not ready,
  with no light slotted). `voe_render_frame_draw`'s comment: in a point-shadow pass, one instanced
  draw over the faces the geometry's sphere reaches, none when it reaches none (true, no object
  spent). `_draw_blended`'s assert list names this pass too.
- `render/src/device_parts.h`: a geometry range keeps its bounding sphere; the pass kind gains
  point shadow.
- `render/src/geometry.c`: both creates take the sphere with card 01's function.
- `render/shaders/point_shadow.slangh`, new: the face of a vector by its major axis, and the clip
  position of a light-relative point on face f, 90° reversed depth, near
  `VOE_RENDER_POINT_SHADOW_NEAR` 0.05 (define it in `device.h` too), far the light's range, faces as
  0325 point 1. Header: card 05's lookup uses the same function, so the two cannot disagree.
- `render/shaders/bindings.slangh`: the push constant becomes the object and three words of face
  mask (bit 6(s − 1) + f); its comment.
- `render/shaders/draw.slang`: entry `point_shadow_vertex`: the instance's layer is the n-th set
  bit of the mask; the light is the pass's point light at layer / 6; writes
  `SV_RenderTargetArrayIndex` and the clip position from `point_shadow.slangh`.
- `render/src/pipeline.c`: a fifth mesh pipeline, point shadow: that vertex entry, no fragment
  stage, nothing culled, depth bias as the shadow pipeline's, depth `GREATER`, the push range 16
  bytes for every pipeline in the shared layout. Its header's count.
- `render/src/point_shadow.c`: the pass begin: barriers to attachment, the camera block as a shadow
  pass's (sun zeroed), the slotted lights written into this pass's point-light region at index
  slot − 1 through the copy `pass.c` uses, rendering over all 96 layers cleared; its end barrier
  back to shader-read, called from `pass.c`'s end for this kind.
- `render/src/draw.c`: in a point-shadow pass, the sphere under the world matrix, the mask over
  every slotted light with card 01's faces call, popcount instances, the mask pushed.
- `render/src/frame_internal.h`, `pass.c`: whatever the two need across (the copy, the kind's
  end); nothing else in pass.c.
- `render/src/src.md`, `render/shaders/shaders.md`, `render/render.md`: entries.
- `render/tests/point_shadows.c`: a frame with two slotted lights, a point-shadow pass, then a
  camera pass: a cube 2 m from a light adds one to `voe_render_frame_draw_count`, one 100 m off adds
  none; with `passes` spent the begin is false. Entry updated.

## Done when
`ctest --test-dir build/debug -R "^render/point_shadows$"` passes.
