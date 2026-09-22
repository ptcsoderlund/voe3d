# 10 — A wait is a ceiling, and an old shell says it cannot see hidden
folder: platform
decisions: 0168, 0215, 0216

## Change
No signature changes; what changes is what the wait promises and one warning.

`platform/include/platform/window.h`, the paragraph that begins "_poll returns
immediately": it currently promises that `_wait` "blocks until the window's
connection has an event or the seconds pass". That promise is wrong and card 11
depends on the honest one. Rewrite it to make these points, in the file's own
voice: `seconds` is a ceiling and never a guarantee; the connection's own
traffic — a compositor's bookkeeping, a swapchain's buffer releases — returns
the wait at once and says nothing about what a program cares about; a caller
that wants a deadline loops around `_wait` and `_poll` and asks a clock whether
the time has passed (ADR-0216); and, as before, the wait folds nothing, the next
`_poll` does.

The `_focused` and `_visible` paragraph keeps its Wayland and Windows gaps and
gains one phrase: a compositor older than xdg-shell 6 is warned about at open,
so its always-visible window is a known fact rather than a guess.

`platform/src/window_wayland.c`:
- In `open`, after the registry roundtrip and the test that `compositor` and
  `wm_base` are both bound (the `shell_version` is final there), when
  `shell_version` is below 6: one `VOE_BASE_WARNING` on module `platform`,
  naming the version bound and that a window on it can never report itself
  hidden, so it always reads visible and a program that paces on visibility
  falls back to the unfocused heartbeat. Once per window, nowhere else. Add
  `#include <base/report.h>` if the file has none.
- The comment above `voe_platform_window_wait` says it returns on anything at
  all, including the `dispatched == 0` early return, that this is allowed and
  expected (ADR-0216), and that the caller loops. The code of the wait itself is
  unchanged.

`platform/platform.md`, the `window.h` entry: `_wait` blocks for at most the
timeout rather than until an event or the timeout.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder platform` exits 0, and
`grep -n "shell_version < 6" platform/src/window_wayland.c` prints the warning's
guard.
