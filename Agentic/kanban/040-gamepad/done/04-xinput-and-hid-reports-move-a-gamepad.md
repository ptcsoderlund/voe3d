# 04 — XInput and HID reports move a gamepad
folder: platform
after: 02
decisions: 0168, 0292

## Change
0292 point 5's mapping, OS-free in `src/gamepad.c` beside 02's evdev part, so Linux tests it.
XInput bits and HID usages are written as numbers with their SDK names beside them; no
Windows header here.

- `src/gamepad.h` / `src/gamepad.c`:
  - `void voe_platform_gamepad_xinput(voe_platform_gamepad *pad, uint16_t buttons,
    uint8_t left_trigger, uint8_t right_trigger, int16_t left_x, int16_t left_y,
    int16_t right_x, int16_t right_y)`: an `XINPUT_GAMEPAD` whole. Buttons A south, B east,
    X west, Y north, shoulders, BACK, START, thumbs, DPAD; triggers /255; sticks through
    `_axis` over −32768..32767, Y already up-positive so not inverted.
  - `void voe_platform_gamepad_hid_value(voe_platform_gamepad *pad, uint16_t usage,
    int32_t value, struct voe_platform_gamepad_range range)`: generic desktop usages 0x30 X
    and 0x31 Y left, 0x32 Z and 0x35 Rz right (Y and Rz inverted), 0x33 Rx and 0x34 Ry the
    left and right triggers through `_trigger`, 0x39 the hat: `value - range.min` of 0..7
    clockwise from up sets the pad's buttons, anything else releases all four.
  - `void voe_platform_gamepad_hid_buttons(voe_platform_gamepad *pad, const uint16_t *usages,
    uint32_t count)`: the button usages down now, PlayStation order (1 west, 2 south, 3 east,
    4 north, 5 and 6 shoulders, 9 back, 10 start, 11 and 12 sticks); every face, shoulder,
    back, start and stick button not listed goes up. Pad buttons are the hat's, untouched.
  - Header points: why PlayStation order (0292 point 5); what a generic pad reads as.
- `tests/gamepad.c`: XInput with A and DPAD_LEFT down, left trigger 255, left stick 32767 up
  reads south, left, 1, 1; all zero reads at rest within 1/32767. HID: X at max of 0..255 reads
  `left_x` 1; Rz at 0 reads `right_y` 1; hat 2 presses right only, hat 8 releases; buttons
  {2, 10} press south and start, then {} releases them.
- `src/src.md`, `tests/tests.md`: the Windows mapping.

## Done when
`ctest --test-dir build/debug -R '^platform/gamepad$'` passes, after the folder's build.
