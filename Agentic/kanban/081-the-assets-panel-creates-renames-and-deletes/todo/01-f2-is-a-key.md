# 01 — F2 is a key
folder: platform
after: none
decisions: 0168, 0378

## Change
The editor's rename shortcut needs F2, which `platform` does not report yet. Linux only (0339).

- `platform/include/platform/input.h` — add `VOE_PLATFORM_KEY_F2` to `voe_platform_key`, before
  `VOE_PLATFORM_KEY_COUNT`.
- `platform/src/seat_wayland.c` — the evdev-code-to-key switch maps `KEY_F2` to it.
- Leave `seat_win32.c` alone (0339): its table's missing entry reads as no key.

## Done when
`grep -n KEY_F2 platform/src/seat_wayland.c` prints the new case, and the folder's own check passes.
