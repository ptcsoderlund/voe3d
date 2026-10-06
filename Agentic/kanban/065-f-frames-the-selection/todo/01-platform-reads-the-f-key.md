# 01 — platform reads the F key
folder: platform
after: none
decisions: 0168, 0371

## Change
Add F to the keys `platform` reports, by place like every other key (decision 0371 point 4).

- `platform/include/platform/input.h` — add `VOE_PLATFORM_KEY_F` to `voe_platform_key`, after
  `VOE_PLATFORM_KEY_R` and before `VOE_PLATFORM_KEY_COUNT`. In the header's "the list is short"
  paragraph, F becomes the most recent arrival: the editor's frame-the-selection shortcut
  (spec 065) reads it; R moves to the "came before" sentence.
- `platform/src/seat_wayland.c` — the evdev scancode switch beside `case KEY_R:` maps `KEY_F` to
  the new key.
- `platform/src/seat_win32.c` — both places `VOE_PLATFORM_KEY_R` appears: the virtual-key switch
  maps `'F'` to the new key, and the table indexed by `voe_platform_key` gets
  `[VOE_PLATFORM_KEY_F] = 'F'`. Windows is paused (0339); the edit keeps the table whole.
- `platform/platform.md` — no entry changes unless its `input.h` line names the keys.

## Done when
- `cmake --build --preset debug --target voe_platform` exits 0.
- `grep -c "VOE_PLATFORM_KEY_F\b" platform/src/seat_wayland.c platform/src/seat_win32.c` shows
  1 and 2.
