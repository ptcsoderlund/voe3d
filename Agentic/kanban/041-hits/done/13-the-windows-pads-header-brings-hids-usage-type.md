# 13 — The Windows pads' header brings HID's usage type
folder: platform
after: none
decisions: 0168, 0130

## Change
Bug 04: on Windows SDK 10.0.26100.0, `window_win32.c`, `seat_win32.c` and
`gamepad_win32.c` stop with `unknown type name 'USAGE'` inside `hidpi.h`.
That SDK's `shared/hidpi.h` no longer pulls in `hidusage.h`, where `USAGE`
is typedef'd, so it must be included first. The one owner is
`platform/src/window_win32.h`, which all three files include; fix it there
and in no `.c` file.

`platform/src/window_win32.h`:
- Add `#include <hidusage.h>` after `<windows.h>` and before `<hidpi.h>`
  (beside the existing `NTSTATUS` typedef, which stays).
- Extend the comment above the `NTSTATUS` typedef to say the same of
  `USAGE`: some SDKs' `hidpi.h` expects `hidusage.h` already included.

Nothing else changes; Linux never compiles this header.

## Done when
- `awk '/#include <hidusage.h>/{u=NR} /#include <hidpi.h>/{h=NR} END{exit !(u && h && u<h)}' platform/src/window_win32.h`
  exits 0.
- Human, on Windows with SDK 10.0.26100.0 and LLVM clang:
  `cmake --preset debug` then `cmake --build --preset debug` builds the
  engine, editor and tank game with no error in `voe_platform` (bug 04's
  How to reproduce, now passing).
