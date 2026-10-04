# 02 — Render opens without its mesh pipelines and prepares them a step at a time
folder: render
after: none
decisions: 0168, 0345

## Change
Opening a device builds the pipeline layout and the element pipeline only; the rest
is built by a new public call, or on first need.

- `render/include/render/device.h`: add
  `typedef enum { VOE_RENDER_PREPARING, VOE_RENDER_PREPARED, VOE_RENDER_PREPARE_FAILED } voe_render_prepare;`
  and `voe_render_prepare voe_render_device_prepare(voe_render_device *device)`, near
  `voe_render_device_new`. Its comment says: each call builds the next thing this
  device still lacks, in order solid, blended, shadow, point shadow and capture
  (the last two only with shaderOutputLayer, as now), then the relight's startup;
  PREPARED once nothing is left (and on every later call); FAILED with a line on
  stderr, the device still drawing elements; why (a program shows a starting line
  between steps, 0345); that a caller who never calls it loses nothing, because the
  passes below build what is left first. The `voe_render_device_new` comment says
  it opens unprepared.
- `render/src/pipeline.c` and `render/src/startup.h`: split
  `voe_render_pipelines_create` into `voe_render_pipeline_layout_create` (the layout
  alone, at open) and the five pipelines built one per step. A shared
  internal `[[nodiscard]] bool voe_render_device_ready(voe_render_device *device)`
  (declared in `device_internal.h`) calls prepare until PREPARED, false on FAILED.
  `voe_render_device_prepare` lives here too.
- `render/src/device.c`: `open_device` makes the layout, then the element pipeline
  (element.c, unchanged except that the layout now exists before any mesh
  pipeline), and no longer calls the mesh pipelines nor
  `voe_render_bounce_relight_startup`. Close-down is safe for every handle never
  built. The order account in its header and in `startup.h` is brought up to date.
- `render/src/bounce_relight.c`: shutdown is safe when startup never ran; the
  relight does nothing on a device whose relight has not started.
- `render/src/pass.c` (`voe_render_pass_begin` with a non-NULL camera),
  `shadow.c`, `point_shadow.c`, `bounce_capture.c`, `bounce_shadow.c` (their pass
  begins) and `voe_render_bounce_begin`: call `voe_render_device_ready` first and
  return false (or refuse as that call already does on failure) when it fails.
  A NULL-camera pass never builds anything.
- `render/src/src.md`, `render/include/render/render.md`: entries for pipeline.c
  and device.c say open versus prepare.
- `render/tests/prepare.c` (new, headless): `prepare_steps_then_prepared` — a new
  device answers PREPARING at least once and PREPARED within 6 calls, and PREPARED
  again after; `element_pass_needs_no_prepare` — on an unprepared device a
  NULL-camera window pass draws a solid element and the read-back pixel has its
  colour, and the next prepare call still answers PREPARING; `camera_pass_prepares`
  — on an unprepared device a pass with a camera opens, a mesh draws (copy the
  setup of an existing headless test such as `offscreen.c`), and a following
  prepare answers PREPARED. List it in `render/tests/tests.md`.

## Done when
`render/tests/prepare.c`'s three cases pass, and every other test in `render/tests/`
still passes.
