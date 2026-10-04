# 01 — The clock says when the OS started this process
folder: platform
after: none
decisions: 0168, 0339, 0345

## Change
- `platform/include/platform/clock.h`: add
  `[[nodiscard]] bool voe_platform_clock_launched(double *at)` — the reading of
  `voe_platform_clock_now`'s own clock at which the OS started this process, so
  `now - at` is how long the process has existed, including the time before `main`
  (loading, a scan of a new program). False, with `*at` untouched, when the OS cannot
  say. The header gains: why (a start's time before the program's own code is
  otherwise invisible), that the resolution is the OS's tick (10 ms on Linux), and
  that it is Linux only while Windows is paused (0339).
- `platform/src/clock_wayland.c`: implement it from field 22 of `/proc/self/stat`
  (start time in clock ticks since boot, `sysconf(_SC_CLK_TCK)`), taken off a
  `CLOCK_BOOTTIME` reading and the result moved onto the monotonic clock `now` uses
  (now − (boottime − started)). Parse after the last `)` so a program name with
  spaces or brackets cannot shift the fields. Do not touch `clock_win32.c` (0339).
- `platform/platform.md`: the `clock.h` entry names the launch reading.
- `platform/tests/clock.c`: add a case `launched_is_before_now`: the call answers
  true, and the reading is at or before `voe_platform_clock_now()` and less than
  60 s before it. `platform/tests/tests.md`'s `clock.c` entry names it.

## Done when
`launched_is_before_now` in `platform/tests/clock.c` passes.
