# 04 — A pass draws the sky behind everything
folder: render
after: 03
decisions: 0168, 0405

## Change
0405 point 4 and the shader half of point 3's twin. Read the headers of the files below, `render/src/sky.h`
(card 03) and `render/tests/pass_times.c` for reading a span.

- `render/shaders/sky.slangh` (new part, header comment saying it is `render/src/sky.h`'s twin):
  `float3 voe_render_sky_radiance(float3 direction, float3 sunlight, float3 view)`, the same
  arithmetic and constants as `voe_render_sky_radiance` in `render/src/sky.c`.
- `render/shaders/draw.slang`: includes it and `tone.slangh`; two entry points
  `voe_render_sky_vertex` (one full-screen triangle from the vertex index, the view direction from
  the projection's scale terms and the view's rotation transposed, an orthographic projection's the
  view's −Z) and `voe_render_sky_fragment` (the radiance along it from the frame block's light, black
  for an `unshaded` light, through `voe_render_expose`). The file's count of entry points rises.
- `render/src/pipeline.c`: a sixth pipeline from the same module, no vertex input, no depth test or
  write, no blend, built through the cache as the others and counted in the prepare steps; header and
  the "five" wording say six. `render/src/device_internal.h`: the pipeline's handle beside the others.
- `render/src/sky_draw.c` (new, header comment): `void voe_render_sky_draw(voe_render_device *device)`,
  declared in `render/src/device_calls.h` under its file: inside an open camera pass whose block's sky
  is `drawn`, opens the span `sky`, binds the sky pipeline, draws three vertices, ends the span, and
  leaves `bound` so the next mesh draw rebinds. Called by `render/src/pass.c` right after a camera
  pass's block is written, before anything else is recorded; never in shadow, point-shadow or capture
  passes.
- `render/include/render/device.h`: the `voe_render_pass_begin` and `voe_render_frame_pass_times`
  comments say the sky is drawn first and timed as `<pass>: sky`; the `voe_render_sky` comment that
  `drawn` does it. `render/tests/prepare.c`: its count of steps and its comment say six pipelines.
- `render/tests/sky.c` (new, headless): a target looking along −Z, horizon through its middle, sun 30°
  up ahead: with `drawn`, exposure 0, Clip, the centre pixel and one near the top equal the sRGB of
  `voe_render_sky_radiance` (included from `../src/sky.h`) along their directions within 2/255; a quad
  drawn in the pass covers it; the sky zeroed reads the clear colour; a sun of intensity 0 a black sky;
  `voe_render_frame_pass_times` lists `view target 0: sky` (as `pass_times.c` names a target's pass)
  two frames later.
- `render/render.md`, `render/src/src.md`, `render/shaders/shaders.md`, `render/tests/tests.md`:
  entries changed (six pipelines, draw.slang's entry points) and one new each for `sky.slangh`,
  `sky_draw.c` and the test `sky.c`. Each at most 300 characters.

## Done when
The tests `render/sky`, `render/prepare`, `render/pass_times` and `render/exposure` pass after the
folder's build.
