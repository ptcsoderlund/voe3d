# 03 — Linux finds and reads gamepads
folder: platform
after: 02
decisions: 0168, 0292

## Change
0292 point 4's device side. Read the headers of `src/window_wayland.h`,
`src/window_wayland.c` and `src/gamepad.h`; change only what is named.

- `src/gamepad_wayland.c` and its declarations in `src/window_wayland.h`, new, Linux only:
  - A device table beside the window: per open device its fd, its `dev_t` (so one node is not
    opened twice), its slot and its `struct voe_platform_gamepad_evdev`; plus the inotify fd.
  - `void voe_platform_gamepads_open(voe_platform_window *window)`: inotify on `/dev/input`
    for create and attribute change, non-blocking; then every `event*` there tried.
  - Trying a node: open read-only non-blocking with close-on-exec; kept when `EVIOCGBIT`
    shows `BTN_GAMEPAD` or `BTN_JOYSTICK` and `ABS_X`, and a slot attaches; ranges from
    `EVIOCGABS`, current keys from `EVIOCGKEY` and axes from `EVIOCGABS` fed through
    02's functions, so a held stick reads from the first frame. A node that will not open is
    skipped silently (not a pad, or no permission yet: the attribute change retries it).
  - `void voe_platform_gamepads_poll(voe_platform_window *window)`: drain inotify and try new
    `event*` names; read each device's events until `EAGAIN`, feeding `EV_KEY` and `EV_ABS`
    to 02's functions; `SYN_DROPPED` resyncs as at open; `ENODEV` or any other read failure
    closes it and detaches its slot.
  - `void voe_platform_gamepads_close(voe_platform_window *window)`: closes every fd.
  - Header points: why evdev and not joydev; why attribute changes are watched; that a pad is
    no window-system object, so nothing here is Wayland despite the name.
- `src/window_wayland.h`: the table in `struct voe_platform_window`.
- `src/window_wayland.c`: `voe_platform_window_new` calls `_open` once the rest succeeds, `_poll`
  after the Wayland dispatch, `voe_platform_window_destroy` `_close` before the rest goes; a
  failing inotify is no pads arriving later, not a failed window.
- `src/src.md`: the file.

## Done when
The folder builds and its tests pass; `grep -q voe_platform_gamepads_poll
platform/src/window_wayland.c` exits 0.

For the human, with a pad on Linux, at the end of 07: the steps of `feature.md` 1–4.
