# 08 — A pass draws its clouds over the world
folder: render
after: 07
decisions: 0168, 0305, 0358, 0480, 0492

## Change
0492 point 5: the call that marches and composites a pass's clouds. Read the headers of
`render/src/depth_copy.c` (a block split and reopened), `render/src/sky_draw.c` (a full-screen
draw and its span), `render/src/frame_internal.h` and `render/src/pass.c`; in
`render/include/render/device.h` the comments of `voe_render_frame_copy_depth`,
`voe_render_frame_clear_depth`, the span calls and `voe_render_clouds`; the target and texture
slot records in `render/src/device_parts.h`; `render/shaders/draw_clouds.slangh`; and
`render/tests/haze_sky.c` and `render/tests/depth_copy.c` as test patterns.

- `render/include/render/device.h`: `[[nodiscard]] bool voe_render_frame_draw_clouds(
  voe_render_device *device)` with its comment: draws the open camera pass's clouds over what it
  holds; called after the world's blended draws and before the overlay's depth clear, once a
  pass; copies depth first when the pass has not; a pass whose clouds kind is none records
  nothing and is true; timed as the span `<pass>: clouds`; no pass, no camera or a shadow,
  point-shadow or capture pass asserts, as the depth clear does; false only when the depth copy
  is. `voe_render_clouds`'s comment says where they are drawn, pointing here.
- `render/src/clouds_draw.c` (new): the call. In the span `clouds`: the copy if the block's
  `depth_copy` is unset; the pass's block ended; the target's frame-slot clouds image to colour
  attachment; a block of its own onto it at its half size, the march pipeline, three vertices;
  the image back to shader read; the pass's block reopened loading colour and depth through
  `voe_render_open_rendering`, viewport and scissor as before; `clouds_image` in the pass's block
  set to the target's clouds slot; the composite pipeline, three vertices; `bound` left so the
  next mesh draw binds its own; not counted as a caller's draw. Header: what, the order, why the
  block splits, why the slot is written into the block.
- `render/src/frame_internal.h`: its list of files including it names `clouds_draw.c`.
- `render/tests/clouds.c` (new), headless, a target looking straight up under a drawn sky and one
  light: kind none draws the same bytes as no call and lists no `clouds` span; Volumetric with
  coverage 1, bottom 500, thickness 500 changes the centre pixel by more than 8/255 and lists
  `<pass>: clouds`; coverage 0 is within 1/255 of none; Flat coverage 1 at 500 changes the centre;
  an opaque quad 10 m above the eye under those clouds keeps its pixels within 1/255 of none;
  the eye at 700 m inside that Volumetric layer changes the centre too.
- `render/src/src.md`, `render/tests/tests.md`, `render/include/render/render.md`: an entry or
  a phrase each, at most 300 characters.

## Done when
The tests `render/clouds`, `render/depth_copy` and `render/pass_times` pass after the folder's
build.
