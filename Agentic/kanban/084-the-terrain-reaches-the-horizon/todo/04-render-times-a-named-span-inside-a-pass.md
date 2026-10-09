# 04 — render times a named span inside a pass
folder: render
after: 03
decisions: 0168, 0358, 0367, 0388, 0396

## Change
0396 point 6: the frame breakdown names the terrain's share of the view pass.

- `render/include/render/device.h`:
  - `void voe_render_frame_span_begin(voe_render_device *device, const char *name)` and
    `void voe_render_frame_span_end(voe_render_device *device)`: inside an open pass, not nested,
    at most `VOE_RENDER_FRAME_SPANS` (8) a frame; one past it is not timed and says nothing.
    Asserts with no pass open, a span already open, or a pass ending with one open.
  - `voe_render_frame_pass_times`' contract: a span is listed right after the pass that held it,
    named `<pass name>: <span name>` (cut to `VOE_RENDER_PASS_NAME`), its time part of that pass
    and not beside it, so the sum over passes alone stays at most the frame's.
- `render/src/pass_timing.c` (header points too): two more timestamps per span in the slot's
  query pool, read back with the passes' at the same lag.
- `render/tests/pass_times.c` and `render/tests/tests.md`: a view pass with a span `terrain`
  around a draw lists `view window: terrain` (or the target's name) after it, its seconds not
  above the pass's; with no span the list is as before.
- `editor/src/frame_breakdown.c` and `dev/src/breakdown.c` list what they are given and need no
  change; do not open them.

## Done when
`render/tests/pass_times.c` passes with the span case.
