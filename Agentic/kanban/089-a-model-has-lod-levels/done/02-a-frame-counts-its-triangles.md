# 02 — A frame counts the triangles it draws
folder: render
after: none
decisions: 0168, 0425

## Change
0425 point 7: the Frame panel shows how many triangles the frame draws; `render` counts them, as it
counts draw commands (`voe_render_frame_draw_count`).

- `render/include/render/device.h`: beside `voe_render_frame_draw_count`, add
  `[[nodiscard]] uint64_t voe_render_frame_triangle_count(const voe_render_device *device)`. Its
  comment: every mesh draw adds its index count ÷ 3 × the instances it records (a point shadow's
  faces included), in every pass, solid and blended; element draws add nothing; reset by every
  begin; read once `_end` has run for the frame just submitted, as the draw count is.
- `render/src/device_internal.h`: the device gains `uint64_t triangles` beside `draw_commands`.
- `render/src/frame.c`: begin resets it where it resets `draw_commands`; the header's sentence on
  what begin resets names it.
- `render/src/draw.c`: `draw_with` adds to it where it counts the command; the function above
  `voe_render_frame_draw_count` gets its twin. The file's header names the count.
- `render/tests/passes.c`: where a check reads `voe_render_frame_draw_count` after one mesh draw,
  also check `voe_render_frame_triangle_count` is that mesh's index count ÷ 3, and nought after the
  next begin. The header's list of checks names it.

## Done when
The test `render/passes` passes after the folder's build, with the triangle check in it.
