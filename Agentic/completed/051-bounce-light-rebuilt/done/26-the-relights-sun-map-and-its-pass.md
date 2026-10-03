# 26 — The relight's own sun map, and the pass that draws it
folder: render
after: none
decisions: 0168, 0326, 0329

## Change
0329 points 2 and 3, the map and its pass only; the relight still reads the cascades until card 27.
Read `bounce_capture.c`'s and `pass.c`'s headers first: this pass is a shadow pass on a map of its
own, opened as the capture pass is.
- `render/include/render/device.h`: `VOE_RENDER_BOUNCE_SHADOW_TEXELS` 1024u beside the bounce
  constants, a one-line comment naming 0329. Beside the capture pass,
  `[[nodiscard]] bool voe_render_bounce_shadow_pass_begin(voe_render_device *device, const
  voe_render_view *light, bool *opened)`. Comment: the sun's map the relight shadows by, opened after
  the begun target's capture passes; `light` eye-relative with an orthographic projection, as a
  cascade's; `opened` false when the volume is not built, the begun sun has no bounces, no intensity
  or is unshaded, or no relight is needed this frame (card 04's call); a pass, counted against
  `passes` and its draws against `objects`, false with a line when spent; asserts outside a frame,
  with a pass open, with no begin, or a second one in a frame. `voe_render_frame_draw`: in it as in a
  shadow pass. The `passes` paragraph: one more on a frame that relights a casting sun. The
  capacities' memory comment: 4 MB of depth a frame slot, only with shaderOutputLayer.
- `render/src/bounce_shadow.c`, new: per frame slot a D32 image of the map's side (depth attachment
  and sampled) and its view, built at startup beside the capture scratch when `output_layer` and torn
  down beside it; the pass begin (depth cleared, the shadow pipeline, viewport and scissor at the
  map's side, `light` as the pass's camera) and end (barrier to a compute read); the slot keeps the
  light's view × projection and whether this frame drew it. Header: why a map of its own (the
  cascades follow the view, 0329), its lifetime, its cost.
- `render/src/device_parts.h`, `render/src/device_internal.h`: the pass kind, the per-slot map,
  matrix and drawn flag, the calls.
- `render/src/frame.c`: the drawn flag cleared as each frame opens.
- `render/src/pass.c`: `voe_render_pass_end` calls this pass's end; its depth copy refuses it as a
  shadow pass's does.
- `render/src/draw.c`: draws in it take the shadow pass's path; `_draw_blended` and `_clear_depth`
  assert in it. `render/src/element.c`: refused in it as in a shadow pass.
- `render/src/device.c`: built and torn down.
- `render/tests/bounce_shadow.c`, new, headless, skipping without shaderOutputLayer as
  `bounce_capture.c` does: a built volume, sun bounces 1, a cube 2 m from the eye: after the capture
  passes of the first frame that captures, the pass opens and the cube adds one draw; a copy of the
  map to a host buffer holds a texel other than the clear; a following frame with nothing changed
  does not open it; sun bounces 0 with a lamp of bounces 1 does not open it; with `passes` spent it
  is false.
- `render/src/src.md`, `render/tests/tests.md`: entries.

## Done when
`ctest --test-dir build/debug -R "^render/bounce_shadow$"` passes.
