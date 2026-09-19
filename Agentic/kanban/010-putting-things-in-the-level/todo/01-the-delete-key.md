# 01 — The Delete key
folder: platform
decisions: 0168

## Change
`include/platform/input.h`: add `VOE_PLATFORM_KEY_DELETE` to `voe_platform_key`, before `VOE_PLATFORM_KEY_COUNT`.
The editor's Delete command (spec 010) is what reads it. Update the header's sentence on the most recent key to
arrive.

`src/window_wayland.c`: map evdev `KEY_DELETE` to it, beside `KEY_ENTER`. `src/window_win32.c`: map `VK_DELETE`
both ways, in the two tables that hold `VK_RETURN`.

If `tests/input.c` checks the key list or a table length, extend it to the new key.

## Done when
The folder's check passes (`checks.sh` for `platform`), and `grep -n KEY_DELETE platform/src/window_wayland.c
platform/src/window_win32.c` shows the Wayland case and both Win32 entries.
