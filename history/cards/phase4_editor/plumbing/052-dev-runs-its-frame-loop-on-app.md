# 052 — `dev` runs its frame loop on `app`

claimed-by: kanban-coder (Opus 5)
blocked-by: 051
status: review
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

**Verified on Linux** (Fedora, Wayland, clang 22, NVIDIA RTX 4070 Laptop, real
driver). `cmake -P check.cmake` exits zero — every standalone configure, the
guards, 41 tests and the analyser over 110 files. Windows is unchecked; nothing
here is platform-specific, and what that machine turns up is a bug report.

**Checked by eye, running `voe_dev`:** the same picture — the model, the two
cubes, the sprites, the two quads, the sign, the line locked to the camera on
its panel, the exhibit, the badge, the screen-filling surface, the interface with
its heading and two buttons — and the same stdout, line for line: `render`,
`model`, `opened 960x540`, `decorated yes`, `camera orbit`, `present fifo`, the
legend, `readout 80 glyphs`, `interface 28 element records`, `elements`, `draws
31 commands for 122 element records`, then a `timing` block every two seconds
with its five numbers, saying `mailbox`. That last word is the evidence that the
present-mode request still reaches the device through `voe_app_device`.

**NOT checked by eye: Tab, Escape twice, P and minimising.** This session has no
way to put a keypress into a Wayland window — there is no `wtype`, no `ydotool`
and no `/dev/uinput` — so the four interactive checks the Verify section asks for
need a person at the keyboard. Everything they exercise is untouched `dev` code
reading the same window pointer, and the one line near them that did change,
`if (!opened.minimised)`, is `size.width <= 0 || size.height <= 0` negated, which
is the old `now_size.width > 0 && now_size.height > 0` exactly. Worth a minute at
review all the same.

**Three judgement calls, each narrow:**

- **The world's arena is now made before the window**, because the app struct
  lives in it and has to outlive every call made through it. `voe_base_arena_destroy(arena)`
  under `stop:` therefore comes after `voe_app_destroy(app)`, and the window is no
  longer destroyed separately. Order at shutdown is font, app, arena — the font
  still goes first, as the card asks.
- **`dev`'s two startup failure lines are gone**, not replaced. `app` prints which
  of the window and the device refused and `render` prints why, both on stderr,
  so a third line from `dev` would only repeat them — the same reasoning the card
  applies to `the GPU stopped answering` in point 4. A comment at the call site
  says so.
- **The file header was updated** where it named `voe_render_frame_begin` and
  `_end` and said the clock was read from `voe_platform_clock_now`. Both sentences
  had become false. The legend printed on stdout still says "begin to end", and
  was left alone because it is output.

No `DEVIATION:` and no `BLOCKED:`. `app` was not edited. `git diff --stat` is
`dev/CMakeLists.txt`, `dev/dev.md`, `dev/src/main.c` and this card. The card's
three greps return nothing; from the planning root `bash tools/hot.sh` reports no
`OVER` (`dev/dev.md` is 118 of 120 and was 115 — worth knowing before the next
line goes in it).
