# 03 — Render draws the bounce map
folder: render
after: 02
decisions: 0168, 0307, 0308

## Change
0308 point 1. Read `render/include/render/device.h` (capacities, light,
shadow pass and draw calls), `render/src/device_parts.h`,
`render/src/device_internal.h`, `render/src/shadow.c` (the model: the
cascade images and the shadow pass), `render/src/pass.c`,
`render/src/pipeline.c`, `render/src/draw.c`, `render/shaders/draw.slang`,
`render/src/src.md`, and `render/tests/shadow.c` for a test's shape.

- `device.h`: `VOE_RENDER_BOUNCE_TEXELS` 512; `[[nodiscard]] bool
  voe_render_bounce_pass_begin(voe_render_device *device, const
  voe_render_view *light, const voe_render_light *sun)`. Comment points: one
  more pass of the sun after the cascades, ended by the next pass or the
  frame's end; draws in it are the casters, recorded as in a shadow pass and
  counted against `objects`; the pass counts against `passes`; false inside
  no frame or when passes are spent; the last one this frame is what an
  update reads (card 05); the capacities comment's pass count names it.
- `device_parts.h`: the per-slot record gains the bounce map's depth (D32),
  flux (RGBA16F) and normal (RGBA16F) images, memory and views, colour
  images sampled and storage-readable; the frame slot records this frame's
  bounce view and sun, and whether a bounce pass ran.
- `render/src/bounce_map.c`, new, with header comment: build and free the
  images with the device, the pass begin (barriers to attachment, rendering
  with two colour attachments and depth, cleared), and its end barriers to
  shader-read for compute. `device_internal.h` declares what `pass.c` and
  the device build call.
- `pipeline.c`: a fourth graphics pipeline, `bounce`: the draw vertex stage,
  the new fragment entry, two RGBA16F colour formats, D32, no blending, cull
  as the shadow pipeline does.
- `pass.c` / `draw.c`: the pass kind, so a draw in it binds the bounce
  pipeline and the sun's view, and the next pass closes it.
- `render/shaders/draw.slang`: a `bounce` fragment entry writing flux = sun
  colour × intensity × base colour (factor × texture × object colour), no
  N·L, alpha 1, and the world normal in xyz.
- `render/tests/bounce_map.c`, new, headless, cases:
  - outside a frame the call is false;
  - with passes = 1 + cascades + 1, four cascades and a bounce pass open and
    a cube draws in each; one more pass is false;
  - a camera pass after it still draws: a red cube reads red.
- `render/include/render/render.md`, `render/src/src.md`,
  `render/shaders/shaders.md`, `render/tests/tests.md`: entries changed or
  added.

## Done when
The test `render/bounce_map` passes, and `render/shadow`, `render/passes`
and `render/water` still pass, after the folder's build.
