# 02 — A window says it is focused and shown, and can wait
folder: platform
decisions: 0168, 0215

## Change
Three calls on `platform/include/platform/window.h`, on both backends:

- `bool voe_platform_window_focused(voe_platform_window *window);` — the window is the one the
  person is working in.
- `bool voe_platform_window_visible(voe_platform_window *window);` — any of it can be on screen.
- `void voe_platform_window_wait(voe_platform_window *window, double seconds);` — blocks until the
  window's connection has an event or `seconds` pass; a negative `seconds` waits with no timeout. It
  reads what arrived but folds nothing: the next `_poll` does that, as ever.

Both readers answer true before the first configure. The header of window.h loses "platform has no way
to wait yet" and says: what each answer is on each platform and its known gap (0215), that the answers
change only at `_poll`, and that `_wait` is how a loop costs nothing between frames.
`platform/include/platform/clock.h`'s header says the wait is on the window, not here, in place of
"the folder has no way to wait yet". `platform/platform.md`'s `window.h` entry names the three.

`platform/src/window_wayland.h` and `window_wayland.c` (after card 01):
- The struct gains `focused`, `suspended` and the bound shell version.
- `registry_global` binds `xdg_wm_base` at the lower of the advertised version and 6. Every
  listener slot a version up to 6 can send gets a function: `xdg_toplevel`'s `configure_bounds` (4)
  and `wm_capabilities` (5), both ignoring their arguments. The header's "every global is bound at
  version 1" becomes: all but the shell, and why the shell is higher.
- `toplevel_configure` reads its `states` array: `XDG_TOPLEVEL_STATE_ACTIVATED` sets focused,
  `XDG_TOPLEVEL_STATE_SUSPENDED` sets suspended; kept pending and folded at `_poll` with the size.
- `voe_platform_window_wait`: flush, `wl_display_prepare_read` (dispatching pending until it
  succeeds), `poll()` on `wl_display_get_fd` with the timeout in milliseconds or −1,
  `wl_display_read_events` on POLLIN, else `wl_display_cancel_read`. A dead connection returns and
  is left for `_poll` to fold into should_close, as `pump` already does.

`platform/src/window_win32.c`:
- focused follows `WM_ACTIVATE` (`WA_INACTIVE` clears it); visible is `!IsIconic(hwnd)`, read at
  `_poll`.
- `voe_platform_window_wait`: `MsgWaitForMultipleObjectsEx(0, NULL, ms or INFINITE, QS_ALLINPUT,
  MWMO_INPUTAVAILABLE)`.
Written, not verified here (ADR-0130).

## Done when
`checks.sh --folder platform` exits 0, and `grep -n "xdg_wm_base_interface" platform/src/window_wayland.c`
shows a version other than a bare 1.
