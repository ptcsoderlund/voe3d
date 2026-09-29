# 02 — A window can open fullscreen
folder: platform
after: none
decisions: 0168, 0291

## Change
0291 point 2. Files: `platform/include/platform/window.h`, `platform/src/window_wayland.c`,
`platform/src/window_win32.c`, `platform/platform.md`. The one caller, in `app`, is card 03's;
do not touch it.

- `window.h`: `voe_platform_window_new(int width, int height, bool fullscreen, const char
  *title)`. Header points: fullscreen covers a whole screen and the width and height are then
  only what the window returns to if the system un-fullscreens it; ask `_size`, as ever; there
  is no switch while open (rule 10); the example at the top passes `false`.
- `window_wayland.c`: when asked, `xdg_toplevel_set_fullscreen(toplevel, NULL)` before the empty
  commit that asks for the first configure, so the first configure already carries the screen's
  size. Check the configure handling takes a proposed size as it does for a resize (read the
  comments around `toplevel_configure`; fix only if it ignores one).
- `window_win32.c`: when asked, the window is `WS_POPUP` placed over the primary monitor's
  rectangle (`MonitorFromPoint` with `MONITOR_DEFAULTTOPRIMARY`, `GetMonitorInfoW`) instead of
  `WS_OVERLAPPEDWINDOW` at the asked client size. Linux verifies (ADR-0130); write it anyway.
- `platform.md`: the `window.h` entry says it can open fullscreen.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_platform` exits 0 and
`grep -q xdg_toplevel_set_fullscreen platform/src/window_wayland.c && grep -q WS_POPUP
platform/src/window_win32.c` exits 0. `voe_app` stops building until card 03.
