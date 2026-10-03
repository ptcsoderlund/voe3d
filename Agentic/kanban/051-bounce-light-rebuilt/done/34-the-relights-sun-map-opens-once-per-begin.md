# 34 — The relight's sun map opens once per bounce begin, not once per frame
folder: render
after: 33
decisions: 0168, 0329, 0330

## Change
0330 point 1, the fault of bug 02 of 051: `voe_render_bounce_shadow_pass_begin` asserts on the
second view's open in a frame (`!frame->bounce_shadow.drawn`, `bounce_shadow.c`). It is fixed here
once, in its owner; 3d and the editor call it once per view and stay as they are. Read
`bounce_shadow.c`'s and `bounce_volume.c`'s headers first.
- `render/src/bounce_shadow.c`: the second-open assert is per begin: it fires on a second open
  after one `voe_render_bounce_begin`, with a message saying so. The begin's barrier out of
  UNDEFINED waits on the compute shader stage as well, so it follows the previous view's relight
  reading the map this frame. Header: THE MAP's flag is per begin, cleared by the bounce begin;
  THE BEGIN's barrier and why; COST at most one pass per begun target (0330).
- `render/src/bounce_volume.c`: `voe_render_bounce_begin` clears this slot's `bounce_shadow.drawn`.
- `render/src/frame.c`: its clear of that flag goes.
- `render/src/device_parts.h`: `struct voe_render_bounce_shadow`'s comment: `drawn` is whether the
  current begin drew it, cleared by each begin.
- `render/include/render/device.h`: `voe_render_bounce_shadow_pass_begin`'s comment: asserts on a
  second open after one bounce begin, not per frame; the `passes` paragraph and the begin's and
  relight's comments say "this begin's" sun map where they say "this frame's".
- `render/tests/bounce_shadow.c`: a TWO TARGETS case: on a device with a target, the window's and the
  target's volumes built; one frame begins the window, captures, opens the bounce shadow pass (the
  cube one draw), relights; then begins the target, opens it again (opened, the cube one draw), and
  relights: no assert, and through `device_internal.h` both volumes' relight records (card 33's
  regions) say drawn. In one frame, the target begun after the window's open with a sun of
  bounces 0 and a lamp of bounces 1: its pass does not open and its region says not drawn.
  Header paragraph for the case.
- `render/src/src.md` (`bounce_shadow.c`), `render/tests/tests.md` (`bounce_shadow.c`): entries.

## Done when
`ctest --test-dir build/debug -R "^render/bounce_"` passes and
`! grep -n "bounce_shadow.drawn" render/src/frame.c` exits 0.
