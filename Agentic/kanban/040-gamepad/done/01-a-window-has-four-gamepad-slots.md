# 01 — A window has four gamepad slots
folder: platform
after: none
decisions: 0168, 0292

## Change
0292 points 1, 2, 3 and 6, the OS-free core. After this card every slot reads not connected on
both platforms; the backends come in 03 and 05.

- `include/platform/input.h`: `VOE_PLATFORM_GAMEPAD_SLOTS` (4); `voe_platform_gamepad_button`,
  the fourteen places of 0292 point 1 plus a `_COUNT`; `voe_platform_gamepad`, the struct of
  0292 point 1; `voe_platform_gamepad voe_platform_input_gamepad(voe_platform_window *window,
  int slot)`, asserting a window and a slot in range. Header points: "No gamepad" goes (0292
  replaces it); a pad is polled state on the window; buttons by place like keys; the signs and
  ranges; the slot and id rule; no dead zone, no focus gate, and why.
- `src/gamepad.h` and `src/gamepad.c`, new, OS-free (no OS header, built on both platforms like
  `keymap.c`):
  - `struct voe_platform_gamepads { voe_platform_gamepad slots[VOE_PLATFORM_GAMEPAD_SLOTS];
    uint32_t next_id; }`.
  - `int voe_platform_gamepad_attach(struct voe_platform_gamepads *pads)`: lowest free slot,
    connected, at rest, id one past any before; −1 when all four are taken.
  - `void voe_platform_gamepad_detach(struct voe_platform_gamepads *pads, int slot)`: back to a
    zeroed, not connected slot.
  - `float voe_platform_gamepad_axis(int32_t value, int32_t min, int32_t max)`: min..max onto
    −1..1 about the middle, clamped; a range of no width is 0.
  - `float voe_platform_gamepad_trigger(int32_t value, int32_t min, int32_t max)`: onto 0..1,
    clamped.
  - Header points: why OS-free (0292 point 6); a caller inverts a down-positive Y.
- `src/input.h`: `struct voe_platform_input` gains `struct voe_platform_gamepads gamepads`;
  its comment says no poll, focus loss or pointer loss clears it (0292 point 3).
- `src/input.c`: `voe_platform_input_gamepad` returns a copy of the slot from
  `voe_platform_window_input(window)->gamepads`.
- `tests/gamepad.c`, new: attach fills 0, 1, 2, 3 then refuses; detach of 1 then attach gives 1
  with a higher id than any before; a detached slot is all zero; axis at min, middle, max and
  past both ends; trigger at both ends and the middle; a zero-width range.
- `src/src.md`, `tests/tests.md`, `platform.md` (the `input.h` line): the new files and the pads.

## Done when
`ctest --test-dir build/debug -R '^platform/gamepad$'` passes, after the folder's build.
