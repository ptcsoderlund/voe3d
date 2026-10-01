# 02 — Every target keeps a depth copy
folder: render
after: 01
decisions: 0168, 0305

## Change
0305 point 1. Read `render/include/render/device.h` (the pass and draw
calls), `render/src/device_internal.h`, `render/src/device_parts.h`,
`render/src/frame_internal.h`, `render/src/target.c`,
`render/src/target_own.c`, `render/src/texture.c` (slots),
`render/src/pass.c`, `render/src/draw.c`, `render/src/descriptors.c`,
`render/shaders/draw.slang` (the camera block only), and
`render/tests/targets.c` for a headless test's shape.

- `device_parts.h`: the per-slot image record (window and caller target)
  gains the depth copy image, its memory and a sampled depth-aspect view; a
  target slot gains the copy's texture slot.
- `target.c` / `target_own.c`: build the copy with each depth image (D32,
  transfer-dst and sampled), free and resize it with it; take one texture
  slot for the window's copy and one per caller target, named the way a
  target's colour slot is, so the slot reads the frame slot's own copy.
- `device.h`: `[[nodiscard]] bool voe_render_frame_copy_depth(
  voe_render_device *device)`. Its comment's points: only inside an open
  camera pass (false otherwise, and in a shadow pass); ends the rendering
  block, copies depth into the copy with the barriers either side, resumes
  loading colour and depth; draws after it read the copy; at most once a
  pass is the caller's business, each call costs a copy; the capacities
  comment says a target now costs two texture slots.
- `pass.c` / `draw.c`: the copy call; the pass's camera block gains the open
  target's copy slot, a sentinel (~0u) until the pass has copied.
- `descriptors.c`: the camera block's added word and its asserts.
- `draw.slang`: the block's mirror; nothing reads it yet.
- `render/tests/depth_copy.c`, new, headless, cases:
  - outside a pass and in a shadow pass the call is false;
  - a red cube drawn, the copy, then a green cube behind it: the centre stays
    red, so depth survived the resume;
  - colour drawn before the copy is kept;
  - the same on a caller target, read back through `voe_render_target_read`.
- `render/include/render/render.md`, `render/src/src.md`,
  `render/tests/tests.md`: entries changed or added.

## Done when
The test `render/depth_copy` passes, and `render/targets`, `render/passes`
and `render/textures` still pass, after the folder's build.
