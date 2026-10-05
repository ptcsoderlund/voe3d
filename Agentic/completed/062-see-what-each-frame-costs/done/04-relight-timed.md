# 04 — The bounce relight timed and labelled
folder: render
after: 03
decisions: 0168, 0358, 0367

## Change
- `render/src/bounce_relight.c`: `voe_render_bounce_relight` wraps the dispatches it records in
  `voe_render_pass_timing_open(device, frame, "bounce relight")` and `_close` (declared in
  `render/src/device_calls.h` by card 03), only when it records any; a settled call that dispatches
  nothing adds no entry. Header: one phrase saying it is timed as a pass.
- `render/include/render/device.h`: `voe_render_bounce_relight`'s comment says it appears in the
  breakdown as `bounce relight`.
- `render/tests/bounce_settle.c`: after a frame whose relight dispatched, the breakdown read
  `VOE_RENDER_FRAMES_IN_FLIGHT` frames later holds `bounce relight`; after the settled frame, none
  (skipped with a line when the card writes no timestamps). Its entry in `render/tests/tests.md`.

## Done when
- `ctest --test-dir build/debug -R '^render/bounce_settle$'` passes, and the folder's tests pass.
