# 17 — A moving eye captures one bounce pass a frame
folder: 3d
after: 16
decisions: 0168, 0389, 0397
read: feature.md

## Change
0397 point 2: when a target's eye moved more than 1 mm on an axis since its last bounce, that
frame opens at most one capture pass, coarse to fine; else 0389 point 3 stands.

- `3d/include/3d/bounce_casters.h`: `voe_3d_bounce_casters` gains the eye of the target's last
  bounce (double, about the world origin) and whether it holds one; zeroed still is empty, so
  the callers in `editor` and `game` change nothing. Header: the memory also keeps the eye, and
  why (the frame is built afresh each frame).
- `3d/include/3d/draw_system.h` (779 lines; edit only this): `VOE_3D_BOUNCE_MOVING_PASSES 1`
  beside the shadows call, and its contract's "up to VOE_RENDER_BOUNCE_CAPTURE_PASSES more
  passes" sentence says: at most that many with the eye moving, by `frame->casters`' eye.
- `3d/src/draw_bounce.c`: at the start of the bounce, moving is `frame->casters` with an eye
  remembered and `frame->eye` more than 1 mm from it on an axis; at the end (whether or not a
  pass was refused) the memory takes `frame->eye`. Where each volume's `until` is worked out
  (around lines 559 and 576), a moving frame's `until` is `VOE_3D_BOUNCE_MOVING_PASSES` for every
  volume, so the first volume with probes queued takes the one pass and the rest open none.
  Header: the budget paragraph names the moving frame (0397).
- `3d/src/draw_bounce.h`: `voe_3d_draw_bounce`'s contract adds the moving budget.
- New `3d/tests/bounce_pacing.c` (header comment; entry in `3d/tests/tests.md`), on
  `3d/tests/bounce_world.inc` with a small box, sun at bounces 1. If the harness holds the eye
  fixed, give its frame an eye offset, its other users unchanged. Settle with the eye still, then
  move the eye 0.15 m along x a frame for 120 frames. Each moving frame first opens empty passes
  so the device has room for exactly the cascades, one capture pass, a sun map per begun volume
  and the window's pass; the shadows call is true on every one (before this card the 1 m nest's
  slices open four and it fails). Then, with the eye still again, the probing frames settle
  within the harness's bound.

## Done when
`3d/tests/bounce_pacing.c` passes, and `3d/tests/bounce.c` and `3d/tests/bounce_scene.c` still
pass.

Human, feature.md's How to test step 4, on the laptop (the coder does none of this): a 1000 m,
512-cell landscape, a sun of bounces 1 that casts; wait until the capture passes leave the Frame
panel; fly across it: every frame shows at most one `bounce capture`, and the view moves smoothly.
