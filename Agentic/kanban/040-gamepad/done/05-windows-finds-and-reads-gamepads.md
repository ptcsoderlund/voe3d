# 05 — Windows finds and reads gamepads
folder: platform
after: 03, 04
decisions: 0168, 0292, 0248

## Change
0292 point 5's device side. Linux never compiles these files; write them to build on Windows
under the flags of `cmake/voe.cmake`, and say "Written, not verified" in `src/src.md`, as
`sound_win32.c` does. Read the headers of `src/window_win32.h`, `src/window_win32.c`,
`src/seat_win32.c`, `src/sound_win32.c` (loading a DLL at run time) and `src/gamepad.h`.

- `src/gamepad_win32.c`, new, and its declarations in `src/window_win32.h`:
  - State in `struct voe_platform_window`: the loaded `XInputGetState` and the `hid.dll`
    functions used (`HidP_GetCaps`, `HidP_GetValueCaps`, `HidP_GetUsageValue`,
    `HidP_GetUsages`), per XInput user its slot and when it was last asked, and a table of HID
    devices: `HANDLE`, slot, preparsed data, value caps.
  - `void voe_platform_gamepads_open(voe_platform_window *window)`: load both DLLs (either
    missing is those pads absent, not a failure); register raw input for generic desktop
    usages 4 and 5 with `RIDEV_INPUTSINK | RIDEV_DEVNOTIFY` on the window's `hwnd`.
  - `void voe_platform_gamepads_device_change(voe_platform_window *window, WPARAM, LPARAM)`:
    `GIDC_ARRIVAL` reads the device's name (skip `IG_`), preparsed data and value caps, attaches
    a slot; `GIDC_REMOVAL` frees and detaches.
  - `void voe_platform_gamepads_hid(voe_platform_window *window, const RAWINPUT *raw)`: each
    report through `HidP_GetUsageValue` per value cap into `_hid_value`, and `HidP_GetUsages`
    on the button page into `_hid_buttons`.
  - `void voe_platform_gamepads_poll(voe_platform_window *window)`: each of 4 XInput users,
    a connected one every poll, an unconnected one at most once a second (0292 point 5):
    success attaches if needed and feeds `_xinput`; `ERROR_DEVICE_NOT_CONNECTED` detaches.
  - `void voe_platform_gamepads_close(voe_platform_window *window)`: frees and unloads.
- `src/seat_win32.c`: the raw input handler hands `RIM_TYPEHID` to `_hid`.
- `src/window_win32.c`: `voe_platform_window_new` opens the pads after the mouse's
  registration; `window_proc` routes `WM_INPUT_DEVICE_CHANGE`; `voe_platform_window_poll` polls
  them after its messages; `voe_platform_window_destroy` closes them.
- `src/src.md`: the file.

## Done when
The folder builds and its tests pass on Linux; `grep -c voe_platform_gamepads_
platform/src/window_win32.c` prints at least 4.

For the human, on Windows at the end of 07: step 5 of `feature.md`.
