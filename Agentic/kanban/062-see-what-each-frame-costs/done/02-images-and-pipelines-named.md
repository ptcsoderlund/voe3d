# 02 — Every other image and every pipeline named
folder: render
after: 01
decisions: 0168, 0358, 0367

## Change
Name, through `voe_render_debug_name` (declared in `render/src/device_calls.h`, from card 01), each
image, image view and pipeline these files create, right after its create succeeds. Names say what
the thing is and which slot or layer set, e.g. "sun shadow map slot 0", "texture 12", "swapchain
image 1", "solid pipeline", "relight sum pipeline".

- `render/src/shadow.c` — the shadow map images and views.
- `render/src/point_shadow.c` — the point shadow images and views.
- `render/src/texture.c` — each texture's image and view by its slot.
- `render/src/swapchain.c` — the swapchain images (not created here; named when fetched).
- `render/src/bounce_volume.c`, `render/src/bounce_capture.c`, `render/src/bounce_shadow.c` —
  their images and views.
- `render/src/pipeline.c`, `render/src/element.c`, `render/src/probe.c`,
  `render/src/bounce_relight.c` — each pipeline.

No header changes beyond a phrase where a file's header lists what it makes.

## Done when
- The folder's build and tests pass.
- `grep -L voe_render_debug_name render/src/{shadow,point_shadow,texture,swapchain,bounce_volume,bounce_capture,bounce_shadow,pipeline,element,probe,bounce_relight}.c`
  prints nothing.
