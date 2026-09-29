# 02 — Evdev events move a gamepad
folder: platform
after: 01
decisions: 0168, 0292

## Change
0292 point 4's mapping, OS-free in `src/gamepad.c` so it is tested on any machine. Evdev
codes are written as numbers with their kernel names beside them, as `src/keymap.c` does with
key codes; no `linux/input.h` here.

- `src/gamepad.h` / `src/gamepad.c`:
  - `struct voe_platform_gamepad_range { int32_t min; int32_t max; }` and
    `struct voe_platform_gamepad_evdev { struct voe_platform_gamepad_range abs[64];
    bool has_trigger_axes; }`: a device's axis ranges by `ABS_*` code, filled by the backend
    from `EVIOCGABS`; whether it has `ABS_Z`/`ABS_RZ`.
  - `void voe_platform_gamepad_evdev_key(voe_platform_gamepad *pad,
    const struct voe_platform_gamepad_evdev *device, uint16_t code, int32_t value)`:
    `BTN_SOUTH/EAST/NORTH/WEST` (the kernel's north and west are 0x133 and 0x134), `BTN_TL/TR`,
    `BTN_SELECT` back, `BTN_START`, `BTN_THUMBL/R`, `BTN_DPAD_*`; `BTN_TL2/TR2` set the trigger
    to 0 or 1 only when the device has no trigger axes; a code not listed is ignored.
  - `void voe_platform_gamepad_evdev_abs(voe_platform_gamepad *pad,
    const struct voe_platform_gamepad_evdev *device, uint16_t code, int32_t value)`: `ABS_X/Y`
    left, `ABS_RX/RY` right through `_axis`, Y inverted; `ABS_Z/RZ` triggers through
    `_trigger`; `ABS_HAT0X/Y` the pad's four buttons (−1 left or up); others ignored.
  - Header points: the codes are the kernel's standard gamepad mapping (0292 point 4); why the
    digital triggers yield to axes.
- `tests/gamepad.c`: an Xbox-like device (sticks −32768..32767, triggers 0..1023): full left
  stick up reads `left_y` 1; right stick at centre reads 0; `ABS_RZ` 1023 reads
  `right_trigger` 1; `BTN_SOUTH` 1 then 0; hat X −1 presses left and 0 releases it; a device
  without trigger axes takes `BTN_TR2` as 1; an unknown code changes nothing.
- `src/src.md`, `tests/tests.md`: the evdev part.

## Done when
`ctest --test-dir build/debug -R '^platform/gamepad$'` passes, after the folder's build.
