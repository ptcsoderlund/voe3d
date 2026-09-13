# 074 — `platform` reports the wheel

claimed-by: -
blocked-by: -
decision: *Overflow is opt-in: a container may wrap or clip, and a scroll area remembers its offset* (ADR-0153) point 10.

## Goal

A program can ask how far the wheel turned since the last poll, on both axes, in notches, with the
same sign on both platforms. Nothing reads it yet; card 076 does.

## Scope

**1. `platform/include/platform/input.h`.**

```c
// Wheel turn accumulated since the previous voe_platform_window_poll, in
// notches. +x shows content further right, +y further down — the wheel turned
// towards the person. Fractional where the device reports fractions.
typedef struct {
	float x;
	float y;
} voe_platform_wheel;

voe_platform_wheel voe_platform_input_wheel(voe_platform_window *window);
```

Beside `voe_platform_input_motion`. Replace the paragraph *SCROLL IS NOT HERE* with the unit, the
sign, that it drains exactly as motion does, and that how far a notch moves anything is the
program's.

**2. The accumulator, `platform/src/input.h` and `input.c`.** Two floats beside `motion_x` and
`motion_y`, zeroed by `voe_platform_input_begin_poll` exactly as motion is. Losing focus or the
pointer does not need to clear them beyond that.

**3. Wayland, `platform/src/window_wayland.c`.** Fill the `axis` slot of the `wl_pointer` listener
(version 1, as every global is bound today — do not raise a bound version).
- `WL_POINTER_AXIS_VERTICAL_SCROLL` adds `value / 10` to y; `WL_POINTER_AXIS_HORIZONTAL_SCROLL`
  adds `value / 10` to x. Wayland's positive already means further down and further right.
- A named constant for the 10, with a comment: it is the length per wheel detent Weston and Mutter
  send; other compositors differ (wlroots sends 15), so a notch there is not exactly one; version 5's
  `axis_discrete` is the exact answer and is not bound.

**4. Windows, `platform/src/window_win32.c`.**
- `WM_MOUSEWHEEL`: y adds `−GET_WHEEL_DELTA_WPARAM(wparam) / (float)WHEEL_DELTA` — Windows'
  positive is the wheel turned away, which shows content further up.
- `WM_MOUSEHWHEEL`: x adds `GET_WHEEL_DELTA_WPARAM(wparam) / (float)WHEEL_DELTA`.
- Written, not verified on Windows (ADR-0130).

**5. `platform/tests/input.c`.** Extend the motion accumulator's case: a poll with no wheel leaves
nought on both axes; three turns before one poll leave their sum; a poll zeroes them. Update the
file's top comment, which names what is tested.

**6. `platform/platform.md`** — the wheel in the public surface.

## What must not change

- Every existing listener, bound version and input call.
- `ui`, `editor`, `dev`: nothing reads the wheel on this card.
- No smoothing, no acceleration, no notches-to-lines conversion.

## Verify

- Linux: `cmake -P check.cmake` green; `ctest -R platform` passes.
- Linux, by hand: a temporary print of `voe_platform_input_wheel` in `editor/src/main.c`, one notch
  down then one up then a sideways tilt if the mouse has one — paste the lines into Notes, then
  revert the print. `git diff -- editor` is empty at the end.
- From the planning root, `bash tools/hot.sh` reports no new `OVER`.

## Done looks like

One notch towards you reads `y = 1` on Linux, and the Windows backend is written to read the same.

## Notes
