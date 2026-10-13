# 27 — A draw can show both sides
folder: render
after: 22
decisions: 0168, 0421

## Change
0421 point 2: back-face culling becomes a per-draw choice through one call, with no new pipeline.
Read the header of `render/src/draw.c` (the bind tracking) and of `render/src/pass.c` first.

- `render/include/render/device.h`: `void voe_render_frame_two_sided(voe_render_device *device, bool
  two_sided)`, beside `voe_render_frame_draw`. Comment: mesh draws after it in this pass draw back faces
  too (true) or cull them (false); every pass opens culling them; in the shadow, bounce shadow,
  point-shadow and capture passes it changes nothing, since they cull nothing; asserts a pass is open.
- `render/src/pipeline.c`: the solid and blended pipelines take `VK_DYNAMIC_STATE_CULL_MODE`; their
  static cull mode stays BACK as the documented default. The file's header says which are dynamic.
- `render/src/draw.c`, `render/src/pass.c` and the frame or device state they track `bound` in: the
  cull mode wanted is kept, set when a pass opens and set again after every bind of a pipeline with
  the dynamic state, and recorded only when it differs from what was last set. The headers say so.
- `render/shaders/draw.slang`: the fragment stage takes the front-face flag; on a back face the
  interpolated normal is turned round before the normal map and lighting use it. The header says why
  this is safe for one-sided draws (they never reach it).
- `render/tests/two_sided.c` (new, headless): a quad facing away from the camera, lit record, sun from
  the camera's side: one-sided draws nothing over the clear colour; after `_two_sided(true)` it draws,
  lit (brighter than the same quad with the sun behind it); a following pass draws it one-sided again.
- `render/tests/tests.md`, `render/src/src.md`: entries follow (each at most 300 characters).

## Done when
The test `render/two_sided` passes after the folder's build, `render/best_practices` among the folder's
other tests still passing.
