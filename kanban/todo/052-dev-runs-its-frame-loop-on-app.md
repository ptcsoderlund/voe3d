# 052 — `dev` runs its frame loop on `app`

claimed-by: -
blocked-by: 051
status: todo
decision: *`app` is parts a program calls in its own loop* (ADR-0135) point 5 — `dev` uses `app`, which proves `app` suffices for a game made with no editor.

## Goal

`dev/src/main.c` opens its window and device with `voe_app_new`, starts every frame with
`voe_app_frame_open`, and opens and closes the draw with `voe_app_draw_open` and
`voe_app_draw_close`. Nothing `dev` shows or prints changes.

## Scope

**1. `dev/CMakeLists.txt`** — `app` added to `DEPENDS`. The `dev` row in `cmake/voe.cmake`
already allows it.

**2. Startup in `dev/src/main.c`.** The window's and the device's creation —
`voe_platform_window_new`, the `STARTUP_SCRATCH` arena and `voe_render_device_new`, today
from about line 2092 — become one `voe_app_new`, with `longest_step` set to
`MAX_FRAME_SECONDS`. The `mailbox_wanted` request stays `dev`'s and is made on
`voe_app_device(app)` straight after, as it is now. Later uses of `window` and `gpu` read
`voe_app_window(app)` and `voe_app_device(app)`, or locals taken from them once.

**3. The top of the loop.** `voe_platform_clock_now`, the interval, the clamp,
`voe_platform_window_poll`, the size and `voe_platform_window_should_close` (today lines
2238–2275) become `voe_app_frame_open`, with `break` on `closing`.

- `timing.frame` is still fed the raw interval, and still skips the first frame
  (`tick.first`).
- `seconds` still advances by `tick.step`, and only when the frame is not `minimised`.
- `top` is `tick.now`, so the update and draw timings still bracket what they bracket.
- The size and decoration lines on stdout still print on a change.

**4. The draw.** `voe_render_frame_begin` and `voe_render_frame_end` (today about lines
2448 and 2529) become `voe_app_draw_open` and `voe_app_draw_close`. Their `the GPU stopped
answering` lines now come from `app`; `dev`'s own copies go.

**5. Shutdown.** `voe_render_device_destroy` and `voe_platform_window_destroy` under
`stop:` become `voe_app_destroy`, still after `voe_text_font_destroy`.

**6. `dev/dev.md`** — the `src/main.c` entry says the window, the device, the clock and
the draw's open and close come from `app`.

## What must not change

- **Everything on screen and on stdout**: the scene, both camera modes, Tab, Escape twice to
  close, P, the readout and its four timings, the `draws` line, the interface.
- **The order of systems, and the placements between the camera and transform systems**
  (`facing_the_camera` and the three submits after it). That freedom is why `app` does not
  own the order.
- The present-mode request stays in `dev`.
- `app` is not edited. A gap there is `BLOCKED: app, <why>`.

## Verify

- Linux: `cmake -P check.cmake` green.
- Run `voe_dev`: the same picture and readout; P toggles; Escape hands the camera back, then
  closes; minimising stops the clock. Say in Notes what was checked by eye.
- `grep -n 'voe_platform_window_new\|voe_render_device_new\|voe_render_frame_begin\|voe_render_frame_end\|voe_platform_window_poll' dev/src/main.c`
  returns nothing.
- From the planning root, `bash tools/hot.sh` reports no new `OVER`.

## Done looks like

`dev` looks and behaves exactly as before, and its loop reads as `dev`'s own work between
calls into `app`.

## Notes
